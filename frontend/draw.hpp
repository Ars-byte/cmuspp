#pragma once
#include "ansi.hpp"
#include "cover_art.hpp"
#include "../backend/player.hpp"

#include <filesystem>
#include <pwd.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

inline std::string strip_ext(const std::string& name) {
    static const char* exts[] = {
        ".mp3", ".flac", ".wav", ".ogg", ".opus", ".aiff", ".aif", ".au", nullptr
    };
    std::string lc = name;
    std::transform(lc.begin(), lc.end(), lc.begin(), ::tolower);
    for (int i = 0; exts[i]; ++i) {
        size_t el = strlen(exts[i]);
        if (lc.size() > el && lc.compare(lc.size() - el, el, exts[i]) == 0)
            return name.substr(0, name.size() - el);
    }
    return name;
}

struct BD {
    std::vector<std::string> dirs, songs;
    mutable std::unordered_map<std::string, int> audio_count_cache;
};

inline BD scan(const std::string& path) {
    BD d;
    try {
        for (auto& e : fs::directory_iterator(path)) {
            std::string nm = e.path().filename().string();
            if (nm.empty() || nm[0] == '.') continue;
            if (e.is_directory())                          d.dirs.push_back(nm);
            else if (e.is_regular_file() && is_audio(nm)) d.songs.push_back(nm);
        }
    } catch (...) {}
    auto ic = [](const std::string& a, const std::string& b) {
        return icase_sort_key(a) < icase_sort_key(b);
    };
    std::sort(d.dirs.begin(),  d.dirs.end(),  ic);
    std::sort(d.songs.begin(), d.songs.end(), ic);
    return d;
}

inline int count_au_cached(const BD& d, const std::string& parent,
                            const std::string& nm) {
    auto it = d.audio_count_cache.find(nm);
    if (it != d.audio_count_cache.end()) return it->second;
    int n = 0;
    try {
        std::string full = parent + "/" + nm;
        for (auto& e : fs::directory_iterator(full))
            if (e.is_regular_file() && is_audio(e.path().filename().string())) ++n;
    } catch (...) {}
    d.audio_count_cache[nm] = n;
    return n;
}

