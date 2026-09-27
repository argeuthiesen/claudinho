#!/usr/bin/env bash
# Hook do Claude Code -> evento para o Claudinho (a cara dele).
# Chamado pelo hooks/hooks.json do plugin: evento.sh <tipo>
#   inicio SessionStart | prompt UserPromptSubmit | ferramenta PreToolUse
#   erro PostToolUseFailure | parou Stop | atencao Notification(permissao)
#   compact PreCompact | fim SessionEnd
# Mudo de proposito: o que um hook imprime pode entrar no contexto do modelo.
# Roda em segundo plano e nunca falha.
source "$(dirname "$0")/comum.sh" 2>/dev/null || exit 0
tipo="$1"; IP=$(le_config CLAUDINHO_IP); TOKEN=$(le_config CLAUDINHO_TOKEN)
[ -n "$tipo" ] && [ -n "$IP" ] && [ -n "$TOKEN" ] || { cat >/dev/null 2>&1; exit 0; }

# stdin: JSON do evento. session_id vira um hash curto (o id real nao sai do
# PC); no prompt, o texto so serve para escolher o humor e nao e enviado.
leitura=$(python3 -c 'import sys,json,hashlib
try: d=json.load(sys.stdin)
except Exception: d={}
print(hashlib.sha1((d.get("session_id") or "").encode()).hexdigest()[:8])
print((d.get("prompt") or "").lower().replace("\n"," "))' 2>/dev/null || true)
sessao=$(printf '%s\n' "$leitura" | sed -n 1p); texto=$(printf '%s\n' "$leitura" | sed -n 2p)

humor=""
if [ "$tipo" = prompt ]; then
  if   printf '%s' "$texto" | grep -qE 'merda|porra|caralho|puta que|foda-se|shit|fuck|damn'; then humor=susto
  elif printf '%s' "$texto" | grep -qE 'bug|erro|error|quebrou|broke|nao funciona|não funciona|doesn.t work|travou|falhou|failed|deu ruim'; then humor=preocupado
  elif printf '%s' "$texto" | grep -qE 'obrigad|valeu|perfeito|funfou|funcionou|excelente|thanks|thank you|perfect|awesome|great job|it works'; then humor=feliz
  fi
fi
# tipo, humor e sessao sao palavras fixas ou hexadecimal: nao precisam de escape.
# Segredo e corpo vao pela entrada padrao do curl (-K -), fora da linha de comando.
{ printf 'header = "Authorization: Bearer %s"\nheader = "Content-Type: application/json"\ndata = "{\\"tipo\\":\\"%s\\",\\"humor\\":\\"%s\\",\\"sessao\\":\\"%s\\"}"\n' \
    "$TOKEN" "$tipo" "$humor" "$sessao" | curl -s -m 2 -o /dev/null -X POST "http://$IP/evento" -K - ; } >/dev/null 2>&1 &
exit 0
