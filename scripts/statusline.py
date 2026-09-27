#!/usr/bin/env python3
"""
Status line do Claude Code -> Claudinho.

O Claude Code roda isto a cada evento e entrega no stdin o JSON oficial da
status line (janelas de 5 h e 7 dias do plano, contexto, modelo). Este script:
  1. manda um resumo para o Claudinho na rede local (curl em segundo plano);
  2. imprime a linha de status. Se a pessoa ja tinha uma status line, ela
     continua sendo a que aparece (CLAUDINHO_STATUSLINE_ANTERIOR no config).

Nenhuma credencial da Anthropic e lida ou enviada: so os numeros que o proprio
Claude Code entrega.
"""
import hashlib, json, os, subprocess, sys, time
from pathlib import Path

DADOS = Path(os.environ.get("CLAUDINHO_DADOS") or Path.home() / ".claudinho")

def config():
    cfg = {}
    try:
        for l in (DADOS / "claudinho.env").read_text().splitlines():
            if "=" in l and not l.startswith("#"):
                k, v = l.split("=", 1); cfg[k.strip()] = v.strip()
    except OSError:
        pass
    return cfg

def janela(j):
    return {"usado_pct": j.get("used_percentage"), "reseta_em": j.get("resets_at")} if j else None

def resumo(d):
    lim = d.get("rate_limits") or {}; ctx = d.get("context_window") or {}; m = d.get("model") or {}
    return {
        "enviado_em": int(time.time()),
        "sessao": hashlib.sha1((d.get("session_id") or "").encode()).hexdigest()[:8],
        "modelo": m.get("display_name") or m.get("id"),
        "limites": {"cinco_horas": janela(lim.get("five_hour")), "sete_dias": janela(lim.get("seven_day"))},
        "contexto": {"usado_pct": ctx.get("used_percentage")},
    }

def pct(j):
    return "—" if not j or j.get("used_percentage") is None else f"{round(j['used_percentage'])}%"

def linha_propria(d):
    m = (d.get("model") or {}).get("display_name") or "?"; ctx = d.get("context_window") or {}; lim = d.get("rate_limits") or {}
    partes = [m]
    if ctx.get("used_percentage") is not None: partes.append(f"ctx {round(ctx['used_percentage'])}%")
    partes += [f"5h {pct(lim.get('five_hour'))}", f"7d {pct(lim.get('seven_day'))}"]
    return "  ".join(partes)

def main():
    bruto = sys.stdin.read()
    try: d = json.loads(bruto)
    except Exception: d = {}
    cfg = config()
    anterior = cfg.get("CLAUDINHO_STATUSLINE_ANTERIOR")
    if anterior:
        try:
            r = subprocess.run(anterior, shell=True, input=bruto, capture_output=True, text=True, timeout=5)
            sys.stdout.write(r.stdout)
        except Exception:
            print(linha_propria(d))
    else:
        print(linha_propria(d))
    sys.stdout.flush()
    ip, token = cfg.get("CLAUDINHO_IP"), cfg.get("CLAUDINHO_TOKEN")
    if ip and token and d:
        try:
            # segredo e corpo pela entrada padrao (-K -): fora da linha de comando
            corpo = json.dumps(resumo(d)).replace("\\", "\\\\").replace('"', '\\"')
            cfg_curl = (f'header = "Authorization: Bearer {token}"\n'
                        'header = "Content-Type: application/json"\n'
                        f'data-binary = "{corpo}"\n')
            p = subprocess.Popen(["curl", "-s", "-o", os.devnull, "-m", "3", "-X", "POST", f"http://{ip}/estado", "-K", "-"],
                                 stdin=subprocess.PIPE, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                 start_new_session=True)
            p.stdin.write(cfg_curl.encode()); p.stdin.close()
        except Exception:
            pass

if __name__ == "__main__":
    main()
