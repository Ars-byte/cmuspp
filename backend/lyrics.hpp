#pragma once
/*
  backend/lyrics.hpp
  Lyrics: synchronized .lrc or plain text embedded in the audio tags.

  Offline resolution order for the current song:
    1. local <song>.lrc next to the audio file (synchronized)
    2. lyrics embedded in the file's tags
       (ID3v2 USLT in MP3, LYRICS in FLAC/OGG) — unsynchronized text

  No network, no curl, no daemons: lyrics come from the files themselves.
*/

#include "metadata.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

struct LyricLine {
    double      t = 0.0;   // start time in seconds
    std::string text;
};

struct Lyrics {
    std::vector<LyricLine> lines;   // sorted by t
    bool synced = true;             // .lrc timestamps vs. embedded text
    bool valid() const { return !lines.empty(); }
};

// ── .lrc parsing ─────────────────────────────────────────────────────────────
// Does `tag` (the text inside [..]) look like a mm:ss(.xx) timestamp?
static inline bool lrc_is_time_tag(const std::string& tag) {
    if (tag.empty()) return false;
    size_t col = tag.find(':');
    if (col == std::string::npos || col == 0 || col + 1 >= tag.size())
        return false;
    for (size_t k = 0; k < tag.size(); ++k) {
        char c = tag[k];
        if (c == ':' || c == '.' || c == ',') continue;
        if (!isdigit((unsigned char)c)) return false;
    }
    return true;
}

inline Lyrics parse_lrc(const std::string& s) {
    Lyrics out;
    double offset_ms = 0.0;
    size_t pos = 0;
    while (pos < s.size()) {
        size_t e = s.find('\n', pos);
        if (e == std::string::npos) e = s.size();
        std::string line = s.substr(pos, e - pos);
        pos = e + 1;
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();
        if (line.empty()) continue;

        std::vector<double> times;
        size_t i = 0;
        while (i < line.size() && line[i] == '[') {
            size_t c = line.find(']', i);
            if (c == std::string::npos) break;
            std::string tag = line.substr(i + 1, c - i - 1);
            if (tag.compare(0, 7, "offset:") == 0) {
                offset_ms = atof(tag.c_str() + 7);
            } else if (lrc_is_time_tag(tag)) {
                size_t col = tag.find(':');
                times.push_back(atof(tag.substr(0, col).c_str()) * 60.0
                              + atof(tag.substr(col + 1).c_str()));
            }
            i = c + 1;
        }
        if (times.empty()) continue;

        std::string text = line.substr(i);
        size_t tb = text.find_first_not_of(" \t");
        if (tb == std::string::npos) continue;
        text = text.substr(tb);

        for (double t : times)
            out.lines.push_back({ t + offset_ms / 1000.0, text });
    }

    std::sort(out.lines.begin(), out.lines.end(),
              [](const LyricLine& a, const LyricLine& b) { return a.t < b.t; });
    return out;
}

inline Lyrics load_lrc(const std::string& audio_path) {
    std::string lrc_path = audio_path;
    size_t dot = lrc_path.find_last_of('.');
    if (dot == std::string::npos) return {};
    lrc_path = lrc_path.substr(0, dot) + ".lrc";

    FILE* f = fopen(lrc_path.c_str(), "rb");
    if (!f) return {};
    std::vector<char> buf(1024 * 1024);
    size_t nr = fread(buf.data(), 1, buf.size(), f);
    fclose(f);
    if (nr == 0) return {};
    return parse_lrc(std::string(buf.data(), nr));
}

// Index of the line active at time t (-1 before the first line).
inline int lyric_index(const Lyrics& L, double t) {
    int lo = 0, hi = (int)L.lines.size();
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (L.lines[mid].t <= t) lo = mid + 1;
        else                     hi = mid;
    }
    return lo - 1;
}

// ── Embedded tag lyrics (offline) ────────────────────────────────────────────
// Plain text (USLT / LYRICS) → one LyricLine per line, unsynchronized.
inline Lyrics embedded_lyrics(const std::string& audio_path) {
    TrackMeta m = read_meta_tags(audio_path);
    if (m.lyrics.empty()) return {};

    Lyrics out;
    out.synced = false;
    size_t pos = 0;
    while (pos < m.lyrics.size()) {
        size_t e = m.lyrics.find('\n', pos);
        if (e == std::string::npos) e = m.lyrics.size();
        std::string line = m.lyrics.substr(pos, e - pos);
        pos = e + 1;
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();
        out.lines.push_back({ 0.0, line });
    }
    while (!out.lines.empty() && out.lines.back().text.empty())
        out.lines.pop_back();                // trailing blanks
    if (!out.lines.empty() && out.lines.front().text.empty())
        out.lines.erase(out.lines.begin());  // leading blank
    return out;
}
