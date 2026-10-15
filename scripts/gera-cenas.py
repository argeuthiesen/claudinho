#!/usr/bin/env python3
"""Roteiros das cenas de editor e terminal, numa fonte so.

Gera firmware/claudinho/cenas.h e o bloco entre "// <cenas>" e "// </cenas>"
do virtual/claudinho.html, para a placa e o virtual mostrarem o mesmo.
Rodar depois de mudar qualquer roteiro: python3 scripts/gera-cenas.py

Editor: cada linha e uma sequencia de pedacos marcados com a cor de sintaxe:
  §k palavra-chave  §f funcao  §s texto  §c comentario  §n numero  §p resto  §t tipo
Terminal: linha com "$" no comeco e comando digitado; o resto e resposta.
Limites de largura (o mini Clawd ocupa o canto de baixo): codigo ate 32
letras, comando ate 33, resposta ate 35.
"""
import json, os, re, sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

ARQUIVOS = {
"claudinho.ino": [
  "§c// acorda o Claudinho",
  "§tvoid §facorda§p() {",
  "§t  int §pn = §n42§p;",
  "§k  if §p(n > §n0§p) {",
  "§p    tela.§fpisca§p(§n3§p);",
  "§f    fala§p(§s'oi!'§p);",
  "§p  }",
  "§k  for §p(§tint §pi = §n0§p; i < n; i++)",
  "§p    olho[i] = §ktrue§p;",
  "§p}",
  "",
  "§c// quanto custa? nada.",
  "§tint §ftokens§p() {",
  "§k  return §n0§p;",
  "§p}",
  "",
  "§tbool §ffeliz§p(§tint §phumor) {",
  "§k  while §p(humor < §n10§p)",
  "§p    humor += §fcafe§p();",
  "§k  return true§p;",
  "§p}",
  "",
  "§c/* TODO: dominar o mundo */",
  "§tvoid §floop§p() {",
  "§f  acorda§p();",
  "§f  delay§p(§n10§p);",
  "§p}",
  "",
],
"auth.py": [
  "§kimport §ptime",
  "§kfrom §pjwt §kimport §pdecode",
  "",
  "§pTTL = §n900  §c# 15 min",
  "",
  "§kdef §fcheck§p(tok: §tstr§p) -> §tdict§p:",
  "§k    try§p:",
  "§p        c = §fdecode§p(tok, KEY)",
  "§k    except §tExpired§p:",
  "§k        raise §tAuthErr§p(§s\"expired\"§p)",
  "§k    if §pc[§s\"iat\"§p] + TTL < §fnow§p():",
  "§k        raise §tAuthErr§p(§s\"stale\"§p)",
  "§k    return §pc",
  "",
  "§kdef §frefresh§p(user):",
  "§p    tok = §fsign§p({§s\"sub\"§p: user.id})",
  "§p    cache.§fset§p(user.id, tok, TTL)",
  "§k    return §ptok",
  "",
],
"useCart.ts": [
  "§kimport §p{ useState } §kfrom §s\"react\"",
  "",
  "§kexport function §fuseCart§p() {",
  "§k  const §p[items, setItems] =",
  "§f    useState§p<§tItem§p[]>([]);",
  "",
  "§k  const §fadd§p = (it: §tItem§p) =>",
  "§f    setItems§p(xs => [...xs, it]);",
  "",
  "§k  const §ptotal = items.§freduce§p(",
  "§p    (s, x) => s + x.price, §n0§p);",
  "",
  "§k  return §p{ items, add, total };",
  "§p}",
  "",
],
"bucket.go": [
  "§kpackage §pratelimit",
  "",
  "§kimport §s\"time\"",
  "",
  "§ktype §tBucket §kstruct §p{",
  "§p  tokens §tint",
  "§p  last   §ttime.Time",
  "§p}",
  "",
  "§kfunc §p(b *§tBucket§p) §fTake§p() §tbool §p{",
  "§p  b.§frefill§p()",
  "§k  if §pb.tokens == §n0 §p{",
  "§k    return false",
  "§p  }",
  "§p  b.tokens--",
  "§k  return true",
  "§p}",
  "",
  "§c// one token per second",
  "§kfunc §p(b *§tBucket§p) §frefill§p() {",
  "§p  dt := time.§fSince§p(b.last)",
  "§p  b.tokens += §fint§p(dt.§fSeconds§p())",
  "§p  b.last = time.§fNow§p()",
  "§p}",
  "",
],
"schema.sql": [
  "§c-- users and their plans",
  "§kCREATE TABLE §pusers (",
  "§p  id    §tSERIAL §kPRIMARY KEY§p,",
  "§p  email §tTEXT §kUNIQUE NOT NULL§p,",
  "§p  plan  §tTEXT §kDEFAULT §s'pro'",
  "§p);",
  "",
  "§kCREATE INDEX §pidx_email",
  "§k  ON §pusers (email);",
  "",
  "§kSELECT §pplan, §fcount§p(*)",
  "§k  FROM §pusers",
  "§k GROUP BY §pplan",
  "§k ORDER BY §n2 §kDESC§p;",
  "",
],
}

