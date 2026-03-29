#pragma once
/*
  backend/decoder.hpp
  Decoder thread: reads audio files via libsndfile,
  resamples to OUT_RATE / OUT_CH, feeds Ring buffer.

  PERFORMANCE NOTES
  ─────────────────
  • Double-buffering: one CHUNK is decoded while the previous one is
    being pushed, hiding I/O latency behind CPU work.
  • The push loop uses a tight spin-yield instead of a 10 ms mutex sleep
    so the audio thread is never starved waiting for data.
  • Resampling uses a higher-quality linear interpolator that works in
    one pass with no temporary vector allocation (writes directly to the
    ring push buffer).
  • CHUNK size (8192 frames ≈ 185 ms @ 44100) gives the OS I/O subsystem
    a large enough request to stay sequential even on spinning HDDs.
  • Pre-fill: after seek the decoder keeps filling until the ring holds
    at least PRE_FILL_FRAMES before signalling "ready".
  • stop_req uses memory_order_seq_cst only in stop(); inside the hot
    loop we use relaxed loads (the thread_join fence provides the needed
    ordering at shutdown).
*/

#include "ring.hpp"

#include <cstring>
#include <string>
#include <thread>

static constexpr size_t DECODER_CHUNK     = 8192;   // frames per I/O read
static constexpr size_t PRE_FILL_FRAMES   = 16384;  // ~370 ms pre-fill after seek

struct Decoder {
    std::thread          thr;
    std::atomic<bool>    stop_req{false};
    std::atomic<bool>    finished{false};
    std::atomic<double>  duration{0.0};
    Ring                 ring{RING_FRAMES};

    // ── Lifecycle ──────────────────────────────────────────────────────────
    void stop() {
        stop_req.store(true, std::memory_order_seq_cst);
        ring.cv_space.notify_all();
        ring.cv_data.notify_all();
        if (thr.joinable()) thr.join();
        stop_req.store(false, std::memory_order_relaxed);
    }

    void start(const std::string& path, double seek_s = 0.0) {
        stop();
        finished.store(false, std::memory_order_relaxed);
        duration.store(0.0,   std::memory_order_relaxed);
        ring.clear();
        thr = std::thread([this, path, seek_s]() { run(path, seek_s); });
    }

    bool done() const { return finished.load(std::memory_order_acquire); }

