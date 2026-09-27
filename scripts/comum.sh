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
