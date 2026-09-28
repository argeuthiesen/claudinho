---
name: configurar
description: Configura o Claudinho (mascote do Claude Code em ESP32 + display Nextion) do zero ou depois de trocar de rede - acha a placa no USB, grava o firmware, configura o Wi-Fi, grava a tela do Nextion, liga a status line e testa. Use também para diagnosticar quando o Claudinho não reage, e para atualizar o firmware.
---

# Configurar o Claudinho

Você vai conduzir a pessoa pela configuração, rodando os scripts do plugin. Fale
em linguagem simples, um passo por vez, e diga o que está fazendo antes de cada
comando. **Nunca peça a senha do Wi-Fi no chat:** ela é digitada pela pessoa
num terminal, escondida, pelo `wifi.sh` (passo 4), e não passa pela conversa.

Em **todo** comando, use estas variáveis (já resolvidas pelo Claude Code):

```bash
export R="${CLAUDE_PLUGIN_ROOT}"; export CLAUDINHO_DADOS="${CLAUDE_PLUGIN_DATA}"
```

Os scripts funcionam em Linux, macOS, WSL e Windows (Git Bash). No WSL e no
Windows a porta USB é do Windows (ex.: `COM6`) e o `esptool.exe` é usado
automaticamente. `bash "$R/scripts/comum.sh"` não imprime nada; para saber o
ambiente: `bash -c 'source "$R/scripts/comum.sh"; ambiente'`.

## Antes de tudo: o ambiente foi testado?

Confira o ambiente (`ambiente` do `comum.sh`) e a placa contra esta lista.

**Testado de ponta a ponta (tudo funciona):**
- WSL2 no Windows 11, ESP32-C3 Super Mini, Nextion NX3224F024_011.

**Escrito para funcionar, mas ainda sem teste real:**
- Linux nativo, macOS (Intel e Apple), Windows com Git Bash.
- ESP32-S3 DevKitC-1 (o firmware compila, o fluxo é o mesmo).
- Qualquer outro modelo de Nextion (a tela incluída é só para o NX3224F024).

Se o ambiente ou a placa **não estiver na lista testada**, diga isso à pessoa
**antes do passo 1**, em uma frase, e combine o jeito de trabalhar:

1. Rode um passo por vez e **confira a saída** antes do próximo (não confie no
   código de saída sozinho): porta encontrada, `Hash of data verified`,
   `CFG OK`, `"wifi":"conectado"`, `ok, reiniciando`.
2. Se um script falhar, **não repita às cegas**. Leia o erro, descubra a causa
   no ambiente dela (nome da porta, permissão, ferramenta faltando, `stty`
   diferente, PowerShell ausente) e proponha a correção. Se precisar, escreva
   uma versão ajustada do script para o caso dela e explique o que mudou.
3. Peça à pessoa para agir só no que exige mãos (trocar cabo, apertar BOOT,
   dar permissão, instalar algo com senha de administrador).
4. Ao final, sugira que ela conte o que precisou mudar (issue no repositório),
   para o ambiente entrar na lista de testados.

Nunca afirme que algo funcionou sem ter visto a confirmação correspondente.

## Como funcionam as atualizações

- **Primeira gravação: sempre pelo USB** (passo 3). A placa nova não tem Wi-Fi
  configurado nem o servidor HTTP do Claudinho.
- **Depois: pela rede (OTA),** com `claudinho.sh atualizar`. O PC envia o
  firmware por HTTP direto para a placa (`POST /ota`, com o segredo). Nada de
  servidor ou nuvem. Leva ~20 s e a placa reinicia sozinha.
- **Precisa de alguém na frente do Claudinho** (firmware 1.0.9 em diante): o
  script pede, a tela mostra "Toque na tela para permitir" e a pessoa tem
  1 minuto para tocar (ou apertar BOOT na placa). **Avise antes de rodar**
  `atualizar` ou `tela`. Sem o toque nada é enviado. Isso impede que alguém
  que descubra o segredo na rede grave outro firmware de longe.
- **É seguro:** o ESP32 grava o firmware novo na partição reserva e só troca
  se a gravação terminar e for validada. Se a rede cair no meio, continua
  rodando o firmware antigo; basta repetir.
- **A tela do Nextion também vai pela rede** (`claudinho.sh tela`), passando
  pelo ESP32. Se falhar no meio aparece "System Data Error": repita, não estraga.
- **Quando o OTA não é possível:** placa fora da rede, IP mudou (rode esta skill
  de novo para achar o IP pelo USB), segredo diferente (reconfigure pelo passo
  4), ou firmware anterior a 1.0.0 (grave pelo USB). Nesses casos, volte ao USB.
