#!/usr/bin/env bash
# Grava o firmware do Claudinho pelo USB (primeira vez). Detecta o chip e
# escolhe o .bin certo. Uso: gravar.sh [PORTA]
# Reinicia pelo watchdog no fim: no C3 com USB nativo, se a placa entrou em
# gravacao pelo botao BOOT, o reset comum (RTS) a deixa presa no bootloader.
source "$(dirname "$0")/comum.sh"
PORTA="${1:-$("$(dirname "$0")/porta.sh")}" || exit 1
echo "porta: $PORTA"
# O programa que ja esta na placa pode deixar a porta num estado que nao abre
# ("Cannot configure port"). Tenta algumas vezes; se nao der, a saida e o modo
# de gravacao pelo botao BOOT.
CHIP=""
for tentativa in 1 2 3; do
  # --after no-reset: sem isso a placa sai do modo de gravacao antes de gravar
  SAIDA=$("$(dirname "$0")/esptool.sh" --port "$PORTA" --after no-reset chip-id 2>&1 | tr -d '\r')
  CHIP=$(printf '%s' "$SAIDA" | grep -ioE 'ESP32-?(C3|S3)' | head -1 | tr 'A-Z' 'a-z' | tr -d '-')
  [ -n "$CHIP" ] && break
  sleep 3
done
if [ -z "$CHIP" ]; then
  if printf '%s' "$SAIDA" | grep -qiE 'chip (type|is)'; then
    echo "chip nao suportado (so ESP32-C3 e ESP32-S3): $(printf '%s' "$SAIDA" | grep -iE 'chip (type|is)' | head -1)" >&2
  else
    echo "PRECISA_BOOT: a porta nao abriu. Segure o botao BOOT, tire e ponha o USB, solte o BOOT e rode de novo." >&2
  fi
  exit 1
fi
BINARIO=$(ls "$RAIZ"/firmware/bin/claudinho-$CHIP-*-completo.bin | sort | tail -1)
echo "chip: $CHIP  firmware: $(basename "$BINARIO")"
"$(dirname "$0")/esptool.sh" --chip "$CHIP" --port "$PORTA" --baud 460800 --after watchdog-reset write-flash 0x0 "$(caminho_windows "$BINARIO")" 2>&1 | tr -d '\r' | grep -a -E "Wrote|Hash of data|Hard reset|rror" || true