// ── Draw: player ──────────────────────────────────────────────────────────────
inline void draw_player(Player& p, const ThemeManager& mgr) {
    TSz sz = tsz(); int cols = sz.cols, rows = sz.rows;

    const int FIXED_ROWS   = 4;
    const int PLAYLIST_MIN = 28;
    const int COVER_MIN_W  = 50;

    int content_rows = std::max(2, rows - FIXED_ROWS);

    bool show_cover = (cols >= COVER_MIN_W);
    int  cover_w    = 0;
    int  cover_h    = 0;

    if (show_cover) {
        cover_h = content_rows;
        cover_w = std::min(cover_h * 2, cols - PLAYLIST_MIN - 1);
        if (cover_w < 10) { show_cover = false; cover_w = 0; cover_h = 0; }
        else               cover_h = cover_w / 2;
    }

    int playlist_w = cols - (show_cover ? cover_w + 1 : 0);
    int max_vis    = content_rows;

    static CoverCache cover_cache;

    std::string full_path;
    if (!p.playing_now.empty()) {
        full_path = (fs::path(p.dir) / p.playing_now).string();
    }

    const int COVER_TERM_ROW = 2;
    const int COVER_TERM_COL = 1;

    std::string kitty_seq;
    const std::vector<std::string>* cover_lines = nullptr;

    if (show_cover) {
        kitty_seq  = cover_cache.get_kitty_seq(
            full_path, cover_w, cover_h,
            A::W2, A::W3, A::GRN,
            COVER_TERM_ROW, COVER_TERM_COL);
        cover_lines = &cover_cache.get_lines();
    }

    int total = (int)p.songs.size();
    int start = std::max(0, p.row - max_vis / 2);
    int end   = std::min(total, start + max_vis);
    if (end == total) start = std::max(0, total - max_vis);

    std::string o;
    o.reserve((size_t)cols * rows * 8);
    o += "\033[?2026h"; // BSU
    o += "\033[H\033[2J";
    o += A::HIDE;

    if (!kitty_seq.empty()) o += kitty_seq;

    // ── Header row ────────────────────────────────────────────────────────────
    o += A::BG_HDR; o += A::BOLD;
    o += " "; o += A::GRN; o += A::NOTE;
    o += " "; o += A::W1; o += "CMUS++";
    {
        std::string dn = fs::path(p.dir).filename().string();
        if (dn.empty()) dn = p.dir;

        int theme_w = 2 + (int)mgr.active().name.size();
        o += A::W2; o += center_in(dn, cols - 9 - theme_w);
        o += A::W3; o += " "; o += mgr.active().name; o += " ";
    }
    o += A::RST; o += "\n";

    // ── Content rows ──────────────────────────────────────────────────────────
    for (int row_i = 0; row_i < max_vis; ++row_i) {
        if (show_cover) {
            if (cover_lines && row_i < (int)cover_lines->size()) {
                o += (*cover_lines)[row_i];
            } else {
                o += A::RST;
                o += std::string(cover_w, ' ');
            }
            o += A::W3; o += "│"; o += A::RST;
        }

        int i = start + row_i;
        if (i < end) {
            const std::string& s = p.songs[i];
            bool cur  = (i == p.row);
            bool play = (s == p.playing_now);
            std::string disp = strip_ext(s);

            if (cur && play) {
                o += A::BG_PLAY; o += A::BOLD;
                o += " "; o += A::PLAY_I; o += "  ";
                o += pad_r(disp, playlist_w - 5);
            } else if (cur) {
                o += A::BG_SEL; o += A::BOLD;
                o += "  "; o += A::ARR; o += "  ";
                o += pad_r(disp, playlist_w - 5);
            } else if (play) {
                o += A::GRN;
                o += " "; o += A::NOTE; o += "  ";
                o += pad_r(disp, playlist_w - 5);
            } else {
                o += A::W3;
                o += "     ";
                o += pad_r(disp, playlist_w - 5);
            }
        } else {
            o += A::RST;
            o += std::string(playlist_w, ' ');
        }
        o += A::RST; o += "\n";
    }

    // ── Title + time row ──────────────────────────────────────────────────────
    {
        double el  = p.elapsed(), dur = p.duration();
        std::string now = p.playing_now.empty() ? "Stopped" : strip_ext(p.playing_now);

        std::string timestr;
        timestr.reserve(48);
        timestr += A::W2; timestr += fmt_t(el);
        if (dur > 0) {
            timestr += A::W3; timestr += " / ";
            timestr += A::W2; timestr += fmt_t(dur);
        }

        int tw = (int)(dur > 0 ? fmt_t(el).size() + 3 + fmt_t(dur).size() : fmt_t(el).size());
        int nw = cols - tw - 2;
        o += A::W1; o += A::BOLD; o += trunc_str(now, nw);
        o += A::RST;
        int fill = std::max(0, nw - cpw(now));
        o += std::string(fill, ' ');
        o += timestr; o += A::RST; o += "\n";
    }

    // ── Progress bar ──────────────────────────────────────────────────────────
    {
        double el = p.elapsed(), dur = p.duration();
        int bw = cols;
        if (dur > 0 && bw > 0) {
            int filled = std::min(bw, (int)(el / dur * bw));
            o += A::GRN;  o += rep(A::PROG,  filled);
            o += A::W3;   o += rep(A::TRACK, bw - filled);
        } else {
            static const char* sp[] = {"⣾","⣽","⣻","⢿","⡿","⣟","⣯","⣷"};
            o += A::AMB; o += sp[(int)(el * 6) % 8]; o += A::RST;
            o += A::W3;  o += rep(A::TRACK, bw - 1);
        }
        o += A::RST; o += "\n";
    }

    // ── Status bar ────────────────────────────────────────────────────────────
    o += A::BG_STAT;
    if (p.paused)                   { o += " "; o += A::AMB; o += A::PAUS_I; }
    else if (p.playing_now.empty()) { o += " "; o += A::W3;  o += A::STOP_I; }
    else                            { o += " "; o += A::GRN; o += A::PLAY_I; }
    o += A::W2;

    int vn = (int)(p.volume * 8);
    o += " "; o += A::VOL_I; o += " ";
    o += A::GRN; o += rep(A::FULL,  vn);
    o += A::W3;  o += rep(A::EMPTY, 8 - vn);
    o += A::W2;

    char tc[24];
    snprintf(tc, sizeof(tc), "  %d/%d", p.row + 1, total);
    o += tc;
    o += " ";
    o += (p.shuffle ? A::GRN : A::W3);
    o += A::SHUF_I; o += (p.shuffle ? " SHUF" : " shuf");
    o += A::W3; o += "  ";
    o += (p.loop_on ? A::GRN : A::W3);
    o += A::LOOP_I; o += (p.loop_on ? " LOOP" : " loop");
    o += A::W2;

    static const char* hints =
        " · ↑/↓ nav · Enter play · Space pause · ←/→ seek · o browse · t theme · q quit";
    int left_vis = 2 + 12 + (int)strlen(tc) + 7 + 7;
    int hfill = std::max(0, cols - left_vis - cpw(hints));
    o += std::string(hfill, ' ');
    o += A::W3; o += hints;
    o += A::RST;

    o += "\033[?2026l"; // ESU
    emit(o);
}

