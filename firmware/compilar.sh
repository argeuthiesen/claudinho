#!/usr/bin/env bash
# Recompila o firmware para C3, S3 (Nextion) e E32R28T e atualiza firmware/bin/. So para quem
# mexe no codigo: quem so usa o Claudinho recebe os .bin prontos.
# Requer arduino-cli com o core esp32 e as bibliotecas ArduinoJson (v7) e
# LovyanGFX (so a E32R28T usa).
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
V=$(sed -n 's/^#define VERSAO *"\(.*\)"/\1/p' "$DIR/claudinho/config.h")
mkdir -p "$DIR/bin"
# placa | FQBN | defines extras
for ALVO in "esp32c3|esp32:esp32:esp32c3:CDCOnBoot=cdc,PartitionScheme=min_spiffs|" \
            "esp32s3|esp32:esp32:esp32s3:CDCOnBoot=cdc,PartitionScheme=min_spiffs|" \
            "e32r28t|esp32:esp32:esp32:PartitionScheme=min_spiffs|-DPLACA_E32R28T"; do
  IFS='|' read -r CHIP FQBN DEFS <<< "$ALVO"
  echo "==> $CHIP $V"
  # -fmacro-prefix-map: tira do binario os caminhos da maquina de quem compila
  # (o __FILE__ das mensagens internas de erro levaria o nome do usuario)
  PM="-fmacro-prefix-map=$HOME/=~/ -fmacro-prefix-map=$DIR/= $DEFS"
  arduino-cli compile --clean --fqbn "$FQBN" --export-binaries --output-dir "$DIR/build/$CHIP" \
    --build-property "compiler.c.extra_flags=$PM" --build-property "compiler.cpp.extra_flags=$PM" \
    "$DIR/claudinho" | grep -E "Sketch uses"
  rm -f "$DIR"/bin/claudinho-$CHIP-*.bin
  cp "$DIR/build/$CHIP/claudinho.ino.merged.bin" "$DIR/bin/claudinho-$CHIP-$V-completo.bin"
  cp "$DIR/build/$CHIP/claudinho.ino.bin"        "$DIR/bin/claudinho-$CHIP-$V-ota.bin"
done
rm -rf "$DIR/build"
ls -la "$DIR/bin"
