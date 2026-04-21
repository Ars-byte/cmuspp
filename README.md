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

> 🇦🇷 Español | [🇬🇧 English](README-EN.md)

**CMUS++** es un reproductor de música para terminal escrito en C++17. Ultraligero, rapidísimo, controlado por teclado y sin dependencias gráficas.

**Características:**
*   **Formatos:** MP3, FLAC, WAV, OGG, OPUS, AIFF.
*   **Multiplataforma:** Linux (ALSA), macOS (CoreAudio), Windows (WinMM).
*   **Personalizable:** 9 temas integrados + soporte para temas XML personalizados.
*   **Rápido:** Cero tiempos de carga, decodificación eficiente y renderizado sin parpadeos.

---

## Previews



| Reproductor (Con un theme .xml) |
|:---:|
|<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/1af7496e-e625-49ea-a0df-276b549e8e7d" />



---

##  Instalación y Compilación

### 1. Instalar dependencias

Solo necesitas un compilador C++ y `libsndfile` (y ALSA en Linux).

*   **Ubuntu / Debian:** `sudo apt install g++ libsndfile1-dev libasound2-dev`
*   **Arch Linux:** `sudo pacman -S gcc libsndfile alsa-lib`
*   **macOS:** `brew install libsndfile` (requiere Homebrew)
  
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

## Controles

CMUS++ está diseñado para usarse completamente sin ratón.

| Tecla | Acción |
| :--- | :--- |
| `↑` / `↓` (o `k`/`j`) | Navegar por la lista |
| `Enter` | Reproducir / Entrar a carpeta |
| `Space` | Pausar / Reanudar |
| `←` / `→` (o `h`/`l`) | Atrás 5s / Adelante 5s (En navegador: Subir nivel) |
| `n` / `p` | Siguiente / Anterior canción |
| `+` / `-` | Subir / Bajar volumen |
| `s` | Activar/Desactivar Shuffle (Aleatorio) |
| `r` | Activar/Desactivar Repeat (Bucle) |
| `t` | Cambiar tema de color |
| `o` | Abrir explorador de archivos |
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
