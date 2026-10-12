#!/usr/bin/env bash
# Funcoes comuns aos scripts do Claudinho. Funciona em Linux, macOS, WSL e
# Windows (Git Bash, que e o shell do Claude Code no Windows).

umask 077                       # tudo o que estes scripts criam (config, segredo, cache) so para o dono
# Pasta de dados: a do plugin (a mesma que os hooks usam) quando existir.
if [ -z "${CLAUDINHO_DADOS:-}" ] && [ -d "$HOME/.claude/plugins/data/claudinho-claudinho" ]; then
  CLAUDINHO_DADOS="$HOME/.claude/plugins/data/claudinho-claudinho"
fi
DADOS="${CLAUDINHO_DADOS:-$HOME/.claudinho}"
mkdir -p "$DADOS"
CONFIG="$DADOS/claudinho.env"
RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ESPTOOL_VERSAO="v5.4.0"

ambiente() {
  case "$(uname -s)" in
    Linux*)  if grep -qi microsoft /proc/version 2>/dev/null; then echo wsl; else echo linux; fi ;;
    Darwin*) echo mac ;;
    MINGW*|MSYS*|CYGWIN*) echo windows ;;
    *) echo desconhecido ;;
  esac
}

# Caminho que um programa do Windows entende (para o esptool.exe).
caminho_windows() {
  case "$(ambiente)" in
    wsl) wslpath -w "$1" ;;
    windows) cygpath -w "$1" ;;
    *) echo "$1" ;;
  esac
}

# Roda um trecho de PowerShell sem arquivo intermediario (sem politica de execucao).
powershell_roda() {
  local b64; b64=$(printf '%s' "$1" | iconv -f UTF-8 -t UTF-16LE | base64 -w0 2>/dev/null || printf '%s' "$1" | iconv -f UTF-8 -t UTF-16LE | base64 | tr -d '\n')
  powershell.exe -NoProfile -NonInteractive -EncodedCommand "$b64" 2>/dev/null | iconv -f UTF-8 -t UTF-8 -c | tr -d '\r'
  return "${PIPESTATUS[0]}"      # o codigo de saida do PowerShell, nao o do tr
}

le_config() { [ -f "$CONFIG" ] && sed -n "s/^$1=//p" "$CONFIG" | tail -1; }

grava_config() {   # grava_config CHAVE valor
  local tmp; tmp=$(mktemp "$DADOS/.claudinho.env.XXXXXX") || return 1   # nasce 600 (umask 077)
  [ -f "$CONFIG" ] && { grep -v "^$1=" "$CONFIG" > "$tmp" || true; }
  printf '%s=%s\n' "$1" "$2" >> "$tmp"; chmod 600 "$tmp"; mv "$tmp" "$CONFIG"
}

# curl com o segredo do Claudinho. O cabecalho vai por configuracao na entrada
# padrao (-K -), nunca na linha de comando, onde outros processos o veriam.
curl_claudinho() {
  printf 'header = "Authorization: Bearer %s"\n' "$(le_config CLAUDINHO_TOKEN)" | curl -K - "$@"
}

sha256_de() {
  if command -v sha256sum >/dev/null 2>&1; then sha256sum "$1" | cut -d' ' -f1; else shasum -a 256 "$1" | cut -d' ' -f1; fi
}

# SHA-256 de um arquivo (sha256sum, shasum ou Python, o que existir).
sha256_de() {
  if command -v sha256sum >/dev/null 2>&1; then sha256sum "$1" | cut -d' ' -f1
  elif command -v shasum >/dev/null 2>&1; then shasum -a 256 "$1" | cut -d' ' -f1
  else python3 -c 'import hashlib,sys;print(hashlib.sha256(open(sys.argv[1],"rb").read()).hexdigest())' "$1"; fi
}
# Confere um firmware contra o SHA256SUMS da pasta dele (o compilar.sh gera).
# Arquivo que nao esta na lista (um .bin seu, de outro lugar) passa sem conferir.
confere_sha() {
  local lista esperado
  lista="$(dirname "$1")/SHA256SUMS"
  [ -f "$lista" ] || return 0
  esperado=$(tr -d '\r' < "$lista" | grep " $(basename "$1")\$" | cut -d' ' -f1)   # sem \r: checkout no Windows
  [ -n "$esperado" ] || return 0
  [ "$(sha256_de "$1" | tr -d '\r')" = "$esperado" ] && return 0
  echo "o firmware $(basename "$1") nao confere com o SHA256SUMS (arquivo corrompido ou alterado); nada foi gravado" >&2
  return 1
}
