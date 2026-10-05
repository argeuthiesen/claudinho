# Traduzindo o Claudinho

[English](TRANSLATING.md) · **Português**

Todo texto da tela do Claudinho fica nesta pasta, um arquivo por idioma:
`en.txt` (inglês, a referência), `pt-BR.txt` (português do Brasil) e o seu,
se quiser acrescentar um. Não precisa saber programar.

## Acrescentar um idioma

1. Copie o `en.txt` para um arquivo novo com o código do idioma: `de.txt`,
   `es.txt`, `fr.txt`, `it.txt`...
2. Traduza o que vem depois do ` = ` em cada linha. Deixe a chave (antes do
   ` = `) como está. Linhas que começam com `#` são comentários (no `en.txt`,
   as linhas `# Bambu:` mostram a mensagem original da impressora, para
   ajudar).
3. Troque o `idioma.nome` pelo nome do idioma, no próprio idioma
   (`Deutsch`, `Español`...).
4. Abra um pull request. Dá para fazer tudo pelo editor do site do GitHub.

Linha que ficar de fora, ou vazia, aparece em inglês: dá para mandar uma
tradução parcial e terminar depois.

## As regras (o gerador confere para você)

- **Mantenha `%d`, `%s`, `%02d`, `%%`...** exatamente como estão, do mesmo
  tipo e na mesma quantidade. Ali entra um número ou um nome. Pode mudar a
  posição deles na frase.
- **Tem que caber na tela.** A tela tem 320 × 240 pixels e cada texto tem uma
  caixa (o `caixas.txt` diz a fonte e quantos pixels). O gerador mede cada
  linha com a fonte de verdade e avisa qual passou, e quanto. Se a frase não
  couber, procure um jeito mais curto de dizer.
- **Avisos da impressora** (`hms.NNN`): no máximo 50 caracteres. Aparecem em
  duas linhas de letra grande.
- **Só caracteres latinos**: a fonte da tela tem o ISO-8859-1 (acentos da
  Europa ocidental: á ç ñ ö ß...). Nada de emoji, aspas curvas ou "…"; também
  não pode `"` nem `\`. Idiomas com outro alfabeto precisam de fontes novas:
  abra uma issue e a gente conversa.

## Conferir e gerar

```bash
pip install pillow                 # uma vez, para conferir a largura
python3 idiomas/gerar.py --check   # confere todos os idiomas, não grava nada
python3 idiomas/gerar.py           # gera o firmware/claudinho/textos.h
```

Para ver num Claudinho de verdade: compile e grave o firmware
(`firmware/compilar.sh`, depois `scripts/claudinho.sh atualizar`) e escolha o
idioma com `scripts/claudinho.sh idioma de`. Telas de exemplo:
`claudinho.sh alerta bom|ruim|filamento|hms`, `claudinho.sh cena codando`,
`claudinho.sh consumo`.

## Agradecimentos

Os avisos da impressora em inglês vieram da versão com tela redonda do
Mathias (bserking), [claudinho_GC9A01](https://github.com/bserking/claudinho_GC9A01).
Klingon é bem-vindo, desde que escrito em letras latinas (`Qapla'!`).
