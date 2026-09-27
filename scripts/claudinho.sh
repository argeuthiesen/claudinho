#!/usr/bin/env bash
# Manutencao do Claudinho pela rede (depois de configurado).
#   claudinho.sh info                 estado atual (GET /mini.json)
#   claudinho.sh log                  log interno do aparelho
#   claudinho.sh cara <tipo> [humor]  dispara uma cara (inicio, prompt, ferramenta, erro, parou, atencao, compact, fim, dormir)
#   claudinho.sh cor R G B            cor do fundo ao vivo (nao persiste)
#   claudinho.sh reiniciar
#   claudinho.sh atualizar [arquivo.bin]   firmware pela rede (padrao: o do plugin); pede um toque na tela
#   claudinho.sh tela [arquivo.tft]   grava a tela no Nextion pela rede (padrao: a do plugin); pede um toque na tela
#   claudinho.sh consumo [segundos]   mostra a tela de consumo agora (padrao 20 s)
# O segredo nunca vai na linha de comando (ver curl_claudinho no comum.sh).
source "$(dirname "$0")/comum.sh"
IP=$(le_config CLAUDINHO_IP)
[ -n "$IP" ] || { echo "Claudinho ainda nao configurado (rode a skill /claudinho:configurar)" >&2; exit 1; }
J=(-H 'Content-Type: application/json')
uso() { sed -n '2,10p' "$0"; exit 1; }
num() { case "$1" in ''|*[!0-9]*) return 1 ;; esac; }
palavra() { case "$1" in *[!a-z]*) return 1 ;; esac; }      # so letras minusculas (ou vazio)
cmd() { curl_claudinho -s -m 10 "${J[@]}" -X POST "http://$IP/cmd" -d "$1"; }
mini() { curl_claudinho -s -m 5 "http://$IP/mini.json"; }

# Firmware e tela pela rede exigem alguem na frente do Claudinho: pede o
# toque e espera a placa liberar (firmware anterior a 1.0.9 nao pede).
libera() {
  local m i
  m=$(mini); case "$m" in *'"manut"'*) ;; *) return 0 ;; esac
  cmd '{"manutencao":true}' >/dev/null || { echo "o Claudinho nao respondeu" >&2; return 1; }
  echo "Toque na tela do Claudinho para permitir (ou aperte BOOT na placa)..."
  for i in $(seq 35); do
    sleep 2; m=$(mini)
    case "$m" in *'"manut":"liberada"'*) echo "liberado."; return 0 ;; esac
    case "$m" in *'"manut":""'*) [ "$i" -gt 3 ] && break ;; esac
  done
  echo "ninguem tocou a tempo; nada foi enviado" >&2; return 1
}

case "${1:-}" in
  info) mini ;;
  log)  curl_claudinho -s -m 5 "http://$IP/log" ;;
  cara)
    palavra "${2:-x}" && palavra "${3:-}" || uso
    curl_claudinho -s -m 10 "${J[@]}" -X POST "http://$IP/evento" -d "{\"tipo\":\"$2\",\"humor\":\"${3:-}\",\"sessao\":\"teste\"}" ;;
  cor)
    for v in "${2:-}" "${3:-}" "${4:-}"; do num "$v" && [ "$v" -le 255 ] || uso; done
    cmd "{\"cor\":[$2,$3,$4]}" ;;
  reiniciar) cmd '{"reiniciar":true}' ;;
  consumo) S="${2:-20}"; num "$S" || uso; cmd "{\"consumo\":$S}" ;;
  atualizar)
    CHIP=$(mini | grep -o '"placa":"[^"]*"' | cut -d'"' -f4)
    ARQ="${2:-$(ls "$RAIZ"/firmware/bin/claudinho-${CHIP:-esp32c3}-*-ota.bin 2>/dev/null | sort -V | tail -1)}"
    [ -f "$ARQ" ] || { echo "firmware nao encontrado" >&2; exit 1; }
    libera || exit 1
    echo "enviando $(basename "$ARQ") para $IP"
    curl_claudinho -s -m 180 -F "f=@$ARQ" "http://$IP/ota" ;;
  tela)
    ARQ="${2:-$RAIZ/nextion/NX3224F024_270.tft}"
    [ -f "$ARQ" ] || { echo "tela nao encontrada: $ARQ" >&2; exit 1; }
    TAM=$(wc -c < "$ARQ" | tr -d ' ')
    libera || exit 1
    echo "enviando $TAM bytes para $IP (uns 40 s)"
    curl_claudinho -s -m 300 -F "f=@$ARQ" "http://$IP/tft?tam=$TAM" ;;
  *) uso ;;
esac
