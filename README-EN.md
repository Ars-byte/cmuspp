```markdown
# CMUS++

```text
 ██████╗███╗   ███╗██╗   ██╗███████╗    ██╗  ██╗
██╔════╝████╗ ████║██║   ██║██╔════╝    ╚██╗██╔╝
██║     ██╔████╔██║██║   ██║███████╗     ╚███╔╝
██║     ██║╚██╔╝██║██║   ██║╚════██║     ██╔██╗
╚██████╗██║ ╚═╝ ██║╚██████╔╝███████║ ██╗██╔╝ ██╗
 ╚═════╝╚═╝     ╚═╝ ╚═════╝ ╚══════╝ ╚═╝╚═╝  ╚═╝
                   ++ C++ Terminal Music Player
```

> [🇦🇷 Español](README.md) | 🇬🇧 English

**CMUS++** is a terminal music player written in C++17. Ultra-lightweight, blazingly fast, keyboard-driven, and completely free of graphical dependencies.

**Features:**
*   **Formats:** MP3, FLAC, WAV, OGG, OPUS, AIFF.
*   **Cross-platform:** Linux (ALSA), macOS (CoreAudio), Windows (WinMM).
*   **Customizable:** 9 built-in themes + support for custom XML themes.
*   **Fast:** Zero loading times, efficient decoding, and flicker-free rendering.

---

## Previews

| Player (with .xml theme) |
|:---:|
|<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/2b6cf564-ecc1-4422-90fc-3db0f896fb4d" />|

---

## Installation & Build

### 1. Install dependencies

You only need a C++ compiler and `libsndfile` (plus ALSA on Linux).

*   **Ubuntu / Debian:** `sudo apt install g++ libsndfile1-dev libasound2-dev`
*   **Arch Linux:** `sudo pacman -S gcc libsndfile alsa-lib`
*   **macOS:** `brew install libsndfile` (requires Homebrew)

### 2. Build

Clone the repository and use the included auto-build script:

```bash
# Grant execution permissions
chmod +x bootstrap.sh

# Build
./bootstrap.sh

# Run!
./cmuspp
```

---

## Controls

CMUS++ is designed to be used entirely without a mouse.

| Key | Action |
| :--- | :--- |
| `↑` / `↓` (or `k`/`j`) | Navigate list |
| `Enter` | Play track / Enter folder |
| `Space` | Pause / Resume |
| `←` / `→` (or `h`/`l`) | Seek backward/forward 5s (In browser: Go up a level) |
| `n` / `p` | Next / Previous track |
| `+` / `-` | Volume Up / Down |
| `s` | Toggle Shuffle |
| `r` | Toggle Repeat |
| `t` | Cycle color theme |
| `o` | Open file browser |
| `q` | Quit |

---

## Custom Themes

Press `t` inside the app to change the theme. You can create your own themes by making `.xml` files inside the `themes/` folder (next to the executable) or in `~/.config/cmuspp/themes/`.

**Structure example (`themes/my-theme.xml`):**
```xml
<?xml version="1.0" encoding="UTF-8"?>
<theme name="My Custom Theme">
  <!-- Text colors (Bright -> Dark) -->
  <fg0 r="248" g="248" b="242"/> <fg1 r="215" g="210" b="195"/>
  <fg2 r="117" g="113" b="94"/>  <fg3 r="75"  g="71"  b="60"/>
  
  <!-- Accents -->
  <acc  r="166" g="226" b="46"/> <warn r="230" g="219" b="116"/>
  
  <!-- Backgrounds (bgr/bgg/bgb = background | fgr/fgg/fgb = text) -->
  <bghdr  bgr="39" bgg="40" bgb="34" fgr="102" fgg="217" fgb="239"/>
  <bgsel  bgr="73" bgg="72" bgb="62" fgr="248" fgg="248" fgb="242"/>
  <bgplay bgr="30" bgg="44" bgb="18" fgr="166" fgg="226" fgb="46"/>
  <bgstat bgr="29" bgg="29" bgb="24" fgr="117" fgg="113" fgb="94"/>
</theme>
```
*New themes will be automatically detected when you restart the app.*

---
**MIT License** — Feel free to modify and use the code.
```
