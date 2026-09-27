#!/usr/bin/env bash
# Conversa com o Claudinho pela serial USB (115200).
# Uso: serial.sh PORTA "COMANDO" PREFIXO_ESPERADO [LINHA_FINAL] [SEGUNDOS]
#   serial.sh COM6 INFO CLAUDINHO
#   serial.sh COM6 SCAN REDE "SCAN FIM" 40
#   serial.sh COM6 'CFG {"ssid":"x"}' "CFG OK"
#   ... | serial.sh COM6 - "CFG OK"    comando lido da entrada padrao (senha sem arquivo nem argumento)
# Reenvia o comando a cada 5 s ate ver a resposta (o ESP pode estar
# reiniciando quando a porta abre). Imprime so as linhas com o prefixo.
source "$(dirname "$0")/comum.sh"
PORTA="$1"; CMD="$2"; PREF="$3"; FIM="${4:-}"; SEG="${5:-30}"
case "$SEG" in ''|*[!0-9]*) echo "tempo invalido: $SEG" >&2; exit 1 ;; esac
case "$(ambiente)" in
  wsl|windows)
    q() { printf '%s' "$1" | sed "s/'/''/g"; }
    if [ "$CMD" = "-" ]; then LECMD="[Console]::In.ReadLine()"; else LECMD="'$(q "$CMD")'"; fi
    powershell_roda "
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
\$p = New-Object System.IO.Ports.SerialPort('$(q "$PORTA")', 115200)
\$p.Encoding = [System.Text.Encoding]::UTF8   # nomes de rede com acento
\$p.NewLine = \"\`n\"; \$p.ReadTimeout = 500; \$p.DtrEnable = \$true; \$p.RtsEnable = \$false
try { \$p.Open() } catch { Write-Output ('ERRO porta: ' + \$_.Exception.Message); exit 2 }
\$cmd = $LECMD
\$fim = (Get-Date).AddSeconds($SEG); \$ok = \$false
while ((Get-Date) -lt \$fim -and -not \$ok) {
  \$p.WriteLine(\$cmd)
  \$t = (Get-Date).AddSeconds(5)
  while ((Get-Date) -lt \$t) {
    try { \$l = \$p.ReadLine().TrimEnd([char]13) } catch { continue }
    if (\$l.StartsWith('$(q "$PREF")')) { Write-Output \$l; if ('$(q "$FIM")' -eq '') { \$ok = \$true; break } }
    if ('$(q "$FIM")' -ne '' -and \$l -eq '$(q "$FIM")') { \$ok = \$true; break }
  }
}
\$p.Close()
if (-not \$ok) { exit 1 }"
    ;;
  linux|mac)
    if [ "$(ambiente)" = mac ]; then stty -f "$PORTA" 115200 raw -echo clocal; else stty -F "$PORTA" 115200 raw -echo clocal; fi
    exec 3<>"$PORTA"
    [ "$CMD" = "-" ] && IFS= read -r CMD
    fim=$((SECONDS + SEG)); ok=1
    while [ $SECONDS -lt $fim ] && [ $ok -ne 0 ]; do
      printf '%s\n' "$CMD" >&3
      t=$((SECONDS + 5))
      while [ $SECONDS -lt $t ]; do
        IFS= read -r -t 1 l <&3 || continue
        l="${l%$'\r'}"
        case "$l" in "$PREF"*) echo "$l"; [ -z "$FIM" ] && { ok=0; break; } ;; esac
        [ -n "$FIM" ] && [ "$l" = "$FIM" ] && { ok=0; break; }
      done
    done
    exec 3>&-
    exit $ok ;;
  *) echo "ambiente nao suportado" >&2; exit 1 ;;
esac
