#!/bin/bash
# bootstrap.sh - Script de compilación para CMUS++

set -e # Detener el script si hay algún error

# Colores para la terminal
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}==> Iniciando compilación de CMUS++...${NC}"

# 1. Verificar estructura de carpetas
if [ ! -f "main.cpp" ] || [ ! -d "frontend" ] || [ ! -d "backend" ]; then
    echo -e "${RED}Error: Estructura de carpetas incorrecta.${NC}"
    echo "Asegúrate de que 'main.cpp', y las carpetas 'frontend' y 'backend' estén en este directorio."
    exit 1
fi

# 2. Detectar Compilador
CXX=""
if command -v g++ >/dev/null 2>&1; then
    CXX="g++"
elif command -v clang++ >/dev/null 2>&1; then
    CXX="clang++"
else
    echo -e "${RED}Error: No se encontró g++ ni clang++. Instala un compilador de C++.${NC}"
    exit 1
fi

# 3. Detectar Sistema Operativo y configurar Flags
OS="$(uname -s)"
FLAGS="-std=c++17 -O3 -Wall -Wextra -pthread"
MACROS="-DCMUSPP_HAS_JPEG -DCMUSPP_HAS_PNG"
LIBS="-lsndfile -lpng -ljpeg"

echo -e "Sistema detectado: ${GREEN}$OS${NC}"
echo -e "Compilador: ${GREEN}$CXX${NC}"

if [ "$OS" = "Linux" ]; then
    LIBS="$LIBS -lasound"
elif [ "$OS" = "Darwin" ]; then
    # En macOS necesitamos los frameworks de audio nativos
    LIBS="$LIBS -framework CoreAudio -framework AudioToolbox"
else
    echo -e "${RED}Sistema Operativo no soportado automáticamente por este script ($OS).${NC}"
    exit 1
fi

# 4. Crear carpeta de temas por defecto si no existe
if [ ! -d "themes" ]; then
    echo "Creando directorio de temas ('themes/')..."
    mkdir -p themes
fi

# 5. Compilar
echo -e "${BLUE}==> Compilando el código...${NC}"
COMANDO="$CXX main.cpp -o cmuspp $FLAGS $MACROS $LIBS"

echo "Ejecutando: $COMANDO"
if $COMANDO; then
    echo -e "${GREEN}==> ¡Compilación exitosa!${NC}"
    echo -e "Puedes ejecutar el reproductor usando: ${BLUE}./cmuspp${NC}"
else
    echo -e "${RED}==> Hubo un error durante la compilación.${NC}"
    echo "Asegúrate de tener instaladas las librerías: libsndfile, libasound (Linux), libjpeg y libpng."
    exit 1
fi
