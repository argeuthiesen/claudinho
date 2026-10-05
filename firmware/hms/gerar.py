#!/usr/bin/env python3
"""Gera firmware/claudinho/hms_codigos.h: a tabela de codigos HMS das
impressoras Bambu Lab (codigo -> mensagem, categoria, nivel, unidade do AMS
e slot). Os TEXTOS ficam nos arquivos de idioma (idiomas/*.txt, chaves
hms.NNN e hms.cat.NN), gerados por idiomas/gerar.py.

Entrada:
  - a base oficial de avisos da Bambu (em ingles), baixada de
    https://e.bambulab.com/query.php?lang=en (ou um arquivo ja baixado: --bambu);
  - hms.json (nesta pasta): para cada texto ingles da Bambu, o numero da
    mensagem curta (msg), a categoria (cat) e o nivel (info | atencao | grave).

Os textos que mudam so a unidade do AMS (A..H) e o slot viram um so
("AMS #", "slot #"); a unidade e o slot vao em campos proprios da tabela.
Codigo sem entrada no hms.json fica de fora: o firmware mostra a categoria
pela familia do codigo e manda procurar no wiki.

Uso: python3 firmware/hms/gerar.py [--bambu arquivo.json]
"""
import json, re, sys, urllib.request
from pathlib import Path

AQUI = Path(__file__).resolve().parent
SAIDA = AQUI.parent / "claudinho" / "hms_codigos.h"
NIVEIS = {"info": 0, "atencao": 1, "grave": 2}


def normaliza(t):
    t = re.sub(r"AMS(-HT)? [A-H]\b", r"AMS\1 #", t)
    t = re.sub(r"[Ss]lot \d", "slot #", t)
    return t.strip()


def main():
    args = sys.argv[1:]
    if "--bambu" in args:
        base = json.load(open(args[args.index("--bambu") + 1]))
    else:
        req = urllib.request.Request("https://e.bambulab.com/query.php?lang=en", headers={"User-Agent": "Mozilla/5.0"})
        with urllib.request.urlopen(req, timeout=30) as r:
            base = json.load(r)
    hms = base["data"]["device_hms"]["en"]
    estr = {x["en"]: x for x in json.load(open(AQUI / "hms.json"))}

    codigos, sem = [], set()
    for x in hms:
        en = x["intro"].strip()
        if not en: continue
        t = estr.get(normaliza(en))
        if not t: sem.add(normaliza(en)); continue
        e = x["ecode"]
        u = re.search(r"AMS(-HT)? ([A-H])\b", en); s = re.search(r"[Ss]lot (\d)", en)
        codigos.append((int(e[:8], 16), int(e[8:], 16), t["msg"], t["cat"], NIVEIS[t["nivel"]],
                        u.group(2) if u else "", 1 if u and u.group(1) else 0, int(s.group(1)) if s else 0))
    codigos.sort()

    L = ["// Gerado por firmware/hms/gerar.py a partir da base oficial de avisos HMS da",
         "// Bambu Lab (versao %s) e de firmware/hms/hms.json. Nao editar." % base["data"]["device_hms"].get("ver", "?"),
         "// Os textos (por idioma) estao em textos.h, gerado de idiomas/*.txt.",
         "#pragma once", "#include <stdint.h>", "",
         "struct HmsCodigo { uint32_t a, c; uint16_t msg; uint8_t cat, nivel; char unid; uint8_t ht, slot; };",
         "static const HmsCodigo HMS_COD[] = {"]
    for a, c, mi, ci, n, u, ht, sl in codigos:
        L.append("  {0x%08X, 0x%08X, %d, %d, %d, %s, %d, %d}," % (a, c, mi, ci, n, "'%s'" % u if u else "0", ht, sl))
    L += ["};", "static const int HMS_N = sizeof HMS_COD / sizeof HMS_COD[0];", "",
          "// Busca binaria pelo par (attr, code) que vem no campo hms da impressora.",
          "inline const HmsCodigo* hmsBusca(uint32_t a, uint32_t c) {",
          "  int lo = 0, hi = HMS_N - 1;",
          "  while (lo <= hi) {",
          "    int m = (lo + hi) / 2; const HmsCodigo& e = HMS_COD[m];",
          "    if (e.a == a && e.c == c) return &e;",
          "    if (e.a < a || (e.a == a && e.c < c)) lo = m + 1; else hi = m - 1;",
          "  }",
          "  return nullptr;",
          "}", ""]
    SAIDA.write_text("\n".join(L), encoding="ascii")
    print("%s: %d codigos; %d textos da Bambu sem entrada no hms.json" % (SAIDA.name, len(codigos), len(sem)))


if __name__ == "__main__":
    main()
