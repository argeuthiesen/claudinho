#!/usr/bin/env python3
"""
Liga a status line do Claudinho no ~/.claude/settings.json (plugins nao podem
fazer isso sozinhos). Faz backup, preserva uma status line que ja exista
(guardando o comando dela para continuar aparecendo) e copia o script para a
pasta de dados, que nao muda quando o plugin atualiza.
Uso: instalar-statusline.py <pasta_de_dados> [--remover]
"""
import json, shlex, shutil, sys, time
from pathlib import Path

dados = Path(sys.argv[1]); remover = "--remover" in sys.argv
raiz = Path(__file__).resolve().parent
cfg_path = Path.home() / ".claude" / "settings.json"
s = json.loads(cfg_path.read_text()) if cfg_path.exists() else {}
backup = cfg_path.with_name(f"settings.json.bak-claudinho-{time.strftime('%Y%m%d-%H%M%S')}")
if cfg_path.exists(): shutil.copy2(cfg_path, backup)

env = dados / "claudinho.env"
linhas = env.read_text().splitlines() if env.exists() else []
def grava(chave, valor):
    global linhas
    linhas = [l for l in linhas if not l.startswith(chave + "=")]
    if valor is not None: linhas.append(f"{chave}={valor}")
    env.write_text("\n".join(linhas) + "\n"); env.chmod(0o600)

atual = (s.get("statusLine") or {}).get("command", "")
if remover:
    ant = next((l.split("=", 1)[1] for l in linhas if l.startswith("CLAUDINHO_STATUSLINE_ANTERIOR=")), None)
    if "claudinho" in atual:
        if ant: s["statusLine"] = {"type": "command", "command": ant}
        else: s.pop("statusLine", None)
    grava("CLAUDINHO_STATUSLINE_ANTERIOR", None)
    print("status line do Claudinho removida")
else:
    dados.mkdir(parents=True, exist_ok=True)
    destino = dados / "claudinho-statusline.py"
    shutil.copy2(raiz / "statusline.py", destino)
    if atual and "claudinho" not in atual:
        grava("CLAUDINHO_STATUSLINE_ANTERIOR", atual)
        print(f"status line existente preservada: {atual}")
    comando = f'CLAUDINHO_DADOS={shlex.quote(str(dados))} python3 {shlex.quote(str(destino))}'
    sl = s.get("statusLine") or {}
    sl.update({"type": "command", "command": comando})
    sl.setdefault("refreshInterval", 60)
    s["statusLine"] = sl
    print("status line ligada")
cfg_path.parent.mkdir(parents=True, exist_ok=True)
cfg_path.write_text(json.dumps(s, indent=2, ensure_ascii=False) + "\n")
if backup.exists(): print(f"backup: {backup}")
