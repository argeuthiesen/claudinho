#!/usr/bin/env python3
"""Gera firmware/claudinho/hms_pt.h: os avisos HMS das impressoras Bambu Lab em
portugues curto, com categoria e nivel, para a tela de alertas do Claudinho.

Entrada:
  - a base oficial de avisos da Bambu (em ingles), baixada de
    https://e.bambulab.com/query.php?lang=en (ou um arquivo ja baixado: --bambu);
  - hms_pt.json (nesta pasta): a traducao, um item por texto ingles, com
    "en", "pt" (ate 50 caracteres), "cat" e "nivel" (info | atencao | grave).

Os textos em ingles que mudam so a unidade do AMS (A..H) e o slot viram um so
("AMS #", "slot #"); a unidade e o slot vao em campos proprios da tabela.
Codigo sem traducao fica de fora: o firmware mostra a categoria pela familia.

Uso: python3 firmware/hms/gerar.py [--bambu hms_en.json]
"""
import json, re, sys, urllib.request
from pathlib import Path

AQUI = Path(__file__).resolve().parent
SAIDA = AQUI.parent / "claudinho" / "hms_pt.h"
NIVEIS = {"info": 0, "atencao": 1, "grave": 2}


def normaliza(t):
    t = re.sub(r"AMS(-HT)? [A-H]\b", r"AMS\1 #", t)
    t = re.sub(r"[Ss]lot \d", "slot #", t)
    return t.strip()


def c_texto(s):
    """Texto em ISO-8859-1 (o Nextion) como literal C; acentos em octal."""
    out = []
    for b in s.encode("iso-8859-1"):
        if b >= 0x80: out.append("\\%03o" % b)
        elif chr(b) in '"\\': raise ValueError("aspas ou barra no texto: " + s)
        else: out.append(chr(b))
    return '"' + "".join(out) + '"'


def main():
    args = sys.argv[1:]
    if "--bambu" in args:
        base = json.load(open(args[args.index("--bambu") + 1]))
    else:
        with urllib.request.urlopen("https://e.bambulab.com/query.php?lang=en", timeout=30) as r:
            base = json.load(r)
    hms = base["data"]["device_hms"]["en"]
    trad = {x["en"]: x for x in json.load(open(AQUI / "hms_pt.json"))}

    cats, txts, codigos, sem = [], [], [], set()
    for x in hms:
        en = x["intro"].strip()
        if not en: continue
        t = trad.get(normaliza(en))
        if not t: sem.add(normaliza(en)); continue
        if t["cat"] not in cats: cats.append(t["cat"])
        if t["pt"] not in txts: txts.append(t["pt"])
        e = x["ecode"]
        u = re.search(r"AMS(-HT)? ([A-H])\b", en); s = re.search(r"[Ss]lot (\d)", en)
        codigos.append((int(e[:8], 16), int(e[8:], 16), txts.index(t["pt"]), cats.index(t["cat"]),
                        NIVEIS[t["nivel"]], u.group(2) if u else "", 1 if u and u.group(1) else 0,
                        int(s.group(1)) if s else 0))
    codigos.sort()

    L = ["// Gerado por firmware/hms/gerar.py a partir da base oficial de avisos HMS da",
         "// Bambu Lab (versao %s) e da traducao em firmware/hms/hms_pt.json. Nao editar." % base["data"]["device_hms"].get("ver", "?"),
         "#pragma once", "#include <stdint.h>", "",
         "struct HmsCodigo { uint32_t a, c; uint16_t txt; uint8_t cat, nivel; char unid; uint8_t ht, slot; };",
         "static const char* const HMS_CAT[] = {"] + ["  %s," % c_texto(c) for c in cats] + ["};",
         "static const char* const HMS_TXT[] = {"] + ["  %s," % c_texto(t) for t in txts] + ["};",
         "static const HmsCodigo HMS_COD[] = {"]
    for a, c, ti, ci, n, u, ht, sl in codigos:
        L.append("  {0x%08X, 0x%08X, %d, %d, %d, %s, %d, %d}," % (a, c, ti, ci, n, "'%s'" % u if u else "0", ht, sl))
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
    print("%s: %d codigos, %d textos, %d categorias; %d textos sem traducao" % (SAIDA.name, len(codigos), len(txts), len(cats), len(sem)))


if __name__ == "__main__":
    main()
