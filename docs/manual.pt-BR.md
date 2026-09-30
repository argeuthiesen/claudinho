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
| Trabalhando | o Claude vai usar uma ferramenta (ou uma das cenas abaixo) |
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

## Cenas enquanto o Claude trabalha

Enquanto o Claude usa ferramentas, no lugar da cara de trabalhando o
Claudinho mostra uma cena, com um Claudinho pequeno (com bracinhos e
perninhas) no canto:

| O Claude está... | Cena |
|---|---|
| editando um arquivo | um editor de código: um terminal digita `code claudinho.ino`, e o código colorido vai sendo digitado e rolando |
| rodando um comando | um terminal digitando comandos, com a saída rolando |
| lendo ou procurando no código | a chuva do Matrix |
| delegando para um subagente | um organograma: o Claude em cima, os agentes surgindo embaixo |

É tudo de mentira, guardado na placa: o código e os comandos da tela são
inventados. Do seu computador sai só a categoria (editando, terminal, lendo,
agente); nome de arquivo e comando nunca saem. Cada cena fica pelo menos 8
segundos, para a tela não piscar; 12 segundos sem ferramenta, volta ao rosto.
Qualquer outro evento (terminou, precisa de você, erro) volta ao rosto na
hora. Tocar mostra o cartão de consumo. Para ver uma: `claudinho.sh cena
codando` (`terminal`, `lendo`, `agente`).

## O consumo do plano

- **Tocando na tela** aparece o cartão de consumo: janelas de 5 horas e de 7
  dias, quando cada uma renova, quantos terminais estão abertos e o IP da
  placa. Depois de 15 segundos ele volta ao rosto (ou toque fora dos botões).
- **Os botões de baixo** (no cartão de consumo e no painel da impressora):
  - **Tokens** e **Impressora** alternam entre as duas telas; uma moldura
    dourada fina marca a atual. Sem impressora configurada, só aparece
    Tokens.
  - **Manter**: a tela fica ligada, atualizando ao vivo, e não volta mais
    para o rosto dormindo. O botão passa a dizer **Dormir**: tocando nele,
    o Claudinho volta a dormir. Bom para levar para outro cômodo como
    monitor portátil da impressora. Os alertas da impressora continuam
    aparecendo e, depois do seu toque, ele volta para a tela mantida.
- **Ele também mostra sozinho**, por alguns segundos, quando o Claude termina
  de responder e o uso subiu bastante, ou quando o uso passa de 50 %, 75 % ou
  90 %. Acima de 90 % o número pisca em vermelho. Qualquer evento novo volta
  ao rosto na hora.
- Os números só aparecem depois da primeira resposta do Claude numa sessão.

## Toques na tela

| Toque | O que faz |
|---|---|
| Toque curto | no rosto: abre o cartão de consumo; nos cartões: os botões (fora deles, volta ao rosto) |
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

## Jogo da velha

Um passatempo: peça ao Claude "vamos jogar velha" (ou rode `claudinho.sh
velha`). Quem joga contra você é o próprio Claudinho, rodando na placa, sem
gastar token, e cada partida ele está com um humor: às vezes joga perfeito, às
vezes vacila.

- Você é o **X**: toque no quadrado. Ele responde logo depois, com o **O**.
- No fim, ele risca a linha vencedora e reage: **triste** se você ganhou,
  **empolgado** se ele ganhou, **desconfiado** no empate. Depois começa outra
  partida sozinho, e quem começa alterna.
- Para sair: **3 toques rápidos no mesmo quadrado**, ou 2 minutos sem tocar.
- Se o Claude Code precisar de você no meio do jogo, aparece um aviso no canto.

## Genius

Peça ao Claude "vamos jogar Genius" (ou rode `claudinho.sh genius`). Também
roda na placa, sem gastar token.

- O Claudinho acende uma sequência de cores nos 4 quadrantes; você repete
  tocando na mesma ordem. O número no meio é a rodada.
- A cada acerto a sequência ganha mais uma cor e acelera um pouco.
- Errou: ele mostra o seu placar ("Score") com uma cara que depende de quão
  longe você foi, e começa outro jogo sozinho.
- Para sair: **3 toques rápidos no mesmo quadrante** (os toques certos da
  sequência não contam, então dá para jogar sem medo), ou 2 minutos sem tocar.

