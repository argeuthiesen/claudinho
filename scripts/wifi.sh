#!/usr/bin/env bash
# Configura o Wi-Fi do Claudinho pela USB pedindo a senha AQUI, no terminal,
# escondida: ela nao passa pela conversa com o Claude, nao aparece na linha de
# comando e so fica gravada na placa. Gera o segredo do PC, espera a placa
# entrar no Wi-Fi e grava IP, segredo e MAC no computador.
# Uso (num terminal de verdade): wifi.sh PORTA "NOME DA REDE"
# Sem terminal, le a senha da primeira linha da entrada padrao.
source "$(dirname "$0")/comum.sh"
DIR="$(dirname "$0")"
PORTA="${1:-}"; SSID="${2:-}"
[ -n "$PORTA" ] && [ -n "$SSID" ] || { echo "uso: wifi.sh PORTA \"NOME DA REDE\"" >&2; exit 1; }
if [ -t 0 ]; then
  IFS= read -r -s -p "Senha do Wi-Fi \"$SSID\" (nao aparece enquanto voce digita): " SENHA; echo
else
  IFS= read -r SENHA          # sem terminal: primeira linha da entrada padrao
fi
[ -n "$SENHA" ] || { echo "senha vazia; nada foi enviado" >&2; exit 1; }
TOKEN=$(python3 -c 'import secrets;print(secrets.token_hex(24))')

# A linha CFG vai por pipe direto para a serial: a senha nao toca o disco nem
# aparece em argumentos (passa ao Python pelo ambiente, que so o dono le).
echo "enviando a configuracao pela USB..."
if ! SSID="$SSID" SENHA="$SENHA" TOKEN="$TOKEN" python3 -c 'import json,os
print("CFG " + json.dumps({"ssid": os.environ["SSID"], "senha": os.environ["SENHA"], "token": os.environ["TOKEN"]}))' \
    | bash "$DIR/serial.sh" "$PORTA" - "CFG OK" "" 20 >/dev/null; then
  unset SENHA; echo "a placa nao confirmou (CFG OK). Confira a porta e rode de novo." >&2; exit 1
fi
unset SENHA

echo "ok. Esperando a placa entrar no Wi-Fi (ate 1 minuto)..."
L=""
for i in 1 2 3; do
  L=$(bash "$DIR/serial.sh" "$PORTA" INFO 'CLAUDINHO {' "" 25 | tail -1)
  case "$L" in *'"wifi":"conectado"'*) break ;; esac
done
case "$L" in
  *'"wifi":"conectado"'*) ;;
  *) echo "a placa nao entrou no Wi-Fi: senha errada ou rede de 5 GHz. Rode de novo." >&2; exit 1 ;;
esac
IP=$(printf '%s' "${L#CLAUDINHO }" | python3 -c 'import json,sys;print(json.load(sys.stdin)["ip"])')
MAC=$(printf '%s' "${L#CLAUDINHO }" | python3 -c 'import json,sys;print(json.load(sys.stdin)["mac"])')
grava_config CLAUDINHO_IP "$IP"; grava_config CLAUDINHO_TOKEN "$TOKEN"; grava_config CLAUDINHO_MAC "$MAC"
echo "Pronto! O Claudinho esta no Wi-Fi."
echo "  IP:  $IP"
echo "  MAC: $MAC"
echo "Reserve esse IP no roteador (DHCP estatico) e volte ao Claude."
