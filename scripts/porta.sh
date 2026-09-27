#!/usr/bin/env bash
# Acha a porta serial do ESP32 plugado no USB. Imprime a porta (COM6,
# /dev/ttyACM0, /dev/cu.usbmodem...) ou sai com erro.
source "$(dirname "$0")/comum.sh"
case "$(ambiente)" in
  wsl|windows)
    # 303A = Espressif (USB nativo do C3/S3); 1A86 = CH340; 10C4 = CP210x
    powershell_roda "Get-CimInstance Win32_PnPEntity | Where-Object { \$_.PNPDeviceID -match 'VID_(303A|1A86|10C4)' -and \$_.Name -match '\((COM\d+)\)' } | ForEach-Object { if (\$_.Name -match '\((COM\d+)\)') { \$matches[1] } }" | head -1 | grep . || { echo "nenhum ESP32 no USB" >&2; exit 1; } ;;
  linux)
    p=$(ls /dev/serial/by-id/ 2>/dev/null | grep -iE 'espressif|cp210|ch34|1a86|303a' | head -1)
    if [ -n "$p" ]; then readlink -f "/dev/serial/by-id/$p"; else ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null | head -1 | grep . || { echo "nenhum ESP32 no USB" >&2; exit 1; }; fi ;;
  mac)
    ls /dev/cu.usbmodem* /dev/cu.usbserial* /dev/cu.wchusbserial* 2>/dev/null | head -1 | grep . || { echo "nenhum ESP32 no USB" >&2; exit 1; } ;;
  *) echo "ambiente nao suportado" >&2; exit 1 ;;
esac
