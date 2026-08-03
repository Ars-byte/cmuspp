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

> [ Español](README.md) | [ English](README-EN.md) | [ Deutsch](README-DE.md) | [ Português](README-PT-BR.md) | [ Français](README-FR.md) | Italiano | [ Русский](README-RU.md)

**CMUS++** è un riproduttore musicale per terminale scritto in C++17. Ultra-leggero, velocissimo, controllato da tastiera e senza dipendenze grafiche.

**Caratteristiche:**
*   **Formati:** MP3, FLAC, WAV, OGG, OPUS, AIFF.
*   **Copertine:** Estrazione delle copertine incorporate da MP3, FLAC, OGG/OPUS + fallback su cover.jpg/png.
*   **Testi:** `.lrc` sincronizzati + testi incorporati nei tag (USLT/LYRICS), 100% offline.
*   **Multipiattaforma:** Linux (ALSA), macOS (CoreAudio), Windows (WinMM).
*   **Personalizzabile:** 86 temi integrati + supporto per temi XML personalizzati.
*   **Veloce:** Nessun tempo di caricamento, decodifica efficiente e rendering senza sfarfallio.
*   **Finestra Info:** Premi `a` per visualizzare le informazioni del progetto, la licenza e i crediti.

---

## Anteprime

<img width="800" src="previews/main.png" />
<img width="800" src="previews/review.png" />
<img width="500" src="previews/about.png" />

---

## Download (Release)

