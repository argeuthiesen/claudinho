# Translating Claudinho

**English** · [Português](TRANSLATING.pt-BR.md)

Every text on Claudinho's screen lives in this folder, one file per language:
`en.txt` (English, the reference), `pt-BR.txt` (Brazilian Portuguese), and
yours, if you'd like to add one. You don't need to know how to program.

## Adding a language

1. Copy `en.txt` to a new file named after the language code: `de.txt`,
   `es.txt`, `fr.txt`, `it.txt`...
2. Translate the part after ` = ` on each line. Leave the key (before the
   ` = `) as it is. Lines starting with `#` are comments (in `en.txt`, the
   `# Bambu:` lines show the printer's original message, to help you).
3. Change `idioma.nome` to the language's name, in the language itself
   (`Deutsch`, `Español`...).
4. Open a pull request. You can do all of this in GitHub's web editor.

A line you leave out or empty shows up in English, so you can send a partial
translation and finish it later.

## The rules (the generator checks them for you)

- **Keep `%d`, `%s`, `%02d`, `%%`...** exactly as they are, same kind and same
  count. A number or a name goes there. You may move them around in the
  sentence.
- **It has to fit on the screen.** The screen is 320 × 240 pixels and each
  text has a box (`caixas.txt` says which font and how many pixels). The
  generator measures every line with the real font and tells you which one is
  too long, by how much. If a sentence doesn't fit, look for a shorter way to
  say it.
- **Printer warnings** (`hms.NNN`): at most 50 characters. They show in two
  lines of large text.
- **Only Latin characters**: the screen font has ISO-8859-1 (Western European
  accents: á ç ñ ö ß...). No emoji, no curly quotes, no "…"; also no `"` or `\`.
  Languages with other alphabets need new fonts: open an issue and let's talk.

## Checking and building

```bash
pip install pillow                 # once, for the width check
python3 idiomas/gerar.py --check   # checks every language, writes nothing
python3 idiomas/gerar.py           # generates firmware/claudinho/textos.h
```

To see it on a real Claudinho: build and flash the firmware
(`firmware/compilar.sh`, then `scripts/claudinho.sh atualizar`) and pick the
language with `scripts/claudinho.sh idioma de`. Sample screens:
`claudinho.sh alerta bom|ruim|filamento|hms`, `claudinho.sh cena codando`,
`claudinho.sh consumo`.

## Thanks

The English printer warnings came from Mathias's (bserking) round-display
build, [claudinho_GC9A01](https://github.com/bserking/claudinho_GC9A01).
Klingon is welcome, as long as it's written in Latin letters (`Qapla'!`).