- Firmware de outra fonte pode tirar o OTA; se a pessoa gravar outro programa
  na placa, a volta é sempre pelo USB (passo 3).

## Pasta do projeto: LittleClaude

A configuração deve ser feita numa pasta só dela, chamada **LittleClaude**
(ex.: `~/LittleClaude` no Linux/macOS/WSL, `C:\Users\<nome>\LittleClaude` no
Windows). Assim o que for feito para o Claudinho (arquivos, anotações, testes)
fica junto e não se mistura com outros projetos.

Se o diretório de trabalho desta sessão não se chamar `LittleClaude`, antes do
passo 0 oriente a pessoa, em poucas palavras, a criar a pasta, sair do Claude
Code e abri-lo de novo dentro dela (`mkdir ~/LittleClaude && cd ~/LittleClaude && claude`),
e então pedir a configuração de novo. Se ela preferir seguir onde está, siga;
não é obrigatório.

## 0. Já está configurado?

```bash
bash "$R/scripts/claudinho.sh" info
```

Se responder um JSON com `"versao"`, o Claudinho está na rede: pule para o que a
pessoa pediu (diagnóstico → passo 7; atualizar → `claudinho.sh atualizar`;
trocar tela → passo 5). Se disser "ainda não configurado" ou não responder, siga
do passo 1.

## 1. Hardware

Confirme com a pessoa (mostre a tabela se ela ainda não montou):

| Fio do Nextion | ESP32-C3 Super Mini | ESP32-S3 DevKitC-1 |
|---|---|---|
| vermelho 5V | 5V | 5V |
| preto GND | GND | GND |
| azul TX | pino RX (GPIO 20) | GPIO 18 |
| amarelo RX | pino TX (GPIO 21) | GPIO 17 |

Peça para ligar o ESP32 no computador com um **cabo USB de dados** (cabo só de
carga não funciona).

**Fonte depois de montado:** ESP32 e Nextion juntos pedem uma fonte de 5 V de
**pelo menos 1 A**; fonte fraca pode derrubar o Wi-Fi nos picos de consumo.
Nunca ligar a fonte e o USB do PC ao mesmo tempo.
Se `atualizar` ou `tela` falhar com resposta vazia: é Wi-Fi perdendo pacotes.
Não estraga nada (a placa descarta o envio incompleto). Olhe o sinal no
`claudinho.sh log` (linha `wifi:`), confira a fonte, e repita; se continuar,
`claudinho.sh reiniciar` e tente de novo (a placa reconecta ao Wi-Fi).

## 2. Achar a placa

```bash
bash "$R/scripts/porta.sh"
```

Guarde a porta impressa (ex.: `COM6`, `/dev/ttyACM0`, `/dev/cu.usbmodem101`).
Se falhar: trocar o cabo; segurar o botão **BOOT** enquanto pluga o USB e tentar
de novo; no Linux, se der permissão negada, `sudo usermod -aG dialout $USER` e
sair/entrar da sessão.

## 3. Gravar o firmware (~30 s)

```bash
bash "$R/scripts/gravar.sh" PORTA
```

Se sair `PRECISA_BOOT`, é o caso mais comum com placa que já tinha outro
programa (visto em teste real): peça para **segurar BOOT, tirar e pôr o USB,
soltar BOOT**, e rode de novo. A porta pode mudar de número; rode o passo 2 de novo.

Espere ver `Hash of data verified`. Isso apaga qualquer configuração anterior da
placa. O display vai mostrar "Olá! Sou o Claudinho." (ou lixo, se o Nextion ainda
tiver a tela de fábrica — normal, o passo 5 resolve).

## 4. Wi-Fi e segredo

Liste as redes que a placa enxerga (só 2,4 GHz):

```bash
bash "$R/scripts/serial.sh" PORTA SCAN REDE "SCAN FIM" 40
```

Pergunte **só o nome** da rede. A senha a pessoa digita ela mesma, num
terminal, sem aparecer: assim ela não passa pela conversa e fica gravada
**só na placa**. Monte o comando com o caminho real (resolva `$R` e
`$CLAUDINHO_DADOS` antes de mostrar) e peça para a pessoa abrir um terminal
**fora do Claude Code** (no Windows, o mesmo tipo de terminal em que o Claude
Code roda: WSL ou Git Bash) e rodar:

```bash
CLAUDINHO_DADOS="<dados>" bash "<R>/scripts/wifi.sh" PORTA "NOME DA REDE"
```

