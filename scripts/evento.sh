#!/usr/bin/env bash
# Hook do Claude Code -> evento para o Claudinho (a cara dele).
# Chamado pelo hooks/hooks.json do plugin: evento.sh <tipo>
#   inicio SessionStart | prompt UserPromptSubmit | ferramenta PreToolUse
#   erro PostToolUseFailure | feito PostToolUse(terminal) | parou Stop | atencao Notification(permissao)
#   compact PreCompact | fim SessionEnd
# Mudo de proposito: o que um hook imprime pode entrar no contexto do modelo.
# Roda em segundo plano e nunca falha.
source "$(dirname "$0")/comum.sh" 2>/dev/null || exit 0
tipo="$1"; IP=$(le_config CLAUDINHO_IP); TOKEN=$(le_config CLAUDINHO_TOKEN)
[ -n "$tipo" ] && [ -n "$IP" ] && [ -n "$TOKEN" ] || { cat >/dev/null 2>&1; exit 0; }

# stdin: JSON do evento. session_id vira um hash curto (o id real nao sai do
# PC); no prompt, o texto so serve para escolher o humor e nao e enviado.
# Na ferramenta vai so a categoria (codando, terminal, lendo, agente, web),
# para a cena na tela: nome de arquivo e comando nunca saem do PC.
# Depois de um comando no terminal, o script olha aqui mesmo se era um
# git commit ou um teste (e se passou) e manda so a palavra: commit,
# teste_ok ou teste_falhou.
leitura=$(python3 -c 'import sys,json,hashlib
try: d=json.load(sys.stdin)
except Exception: d={}
print(hashlib.sha1((d.get("session_id") or "").encode()).hexdigest()[:8])
print((d.get("prompt") or "").lower().replace("\n"," "))
f=d.get("tool_name") or ""
print("codando" if f in ("Edit","Write","MultiEdit","NotebookEdit") else "terminal" if f in ("Bash","PowerShell") else
      "lendo" if f in ("Read","Grep","Glob","LS") else "agente" if f in ("Agent","Task") else "web" if f in ("WebSearch","WebFetch") else "")
import re
cmd=str((d.get("tool_input") or {}).get("command") or "") if f in ("Bash","PowerShell") else ""
tipo=""
for seg in re.split(r"[;&|\n]+", cmd):
    w=seg.split()
    if "git" in w:
        i=w.index("git")+1
        while i < len(w) and w[i].startswith("-"): i += 2 if w[i] in ("-C","-c") else 1
        if i < len(w) and w[i] == "commit" and "--dry-run" not in w: tipo="commit"
    if re.search(r"(^|\s|/)(pytest|py\.test|jest|vitest|phpunit|rspec|ctest|tox)(\s|$)|\b(npm|yarn|pnpm|bun|cargo|go|mvn|gradle|gradlew|dotnet|make|deno|mix)\s+(run\s+)?test\b|python3?\s+-m\s+(pytest|unittest)", seg): tipo=tipo or "teste"
if tipo == "teste":   # passou? com "| tail" o codigo de saida e do tail: confere a saida tambem
    saida=str(d.get("tool_response") or "")
    ruim=re.search(r"\b[1-9][0-9]* (failed|failing|failures?|errors?)\b|\bFAILED\b|^FAIL\b|test result: FAILED|BUILD FAILURE|Tests:.*\bfailed", saida, re.M)
    tipo="teste_falhou" if ruim else "teste_ok"
print(tipo)' 2>/dev/null || true)
sessao=$(printf '%s\n' "$leitura" | sed -n 1p); texto=$(printf '%s\n' "$leitura" | sed -n 2p); acao=$(printf '%s\n' "$leitura" | sed -n 3p)
comando=$(printf '%s\n' "$leitura" | sed -n 4p)
# feito: so interessa commit e teste. erro: um teste que falhou vira teste_falhou.
if [ "$tipo" = feito ]; then
  case "$comando" in commit|teste_ok|teste_falhou) tipo=$comando; acao="" ;; *) exit 0 ;; esac
elif [ "$tipo" = erro ]; then
  case "$comando" in teste_ok|teste_falhou) tipo=teste_falhou; acao="" ;; esac
fi

humor=""
if [ "$tipo" = prompt ]; then
  if   printf '%s' "$texto" | grep -qE 'merda|porra|caralho|puta que|foda-se|shit|fuck|damn'; then humor=susto
  elif printf '%s' "$texto" | grep -qE 'bug|erro|error|quebrou|broke|nao funciona|não funciona|doesn.t work|travou|falhou|failed|deu ruim'; then humor=preocupado
  elif printf '%s' "$texto" | grep -qE 'obrigad|valeu|perfeito|funfou|funcionou|excelente|thanks|thank you|perfect|awesome|great job|it works'; then humor=feliz
  fi
fi
# tipo, humor, acao e sessao sao palavras fixas ou hexadecimal: nao precisam de escape.
# Segredo e corpo vao pela entrada padrao do curl (-K -), fora da linha de comando.
# Registro curto (eventos.log na pasta de dados, ultimas 200 linhas): hora, tipo
# e a resposta da placa. Nada do prompt. Serve para o "ele nao reagiu".
{ r=$(printf 'header = "Authorization: Bearer %s"\nheader = "Content-Type: application/json"\ndata = "{\\"tipo\\":\\"%s\\",\\"humor\\":\\"%s\\",\\"acao\\":\\"%s\\",\\"sessao\\":\\"%s\\"}"\n' \
    "$TOKEN" "$tipo" "$humor" "$acao" "$sessao" | curl -s -m 2 -o /dev/null -w '%{http_code}' -X POST "http://$IP/evento" -K - )
  L="$DADOS/eventos.log"; printf '%s %s %s\n' "$(date '+%m-%d %H:%M:%S')" "$tipo" "${r:-000}" >> "$L"
  [ "$(wc -l < "$L")" -gt 400 ] && tail -n 200 "$L" > "$L.tmp" && mv "$L.tmp" "$L"; } >/dev/null 2>&1 &
exit 0
