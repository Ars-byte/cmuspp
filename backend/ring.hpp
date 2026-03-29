#pragma once
/*
  backend/ring.hpp
  Lock-free SPSC ring buffer for float stereo audio.

  PERFORMANCE NOTES
  ─────────────────
  • head/tail are each on their own 64-byte cache line to eliminate
    false sharing between the producer (decoder) and consumer (audio thread).
  • Hot-path push/pop use only acquire/release atomics — no mutex, no lock.
  • Mutex + CVs are kept ONLY for the blocking wait paths (decoder sleep,
    swap synchronisation) — they are never touched by the audio callback.
  • buf is sized to a power-of-two frames so the modulo becomes a cheap
    bit-mask.  cap_mask = cap - 1.
  • The audio callback reads pop() which is a single CAS-free load/store
    sequence → glitch-free even on a real-time thread.
*/

#include "audio_common.hpp"

// Round up to next power of two (compile-time for constants, runtime for others)
static inline size_t next_pow2(size_t n) {
    if (n == 0) return 1;
    --n;
    for (size_t i = 1; i < sizeof(size_t) * 8; i <<= 1) n |= n >> i;
    return n + 1;
}

struct alignas(64) Ring {
    // ── Storage ────────────────────────────────────────────────────────────
    std::vector<float>  buf;
    size_t              cap_mask;   // = capacity - 1  (bit-mask for fast modulo)

    // ── Producer state (written by decoder thread) ─────────────────────────
    alignas(64) std::atomic<size_t> tail{0};

    // ── Consumer state (written by audio thread) ───────────────────────────
    alignas(64) std::atomic<size_t> head{0};

    // ── Blocking helpers (only used outside the audio hot-path) ────────────
    std::mutex              mx;
    std::condition_variable cv_data;    // signalled when frames become available
    std::condition_variable cv_space;   // signalled when space becomes available

    explicit Ring(size_t frames) {
        size_t cap = next_pow2(frames);
        buf.resize(cap * OUT_CH, 0.f);
        cap_mask = cap - 1;
    }

    // ── Capacity helpers ───────────────────────────────────────────────────
    size_t capacity() const { return cap_mask + 1; }

    // Frames available to read (safe to call from any thread)
    size_t avail() const {
        size_t t = tail.load(std::memory_order_acquire);
        size_t h = head.load(std::memory_order_acquire);
        return (t - h) & cap_mask;   // unsigned wrap-around is defined
    }

    // Frames of write space remaining
    size_t space() const {
        return (capacity() - 1) - avail();
    }

    // ── Push (decoder / producer thread) ──────────────────────────────────
    // Returns frames actually written (may be < n if buffer is almost full).
    size_t push(const float* __restrict__ src, size_t n) {
        size_t cap  = capacity();
        size_t t    = tail.load(std::memory_order_relaxed);
        size_t h    = head.load(std::memory_order_acquire);
        size_t avl  = (h - t - 1) & cap_mask;   // free slots
        size_t w    = (n < avl) ? n : avl;

        // Split into two contiguous segments to avoid per-frame modulo
        size_t t0   = t & cap_mask;
        size_t seg1 = cap - t0;                  // frames until wrap
        if (w <= seg1) {
            std::memcpy(buf.data() + t0 * OUT_CH, src, w * OUT_CH * sizeof(float));
        } else {
            std::memcpy(buf.data() + t0 * OUT_CH, src,       seg1 * OUT_CH * sizeof(float));
            std::memcpy(buf.data(),                src + seg1 * OUT_CH,
                        (w - seg1) * OUT_CH * sizeof(float));
        }
        tail.store(t + w, std::memory_order_release);
        if (w) cv_data.notify_one();
        return w;
    }

    // ── Pop (audio thread / consumer) ─────────────────────────────────────
    // Returns frames actually read (may be < n → caller should zero-fill).
    size_t pop(float* __restrict__ dst, size_t n) {
        size_t cap  = capacity();
        size_t h    = head.load(std::memory_order_relaxed);
        size_t t    = tail.load(std::memory_order_acquire);
        size_t avl  = (t - h) & cap_mask;
        size_t r    = (n < avl) ? n : avl;

        size_t h0   = h & cap_mask;
        size_t seg1 = cap - h0;
        if (r <= seg1) {
            std::memcpy(dst, buf.data() + h0 * OUT_CH, r * OUT_CH * sizeof(float));
        } else {
            std::memcpy(dst,                          buf.data() + h0 * OUT_CH,
                        seg1 * OUT_CH * sizeof(float));
            std::memcpy(dst + seg1 * OUT_CH, buf.data(),
                        (r - seg1) * OUT_CH * sizeof(float));
        }
        head.store(h + r, std::memory_order_release);
        if (r) cv_space.notify_one();
        return r;
    }

    // ── Reset (called by decoder before a new stream, hold no audio lock) ─
    void clear() {
        head.store(0, std::memory_order_release);
        tail.store(0, std::memory_order_release);
    }
};
