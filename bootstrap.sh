#!/usr/bin/env bash
# bootstrap.sh — build CMUS++ (frontend/ + backend/ layout, optional cover art)
set -e

CXX="${CXX:-g++}"

CXXFLAGS="-std=c++17 -O2 -march=native -ffast-math -funroll-loops -flto \
          -Wall -Wextra -Wno-unused-parameter"

# ── Platform audio ────────────────────────────────────────────────────────────
case "$(uname -s)" in
  Linux)   BASE_LIBS="-lasound -lsndfile -lpthread" ;;
  Darwin)  BASE_LIBS="-lsndfile -framework AudioToolbox -framework CoreAudio -lpthread" ;;
  MINGW*|CYGWIN*|MSYS*) BASE_LIBS="-lsndfile -lwinmm" ;;
  *)       echo "Unsupported platform"; exit 1 ;;
esac

# ── Cover art: detect libjpeg + libpng via pkg-config ────────────────────────
COVER_FLAGS=""
COVER_LIBS=""
COVER_INFO=""

detect_lib() {
    local flag="$1" define="$2" lib="$3" name="$4"
    # Try several common pkg-config names (libjpeg-turbo varies by distro)
    local found=0
    for candidate in "$flag" "${flag}-turbo" "${flag}8" "${flag}62"; do
        if pkg-config --exists "$candidate" 2>/dev/null; then
            COVER_FLAGS+=" $(pkg-config --cflags "$candidate") -D${define}"
            COVER_LIBS+=" $(pkg-config --libs   "$candidate")"
            COVER_INFO+=" ${name}(pkg-config:${candidate})"
            found=1
            break
        fi
    done
    if [ "$found" -eq 0 ]; then
        # Header probe fallback
        local hdr
        case "$define" in
          *JPEG*) hdr="jpeglib.h" ;;
          *PNG*)  hdr="png.h"     ;;
        esac
        if echo "#include <${hdr}>" | ${CXX} -x c++ - -fsyntax-only 2>/dev/null; then
            COVER_FLAGS+=" -D${define}"
            COVER_LIBS+=" -l${lib}"
            COVER_INFO+=" ${name}(header-probe)"
        else
            COVER_INFO+=" [no ${name} — install lib${lib}-dev or libjpeg-turbo-devel]"
        fi
    fi
}

detect_lib "libjpeg"  "CMUSPP_HAS_JPEG" "jpeg" "JPEG"
detect_lib "libpng"   "CMUSPP_HAS_PNG"  "png"  "PNG"

echo "Building CMUS++ …"
echo "  Cover art decoders:${COVER_INFO}"

# -I. exposes both frontend/ and backend/ subdirectories relative to project root
${CXX} ${CXXFLAGS} ${COVER_FLAGS} \
    main.cpp \
    -I. \
    ${BASE_LIBS} ${COVER_LIBS} \
    -o cmuspp

echo ""
echo "Done → ./cmuspp"
echo ""
echo "Usage:"
echo "  ./cmuspp                 launch and browse from home directory"
echo ""
echo "Keyboard shortcuts:"
echo "  ↑↓ / j k     navigate       Space       pause/resume"
echo "  Enter         play           n / p       next / prev"
echo "  ← → / h l    seek ±5 s      + / -       volume"
echo "  s             shuffle        r           loop"
echo "  t             cycle theme    o           open folder"
echo "  q             quit"
echo ""
echo "Cover art  : embedded in MP3 (ID3v2 APIC), FLAC, OGG/OPUS,"
echo "             or cover.jpg / folder.jpg / album.jpg next to tracks."
echo "Custom themes: drop *.xml files in ./themes/ and restart."
