#!/usr/bin/env python3
"""Gera firmware/claudinho/fontes_e32.h: as fontes da tela da placa E32R28T,
no formato VLW (o da biblioteca LovyanGFX), com bordas suaves (8 bits de
alfa por pixel) e os caracteres ISO-8859-1 que os textos usam (ASCII e os
acentos da Europa ocidental). Os numeros das fontes sao os mesmos do
Nextion, para o firmware nao precisar saber em que placa esta:

  0  DM Sans 16 px        1  DM Sans Bold 48 px     2  DM Sans Bold 24 px
  3  DM Sans 32 px        5  JetBrains Mono 16 px (7 px por letra, as cenas)

"16 px" e a altura total da letra (ascendente + descendente), igual ao Nextion.
Requer o Pillow (pip install pillow). Uso: python3 firmware/fontes/gerar_vlw.py
"""
import struct
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

AQUI = Path(__file__).resolve().parent
SAIDA = AQUI.parent / "claudinho" / "fontes_e32.h"
CARACTERES = [c for c in range(32, 127)] + [c for c in range(160, 256)]
FONTES = [  # numero, arquivo, peso (fonte variavel), altura total em px
    (0, "DMSans.ttf", "Regular", 16),
    (1, "DMSans.ttf", "Bold", 48),
    (2, "DMSans.ttf", "Bold", 24),
    (3, "DMSans.ttf", "Regular", 32),
    (5, "JetBrainsMono-Regular.ttf", None, 16),
]


def abre(arq, peso, tam):
    f = ImageFont.truetype(str(AQUI / arq), tam)
    if peso: f.set_variation_by_name(peso)
    return f


def tamanho_para(arq, peso, altura):
    """Maior tamanho cuja ascendente + descendente cabe na altura pedida."""
    melhor = 4
    for t in range(4, altura * 2):
        a, d = abre(arq, peso, t).getmetrics()
        if a + d <= altura: melhor = t
    return melhor


def vlw(arq, peso, altura):
    f = abre(arq, peso, tamanho_para(arq, peso, altura))
    asc, desc = f.getmetrics()
    glifos, bitmaps = [], []
    for c in CARACTERES:
        ch = chr(c)
        avanco = round(f.getlength(ch))
        x0, y0, x1, y1 = f.getbbox(ch, anchor="ls")       # relativo a linha de base
        w, h = max(0, x1 - x0), max(0, y1 - y0)
        if w and h:
            im = Image.new("L", (w, h), 0)
            ImageDraw.Draw(im).text((-x0, -y0), ch, font=f, fill=255, anchor="ls")
            bitmaps.append(im.tobytes())
        else:
            w = h = 0; bitmaps.append(b"")
        glifos.append((c, h, w, avanco, -y0 if h else 0, x0 if w else 0))
    out = struct.pack(">6i", len(glifos), 11, asc + desc, 0, asc, desc)
    for c, h, w, av, dy, dx in glifos:
        out += struct.pack(">7i", c, h, w, av, dy, dx, 0)
    out += b"".join(bitmaps)
    return out


def main():
    L = ["// Gerado por firmware/fontes/gerar_vlw.py (DM Sans e JetBrains Mono, ambas OFL).",
         "// So para a placa E32R28T. Nao editar.", "#pragma once", "#include <stdint.h>", ""]
    nomes = {}
    for num, arq, peso, alt in FONTES:
        dados = vlw(arq, peso, alt)
        nome = "FONTE_VLW_%d" % num
        nomes[num] = nome
        L.append("static const uint8_t %s[%d] PROGMEM = {" % (nome, len(dados)))
        for i in range(0, len(dados), 24):
            L.append("  " + ",".join("%d" % b for b in dados[i:i + 24]) + ",")
        L.append("};")
        print("fonte %d: %s %s %d px -> %d bytes" % (num, arq, peso or "", alt, len(dados)))
    L.append("")
    L.append("// indice = numero da fonte do Nextion; nullptr = nao existe")
    L.append("static const uint8_t* const FONTES_VLW[6] = {%s};" % ", ".join(nomes.get(i, "nullptr") for i in range(6)))
    L.append("")
    SAIDA.write_text("\n".join(L), encoding="ascii")


if __name__ == "__main__":
    main()
