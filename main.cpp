#include "frontend/draw.hpp"
#include "backend/player.hpp"

#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

static ThemeManager g_themes;

void handle_sigwinch(int) {}

int main() {
    // Cargar temas SOLO desde la carpeta "themes" local (portable)
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
    draw_player(player, g_themes);

    double last_draw = mono_now();
    static constexpr double DRAW_INTERVAL = 0.125; 
    TSz last_tsz = tsz();

    while (true) {
        if (player.songs.empty()) {
            emit(std::string(A::CLS) + A::SHOW); flush_out();
            sel = browse(rt);
            if (!sel.empty()) {
                player.load_dir(sel);
                draw_player(player, g_themes);
                last_draw = mono_now();
            } else {
                break;
            }
            continue;
        }

        if (!player.playing_now.empty() && !player.paused && player.is_ended()) {
            player.next_song();
            draw_player(player, g_themes);
            last_draw = mono_now();
            continue;
        }

        std::string key = read_key(rt, 0.12);

        TSz cur_tsz = tsz();
        bool resized = (cur_tsz.cols != last_tsz.cols || cur_tsz.rows != last_tsz.rows);
        if (resized) last_tsz = cur_tsz;

        if (key.empty()) {
            if (resized) {
                draw_player(player, g_themes);
                last_draw = mono_now();
            } else if (!player.playing_now.empty() && !player.paused) {
                double now = mono_now();
                if (now - last_draw >= DRAW_INTERVAL) {
                    draw_player(player, g_themes);
                    last_draw = now;
                }
            }
            continue;
        }

        bool redraw = true;
        int  n      = (int)player.songs.size();

        if      (key == "\x1b[A" || key == "k") player.row = (player.row - 1 + n) % n;
        else if (key == "\x1b[B" || key == "j") player.row = (player.row + 1) % n;
        else if (key == "\r")                    player.play_current();
        else if (key == "n" || key == "N")       player.next_song();
        else if (key == "p" || key == "P")       player.prev_song();
        else if (key == " ")                     player.toggle_pause();
        else if (key == "\x1b[D" || key == "h")  player.seek(-5.0);
        else if (key == "\x1b[C" || key == "l")  player.seek(+5.0);
        else if (key == "+" || key == "=")       player.change_vol(+0.05f);
        else if (key == "-" || key == "_")       player.change_vol(-0.05f);
        else if (key == "s" || key == "S")       player.shuffle  = !player.shuffle;
        else if (key == "r" || key == "R")       player.loop_on  = !player.loop_on;
        else if (key == "t" || key == "T")       apply_theme(g_themes, g_themes.current + 1);
        else if (key == "o" || key == "O") {
            emit(std::string(A::CLS) + A::SHOW); flush_out();
            std::string s2 = browse(rt);
            if (!s2.empty()) player.load_dir(s2);
        }
        else if (key == "q" || key == "Q") break;
        else redraw = resized;

        if (redraw) {
            draw_player(player, g_themes);
            last_draw = mono_now();
        }
    }

    player.stop_all();
    emit("\033[?1049l");
    emit(A::SHOW);
    return 0;
}
