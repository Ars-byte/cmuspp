```markdown
# CMUS++

```C++ Terminal Music Player
 ██████╗███╗   ███╗██╗   ██╗███████╗    ██╗  ██╗
██╔════╝████╗ ████║██║   ██║██╔════╝    ╚██╗██╔╝
██║     ██╔████╔██║██║   ██║███████╗     ╚███╔╝
██║     ██║╚██╔╝██║██║   ██║╚════██║     ██╔██╗
╚██████╗██║ ╚═╝ ██║╚██████╔╝███████║ ██╗██╔╝ ██╗
 ╚═════╝╚═╝     ╚═╝ ╚═════╝ ╚══════╝ ╚═╝╚═╝  ╚═╝
                    ++ C++ Terminal Music Player
```

> Español | [ English](README-EN.md) | [ Deutsch](README-DE.md) | [ Português](README-PT-BR.md) | [ Français](README-FR.md) | [ Italiano](README-IT.md) | [ Русский](README-RU.md)

**CMUS++** es un reproductor de música para terminal escrito en C++17. Ultraligero, rapidísimo, controlado por teclado y sin dependencias gráficas.

**Características:**
*   **Formatos:** MP3, FLAC, WAV, OGG, OPUS, AIFF.
*   **Carátulas:** Soporte para carátulas incrustadas en MP3, FLAC, OGG/OPUS + archivos cover.jpg/png.
*   **Letras:** `.lrc` sincronizadas + letras incrustadas en los tags (USLT/LYRICS), 100% offline.
*   **Multiplataforma:** Linux (ALSA), macOS (CoreAudio), Windows (WinMM).
*   **Personalizable:** 86 temas integrados + soporte para temas XML personalizados.
*   **Rápido:** Cero tiempos de carga, decodificación eficiente y renderizado sin parpadeos.
*   **Info integrada:** Presioná `a` para abrir la ventana de información (About) del programa.

---

## Previews



| Reproductor | Reproductor |
|:---:|:---:|
|<img width="800" alt="vista principal" src="previews/main.png" />|<img width="800" alt="vista principal" src="previews/review.png" />|

| Ventana About (Información) |
|:---:|
|<img width="500" alt="ventana about" src="previews/about.png" />|



---

## Descargar (Release)

¿Solo querés probarlo? Descargá el binario precompilado desde el [**release v1.1.0**](https://github.com/Ars-byte/cmuspp/releases/tag/v1.1.0) (Linux x86_64):

```bash
wget https://github.com/Ars-byte/cmuspp/releases/download/v1.1.0/cmuspp-linux-x86_64
chmod +x cmuspp-linux-x86_64
./cmuspp-linux-x86_64
```

> Necesita la carpeta `themes/` (la del repositorio) junto al ejecutable.

---

## Instalación y Compilación

### 1. Instalar dependencias

Necesitas un compilador C++ y las librerías `libsndfile`, `libjpeg` y `libpng` (y ALSA en Linux).

*   **Ubuntu / Debian:** `sudo apt install g++ libsndfile1-dev libasound2-dev libjpeg-dev libpng-dev`
*   **Arch Linux:** `sudo pacman -S gcc libsndfile alsa-lib libjpeg-turbo libpng`
*   **macOS:** `brew install libsndfile jpeg libpng` (requiere Homebrew)
  
### 2. Compilar

Clona el repositorio y usa el script de auto-compilación incluido:

```bash
# Dar permisos de ejecución
chmod +x bootstrap.sh

# Compilar
./bootstrap.sh

