#!/usr/bin/env python3
"""Gera firmware/claudinho/textos.h a partir de idiomas/*.txt.

Cada idioma e um arquivo "chave = texto" em UTF-8 (en.txt e a referencia).
O gerador confere, para cada idioma:
  - chave que falta (vai em ingles, com aviso) ou que sobra (erro);
  - os %d, %s... iguais aos do ingles (erro se mudarem);
  - caracteres que a fonte da tela nao tem (so ISO-8859-1; erro);
  - largura: mede o texto com a fonte de verdade (DM Sans, em fontes/) e
    recusa o que nao cabe na caixa da tela (caixas.txt). Precisa do Pillow
    (pip install pillow); sem ele, avisa e pula essa conferencia.

Uso: python3 idiomas/gerar.py           (gera)
     python3 idiomas/gerar.py --check   (so confere, nao escreve nada)

Generates firmware/claudinho/textos.h from idiomas/*.txt, checking missing
keys, placeholders, characters and on-screen width. See TRANSLATING.md.
"""
import re, sys
from pathlib import Path

AQUI = Path(__file__).resolve().parent
SAIDA = AQUI.parent / "firmware" / "claudinho" / "textos.h"
REF = "en"
FORMATO = re.compile(r"%[-0-9.]*l?[dsuXxc%]")
TOKENS = re.compile(r"%[-0-9.]*l?[dsuXxc%]|%")             # o "%" sozinho e um formato que o firmware nao usa
FONTES = {"p16": (16, "Regular"), "m24": (24, "Bold"), "r32": (32, "Regular"), "g48": (48, "Bold")}
ALTURA_EM = 1.302          # DM Sans: ascendente + descendente, em "em" (o Nextion usa a altura toda)
FOLGA = 0.97               # 3 % de margem: a fonte do Nextion e rasterizada diferente
QUEBRA = 26                # o firmware quebra o detalhe do alerta em duas linhas perto de 26 letras


def le_idioma(arq):
    d, erros = {}, []
    for n, l in enumerate(arq.read_text(encoding="utf-8").splitlines(), 1):
        if not l.strip() or l.lstrip().startswith("#"): continue
        if " = " not in l and not l.rstrip().endswith(" ="):
            erros.append(f"{arq.name}:{n}: falta ' = '"); continue
        k, _, v = l.partition(" = ") if " = " in l else (l.rstrip()[:-2], "", "")
        k = k.strip()
        if k in d: erros.append(f"{arq.name}:{n}: chave repetida {k}")
        d[k] = v.strip()
    return d, erros


def le_caixas():
    cx = {}
    for l in (AQUI / "caixas.txt").read_text(encoding="utf-8").splitlines():
        if not l.strip() or l.startswith("#"): continue
        p = l.split()
        amostra = {}
        for extra in p[3:]:
            if extra.startswith("%s="): amostra["s"] = extra[3:]
        cx[p[0]] = (p[1], int(p[2]), amostra)
    return cx


def caixa_de(k, cx):
    if k in cx: return cx[k]
    for padrao in ("hms.cat.*", "hms.*"):
        if padrao in cx and k.startswith(padrao[:-1]): return cx[padrao]
    return ("p16", 300, {})


def exemplo(t, amostra):
    """Troca os %... por um valor de pior caso, para medir."""
    def troca(m):
        f = m.group(0)
        if f == "%%": return "%"
        if f.endswith("s"): return amostra.get("s", "Wwwwww")
        if f.endswith("X") or f.endswith("x"): return "8888"
        if f.endswith("c"): return "W"
        return "88"
    return FORMATO.sub(troca, t)


def medidor():
    try:
        from PIL import ImageFont
    except ImportError:
        return None
    cache = {}
    def mede(t, fonte):
        if fonte not in cache:
            h, peso = FONTES[fonte]
            f = ImageFont.truetype(str(AQUI / "fontes" / "DMSans.ttf"), round(h / ALTURA_EM))
            f.set_variation_by_name(peso)
            cache[fonte] = f
        return cache[fonte].getlength(t)
    return mede


def quebra(t):
    """Igual ao firmware: ate 26 letras uma linha; senao quebra no espaco mais perto do meio."""
    if len(t) <= QUEBRA: return [t]
    meio, q = len(t) // 2, -1
    for i, ch in enumerate(t):
        if ch == " " and (q < 0 or abs(i - meio) < abs(q - meio)): q = i
    if q < 0: q = meio
    return [t[:q], t[q + (1 if t[q] == " " else 0):]]


def c_texto(s):
    out = []
    for b in s.encode("iso-8859-1"):
        out.append("\\%03o" % b if b >= 0x80 else chr(b))
    return '"' + "".join(out) + '"'


