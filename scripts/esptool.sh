#!/usr/bin/env bash
# Roda o esptool (ferramenta oficial da Espressif) com os argumentos dados.
# Usa o que ja estiver instalado; senao baixa o binario oficial do sistema
# (sem precisar de Python). No WSL usa a versao Windows, porque a porta USB
# so existe do lado do Windows.
source "$(dirname "$0")/comum.sh"
AMB=$(ambiente)
if [ "$AMB" != wsl ] && [ "$AMB" != windows ]; then
  for c in esptool esptool.py; do command -v "$c" >/dev/null 2>&1 && exec "$c" "$@"; done
fi
case "$AMB" in
  wsl|windows) ALVO=windows-amd64; EXE=esptool.exe ;;
  linux) case "$(uname -m)" in aarch64|arm64) ALVO=linux-aarch64 ;; armv7*) ALVO=linux-armv7 ;; *) ALVO=linux-amd64 ;; esac; EXE=esptool ;;
  mac) case "$(uname -m)" in arm64) ALVO=macos-arm64 ;; *) ALVO=macos-amd64 ;; esac; EXE=esptool ;;
  *) echo "ambiente nao suportado" >&2; exit 1 ;;
esac
# SHA-256 oficiais dos pacotes da versao fixada (API de releases do GitHub).
# O download so e usado se bater com isto; mudou a versao, atualize a lista.
case "$ALVO" in
  linux-aarch64) SHA=2964fff085071c1403f2cf812a7a1d425f987f9992851a60236bbee17b6e7dcc ;;
  linux-amd64)   SHA=61648fbae20735cabb342f2fbe8fc89b3046e1ed6f9c3e09528d837dc9a9b152 ;;
  linux-armv7)   SHA=ee542ac6b60aee2604289ee418fccd0ff6eead6f702b51bbe4f0e483477e31d9 ;;
  macos-amd64)   SHA=910bb64fe39a84c792752701293c8aa294faeef229fe8705ecd6955b01db3778 ;;
  macos-arm64)   SHA=ba332671130939e2e6db90c2784488f7e62a1459b0fe3c5ec66e9a366821de7a ;;
  windows-amd64) SHA=b7f6b9dd301a210b31f4829118c909c84aae23107f9ca1fdc14ccf4d7384be2e ;;
esac
DIR="$DADOS/esptool-$ESPTOOL_VERSAO-$ALVO"
BIN=$(find "$DIR" -name "$EXE" -type f 2>/dev/null | head -1)
[ -f "$DIR/.verificado" ] || BIN=""          # cache de antes da verificacao: baixa de novo
if [ -z "$BIN" ]; then
  echo "baixando esptool $ESPTOOL_VERSAO ($ALVO)..." >&2
  rm -rf "$DIR"; mkdir -p "$DIR"
  URL="https://github.com/espressif/esptool/releases/download/$ESPTOOL_VERSAO/esptool-$ESPTOOL_VERSAO-$ALVO"
  if [ "$ALVO" = windows-amd64 ]; then PACOTE="$DIR/e.zip"; URL="$URL.zip"; else PACOTE="$DIR/e.tar.gz"; URL="$URL.tar.gz"; fi
  curl -fsSL -o "$PACOTE" "$URL" || { echo "falha ao baixar o esptool" >&2; exit 1; }
  if [ "$(sha256_de "$PACOTE")" != "$SHA" ]; then
    rm -rf "$DIR"; echo "esptool baixado nao confere com o SHA-256 oficial; nada foi executado" >&2; exit 1
  fi
  if [ "$ALVO" = windows-amd64 ]; then
    (cd "$DIR" && { unzip -q e.zip 2>/dev/null || python3 -c 'import zipfile;zipfile.ZipFile("e.zip").extractall(".")'; })
  else
    tar -xzf "$PACOTE" -C "$DIR"
  fi
  rm -f "$PACOTE"; touch "$DIR/.verificado"
  BIN=$(find "$DIR" -name "$EXE" -type f | head -1); chmod +x "$BIN"
fi
if [ "$AMB" = wsl ]; then
  # o .exe precisa rodar a partir de um caminho do Windows (/mnt/c...) para achar suas DLLs
  WINTMP="$(wslpath "$(powershell_roda '[System.IO.Path]::GetTempPath()' | head -1)")/claudinho-esptool"
  # copia de novo se a do Windows nao for identica a verificada
  if ! cmp -s "$BIN" "$WINTMP/$EXE"; then rm -rf "$WINTMP"; mkdir -p "$WINTMP"; cp -r "$(dirname "$BIN")"/. "$WINTMP/"; fi
  BIN="$WINTMP/$EXE"
fi
exec "$BIN" "$@"
