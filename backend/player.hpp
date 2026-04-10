#pragma once
#include "audio_out.hpp"
#include "decoder.hpp"

#include <algorithm>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

namespace fs = std::filesystem;

inline bool is_audio(const std::string& n) {
    static const char* exts[] = {
        ".mp3", ".flac", ".wav", ".ogg", ".opus", ".aiff", ".aif", ".au", nullptr
    };
    std::string lc = n;
    std::transform(lc.begin(), lc.end(), lc.begin(), ::tolower);
    for (int i = 0; exts[i]; ++i) {
        size_t el = strlen(exts[i]);
        if (lc.size() >= el && lc.compare(lc.size() - el, el, exts[i]) == 0)
            return true;
    }
    return false;
}

inline std::string icase_sort_key(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(), ::tolower);
    return r;
}

struct Player {
    AudioOut ao;
    Decoder  dec;

    std::vector<std::string> songs;
    std::string  dir, playing_now;
    int   row     = 0;
    float volume  = 0.5f;
    bool  paused  = false;
    bool  shuffle = false;
    bool  loop_on = false;

    std::atomic<double> wall{0.0};   
    std::atomic<double> soff{0.0};   

    std::mt19937 rng{std::random_device{}()};
    bool ao_ready = false;

    Player() {
        if (!ao.open()) { fputs("cmus++: cannot open audio device\n", stderr); exit(1); }
        ao.gain.store(volume);
    }
    ~Player() { dec.stop(); ao.stop(); }

    double elapsed() const {
        if (playing_now.empty()) return 0.0;
        return paused ? soff.load(std::memory_order_relaxed)
                      : soff.load(std::memory_order_relaxed) +
                        (mono_now() - wall.load(std::memory_order_relaxed));
    }
    double duration() const { return dec.duration.load(std::memory_order_relaxed); }

    bool is_ended() const {
        return !playing_now.empty()
            && dec.done()
            && dec.ring.avail() == 0;
    }

    void play_current(double from = 0.0) {
        if (songs.empty()) return;
        playing_now = songs[row];
        
        std::string fp = (fs::path(dir) / playing_now).string();

        paused = false;
        ao.paused.store(false, std::memory_order_release);
        ao.gain.store(volume,  std::memory_order_release);
        soff.store(from,       std::memory_order_relaxed);
        wall.store(mono_now(), std::memory_order_relaxed);

        dec.start(fp, from);
        dec.wait_prefill();

        if (!ao_ready) { ao.attach(dec.ring); ao_ready = true; }
        else           { ao.swap_ring(dec.ring); }
    }

    void toggle_pause() {
        if (playing_now.empty()) return;
        if (paused) {
            soff.store(elapsed(),  std::memory_order_relaxed);
            wall.store(mono_now(), std::memory_order_relaxed);
            paused = false;
            ao.paused.store(false, std::memory_order_release);
        } else {
            soff.store(elapsed(), std::memory_order_relaxed);
            paused = true;
            ao.paused.store(true, std::memory_order_release);
        }
    }

    void seek(double delta) {
        if (playing_now.empty()) return;
        double dur = duration();
        double tgt = std::clamp(elapsed() + delta,
                                0.0,
                                dur > 0.0 ? dur - 0.5 : 0.0);

        ao.paused.store(true, std::memory_order_release);
        soff.store(tgt,       std::memory_order_relaxed);
        wall.store(mono_now(),std::memory_order_relaxed);

        std::string fp = (fs::path(dir) / playing_now).string();
                         
        dec.start(fp, tgt);
        dec.wait_prefill();

        if (!ao_ready) { ao.attach(dec.ring); ao_ready = true; }
        else           { ao.swap_ring(dec.ring); }

        if (!paused) ao.paused.store(false, std::memory_order_release);
    }

    void change_vol(float d) {
        volume = std::clamp(volume + d, 0.f, 1.f);
        ao.gain.store(volume, std::memory_order_release);
    }

    void next_song() {
        if (songs.empty()) return;
        if (loop_on) { play_current(); return; }
        row = shuffle
            ? std::uniform_int_distribution<int>(0, (int)songs.size() - 1)(rng)
            : (row + 1) % (int)songs.size();
        play_current();
    }

    void prev_song() {
        if (songs.empty()) return;
        if (loop_on) { play_current(); return; }
        row = shuffle
            ? std::uniform_int_distribution<int>(0, (int)songs.size() - 1)(rng)
            : (row - 1 + (int)songs.size()) % (int)songs.size();
        play_current();
    }

    void stop_all() {
        dec.stop();
        playing_now.clear();
        paused = false;
        ao.paused.store(false, std::memory_order_release);
    }

    void load_dir(const std::string& path) {
        dir = path; songs.clear(); row = 0;
        try {
            for (auto& e : fs::directory_iterator(path))
                if (e.is_regular_file() && is_audio(e.path().filename().string()))
                    songs.push_back(e.path().filename().string());
        } catch (...) {}
        std::sort(songs.begin(), songs.end(),
            [](const std::string& a, const std::string& b) {
                return icase_sort_key(a) < icase_sort_key(b);
            });
    }
};
