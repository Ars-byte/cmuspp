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

> [🇦🇷 Español](README.md) | [🇬🇧 English](README-EN.md) | 🇩🇪 Deutsch

**CMUS++** ist ein Terminal-Musikplayer, geschrieben in C++17. Extrem leicht, blitzschnell, komplett über die Tastatur steuerbar und ohne grafische Abhängigkeiten.

**Funktionen:**
*   **Formate:** MP3, FLAC, WAV, OGG, OPUS, AIFF.
*   **Albumcover:** Extrahiert eingebettete Cover aus MP3, FLAC, OGG/OPUS + Fallback auf cover.jpg/png.
*   **Plattformübergreifend:** Linux (ALSA), macOS (CoreAudio), Windows (WinMM).
*   **Anpassbar:** 86 integrierte Farbschemata + Unterstützung für benutzerdefinierte XML-Designs.
*   **Schnell:** Keine Ladezeiten, effiziente Dekodierung und flackerfreie Darstellung.

---

## Vorschau

| Player (mit .xml-Design) |
|:---:|
|<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/2b6cf564-ecc1-4422-90fc-3db0f896fb4d" />|

---

## 🖼️ Albumcover

CMUS++ extrahiert und zeigt Albumcover automatisch an:

*   **MP3:** APIC-Frame (ID3v2.2/2.3/2.4)
*   **FLAC:** METADATA_BLOCK_PICTURE
*   **OGG / OPUS:** METADATA_BLOCK_PICTURE in Vorbis-Kommentaren
*   **Fallback:** `cover.jpg`, `cover.png`, `folder.jpg`, etc. im gleichen Verzeichnis

**Kompatible Terminals:**
*   **Kitty / WezTerm / ghostty / iTerm2:** Native Bildanzeige (Kitty-Protokoll)
*   **Andere Terminals (Alacritty, GNOME, etc.):** ANSI-Halbblock-Darstellung (▄) mit True-Color

---

## Installation & Kompilierung

### 1. Abhängigkeiten installieren

Du benötigst einen C++-Compiler sowie `libsndfile`, `libjpeg` und `libpng` (plus ALSA unter Linux).

*   **Ubuntu / Debian:** `sudo apt install g++ libsndfile1-dev libasound2-dev libjpeg-dev libpng-dev`
*   **Arch Linux:** `sudo pacman -S gcc libsndfile alsa-lib libjpeg-turbo libpng`
*   **macOS:** `brew install libsndfile jpeg libpng` (erfordert Homebrew)

### 2. Kompilieren

Klone das Repository und verwende das enthaltene Auto-Build-Skript:

```bash
# Ausführungsrechte erteilen
chmod +x bootstrap.sh

# Kompilieren
./bootstrap.sh

# Ausführen!
./cmuspp
```

---

## Für NIXOS

### Führe diesen Befehl aus:

```
nix profile add github:mikuri12/my-lazy-nixos-pkgs#cmuspp
```
### Falls du Flakes verwenden möchtest:

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

## Bedienung

CMUS++ ist für die vollständige Bedienung ohne Maus ausgelegt.

| Taste | Aktion |
| :--- | :--- |
| `↑` / `↓` (oder `k`/`j`) | In der Liste navigieren |
| `Enter` | Titel abspielen / Ordner öffnen |
| `Leertaste` | Pause / Fortsetzen |
| `←` / `→` (oder `h`/`l`) | 5s zurück/vor (Im Browser: Eine Ebene hoch) |
| `n` / `p` | Nächster / Vorheriger Titel |
| `+` / `-` | Lautstärke erhöhen/verringern |
| `s` | Zufallswiedergabe umschalten |
| `r` | Wiederholung umschalten |
| `t` | Farbschema wechseln |
| `o` | Dateibrowser öffnen |
| `a` | Info anzeigen (Über) |
| `q` | Beenden |

---

## Benutzerdefinierte Designs

Drücke `t` in der App, um das Design zu wechseln. Du kannst eigene Designs erstellen, indem du `.xml`-Dateien im Ordner `themes/` (neben der ausführbaren Datei) oder in `~/.config/cmuspp/themes/` ablegst.

**Strukturbeispiel (`themes/mein-design.xml`):**
```xml
<?xml version="1.0" encoding="UTF-8"?>
<theme name="Mein Eigenes Design">
  <!-- Textfarben (Hell -> Dunkel) -->
  <fg0 r="248" g="248" b="242"/> <fg1 r="215" g="210" b="195"/>
  <fg2 r="117" g="113" b="94"/>  <fg3 r="75"  g="71"  b="60"/>
  
  <!-- Akzente -->
  <acc  r="166" g="226" b="46"/> <warn r="230" g="219" b="116"/>
  
  <!-- Hintergründe (bgr/bgg/bgb = Hintergrund | fgr/fgg/fgb = Text) -->
  <bghdr  bgr="39" bgg="40" bgb="34" fgr="102" fgg="217" fgb="239"/>
  <bgsel  bgr="73" bgg="72" bgb="62" fgr="248" fgg="248" fgb="242"/>
  <bgplay bgr="30" bgg="44" bgb="18" fgr="166" fgg="226" fgb="46"/>
  <bgstat bgr="29" bgg="29" bgb="24" fgr="117" fgg="113" fgb="94"/>
</theme>
```
*Neue Designs werden automatisch beim Neustart der App erkannt.*

---

**MIT-Lizenz** — Du darfst den Code frei modifizieren und nutzen.