inline void draw_browser(const std::string& path, const BD& d, int cur, int scr) {
    TSz sz = tsz(); int cols = sz.cols, rows = sz.rows;
    int max_vis = std::max(1, rows - 9);
    int total   = (int)(d.dirs.size() + d.songs.size());

    std::string o;
    o.reserve((size_t)cols * rows * 6);

    o += "\033[?2026h";
    o += HOME_NOFLASH; o += A::HIDE;

    o += A::BG_HDR; o += A::BOLD;
    o += "  "; o += A::GRN; o += A::NOTE; o += "  ";
    o += A::W1; o += "CMUS++";
    o += A::W3; o += "  "; o += A::DIR_I; o += "  ";
    o += A::W2; o += "File Browser";
    o += std::string(std::max(0, cols - 27), ' ');
    o += A::RST; o += "\n";

    o += A::W3; o += " "; o += trunc_str(path, cols - 2);
    o += A::RST; o += "\n";

    if (!d.songs.empty()) {
        o += " "; o += A::GRN; o += A::NOTE;
        o += "  "; o += std::to_string(d.songs.size()); o += " track";
        if (d.songs.size() > 1) o += "s";
    } else {
        o += " "; o += A::W3; o += "no audio files";
    }
    o += A::RST; o += "\n";

    o += A::W3; o += rep(A::HL, cols); o += A::RST; o += "\n";

    int end_i = std::min(total, scr + max_vis);
    for (int i = scr; i < end_i; ++i) {
        bool sel = (i == cur);
        if (i < (int)d.dirs.size()) {
            const std::string& nm = d.dirs[i];
            int sc = count_au_cached(d, path, nm);
            std::string badge;
            if (sc > 0) {
                badge += " "; badge += A::GRN; badge += A::NOTE;
                badge += std::to_string(sc); badge += A::RST;
            }
            int badge_w = sc > 0 ? (2 + (int)std::to_string(sc).size()) : 0;
            int name_w  = cols - 8 - badge_w;
            if (sel) {
                o += A::BG_SEL; o += A::BOLD;
                o += "  "; o += A::ARR; o += "  "; o += A::DIR_O; o += " ";
                o += pad_r(nm, name_w); o += badge;
            } else {
                o += A::W3;
                o += "     "; o += A::DIR_I; o += " ";
                o += pad_r(nm, name_w); o += badge;
            }
            o += A::RST;
        } else {
            const std::string& nm = d.songs[i - (int)d.dirs.size()];
            std::string disp = strip_ext(nm);
            if (sel) {
                o += A::BG_SEL; o += A::GRN; o += A::BOLD;
                o += "  "; o += A::ARR; o += "  "; o += A::AUDIO_I; o += " ";
                o += pad_r(disp, cols - 9);
            } else {
                o += A::W2;
                o += "     "; o += A::AUDIO_I; o += " ";
                o += pad_r(disp, cols - 9);
            }
            o += A::RST;
        }
        o += "\n";
    }
    for (int i = end_i - scr; i < max_vis; ++i) o += "\033[K\n";

    if (total == 0) { o += A::W3; o += "  (empty)"; o += A::RST; o += "\n"; }
    if (total > max_vis) {
        char b[48];
        snprintf(b, sizeof(b), "  %d-%d of %d", scr + 1, end_i, total);
        o += A::W3; o += b; o += A::RST; o += "\n";
    }

    o += A::W3; o += rep(A::HL, cols); o += A::RST; o += "\n";
    o += A::BG_STAT;

    auto kb = [&](const char* k, const char* v) {
        o += " "; o += A::W1; o += k;
        o += A::W3; o += " "; o += v;
    };
    kb("↑↓","nav"); kb("→/Enter","open"); kb("←/Bksp","up");
    kb("o","home"); kb("q","cancel");
    int kb_vis = 47;
    o += std::string(std::max(0, cols - kb_vis), ' ');
    o += A::RST;

    o += "\033[?2026l";
    emit(o);
}

