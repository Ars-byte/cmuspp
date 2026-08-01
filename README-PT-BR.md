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

> [🇦🇷 Español](README.md) | [🇬🇧 English](README-EN.md) | [🇩🇪 Deutsch](README-DE.md) | 🇧🇷 Português | [🇫🇷 Français](README-FR.md)

**CMUS++** é um player de música para terminal escrito em C++17. Ultraleve, rapidíssimo, controlado pelo teclado e sem dependências gráficas.

**Recursos:**
*   **Formatos:** MP3, FLAC, WAV, OGG, OPUS, AIFF.
*   **Capa do álbum:** Extrai capas incorporadas de MP3, FLAC, OGG/OPUS + fallback para cover.jpg/png.
*   **Multiplataforma:** Linux (ALSA), macOS (CoreAudio), Windows (WinMM).
*   **Personalizável:** 86 temas integrados + suporte para temas XML personalizados.
*   **Rápido:** Sem tempo de carregamento, decodificação eficiente e renderização sem cintilação.
*   **Janela Sobre:** Pressione `a` para ver informações do projeto, licença e créditos.

---

## Previews

| Player (com tema .xml) |
|:---:|
|<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/2b6cf564-ecc1-4422-90fc-3db0f896fb4d" />|

---

## Download (Release)

Quer só testar? Baixe o binário pré-compilado do [**release v1.0.0**](https://github.com/Ars-byte/cmuspp/releases/tag/v1.0.0) (Linux x86_64):

```bash
wget https://github.com/Ars-byte/cmuspp/releases/download/v1.0.0/cmuspp-linux-x86_64
chmod +x cmuspp-linux-x86_64
./cmuspp-linux-x86_64
```

> É necessário ter a pasta `themes/` (a do repositório) ao lado do executável.

---

## Instalação & Compilação

### 1. Instalar dependências

Você precisa de um compilador C++ e das bibliotecas `libsndfile`, `libjpeg` e `libpng` (além do ALSA no Linux).

*   **Ubuntu / Debian:** `sudo apt install g++ libsndfile1-dev libasound2-dev libjpeg-dev libpng-dev`
*   **Arch Linux:** `sudo pacman -S gcc libsndfile alsa-lib libjpeg-turbo libpng`
*   **macOS:** `brew install libsndfile jpeg libpng` (requer Homebrew)

### 2. Compilar

Clone o repositório e use o script de compilação automática incluído:

```bash
# Conceder permissão de execução
chmod +x bootstrap.sh

# Compilar
./bootstrap.sh

# Executar!
./cmuspp
```

---

## Para NIXOS

### Execute este comando:

```
nix profile add github:mikuri12/my-lazy-nixos-pkgs#cmuspp
```
### Se quiser usar flakes:

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

## 🖼️ Capa do Álbum

O CMUS++ extrai e exibe capas de álbuns automaticamente:

*   **MP3:** Frame APIC (ID3v2.2/2.3/2.4)
*   **FLAC:** METADATA_BLOCK_PICTURE
*   **OGG / OPUS:** METADATA_BLOCK_PICTURE nos comentários Vorbis
*   **Fallback:** `cover.jpg`, `cover.png`, `folder.jpg`, etc. no mesmo diretório

**Terminais compatíveis:**
*   **Kitty / WezTerm / ghostty / iTerm2:** Exibição de imagem nativa (protocolo Kitty)
*   **Outros terminais (Alacritty, GNOME, etc.):** Renderização ANSI half-block (▄) com true-color

---

## Controles

O CMUS++ foi projetado para ser usado totalmente sem mouse.

| Tecla | Ação |
| :--- | :--- |
| `↑` / `↓` (ou `k`/`j`) | Navegar pela lista |
| `Enter` | Reproduzir / Entrar na pasta |
| `Espaço` | Pausar / Retomar |
| `←` / `→` (ou `h`/`l`) | Voltar 5s / Avançar 5s (No navegador: Subir um nível) |
| `n` / `p` | Próxima / Anterior faixa |
| `+` / `-` | Aumentar / Diminuir volume |
| `s` | Ativar/Desativar Shuffle (Aleatório) |
| `r` | Ativar/Desativar Repeat (Repetição) |
| `/` | Buscar uma faixa na pasta atual (digite para filtrar, `Enter` reproduz) |
| `t` | Mudar o tema de cores |
| `o` | Abrir o explorador de arquivos |
| `a` | Ver informações (Sobre) |
| `q` | Sair |

---

## Temas Personalizados

Pressione `t` dentro do app para mudar de tema. Você pode criar seus próprios temas criando arquivos `.xml` dentro da pasta `themes/` (ao lado do executável) ou em `~/.config/cmuspp/themes/`.

**Exemplo de estrutura (`themes/meu-tema.xml`):**
```xml
<?xml version="1.0" encoding="UTF-8"?>
<theme name="Meu Tema Personalizado">
  <!-- Cores de texto (Claro -> Escuro) -->
  <fg0 r="248" g="248" b="242"/> <fg1 r="215" g="210" b="195"/>
  <fg2 r="117" g="113" b="94"/>  <fg3 r="75"  g="71"  b="60"/>
  
  <!-- Acentos -->
  <acc  r="166" g="226" b="46"/> <warn r="230" g="219" b="116"/>
  
  <!-- Fundos (bgr/bgg/bgb = fundo | fgr/fgg/fgb = texto) -->
  <bghdr  bgr="39" bgg="40" bgb="34" fgr="102" fgg="217" fgb="239"/>
  <bgsel  bgr="73" bgg="72" bgb="62" fgr="248" fgg="248" fgb="242"/>
  <bgplay bgr="30" bgg="44" bgb="18" fgr="166" fgg="226" fgb="46"/>
  <bgstat bgr="29" bgg="29" bgb="24" fgr="117" fgg="113" fgb="94"/>
</theme>
```
*Novos temas serão detectados automaticamente ao reiniciar o app.*

---
**Licença MIT** — Sinta-se à vontade para modificar e usar o código.
```
