# Manual do Claudinho

[English](manual.md) · **Português**

Como usar o Claudinho no dia a dia, depois de montado e configurado. Para
montar e instalar, veja o [README](../README.pt-BR.md).

## O que o rosto mostra

O Claudinho reage sozinho ao que o Claude Code está fazendo:

| Cara | Quando aparece |
|---|---|
| Feliz | abriu uma sessão |
| Pensando | você mandou um prompt |
| Empolgado | prompt com "obrigado", "valeu", "funcionou", "perfeito"... |
| Preocupado | prompt com "erro", "bug", "não funciona", "travou"... |
| Susto | prompt com palavrão |
| Trabalhando | o Claude vai usar uma ferramenta |
| Desconfiado | 5 ferramentas seguidas |
| Bravo | uma ferramenta falhou |
| Esperando você | o Claude precisa da sua permissão ou de uma resposta |
| Zonzo | compactando o contexto |
| Terminou | o Claude acabou de responder |
| Cansado | parado, com o uso da janela de 5 horas acima de 75 % |
| Suando | parado, com o uso acima de 90 % |
| Dormindo | nenhuma sessão aberta, ou 3 minutos sem notícia do computador |

As palavras do humor são procuradas em português e em inglês, e o texto do
prompt nunca sai do seu computador: só a etiqueta ("feliz", "preocupado",
"susto") vai para a placa.

## O consumo do plano

- **Tocando na tela** aparece o cartão de consumo: janelas de 5 horas e de 7
  dias, quando cada uma renova, quantos terminais estão abertos e o IP da
  placa. Depois de 15 segundos ele volta ao rosto (ou toque de novo).
- **Ele também mostra sozinho**, por alguns segundos, quando o Claude termina
  de responder e o uso subiu bastante, ou quando o uso passa de 50 %, 75 % ou
  90 %. Acima de 90 % o número pisca em vermelho. Qualquer evento novo volta
  ao rosto na hora.
- Os números só aparecem depois da primeira resposta do Claude numa sessão.

## Toques na tela

| Toque | O que faz |
|---|---|
| Toque curto | alterna entre o rosto e o cartão de consumo |
| Toque longo (1,5 s) | alterna o brilho entre 100 % e 15 % |
| Toque quando pede "O PC quer me atualizar" | autoriza a atualização |

Dormindo, ele baixa o brilho sozinho depois de 20 segundos.

## Mudar a cor do rosto

Serve para combinar o rosto com a cor do filamento da caixa.

1. Peça ao Claude: "muda a cor do Claudinho" (ou rode `claudinho.sh cor`).
2. Na tela aparecem **6 cores base**. Toque na mais parecida com o filamento.
3. Aparecem **12 tons dela em volta da tela**, com o tom tocado **grande no
   centro**. Com a tela montada na caixa, os tons da borda ficam colados na
   moldura impressa: dá para comparar cada um direto com o filamento, lado a
   lado. (Fora da caixa, encoste um pedaço de filamento no centro.)
4. Achou? **Toque no centro.** Aparece o rosto nessa cor, com três botões:
   - **Gravar**: guarda a cor (continua depois de reiniciar);
   - **Voltar**: volta aos 12 tons;
   - **Cancelar**: volta à cor de antes.

Se ninguém tocar por 1 minuto, ele cancela sozinho. Os olhos ficam sempre
pretos. Quem souber a cor exata pode usar `claudinho.sh cor R G B salvar`.

## Atualizar

Peça ao Claude "atualiza o Claudinho". A tela vai mostrar "O PC quer me
atualizar": **toque nela em até 1 minuto** (ou aperte o botão BOOT da placa).
Sem o toque, nada é enviado. Isso impede que alguém troque o firmware pela
rede sem estar na frente dele.

A atualização leva uns 40 segundos e ele reinicia sozinho. Se a rede cair no
meio, ele continua na versão atual; é só pedir de novo.

## Pedir ao Claude

Não precisa decorar comando. Dentro do Claude Code, é só pedir:

- "atualiza o Claudinho"
- "muda a cor do Claudinho"
- "mostra o consumo no Claudinho"
- "o Claudinho parou de reagir" (a skill faz o diagnóstico)
- "troquei o Wi-Fi, reconfigura o Claudinho"

A lista completa de comandos está no [README](../README.pt-BR.md#comandos), e
os problemas mais comuns, em [Problemas comuns](../README.pt-BR.md#problemas-comuns).