SESSOES = [
  ["$git status", "On branch main", "Changes not staged for commit:",
   "  modified:   src/api.ts", "  modified:   src/api.test.ts",
   "$git add -A", "$git commit -m \"fix: retry on 429\"",
   "[main 3f9c2ab] fix: retry on 429", " 2 files changed, 18 insertions(+)",
   "$git push", "To github.com:you/app.git", "   9be2c10..3f9c2ab  main -> main"],
  ["$npm test", "> app@2.3.1 test", "> vitest run", "",
   " PASS  src/cart.test.ts (12)", " PASS  src/auth.test.ts (8)",
   " FAIL  src/api.test.ts (1/4)", "   expected 200, received 429",
   "Tests  1 failed | 23 passed", "$npm test -- api",
   " PASS  src/api.test.ts (4)", "Tests  4 passed (4)"],
  ["$pytest -q", ".........F......", "FAILED test_auth.py::test_stale",
   "1 failed, 15 passed in 0.84s", "$pytest -q -k stale", ".",
   "1 passed in 0.11s"],
  ["$npm run build", "vite v5.4.8 building for prod...", "transforming...",
   "412 modules transformed.", "dist/index.html       0.46 kB",
   "dist/index.js       142.31 kB", "built in 3.21s"],
  ["$docker compose up -d", " Network app_default  Created",
   " Container app-db-1   Started", " Container app-api-1  Started",
   "$curl -s localhost:8000/health", "{\"status\":\"ok\",\"db\":\"up\"}"],
  ["$cargo build --release", "   Compiling serde v1.0.210",
   "   Compiling tokio v1.40.0", "   Compiling app v0.4.0",
   "    Finished release in 23.4s", "$./target/release/app -V", "app 0.4.0"],
  ["$grep -rn TODO src | wc -l", "17", "$git log --oneline -3",
   "3f9c2ab fix: retry on 429", "a1d07e4 feat: dark mode",
   "9be2c10 chore: bump deps"],
  ["$pio run -t upload", "Building .pio/build/esp32",
   "RAM:   [==        ]  20.6%", "Flash: [========  ]  82.5%",
   "Uploading 1622496 bytes", "Writing at 0x0019c000 (100 %)",
   "Hard resetting via RTS pin..."],
  ["$python -m venv .venv", "$pip install -r requirements.txt",
   "Collecting fastapi==0.115.0", "Collecting uvicorn==0.30.6",
   "Successfully installed 14 packages", "$uvicorn main:app --reload",
   "INFO:  Uvicorn running on :8000"],
]

COR = {"k": 1, "f": 2, "s": 3, "c": 4, "n": 5, "p": 6, "t": 7}

def pedacos(linha):
    return [(COR[m[0]], m[1:]) for m in re.split(r"§", linha)[1:]] if linha else []

def confere():
    erros = []
    for nome, linhas in ARQUIVOS.items():
        if len(nome) > 14: erros.append(f"nome longo: {nome}")
        for l in linhas:
            n = sum(len(t) for _, t in pedacos(l))
            if n > 32: erros.append(f"{nome}: {n} > 32: {l}")
    for s in SESSOES:
        for l in s:
            lim = 34 if l.startswith("$") else 35
            if len(l) > lim: erros.append(f"terminal: {len(l)} > {lim}: {l}")
            if not l.isascii(): erros.append(f"terminal nao ascii: {l}")
    if erros: sys.exit("\n".join(erros))

def c_str(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'

def gera_h():
    out = ["// Gerado por scripts/gera-cenas.py. Nao editar: mude os roteiros la.", "#pragma once", ""]
    nomes = []
    for i, (nome, linhas) in enumerate(ARQUIVOS.items()):
        out.append(f"static const char* const ARQ_{i}[] = {{")
        for l in linhas:
            # cada cor num literal separado: "\x01" "abc" (senao o \x engole letras hex)
            out.append("  " + (" ".join(f'"\\x0{c}" {c_str(t)}' for c, t in pedacos(l)) or '""') + ",")
        out.append("};")
        nomes.append(f"  {{ {c_str(nome)}, ARQ_{i}, sizeof ARQ_{i} / sizeof ARQ_{i}[0] }},")
    out += ["struct Arquivo { const char* nome; const char* const* l; int n; };",
            "static const Arquivo ARQUIVOS[] = {", *nomes, "};",
            "static const int N_ARQUIVOS = sizeof ARQUIVOS / sizeof ARQUIVOS[0];", ""]
    ses = []
    for i, s in enumerate(SESSOES):
        out.append(f"static const char* const SES_{i}[] = {{ " + ", ".join(c_str(l) for l in s) + " };")
        ses.append(f"  {{ SES_{i}, sizeof SES_{i} / sizeof SES_{i}[0] }},")
    out += ["struct Sessao { const char* const* l; int n; };",
            "static const Sessao SESSOES[] = {", *ses, "};",
            "static const int N_SESSOES = sizeof SESSOES / sizeof SESSOES[0];", ""]
    open(os.path.join(RAIZ, "firmware/claudinho/cenas.h"), "w").write("\n".join(out))

def gera_js():
    arqs = [{"nome": n, "l": ["".join(chr(c) + t for c, t in pedacos(l)) for l in ls]} for n, ls in ARQUIVOS.items()]
    bloco = ("// <cenas> gerado por scripts/gera-cenas.py; nao editar aqui\n"
             "const ARQUIVOS = " + json.dumps(arqs, ensure_ascii=True) + ";\n"
             "const SESSOES = " + json.dumps(SESSOES, ensure_ascii=True) + ";\n"
             "// </cenas>")
    p = os.path.join(RAIZ, "virtual/claudinho.html")
    html = open(p).read()
    novo, n = re.subn(r"// <cenas>.*?// </cenas>", lambda m: bloco, html, flags=re.S)
    if n != 1: sys.exit("virtual/claudinho.html: marcador // <cenas> nao encontrado")
    open(p, "w").write(novo)

confere(); gera_h(); gera_js()
print(f"{len(ARQUIVOS)} arquivos, {len(SESSOES)} sessoes de terminal")
