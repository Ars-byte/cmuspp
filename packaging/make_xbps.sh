#!/bin/sh
# Empaqueta cmuspp como .xbps para Void Linux y lo indexa en packages/.
# Uso: ./packaging/make_xbps.sh [VERSION]   (default: 1.1.1)
set -eu

cd "$(dirname "$0")/.."   # raiz del repo

VER="${1:-1.1.1}"
PKG="cmuspp-${VER}_1"
ARCH="x86_64"
REPO_DIR="packages"

[ -f cmuspp ] || { echo "ERROR: compilá el binario primero (./bootstrap.sh)"; exit 1; }

rm -rf /tmp/cmuspp-pkg
mkdir -p /tmp/cmuspp-pkg/usr/bin
mkdir -p /tmp/cmuspp-pkg/usr/share/cmuspp/themes
mkdir -p /tmp/cmuspp-pkg/usr/share/licenses/cmuspp

cp cmuspp     /tmp/cmuspp-pkg/usr/bin/
cp LICENSE    /tmp/cmuspp-pkg/usr/share/licenses/cmuspp/
cp themes/*.xml /tmp/cmuspp-pkg/usr/share/cmuspp/themes/

xbps-create -A "$ARCH" -n "${PKG}" \
  -s "C++ terminal music player with cover art, lyrics and themes" \
  -S "CMUS++ is a lightweight C++17 terminal music player. Features: MP3/FLAC/WAV/OGG/OPUS/AIFF, embedded cover art (kitty protocol or ANSI half-blocks), synchronized .lrc lyrics and lyrics embedded in tags (100% offline), 86 built-in color themes + custom XML themes, fully keyboard-driven." \
  -l "MIT" -H "https://github.com/Ars-byte/cmuspp" -m "Ars-byte" \
  -D "libsndfile alsa-lib libjpeg-turbo libpng" \
  /tmp/cmuspp-pkg

mkdir -p "${REPO_DIR}"
mv "${PKG}.${ARCH}.xbps" "${REPO_DIR}/"
xbps-rindex -a "${REPO_DIR}/${PKG}.${ARCH}.xbps"
rm -rf /tmp/cmuspp-pkg

echo "OK: ${REPO_DIR}/${PKG}.${ARCH}.xbps (repo index actualizado)"
