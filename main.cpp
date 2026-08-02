#include "frontend/draw.hpp"
#include "backend/player.hpp"

#include <csignal>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static ThemeManager g_themes;

void handle_sigwinch(int) {}

int main() {
    // Load themes ONLY from the local "themes" folder (portable)
    g_themes.load_xml_dir("themes");
    
    init_colors(g_themes);   
    signal(SIGPIPE, SIG_IGN);

    struct sigaction sa{};
    sa.sa_handler = handle_sigwinch;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGWINCH, &sa, nullptr);

    RawTerm rt(STDIN_FILENO);

    emit("\033[?1049h");
    emit(A::CLS);

    std::string sel = browse(rt);
    if (sel.empty()) {
        emit("\033[?1049l");
        emit(A::SHOW);
        return 0;
    }

    Player player;
    player.load_dir(sel);

    // ── Full-screen lyrics mode ─────────────────────────────────────────────
    bool lyrics_mode = false;
    int  lyr_scroll  = 0;      // scroll for unsynchronized (embedded) lyrics
    auto draw = [&]() {
        if (lyrics_mode) draw_lyrics(player, g_themes, lyr_scroll);
        else             draw_player(player, g_themes);
    };
    draw();

    auto scroll_lyrics = [&](int d) {
        auto L = player.lyrics();
        if (L->synced || L->lines.empty()) return;   // synced view auto-centers
        lyr_scroll = std::clamp(lyr_scroll + d, 0, (int)L->lines.size() - 1);
    };

    double last_draw = mono_now();
    static constexpr double DRAW_INTERVAL = 0.125; 
    TSz last_tsz = tsz();

    // ── Search state ──────────────────────────────────────────────────────────
    bool searching = false;
    std::string query;
    std::vector<int> matches;
    int mrow = 0;

    auto rebuild_matches = [&]() {
        matches.clear();
        std::string q = icase_sort_key(query);
        for (int i = 0; i < (int)player.songs.size(); ++i)
            if (icase_sort_key(player.songs[i]).find(q) != std::string::npos)
                matches.push_back(i);
        if (matches.empty()) mrow = 0;
        else if (mrow >= (int)matches.size()) mrow = (int)matches.size() - 1;
    };

    while (true) {
        if (player.songs.empty()) {
            emit(std::string(A::CLS) + A::SHOW);
            sel = browse(rt);
            if (!sel.empty()) {
                player.load_dir(sel);
                draw();
                last_draw = mono_now();
            } else {
                break;
            }
            continue;
        }

        if (!player.playing_now.empty() && !player.paused && player.is_ended()) {
            player.next_song();
            draw();
            last_draw = mono_now();
            continue;
        }

        // ── Search mode ───────────────────────────────────────────────────────
        if (searching) {
            std::string k = read_key(rt, -1);
            bool leave = false;

            if      (k == "\x1b" || k == "\x03" || k == "\x04")  leave = true;
            else if (k == "\x7f") { if (!query.empty()) { query.pop_back(); rebuild_matches(); } }
            else if (k == "\x1b[A" || k == "k")
                { if (!matches.empty()) mrow = (mrow - 1 + (int)matches.size()) % (int)matches.size(); }
            else if (k == "\x1b[B" || k == "j")
                { if (!matches.empty()) mrow = (mrow + 1) % (int)matches.size(); }
            else if (k == "\r") {
                if (!matches.empty()) {
                    player.row = matches[mrow];
                    player.play_current();
                }
                leave = true;
            }
            else if (k.size() == 1 && (unsigned char)k[0] >= 0x20 && (unsigned char)k[0] < 0x7f) {
                query += k;
                rebuild_matches();
            }

            if (leave) { searching = false; draw(); }
            else       { draw_search(player, query, matches, mrow); }
            last_draw = mono_now();
            continue;
        }

        std::string key = read_key(rt, 0.12);

        TSz cur_tsz = tsz();
        bool resized = (cur_tsz.cols != last_tsz.cols || cur_tsz.rows != last_tsz.rows);
        if (resized) last_tsz = cur_tsz;

        // Reset the unsynchronized-lyrics scroll whenever the song changes.
        static std::string scroll_song;
        if (player.playing_now != scroll_song) {
            scroll_song = player.playing_now;
            lyr_scroll  = 0;
        }

        if (key.empty()) {
            if (resized) {
                draw();
                last_draw = mono_now();
            } else if (!player.playing_now.empty() && !player.paused) {
                double now = mono_now();
                if (now - last_draw >= DRAW_INTERVAL) {
                    draw();
                    last_draw = now;
                }
            } else if (!player.meta_done()) {
                // Keep refreshing while the metadata loader populates columns
                double now = mono_now();
                if (now - last_draw >= 0.25) {
                    draw();
                    last_draw = now;
                }
            }
            continue;
        }

        bool redraw = true;
        int  n      = (int)player.songs.size();

        if      (key == "\x1b[A" || key == "k") {
            if (lyrics_mode) scroll_lyrics(-1);
            else             player.row = (player.row - 1 + n) % n;
        }
        else if (key == "\x1b[B" || key == "j") {
            if (lyrics_mode) scroll_lyrics(+1);
            else             player.row = (player.row + 1) % n;
        }
        else if (key == "\r")                    player.play_current();
        else if (key == "n" || key == "N")       player.next_song();
        else if (key == "p" || key == "P")       player.prev_song();
        else if (key == " ")                     player.toggle_pause();
        else if (key == "\x1b[D" || key == "h")  player.seek(-5.0);
        else if (key == "\x1b[C")                player.seek(+5.0);
        else if (key == "+" || key == "=")       player.change_vol(+0.05f);
        else if (key == "-" || key == "_")       player.change_vol(-0.05f);
        else if (key == "s" || key == "S")       player.shuffle  = !player.shuffle;
        else if (key == "r" || key == "R")       player.loop_on  = !player.loop_on;
        else if (key == "l" || key == "L")       { lyrics_mode = !lyrics_mode; lyr_scroll = 0; }
        else if (key == "t" || key == "T")       apply_theme(g_themes, g_themes.current + 1);
        else if (key == "o" || key == "O") {
            emit(std::string(A::CLS) + A::SHOW);
            std::string s2 = browse(rt);
            if (!s2.empty()) player.load_dir(s2);
        }
        else if (key == "a" || key == "A") {
            draw_about();
            read_key(rt, -1); // cualquier tecla cierra el about
        }
        else if (key == "/") {
            searching = true;
            query.clear();
            mrow = player.row;
            rebuild_matches();
            draw_search(player, query, matches, mrow);
            last_draw = mono_now();
            redraw = false;
        }
        else if (key == "q" || key == "Q") break;
        else redraw = resized;

        if (redraw) {
            draw();
            last_draw = mono_now();
        }
    }

    player.stop_all();
    emit("\033[?1049l");
    emit(A::SHOW);
    return 0;
}