def main():
    so_confere = "--check" in sys.argv
    arquivos = sorted(AQUI.glob("*.txt"), key=lambda a: (a.stem != REF, a.stem))
    arquivos = [a for a in arquivos if a.name != "caixas.txt"]
    idiomas, erros, avisos = {}, [], []
    for a in arquivos:
        d, e = le_idioma(a); idiomas[a.stem] = d; erros += e
    ref = idiomas[REF]
    cx = le_caixas()
    mede = medidor()
    if not mede: avisos.append("Pillow nao instalado: larguras NAO conferidas (pip install pillow)")

    for cod, d in idiomas.items():
        for k in d:
            if k not in ref: erros.append(f"{cod}: chave que nao existe em {REF}.txt: {k}")
        for k, v_ref in ref.items():
            v = d.get(k)
            if v is None or v == "":
                if cod != REF: avisos.append(f"{cod}: falta {k} (vai em ingles)")
                continue
            if FORMATO.findall(v) != FORMATO.findall(v_ref) or "%" in TOKENS.findall(v):
                erros.append(f"{cod}: {k}: os %... tem de ser iguais aos do ingles e na mesma ordem ({FORMATO.findall(v_ref)})")
            try: v.encode("iso-8859-1")
            except UnicodeEncodeError: erros.append(f"{cod}: {k}: caractere que a tela nao tem: {v!r}")
            if '"' in v or "\\" in v: erros.append(f"{cod}: {k}: aspas e barra invertida nao podem")
            if k.startswith("hms.") and not k.startswith("hms.cat.") and k[4:].isdigit() and len(v) > 50:
                erros.append(f"{cod}: {k}: mais de 50 letras ({len(v)})")
            if mede and k != "idioma.nome":
                fonte, larg, amostra = caixa_de(k, cx)
                linhas = quebra(exemplo(v, amostra)) if larg > 320 else [exemplo(v, amostra)]
                limite = 304 if larg > 320 else larg
                for ln in linhas:
                    w = mede(ln, fonte)
                    if w > limite * FOLGA:
                        erros.append(f"{cod}: {k}: nao cabe na tela ({w:.0f} px, cabe {limite * FOLGA:.0f}): {ln!r}")

    for a in avisos: print("aviso:", a)
    if erros:
        for e in erros: print("ERRO:", e)
        print(f"{len(erros)} erro(s); nada foi gerado." if not so_confere else f"{len(erros)} erro(s).")
        sys.exit(1)
    if so_confere:
        print("tudo certo"); return

    tela = [k for k in ref if not k.startswith("hms.") or k.startswith("hms.familia.") or k == "hms.procure_wiki"]
    tela = [k for k in tela if k != "idioma.nome"]
    n_cat = sum(1 for k in ref if k.startswith("hms.cat."))
    n_msg = sum(1 for k in ref if re.fullmatch(r"hms\.\d{3}", k))
    enum = lambda k: "T_" + re.sub(r"[^A-Za-z0-9]", "_", k).upper()
    cods = list(idiomas)
    val = lambda cod, k: idiomas[cod].get(k) or ref[k]

    L = ["// Gerado por idiomas/gerar.py a partir de idiomas/*.txt. Nao editar: edite os .txt.",
         "// Generated by idiomas/gerar.py from idiomas/*.txt. Do not edit: edit the .txt files.",
         "#pragma once", "",
         "static const int I_N = %d;" % len(cods),
         "static const char* const IDIOMA_COD[I_N] = {%s};" % ", ".join('"%s"' % c for c in cods),
         "static const char* const IDIOMA_NOME[I_N] = {%s};" % ", ".join(c_texto(val(c, "idioma.nome")) for c in cods),
         "", "enum Txt {"] + ["  %s," % enum(k) for k in tela] + ["  T_N", "};", "",
         "static const char* const TXT[I_N][T_N] = {"]
    for c in cods:
        L.append("  {  // " + c)
        L += ["    %s," % c_texto(val(c, k)) for k in tela]
        L.append("  },")
    L += ["};", "", "static const int HMS_N_CAT = %d, HMS_N_MSG = %d;" % (n_cat, n_msg),
          "static const char* const HMS_CAT_TXT[I_N][HMS_N_CAT] = {"]
    for c in cods:
        L.append("  { " + ", ".join(c_texto(val(c, "hms.cat.%02d" % i)) for i in range(n_cat)) + " },")
    L += ["};", "static const char* const HMS_MSG_TXT[I_N][HMS_N_MSG] = {"]
    for c in cods:
        L.append("  {  // " + c)
        L += ["    %s," % c_texto(val(c, "hms.%03d" % i)) for i in range(n_msg)]
        L.append("  },")
    L += ["};", "",
          "static int idioma = 0;   // indice em IDIOMA_COD (0 = ingles)",
          "inline const char* tx(Txt t) { return TXT[idioma][t]; }",
          "inline const char* hmsCatTxt(int i) { return HMS_CAT_TXT[idioma][i]; }",
          "inline const char* hmsMsgTxt(int i) { return HMS_MSG_TXT[idioma][i]; }",
          "inline int idiomaPorCodigo(const char* c) { for (int i = 0; i < I_N; i++) if (!strcmp(IDIOMA_COD[i], c)) return i; return -1; }",
          ""]
    SAIDA.write_text("\n".join(L), encoding="ascii")
    print(f"{SAIDA.name}: {len(cods)} idiomas ({', '.join(cods)}), {len(tela)} textos da tela, {n_msg} avisos HMS")


if __name__ == "__main__":
    main()