O script pede a senha escondida, gera o segredo do PC, envia pela USB, espera
a placa entrar no Wi-Fi e grava IP, segredo e MAC no computador. Ele termina
com `Pronto!`, o IP e o MAC; peça para a pessoa avisar quando aparecer (ou
colar o erro). Se disser que não entrou no Wi-Fi: senha errada ou rede de
5 GHz, e é só rodar de novo. Confira do seu lado com
`bash "$R/scripts/claudinho.sh" info`.

Se a pessoa não tiver como abrir outro terminal e **fizer questão** de passar
a senha pelo chat, avise uma vez que ela ficará registrada na conversa, e só
então rode você mesmo, passando a senha pela entrada padrão:
`printf '%s\n' 'SENHA' | bash "$R/scripts/wifi.sh" PORTA "NOME DA REDE"`.

**Diga à pessoa o MAC e o IP** e recomende reservar esse IP no roteador (DHCP
estático); se o IP mudar, o Claudinho para de reagir até rodar esta skill de novo.

## 5. Tela do Nextion (~40 s, pela rede)

O plugin traz a tela pronta para o NX3224F024 (Discovery 2,4"). A imagem
é feita para a tela girada 270, que é a única montagem possível: a área útil
do Nextion não fica no centro da placa, e só nessa posição ela fica
centralizada na caixa. Não existe versão para outra orientação.

Avise que vai pedir um toque na tela e rode:

```bash
bash "$R/scripts/claudinho.sh" tela
```

A tela mostra "Toque na tela para permitir"; depois do toque, responde
`ok, reiniciando`. Se o Nextion mostrar "System Data Error", repita o
comando (é upload incompleto, não estraga nada). Tela branca depois: 
`claudinho.sh reiniciar`.

## 6. Status line

Os hooks do plugin já estão ativos. A status line (que manda o uso do plano) o
plugin não pode ligar sozinho; este script liga, com backup, e preserva uma
status line que a pessoa já tenha:

```bash
python3 "$R/scripts/instalar-statusline.py" "$CLAUDINHO_DADOS"
```

## 7. Testar

```bash
bash "$R/scripts/claudinho.sh" cara prompt feliz
bash "$R/scripts/claudinho.sh" info
```

Pergunte se o Claudinho fez `> <` (olhos apertados de alegria). O `info` deve
mostrar `"local":true` e os números do plano depois da próxima resposta.
Para diagnóstico sem cabo: `claudinho.sh log`.

## Mudar a cor do rosto

Quando a pessoa quiser trocar a cor (por exemplo, para combinar com o filamento),
rode `bash "$R/scripts/claudinho.sh" cor` e explique o que vai aparecer na tela:
6 cores base; tocando numa, 12 tons dela numa moldura em volta da tela, com o
tom tocado grande no centro. Com a tela montada, os tons da borda ficam colados
na moldura impressa da caixa: diga que dá para comparar cada um direto com o
filamento, lado a lado;
tocando no centro, uma prévia do rosto com **Gravar**, **Voltar** (à moldura) ou
**Cancelar** (volta à cor de antes). 1 minuto sem toque cancela. A cor gravada continua depois de reiniciar. Os olhos ficam
sempre pretos. Quem souber a cor exata: `claudinho.sh cor R G B salvar`.

## Jogo da velha

Quando a pessoa quiser jogar ("vamos jogar velha", "quero jogar com o
Claudinho"), rode `bash "$R/scripts/claudinho.sh" velha`. É um passatempo que
roda inteiro na placa: quem joga contra a pessoa é o próprio ESP32, sem gastar
token. Explique em uma frase: ela é o X, toca nos quadrados; no fim o Claudinho
reage (triste se ela ganhar, empolgado se ele ganhar) e começa outra partida
sozinho; para sair, 3 toques rápidos no mesmo quadrado (ou 2 minutos sem tocar).

## Genius

Quando a pessoa quiser jogar Genius (ou "o jogo das cores", "Simon"), rode
`bash "$R/scripts/claudinho.sh" genius`. Também roda inteiro na placa, sem
token. Explique em uma frase: o Claudinho acende uma sequência de cores e ela
repete tocando; a cada acerto a sequência cresce uma cor; errou, ele mostra o
placar e começa de novo; para sair, 3 toques rápidos no mesmo quadrante (ou 2
minutos sem tocar).

## Referência rápida

`claudinho.sh info | log | cara <tipo> [humor] | cor [R G B [salvar]] | velha | genius | reiniciar | consumo [s] | atualizar [bin] | tela [tft]`

`atualizar` e `tela` pedem um toque na tela do Claudinho (avise a pessoa antes).
Tipos de cara: inicio, prompt, ferramenta, erro, parou, atencao, compact, fim, dormir.
Humor (com prompt): feliz, preocupado, susto.