inline std::string browse(RawTerm& rt, const std::string& start = "") {
    std::string cur = start.empty()
        ? std::string(getpwuid(getuid())->pw_dir)
        : start;
    try { cur = fs::canonical(cur).string(); } catch (...) {}

    int cursor = 0, scroll = 0;
    BD d = scan(cur);
    draw_browser(cur, d, cursor, scroll);
    TSz last_tsz = tsz();

    while (true) {
        std::string key = read_key(rt, -1);

        TSz cur_tsz = tsz();
        bool resized = (cur_tsz.cols != last_tsz.cols || cur_tsz.rows != last_tsz.rows);
        if (resized) last_tsz = cur_tsz;

        if (key.empty() && !resized) continue;

        TSz sz = cur_tsz; int cols = sz.cols, rows = sz.rows;
        int max_vis = std::max(1, rows - 9);
        int total   = (int)(d.dirs.size() + d.songs.size());
        bool redraw = true;

        if (key == "\x1b[A" || key == "k") {
            if (total) cursor = (cursor - 1 + total) % total;
        } else if (key == "\x1b[B" || key == "j") {
            if (total) cursor = (cursor + 1) % total;
        } else if (key == "\x1b[C" || key == "\r" || key == "l") {
            if (total == 0) {
                redraw = resized;
            } else if (cursor < (int)d.dirs.size()) {
                std::string nxt = cur + "/" + d.dirs[cursor];
                try { nxt = fs::canonical(nxt).string(); } catch (...) {}
                cur = nxt;
                d   = scan(cur);
                cursor = scroll = 0;
                if (d.dirs.empty() && !d.songs.empty()) {
                    emit(std::string(A::CLS) + A::SHOW); flush_out();
                    return cur;
                }
            } else {
                emit(std::string(A::CLS) + A::SHOW); flush_out();
                return cur;
            }
        } else if (key == "\x1b[D" || key == "\x7f" || key == "h") {
            std::string parent = fs::path(cur).parent_path().string();
            if (!parent.empty() && parent != cur) {
                cur = parent;
                d   = scan(cur);
                cursor = scroll = 0;
            } else {
                redraw = resized;
            }
        } else if (key == "o" || key == "O") {
            cur = std::string(getpwuid(getuid())->pw_dir);
            try { cur = fs::canonical(cur).string(); } catch (...) {}
            d = scan(cur);
            cursor = scroll = 0;
        } else if (key == "q" || key == "Q") {
            emit(std::string(A::CLS) + A::SHOW); flush_out();
            return "";
        } else {
            redraw = resized;
        }

        if (total > 0) {
            if (cursor < 0)      cursor = 0;
            if (cursor >= total) cursor = total - 1;
        } else {
            cursor = 0;
        }

        if (cursor < scroll)            scroll = cursor;
        if (cursor >= scroll + max_vis) scroll = cursor - max_vis + 1;

        if (redraw || resized)
            draw_browser(cur, d, cursor, scroll);
    }
}
