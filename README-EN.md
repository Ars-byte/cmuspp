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

> [ Español](README.md) | English | [ Deutsch](README-DE.md) | [ Português](README-PT-BR.md) | [ Français](README-FR.md) | [ Italiano](README-IT.md) | [ Русский](README-RU.md)

**CMUS++** is a terminal music player written in C++17. Ultra-lightweight, blazingly fast, keyboard-driven, and completely free of graphical dependencies.

**Features:**
*   **Formats:** MP3, FLAC, WAV, OGG, OPUS, AIFF.
*   **Album art:** Extracts embedded covers from MP3, FLAC, OGG/OPUS + fallback to cover.jpg/png.
*   **Lyrics:** Synchronized `.lrc` + lyrics embedded in the file tags (USLT/LYRICS), 100% offline.
*   **Cross-platform:** Linux (ALSA), macOS (CoreAudio), Windows (WinMM).
*   **Customizable:** 86 built-in themes + support for custom XML themes.
*   **Fast:** Zero loading times, efficient decoding, and flicker-free rendering.
*   **About window:** Press `a` to view project info, license and credits.

---

## Previews

<img width="800" src="previews/main.png" />
<img width="800" src="previews/review.png" />
<img width="500" src="previews/about.png" />

---

## Download (Release)

Just want to try it? Grab the prebuilt binary from the [**v1.1.0 release**](https://github.com/Ars-byte/cmuspp/releases/tag/v1.1.0) (Linux x86_64):

```bash
wget https://github.com/Ars-byte/cmuspp/releases/download/v1.1.0/cmuspp-linux-x86_64
chmod +x cmuspp-linux-x86_64
./cmuspp-linux-x86_64
```

> Requires the `themes/` folder (from the repo) next to the executable.

---

## Installation & Build

### 1. Install dependencies

You need a C++ compiler, `libsndfile`, `libjpeg` and `libpng` (plus ALSA on Linux).

*   **Ubuntu / Debian:** `sudo apt install g++ libsndfile1-dev libasound2-dev libjpeg-dev libpng-dev`
*   **Arch Linux:** `sudo pacman -S gcc libsndfile alsa-lib libjpeg-turbo libpng`
*   **macOS:** `brew install libsndfile jpeg libpng` (requires Homebrew)

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
## For NIXOS

### Run this command:

```
nix profile add github:mikuri12/my-lazy-nixos-pkgs#cmuspp
```
### If you want to use flakes:

```
inputs = {
my-pkgs.url = "github:mikuri12/my-lazy-nixos-pkgs";

}; 
```
```
{ inputs, pkgs, system, ... }:
{
environment.systemPackages = [
inputs.my-pkgs.packages.${system}.cmuspp

];

}
```

---

## Album Art

CMUS++ automatically extracts and displays album art:

*   **MP3:** APIC frame (ID3v2.2/2.3/2.4)
*   **FLAC:** METADATA_BLOCK_PICTURE
*   **OGG / OPUS:** METADATA_BLOCK_PICTURE in Vorbis comments
*   **Fallback:** `cover.jpg`, `cover.png`, `folder.jpg`, etc. in the same directory

**Compatible terminals:**
*   **Kitty / WezTerm / ghostty / iTerm2:** Native image display (Kitty protocol)
*   **Other terminals (Alacritty, GNOME, etc.):** ANSI half-block rendering (▄) with true-color

---

## Lyrics

Press `l` inside the player to view the song's lyrics full-screen (hides the list). 100% offline — no network, no external services. Lyrics are resolved like this:

1. **Synchronized `.lrc`** next to the song (e.g. `song.mp3` → `song.lrc`). The current line is highlighted and advances in sync with playback.
2. **Lyrics embedded in the file's tags** (USLT frame in MP3, LYRICS field in FLAC/OGG). Shown as static text, scrollable with `↑`/`↓`.

`.lrc` format:

```
[ti:Title]
[ar:Artist]
[00:12.50]First line
[00:24.00]Second line
```

Supports `[mm:ss]` / `[mm:ss.xx]` timestamps, multiple timestamps per line (`[00:12][00:24]text`) and the `[offset:±ms]` tag.

---

## Controls

CMUS++ is designed to be used entirely without a mouse.

| Key | Action |
| :--- | :--- |
| `↑` / `↓` (or `k`/`j`) | Navigate list |
| `Enter` | Play track / Enter folder |
| `Space` | Pause / Resume |
| `←` / `→` (or `h`) | Seek backward/forward 5s (In browser: Go up a level) |
| `n` / `p` | Next / Previous track |
| `+` / `-` | Volume Up / Down |
| `s` | Toggle Shuffle |
| `r` | Toggle Repeat |
| `l` | Show/hide lyrics (.lrc or embedded tags) full-screen |
| `/` | Search a track in the current folder (type to filter, `Enter` plays) |
| `t` | Cycle color theme |
| `o` | Open file browser |
| `a` | Show info (About) |
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