Vuoi solo provarlo? Scarica il binario precompilato dalla [**release v1.1.0**](https://github.com/Ars-byte/cmuspp/releases/tag/v1.1.0) (Linux x86_64):

```bash
wget https://github.com/Ars-byte/cmuspp/releases/download/v1.1.0/cmuspp-linux-x86_64
chmod +x cmuspp-linux-x86_64
./cmuspp-linux-x86_64
```

> Richiede la cartella `themes/` (quella del repository) accanto all'eseguibile.

---

## Installazione XBPS (Void Linux)

Su Void Linux puoi installare CMUS++ dal pacchetto `.xbps` precompilato:

```bash
sudo xbps-install -S --repository=https://github.com/Ars-byte/cmuspp/releases/download/v1.1.1 cmuspp-void
```

Il pacchetto installa il binario in `/usr/bin/cmuspp`, gli 86 temi integrati e i temi XML aggiuntivi in `/usr/share/cmuspp/themes/`.

---

## Installazione e Compilazione

### 1. Installare le dipendenze

Ti serve un compilatore C++ e le librerie `libsndfile`, `libjpeg` e `libpng` (oltre ad ALSA su Linux).

*   **Ubuntu / Debian:** `sudo apt install g++ libsndfile1-dev libasound2-dev libjpeg-dev libpng-dev`
*   **Arch Linux:** `sudo pacman -S gcc libsndfile alsa-lib libjpeg-turbo libpng`
*   **macOS:** `brew install libsndfile jpeg libpng` (richiede Homebrew)

### 2. Compilare

Clona il repository e usa lo script di auto-compilazione incluso:

```bash
# Concedi i permessi di esecuzione
chmod +x bootstrap.sh

# Compila
./bootstrap.sh

# Esegui!
./cmuspp
```

---

## Per NixOS

Questo repository è un flake Nix. Per provarlo senza installarlo:

```
nix run github:Ars-byte/cmuspp
```

Per installarlo a livello di sistema, aggiungi il flake come input:

```
{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    cmuspp.url = "github:Ars-byte/cmuspp";
  };
}
```

```
{ inputs, pkgs, ... }:
{
  environment.systemPackages = [ inputs.cmuspp.packages.${pkgs.system}.cmuspp ];
}
```

Oppure installalo in modo imperativo:

```
nix profile install github:Ars-byte/cmuspp
```

Puoi anche installarlo dal flake di mikuri (collaboratore del progetto):

```
nix profile add github:mikuri12/my-lazy-nixos-pkgs#cmuspp
```


---

## Copertine degli Album

CMUS++ estrae e mostra automaticamente le copertine degli album:

*   **MP3:** Frame APIC (ID3v2.2/2.3/2.4)
*   **FLAC:** METADATA_BLOCK_PICTURE
*   **OGG / OPUS:** METADATA_BLOCK_PICTURE nei commenti Vorbis
*   **Fallback:** `cover.jpg`, `cover.png`, `folder.jpg`, ecc. nella stessa directory

**Terminali compatibili:**
*   **Kitty / WezTerm / ghostty / iTerm2:** Visualizzazione immagine nativa (protocollo Kitty)
*   **Altri terminali (Alacritty, GNOME, ecc.):** Rendering ANSI half-block (▄) con true-color

---

## Testi

Premi `l` nel riproduttore per vedere il testo della canzone a schermo intero (nasconde la lista). 100% offline — nessuna rete, nessun servizio esterno. I testi vengono risolti così:

1. **`.lrc` sincronizzato** accanto alla canzone (es. `canzone.mp3` → `canzone.lrc`). La riga corrente è evidenziata e avanza in sincronia con la riproduzione.
2. **Testi incorporati nei tag** del file stesso (frame `USLT` in MP3, campo `LYRICS` in FLAC/OGG). Mostrati come testo statico, scorribili con `↑`/`↓`.

Formato `.lrc`:

```
[ti:Titolo]
[ar:Artista]
[00:12.50]Prima riga
[00:24.00]Seconda riga
```

Supporta timestamp `[mm:ss]` / `[mm:ss.xx]`, più timestamp per riga (`[00:12][00:24]testo`) e il tag `[offset:±ms]`.

---

## Comandi

CMUS++ è progettato per essere usato interamente senza mouse.

| Tasto | Azione |
| :--- | :--- |
| `↑` / `↓` (o `k`/`j`) | Navigare nella lista |
| `Enter` | Riprodurre / Entrare nella cartella |
| `Spazio` | Pausa / Riprendi |
| `←` / `→` (o `h`) | Indietro 5s / Avanti 5s (Nel browser: Salire di un livello) |
| `n` / `p` | Traccia successiva / Precedente |
| `+` / `-` | Alza / Abbassa volume |
| `s` | Attiva/Disattiva Shuffle (Casuale) |
| `r` | Attiva/Disattiva Repeat (Ripetizione) |
| `l` | Mostra/nascondi testi (.lrc o incorporati) a schermo intero |
| `/` | Cerca una traccia nella cartella corrente (digita per filtrare, `Enter` riproduce) |
| `t` | Cambia il tema dei colori |
| `o` | Apri il file browser |
| `a` | Mostra informazioni (Info) |
| `q` | Esci |

---

## Temi Personalizzati

Premi `t` nell'app per cambiare tema. Puoi creare i tuoi temi creando file `.xml` nella cartella `themes/` (accanto all'eseguibile) o in `~/.config/cmuspp/themes/`.

**Esempio di struttura (`themes/mio-tema.xml`):**
```xml
<?xml version="1.0" encoding="UTF-8"?>
<theme name="Il Mio Tema Personalizzato">
  <!-- Colori del testo (Chiaro -> Scuro) -->
  <fg0 r="248" g="248" b="242"/> <fg1 r="215" g="210" b="195"/>
  <fg2 r="117" g="113" b="94"/>  <fg3 r="75"  g="71"  b="60"/>
  
  <!-- Accenti -->
  <acc  r="166" g="226" b="46"/> <warn r="230" g="219" b="116"/>
  
  <!-- Sfondi (bgr/bgg/bgb = sfondo | fgr/fgg/fgb = testo) -->
  <bghdr  bgr="39" bgg="40" bgb="34" fgr="102" fgg="217" fgb="239"/>
  <bgsel  bgr="73" bgg="72" bgb="62" fgr="248" fgg="248" fgb="242"/>
  <bgplay bgr="30" bgg="44" bgb="18" fgr="166" fgg="226" fgb="46"/>
  <bgstat bgr="29" bgg="29" bgb="24" fgr="117" fgg="113" fgb="94"/>
</theme>
```
*I nuovi temi verranno rilevati automaticamente al riavvio dell'app.*

---
**Licenza MIT** — Sentiti libero di modificare e usare il codice.