## Impressora Bambu Lab (opcional)

Se você tem uma impressora 3D da Bambu Lab, o Claudinho acompanha ela também.
Ele conecta direto na impressora, pela rede de casa: sem nuvem, sem servidor,
sem gastar token. **Testado na P2S** (com AMS); outros modelos Bambu com
acesso local (X1, P1, A1) devem funcionar, mas ainda não foram testados.

Para ligar, peça ao Claude "tenho uma impressora Bambu" (ou rode
`claudinho.sh bambu IP_DA_IMPRESSORA`). Você vai precisar do IP e do **código
de acesso** da impressora (configurações > Rede). O código é um segredo, como
a senha do Wi-Fi: você digita escondido, no seu terminal, e ele fica gravado
só na placa.

**O painel.** O modelo da impressora e o estado (imprimindo, pausada...), a
umidade e a temperatura do AMS (medidas dentro dele, onde ficam os
carretéis), três colunas (quanto já imprimiu, tempo que falta, camada atual),
barra de progresso na cor do filamento que está imprimindo, temperatura do
bico e da mesa, e as 4 cores do AMS com a atual em destaque. (O nome que você
deu à impressora no app fica só na nuvem da Bambu, por isso o painel mostra o
modelo. O nome da impressão também não aparece: pela rede local a impressora
só manda o "projeto + placa" do Bambu Studio ou o nome do perfil do
MakerWorld, que não dizem qual é a peça.) Imprimindo, ele aparece sozinho a
cada 5 minutos por 15 segundos. Abra quando quiser pelo botão **Impressora**
do cartão de consumo, e use **Manter** para deixar na tela.

**Os alertas** mostram "IMPRESSORA 3D" e o modelo no alto, um título grande na
cor do alerta, o nome da peça, o detalhe e um Claudinho pequeno embaixo:
pulando nas notícias boas, abanando os braços quando algo precisa de você.
Ficam na tela **até você tocar** (o toque quer dizer "li"); se juntarem
vários, aparecem um depois do outro:

| Alerta | Quando |
|---|---|
| Começou | começou uma impressão, com o tempo previsto |
| Pausada | com o motivo: acabou o filamento, você pausou, bico entupido, erro na 1ª camada, tampa frontal, temperatura... |
| Retomou | voltou a imprimir depois da pausa |
| Faltam 5 min | faltam 5 minutos |
| Trocou o filamento | troca de filamento, com a cor nova (um alerta só, atualizado) |
| Terminou! | com quanto tempo levou |
| Falhou / Cancelada | falhou (com o código do erro) ou foi cancelada |
| Avisos HMS | os avisos de saúde da impressora, em palavras simples (veja abaixo) |
| AMS úmido | a umidade do AMS chegou a 50 % |

**Avisos HMS em português.** As impressoras Bambu avisam problemas com códigos
HMS (como `0500-0200-0002-0005`). O Claudinho leva a lista inteira da Bambu
(uns 2.000 códigos), reescrita em frases curtas: o título é a área (AMS,
bico / extrusora, mesa aquecida, rede / internet...), o detalhe diz o que
aconteceu, e o código fica pequeno embaixo, para procurar no wiki da Bambu. A
cor segue a gravidade: azul é informativo, amarelo pede atenção, vermelho é
grave.

**Toque ou 20 segundos.** Os avisos informativos (sem internet, acerto do
relógio, câmera ao vivo, vida do filtro...) **saem sozinhos em 20 segundos** e
não se repetem por 30 minutos: se a internet cair e voltar, não fica
pipocando. Todo o resto espera o seu toque.

Num jogo, na paleta de cores ou numa atualização, os alertas esperam você
voltar ao rosto. Para ver como ficam: `claudinho.sh alerta bom` (notícia
boa), `ruim` (uma pausa), `filamento` ou `hms`. Para desligar: `claudinho.sh bambu desligar` (apaga o código
da placa).

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
- "tenho uma impressora Bambu" / "mostra o painel da impressora"

A lista completa de comandos está no [README](../README.pt-BR.md#comandos), e
os problemas mais comuns, em [Problemas comuns](../README.pt-BR.md#problemas-comuns).