# ¡Ejecutar!
./cmuspp
```

---

## Para NIXOS  

### Ejuecuta este comando:

```
nix profile add github:mikuri12/my-lazy-nixos-pkgs#cmuspp
```
### si quieres usar flakes:

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

## Carátulas

CMUS++ extrae y muestra carátulas de álbumes automáticamente:

*   **MP3:** Frame APIC (ID3v2.2/2.3/2.4)
*   **FLAC:** METADATA_BLOCK_PICTURE
*   **OGG / OPUS:** METADATA_BLOCK_PICTURE en comentarios Vorbis
*   **Fallback:** `cover.jpg`, `cover.png`, `folder.jpg`, etc. en el mismo directorio

**Terminales compatibles:**
*   **Kitty / WezTerm / ghostty / iTerm2:** Imagen nativa (protocolo Kitty)
*   **Otras terminales (Alacritty, GNOME, etc.):** Renderizado ANSI half-block (▄) con color true-color

---

## Letras

Presioná `l` dentro del reproductor para ver la letra de la canción a pantalla completa (oculta la lista). 100% offline, sin red ni servicios externos. Las letras se resuelven así:

1. **`.lrc` sincronizado** al lado de la canción (ej: `cancion.mp3` → `cancion.lrc`). La línea actual se resalta y avanza sincronizada con la reproducción.
2. **Letras incrustadas en los tags** del propio archivo (frame `USLT` en MP3, campo `LYRICS` en FLAC/OGG). Se muestran como texto estático y se navegan con `↑`/`↓`.

Formato `.lrc`:

```
[ti:Título]
[ar:Artista]
[00:12.50]Primera línea
[00:24.00]Segunda línea
```

Soporta timestamps `[mm:ss]` / `[mm:ss.xx]`, varias marcas por línea (`[00:12][00:24]texto`) y el tag `[offset:±ms]`.

---

## Controles

CMUS++ está diseñado para usarse completamente sin ratón.

| Tecla | Acción |
| :--- | :--- |
| `↑` / `↓` (o `k`/`j`) | Navegar por la lista |
| `Enter` | Reproducir / Entrar a carpeta |
| `Space` | Pausar / Reanudar |
| `←` / `→` (o `h`) | Atrás 5s / Adelante 5s (En navegador: Subir nivel) |
| `n` / `p` | Siguiente / Anterior canción |
| `+` / `-` | Subir / Bajar volumen |
| `s` | Activar/Desactivar Shuffle (Aleatorio) |
| `r` | Activar/Desactivar Repeat (Bucle) |
| `l` | Mostrar/ocultar letra (.lrc o incrustada en tags) a pantalla completa |
| `/` | Buscar una canción en la carpeta actual (escribe para filtrar, `Enter` reproduce) |
| `t` | Cambiar tema de color |
| `o` | Abrir explorador de archivos |
| `a` | Ver información (About) |
| `q` | Salir |

---

## Temas Personalizados

Presiona `t` dentro de la app para cambiar de tema. Puedes crear tus propios temas creando archivos `.xml` dentro de la carpeta `themes/` (junto al ejecutable) o en `~/.config/cmuspp/themes/`.

**Ejemplo de estructura (`themes/mi-tema.xml`):**
```xml
<?xml version="1.0" encoding="UTF-8"?>
<theme name="Mi Tema Custom">
  <!-- Colores de texto (Brillante -> Oscuro) -->
  <fg0 r="248" g="248" b="242"/> <fg1 r="215" g="210" b="195"/>
  <fg2 r="117" g="113" b="94"/>  <fg3 r="75"  g="71"  b="60"/>
  
  <!-- Acentos -->
  <acc  r="166" g="226" b="46"/> <warn r="230" g="219" b="116"/>
  
  <!-- Fondos (bgr/bgg/bgb = fondo | fgr/fgg/fgb = texto) -->
  <bghdr  bgr="39" bgg="40" bgb="34" fgr="102" fgg="217" fgb="239"/>
  <bgsel  bgr="73" bgg="72" bgb="62" fgr="248" fgg="248" fgb="242"/>
  <bgplay bgr="30" bgg="44" bgb="18" fgr="166" fgg="226" fgb="46"/>
  <bgstat bgr="29" bgg="29" bgb="24" fgr="117" fgg="113" fgb="94"/>
</theme>
```
*Los temas nuevos se detectarán automáticamente al reiniciar la app.*

---
**Licencia MIT** — Siéntete libre de modificar y usar el código.
```