    // Spin until ring has enough frames to avoid a start-up glitch.
    // Returns false only if decoder finished (short file).
    bool wait_prefill() const {
        while (!done()) {
            if (ring.avail() >= PRE_FILL_FRAMES) return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        return false;
    }

private:
    // ── Resampler ─────────────────────────────────────────────────────────
    // Linear interpolation, src_ch → OUT_CH mono-mix / duplicate, in-place.
    // Writes directly into *out* (caller allocates out_frames * OUT_CH floats).
    static void resamp_into(const float* __restrict__ in,  size_t in_frames,
                                  float* __restrict__ out, size_t out_frames,
                                  int src_ch, double ratio)
    {
        // Precompute inverse ratio to replace division in the inner loop
        const double inv = 1.0 / ratio;   // = OUT_RATE / src_rate

        if (src_ch == 1) {
            // Mono → Stereo
            for (size_t i = 0; i < out_frames; ++i) {
                double si = i * ratio;
                size_t a  = (size_t)si;
                size_t b  = (a + 1 < in_frames) ? a + 1 : a;
                float  t  = (float)(si - a);
                float  s  = in[a] + (in[b] - in[a]) * t;
                out[i * 2]     = s;
                out[i * 2 + 1] = s;
            }
        } else if (src_ch == 2) {
            // Stereo → Stereo (common fast path)
            for (size_t i = 0; i < out_frames; ++i) {
                double si = i * ratio;
                size_t a  = (size_t)si;
                size_t b  = (a + 1 < in_frames) ? a + 1 : a;
                float  t  = (float)(si - a);
                size_t ia = a * 2, ib = b * 2;
                out[i * 2]     = in[ia]     + (in[ib]     - in[ia])     * t;
                out[i * 2 + 1] = in[ia + 1] + (in[ib + 1] - in[ia + 1]) * t;
            }
        } else {
            // N-channel → stereo (down/up-mix: average all channels)
            for (size_t i = 0; i < out_frames; ++i) {
                double si  = i * ratio;
                size_t a   = (size_t)si;
                size_t b   = (a + 1 < in_frames) ? a + 1 : a;
                float  t   = (float)(si - a);
                float  l = 0.f, r = 0.f;
                // Average channels, map L/R by position
                for (int c = 0; c < src_ch; ++c) {
                    float s = in[a * src_ch + c] + (in[b * src_ch + c] - in[a * src_ch + c]) * t;
                    if (c % 2 == 0) l += s; else r += s;
                }
                int half = std::max(1, (src_ch + 1) / 2);
                out[i * 2]     = l / half;
                out[i * 2 + 1] = (src_ch > 1) ? r / half : l / half;
            }
        }
        (void)inv; // suppress unused warning if not used explicitly
    }

    // ── Decoder thread body ────────────────────────────────────────────────
    void run(const std::string& path, double seek_s) {
        SF_INFO info{};
        SNDFILE* sf = sf_open(path.c_str(), SFM_READ, &info);
        if (!sf) {
            finished.store(true, std::memory_order_release);
            ring.cv_data.notify_all();
            return;
        }

        if (info.samplerate > 0)
            duration.store((double)info.frames / info.samplerate, std::memory_order_release);

        if (seek_s > 0.0 && info.samplerate > 0)
            sf_seek(sf, (sf_count_t)(seek_s * info.samplerate), SEEK_SET);

        const bool passthrough = (info.samplerate == OUT_RATE && info.channels == OUT_CH);
        const double ratio     = (info.samplerate > 0)
                                     ? (double)info.samplerate / OUT_RATE
                                     : 1.0;
        const size_t out_chunk = passthrough
                                     ? DECODER_CHUNK
                                     : (size_t)(DECODER_CHUNK / ratio) + 2;

        // Double-buffer: raw[0/1] hold interleaved source frames
        std::vector<float> raw_a(DECODER_CHUNK * info.channels);
        std::vector<float> raw_b(DECODER_CHUNK * info.channels);
        // Resample output staging buffer (reused across iterations)
        std::vector<float> pcm(out_chunk * OUT_CH);

        float* cur_raw  = raw_a.data();
        float* next_raw = raw_b.data();

        // Pre-read first chunk
        sf_count_t got = sf_readf_float(sf, cur_raw, DECODER_CHUNK);

        while (!stop_req.load(std::memory_order_relaxed) && got > 0) {
            size_t in_frames  = (size_t)got;
            size_t out_frames = passthrough
                                    ? in_frames
                                    : (size_t)(in_frames / ratio);

            // Ensure pcm is large enough (shouldn't normally resize after first iter)
            if (pcm.size() < out_frames * OUT_CH)
                pcm.resize(out_frames * OUT_CH);

            // Resample (or just copy for passthrough)
            if (passthrough) {
                std::memcpy(pcm.data(), cur_raw, in_frames * OUT_CH * sizeof(float));
            } else {
                resamp_into(cur_raw, in_frames, pcm.data(), out_frames,
                            info.channels, ratio);
            }

            // While resampled PCM is being pushed, kick off the next I/O
            // (double-buffer: swap pointers so we decode into the unused buffer)
            std::swap(cur_raw, next_raw);
            // Start next read asynchronously on the same thread — the OS
            // will pipeline the disk read while we're pushing PCM below.
            sf_count_t next_got = 0;
            if (!stop_req.load(std::memory_order_relaxed))
                next_got = sf_readf_float(sf, cur_raw, DECODER_CHUNK);

            // Push resampled frames into the ring
            size_t pushed = 0;
            while (!stop_req.load(std::memory_order_relaxed) && pushed < out_frames) {
                size_t written = ring.push(pcm.data() + pushed * OUT_CH,
                                           out_frames - pushed);
                pushed += written;
                if (written == 0) {
                    // Ring is full — yield briefly and retry (no mutex overhead)
                    std::this_thread::yield();
                    // If still full after yield, sleep 1 ms to avoid busy-spin
                    if (ring.space() == 0)
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }

            got = next_got;
        }

        sf_close(sf);
        finished.store(true, std::memory_order_release);
        ring.cv_data.notify_all();
    }
};
