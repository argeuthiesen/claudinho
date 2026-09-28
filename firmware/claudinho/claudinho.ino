/*
 * Claudinho — o mascote do Claude Code, vivo, na sua mesa.
 * ESP32-C3 Super Mini (ou ESP32-S3) + Nextion Discovery NX3224F024 (320x240).
 *
 * Tela 0 (padrao): os olhos do Clawd, pixelados, na tela inteira, reagindo ao
 *                  que o Claude Code esta fazendo (hooks) e ao uso do plano.
 * Tela 1 (toque):  janelas de 5 h e 7 dias, reset, terminais abertos e o IP.
 * Toque longo:     brilho 100 % / 15 %.
 *
 * Sem servidor e sem credencial da Anthropic: o PC (status line e hooks do
 * Claude Code) faz POST direto aqui, na rede local, com um segredo proprio.
 *
 * Configuracao (Wi-Fi e segredo) fica gravada na placa, nao no codigo. Na
 * primeira vez, pela serial USB (115200), linha a linha:
 *   INFO                      -> CLAUDINHO {"versao":..,"mac":..,"ip":..,...}
 *   SCAN                      -> REDE {"ssid":..,"rssi":..} ... SCAN FIM
 *   CFG {"ssid":..,"senha":..,"token":..}  -> CFG OK e reinicia
 * A skill do plugin faz isso sozinha.
 *
 * HTTP, tudo com "Authorization: Bearer <token>": POST /estado, /evento,
 * /cmd; GET /mini.json e /log; /ota (upload do .bin) e /tft?tam=N (upload da
 * tela) exigem ainda um toque na tela (ver manutencao). GET / so diz a versao.
 */

#include <WiFi.h>
#include <esp_wifi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Update.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include "config.h"

// ---------------------------------------------------------------- cores 565
#define RGB565(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
static const uint16_t COR_FUNDO   = RGB565(0x36, 0x34, 0x35);
static const uint16_t COR_BLOCO   = RGB565(0x42, 0x40, 0x41);
static const uint16_t COR_OURO    = RGB565(0xd0, 0xad, 0x6c);
static const uint16_t COR_ALERTA  = RGB565(0xd9, 0xa2, 0x5a);
static const uint16_t COR_CRITICO = RGB565(0xc9, 0x6f, 0x6f);
static const uint16_t COR_OK      = RGB565(0x7b, 0xbf, 0x7b);
static const uint16_t COR_TEXTO   = RGB565(0xf2, 0xf3, 0xf4);
static const uint16_t COR_APAGADO = RGB565(0xa5, 0xa0, 0x9d);
static const uint16_t COR_TRILHO  = RGB565(0x55, 0x53, 0x54);
static const uint16_t COR_AZUL    = RGB565(0x4a, 0x8f, 0xd9);
// Rosto: o Clawd (mascote do Claude Code) — corpo laranja, dois retangulos
// pretos verticais como olhos, "> <" quando fecha. O fundo e a cor do corpo
// impresso, para o display parecer a propria logo viva.
static const uint16_t COR_ROSTO = RGB565(240, 105, 30);        // afinado ao vivo com o filamento laranja (2026-09-26)
static const uint16_t COR_OLHO  = RGB565(0x1e, 0x1c, 0x1c);   // preto
static const uint16_t COR_ZZZ   = RGB565(0x8a, 0x44, 0x10);   // marrom, discreto sobre o laranja
static const uint16_t COR_BRAVO = RGB565(0x8c, 0x0e, 0x0e);   // olhos vermelho-escuro
static const uint16_t COR_BRANCO = RGB565(0xff, 0xf4, 0xe6);

static const int FONTE_P = 0;   // 16 px
static const int FONTE_G = 1;   // 48 px
static const int FONTE_M = 2;   // 24 px negrito
static const int FONTE_32 = 3;  // 32 px
static const int LARG = 320;

// ---------------------------------------------------------------- estado
struct Dados {
  int  h5 = 0, d7 = 0, ctx = 0, n = 0;
  long h5r = 0, d7r = 0, at = 0;
  char mod[17] = "";
  bool ok = false;
};
Dados dados;

enum Servidor { SRV_INICIANDO, SRV_OK, SRV_SEM_WIFI, SRV_FALHA };
Servidor servidor = SRV_INICIANDO;

enum Cara { C_DORMINDO, C_NEUTRO, C_PENSANDO, C_TRABALHANDO, C_ESPERANDO, C_TERMINOU,
            C_FELIZ, C_EMPOLGADO, C_PREOCUPADO, C_SUSTO, C_ZONZO, C_CANSADO, C_SUANDO,
            C_BRAVO, C_TRISTE, C_DESCONFIADO };
// Um olho pixelado, em CELULAS (nao pixels). Definido antes de qualquer funcao:
// o Arduino gera protótipos no topo e precisa do tipo ja existente.
struct Olho {
  int w = 5, h = 11;        // tamanho em celulas (retangulo vertical, como o Clawd)
  int dx = 0, dy = 0;       // olhar (deslocamento em celulas)
  int palpebra = 0;         // linhas escondidas a partir de cima (cansado, piscar)
  int fundo = 0;            // linhas escondidas a partir de baixo
  bool domo = false;        // topo em arco (feliz)
  int diag = 0;             // 0 nada, +1 corta canto externo de cima (triste), -1 corta o interno
  int diagN = 0;            // quantas linhas o corte diagonal alcanca
  bool xis = false;         // olho em X (zonzo)
  int chev = 0;             // 0 quadrado, 1 = "<" piscadela (aponta para o outro olho), 2 = "^" feliz
};
struct Caixa { int x = 0, y = 0, w = 0, h = 0; };
struct Forma { int chev = -1, xis = 0, diag = 0, diagN = 0, domo = 0; uint16_t cor = 0;
  bool operator==(const Forma& f) const { return chev == f.chev && xis == f.xis && diag == f.diag && diagN == f.diagN && domo == f.domo && cor == f.cor; } };
enum BocaP { BP_NENHUMA, BP_SORRISO, BP_ABERTA, BP_O, BP_O_PEQ, BP_TRISTE, BP_BRAVA, BP_ONDA, BP_RETA };
struct Expr { Olho e, d; BocaP boca = BP_NENHUMA; uint16_t cor = 0; int bocaDy = 0; };
Cara caraNaTela = C_NEUTRO;
bool caraDesenhada = false;
struct { Cara cara = C_NEUTRO; unsigned long ate = 0; } evento;   // cara vinda de evento, com validade

int  pagina = 0;
unsigned long paginaDesde = 0;
unsigned long paginaDur = VOLTA_PAGINA_MS;    // quanto a tela de numeros fica antes de voltar ao rosto

// Atualizacao pela rede (firmware e tela) so com alguem na frente do
// Claudinho: o PC pede (/cmd {"manutencao":true}), a tela pede um toque (ou o
// botao BOOT) e so entao /ota e /tft aceitam arquivo, por MANUT_JANELA_MS.
// Quem so conhece o segredo, de longe, nao troca o firmware.
unsigned long manutPedidaEm = 0, manutAte = 0;

// Consumo automatico: a tela de numeros aparece sozinha em alguns momentos
// (boas-vindas, fim de resposta com uso subindo, limite cruzado). O pedido
// fica na fila e so e atendido com o Claudinho parado (sem cara de evento),
// para nunca esconder "esperando voce" nem atrapalhar o trabalho.
struct {
  bool pendente = false, soSeSubiu = false, intenso = false;
  unsigned long dur = 0;
  bool naTela = false, telaIntensa = false;   // o que esta sendo mostrado agora
} consumo;
int  mostrado5 = -1, mostrado7 = -1;          // uso na ultima exibicao
int  faixa5 = -1, faixa7 = -1;                // maior limite (0/50/75/90) ja avisado na janela
bool renovou5 = false, renovou7 = false;
unsigned long ultimoIntenso = 0;
static const unsigned long INTENSO_INTERVALO_MS = 5UL * 60 * 1000;
bool brilhoAlto = true;

HardwareSerial& nex = Serial1;

// Configuracao gravada na placa (NVS). Vazia de fabrica.
Preferences prefs;
String cfgSsid, cfgSenha, cfgToken;
// Abrir a porta serial reinicia a placa. Enquanto ela ainda tenta entrar no
// Wi-Fi, INFO nao e respondido (responderia "sem conexao" cedo demais); o
// proprio setup imprime o INFO quando termina.
bool conectandoWifi = false;

// ---------------------------------------------------------------- primitivas (protótipos p/ structs)
void preenche(int x, int y, int w, int h, uint16_t cor);
void escreve(int x, int y, int w, int h, int fonte, uint16_t cor, uint16_t fundo, int alin, const char* t);

struct Campo {
  int x, y, w, h, fonte, alin;
  uint16_t fundo;
  char ultimo[40] = "";
  uint16_t corUltima = 0;
  void mostra(const char* t, uint16_t cor) {
    if (strcmp(t, ultimo) == 0 && cor == corUltima) return;
    strlcpy(ultimo, t, sizeof ultimo);
    corUltima = cor;
    escreve(x, y, w, h, fonte, cor, fundo, alin, t);
  }
  void limpa() { ultimo[0] = 0; corUltima = 0; }
};

struct Barra {
  int x, y, w, h;
  int pctUltimo = -1;
  uint16_t corUltima = 0;
  void mostra(int pct, uint16_t cor) {
    if (pct == pctUltimo && cor == corUltima) return;
    pctUltimo = pct; corUltima = cor;
    preenche(x, y, w, h, COR_TRILHO);
    int usado = (long)w * constrain(pct, 0, 100) / 100;
    if (usado > 0) preenche(x, y, usado, h, cor);
  }
  void limpa() { pctUltimo = -1; }
};

// ---------------------------------------------------------------- layout
Campo cTitulo  = {12,  6, 200, 20, FONTE_P, 0, COR_FUNDO};
Campo cRelogio = {240, 6,  68, 20, FONTE_P, 2, COR_FUNDO};
Campo cRodape  = {0, 222, LARG, 18, FONTE_P, 1, COR_FUNDO};

// Tela 0: grade de celulas de 8 px (40 x 30 celulas). Centros dos olhos.
static const int CEL = 8;
static const int OLHO_ESQ_X = 6, OLHO_DIR_X = 34, OLHO_Y = 11;    // em celulas: bem afastados, como no mascote; boca em y 21..24

// Tela 1: blocos completos
struct BlocoJanela {
  int y;
  Campo rotulo, pct, reseta, resta;
  Barra barra;
  BlocoJanela(int y0) : y(y0),
    rotulo {12,  y0 + 4,  220, 18, FONTE_P, 0, COR_BLOCO},
    pct    {12,  y0 + 24, 112, 50, FONTE_G, 0, COR_BLOCO},
    reseta {132, y0 + 28, 176, 18, FONTE_P, 0, COR_BLOCO},
    resta  {132, y0 + 50, 176, 18, FONTE_P, 0, COR_BLOCO},
    barra  {12,  y0 + 80, 296, 8} {}
  void limpa() { rotulo.limpa(); pct.limpa(); reseta.limpa(); resta.limpa(); barra.limpa(); }
};
BlocoJanela b5h(34), b7d(130);

// ---------------------------------------------------------------- nextion
// O Nextion tem buffer de 1 KB e desenha mais devagar do que 115200 baud
// entrega. Sem folga entre comandos, uma troca de cara (30-40 fills) estoura o
// buffer e ele descarta pedacos: sobra lixo na tela. Cada comando espera o
// anterior render; "cls" (tela inteira) ganha folga maior.
void nexCmd(const char* cmd) {
  nex.print(cmd);
  nex.write(0xFF); nex.write(0xFF); nex.write(0xFF);
  nex.flush();
  delay(strncmp(cmd, "cls", 3) == 0 ? 40 : 5);
}
void nexCmdf(const char* fmt, ...) {
  char buf[220];
  va_list ap; va_start(ap, fmt);
  vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);
  nexCmd(buf);
}
#define DEBUG_FILLS 0     // registra cada fill no serial (para simular a tela no PC)
void preenche(int x, int y, int w, int h, uint16_t cor) {
#if DEBUG_FILLS
  Serial.printf("F %d %d %d %d %u\n", x, y, w, h, cor);
#endif
  nexCmdf("fill %d,%d,%d,%d,%u", x, y, w, h, cor);
}
// Limpa a tela com fill (o "cls" se perdeu em campo e deixou lixo); duas
// vezes, porque perder este e o unico erro que fica visivel ate a proxima troca.
void limpaTela(uint16_t cor) { preenche(0, 0, 320, 240, cor); delay(30); preenche(0, 0, 320, 240, cor); delay(30); }
void escreve(int x, int y, int w, int h, int fonte, uint16_t cor, uint16_t fundo, int alin, const char* t) {
  nexCmdf("xstr %d,%d,%d,%d,%d,%u,%u,%d,1,1,\"%s\"", x, y, w, h, fonte, cor, fundo, alin, t);
}
void circulo(int x, int y, int r, uint16_t cor) { nexCmdf("cirs %d,%d,%d,%u", x, y, r, cor); }
// linha grossa (2 px): o Nextion so desenha 1 px
void linha(int x1, int y1, int x2, int y2, uint16_t cor) {
  nexCmdf("line %d,%d,%d,%d,%u", x1, y1, x2, y2, cor);
  nexCmdf("line %d,%d,%d,%d,%u", x1, y1 + 1, x2, y2 + 1, cor);
}

// Log: vai para a serial e para um buffer circular lido em GET /log, para
// dar para diagnosticar sem cabo.
static char logBuf[3072]; static size_t logPos = 0; static bool logCheio = false;
void registra(const char* fmt, ...) {
  char buf[200]; va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
  Serial.println(buf);
  char linha[220]; int n = snprintf(linha, sizeof linha, "%lu %s\n", millis() / 1000, buf);
  for (int k = 0; k < n; k++) { logBuf[logPos++] = linha[k]; if (logPos >= sizeof logBuf) { logPos = 0; logCheio = true; } }
}

// ---------------------------------------------------------------- tempo
void horaStr(time_t t, char* out, size_t n) {
  struct tm tm; localtime_r(&t, &tm);
  snprintf(out, n, "%02d:%02d", tm.tm_hour, tm.tm_min);
}
void diaHoraStr(time_t t, char* out, size_t n) {
  static const char* DIAS[] = {"dom", "seg", "ter", "qua", "qui", "sex", "sab"};
  struct tm tm, hoje; time_t agora = time(nullptr);
  localtime_r(&t, &tm); localtime_r(&agora, &hoje);
  if (tm.tm_yday == hoje.tm_yday && tm.tm_year == hoje.tm_year)
    snprintf(out, n, "hoje %02d:%02d", tm.tm_hour, tm.tm_min);
  else
    snprintf(out, n, "%s %02d:%02d", DIAS[tm.tm_wday], tm.tm_hour, tm.tm_min);
}
void restanteStr(long ate, char* out, size_t n) {
  long s = ate - (long)time(nullptr); if (s < 0) s = 0;
  long d = s / 86400, h = (s % 86400) / 3600, m = (s % 3600) / 60;
  if (d > 0)      snprintf(out, n, "em %ld d %ld h", d, h);
  else if (h > 0) snprintf(out, n, "em %ld h %02ld min", h, m);
  else            snprintf(out, n, "em %ld min", m);
}
bool relogioValido() { return time(nullptr) > 1600000000L; }
void ajustaRelogio(long agora) {
  if (agora > 1600000000L) { struct timeval tv = { (time_t)agora, 0 }; settimeofday(&tv, nullptr); }
}
uint16_t corPct(int pct) { return pct >= 90 ? COR_CRITICO : pct >= 75 ? COR_ALERTA : COR_OURO; }
bool congelado() { return dados.ok && dados.at > 0 && relogioValido() && (long)time(nullptr) - dados.at > CONGELADO_APOS_S; }

// ---------------------------------------------------------------- rosto
// Cada olho e desenhado linha a linha (uma celula de altura), com um "fill"
// por linha. Antes de desenhar, apaga so a caixa que ocupou no quadro anterior.
// Alem dos olhos: boca (so nas emocoes fortes) e "extras" (pontinhos, !, ?,
// gota, lagrima, zzz), cada um com a sua caixa.
void celula(int cx, int cy, int w, int h, uint16_t cor) { preenche(cx * CEL, cy * CEL, w * CEL, h * CEL, cor); }

Caixa ultimaEsq, ultimaDir, ultimaExtra, ultimaBoca;
// O que esta na tela agora (o simulador de PC compara com um desenho limpo).
Expr exprNaTela; Cara extrasCaraNaTela = C_NEUTRO; int extrasFaseNaTela = 0;
uint16_t corRostoAtual = COR_ROSTO;          // escolhida na paleta (claudinho.sh cor) ou por /cmd {"cor":[r,g,b]}; gravada na placa
uint16_t corRosto() { return corRostoAtual; }
void apaga(Caixa& c) { if (c.w) { celula(c.x, c.y, c.w, c.h, corRosto()); c = {}; } }

void recorte(const Olho& o, int r, int& corteExt, int& corteInt) {
  corteExt = corteInt = 0;
  int deBaixo = o.h - 1 - r, arred = 0;
  if (o.w >= 10)     { if (r == 0 || deBaixo == 0) arred = 2; else if (r == 1 || deBaixo == 1) arred = 1; }
  else if (o.w >= 7) { if (r == 0 || deBaixo == 0) arred = 1; }
  corteExt = corteInt = arred;                       // menor que 7: retangulo crisp, como o Clawd
  if (o.domo) { int meio = o.h / 2; if (r < meio) { int c = meio - r; corteExt = max(corteExt, c); corteInt = max(corteInt, c); } }
  if (o.diag != 0 && r < o.diagN) { int c = o.diagN - r; if (o.diag > 0) corteExt = max(corteExt, c); else corteInt = max(corteInt, c); }
}

Forma formaEsq, formaDir;

// Apaga o que sobrou da caixa antiga fora da caixa nova (ate 4 tiras). Como a
// parte comum foi redesenhada por cima na mesma cor, nao ha piscada.
void apagaSobra(const Caixa& velha, const Caixa& nova) {
  if (!velha.w) return;
  int vx2 = velha.x + velha.w, vy2 = velha.y + velha.h, nx2 = nova.x + nova.w, ny2 = nova.y + nova.h;
  bool cruza = velha.x < nx2 && nova.x < vx2 && velha.y < ny2 && nova.y < vy2;
  if (!cruza) { celula(velha.x, velha.y, velha.w, velha.h, corRosto()); return; }
  if (nova.y > velha.y) celula(velha.x, velha.y, velha.w, nova.y - velha.y, corRosto());           // tira de cima
  if (ny2 < vy2)        celula(velha.x, ny2, velha.w, vy2 - ny2, corRosto());                       // tira de baixo
  int ya = max(velha.y, nova.y), yb = min(vy2, ny2);
  if (nova.x > velha.x) celula(velha.x, ya, nova.x - velha.x, yb - ya, corRosto());                 // tira da esquerda
  if (nx2 < vx2)        celula(nx2, ya, vx2 - nx2, yb - ya, corRosto());                            // tira da direita
}

void desenhaOlho(const Olho& o, int centroX, bool ladoEsquerdo, uint16_t cor, Caixa& ultima, Forma& ultimaForma) {
  int x0 = centroX + o.dx - o.w / 2, y0 = OLHO_Y + o.dy - o.h / 2;
  Forma f; f.chev = o.chev; f.xis = o.xis; f.diag = o.diag; f.diagN = o.diagN; f.domo = o.domo; f.cor = cor;
  bool retangulo = (o.chev == 0 && !o.xis && o.diag == 0 && !o.domo && o.w < 7);
  // Forma igual e retangulo: desenha por cima e apaga so a sobra (sem piscar).
  // Forma diferente ou desenho esparso (chevron, X, cortes): apaga antes.
  bool suave = retangulo && (f == ultimaForma) && ultima.w;
  if (!suave) apaga(ultima);

  Caixa nova;
  if (o.chev == 1) {
    // "<" no olho direito (ponta para a esquerda), ">" no esquerdo. A caixa
    // precisa cobrir TODAS as colunas do traco (meio + espessura), nao so o.w:
    // foi daqui que sobravam pixels perdidos entre uma cara e outra.
    int n = o.h | 1, meio = n / 2, oy = y0 + (o.h - n) / 2, esp = o.w >= 5 ? 2 : 1;
    int larg = meio + esp;
    int xIni = ladoEsquerdo ? (x0 + o.w - larg) : x0;
    for (int r = 0; r < n; r++) {
      int passo = abs(r - meio);
      int cx = ladoEsquerdo ? (x0 + o.w - esp - passo) : (x0 + passo);
      celula(cx, oy + r, esp, 1, cor);
    }
    nova = {xIni, oy, larg, n};
  } else if (o.chev == 2) {
    int n = o.w | 1, meio = n / 2, ox = x0 + (o.w - n) / 2;
    for (int c = 0; c < n; c++) celula(ox + c, y0 + 1 + abs(c - meio), 1, 2, cor);
    nova = {ox, y0 + 1, n, meio + 2};
  } else if (o.xis) {
    int n = min(o.w, o.h) - 1, ox = x0 + (o.w - n) / 2, oy = y0 + (o.h - n) / 2;
    if (o.fundo == 99) { celula(ox + n / 2, oy, 1, n, cor); celula(ox, oy + n / 2, n, 1, cor); }
    else for (int k = 0; k < n; k++) { celula(ox + k, oy + k, 1, 1, cor); celula(ox + n - 1 - k, oy + k, 1, 1, cor); }
    nova = {ox, oy, n, n};
  } else if (retangulo) {
    int alt = o.h - o.palpebra - o.fundo;
    if (alt > 0) celula(x0, y0 + o.palpebra, o.w, alt, cor);
    nova = {x0, y0 + o.palpebra, o.w, max(alt, 0)};
  } else {
    for (int r = 0; r < o.h; r++) {
      if (r < o.palpebra || r >= o.h - o.fundo) continue;
      int ce, ci; recorte(o, r, ce, ci);
      int esq = ladoEsquerdo ? ce : ci, dir = ladoEsquerdo ? ci : ce, larg = o.w - esq - dir;
      if (larg > 0) celula(x0 + esq, y0 + r, larg, 1, cor);
    }
    nova = {x0, y0 + o.palpebra, o.w, o.h - o.palpebra - o.fundo};
  }
  if (suave) apagaSobra(ultima, nova);
  ultima = nova; ultimaForma = f;
}

// ---- boca: so nas emocoes fortes; no neutro o Clawd fica sem boca
BocaP bocaNaTela = BP_NENHUMA;
void desenhaBoca(BocaP b, int dy = 0) {
  (void)dy;
  if (b == bocaNaTela && (b == BP_NENHUMA || ultimaBoca.w)) return;
  apaga(ultimaBoca); bocaNaTela = b;
  if (b == BP_NENHUMA) return;
  int y = 21; ultimaBoca = {14, y - 1, 13, 6};
  uint16_t c = COR_OLHO;
  switch (b) {
    case BP_SORRISO: celula(15, y, 1, 1, c); celula(25, y, 1, 1, c); celula(16, y + 1, 1, 1, c); celula(24, y + 1, 1, 1, c); celula(17, y + 2, 7, 1, c); break;
    case BP_ABERTA:  celula(15, y, 11, 1, c); celula(16, y + 1, 9, 2, c); celula(17, y + 3, 7, 1, c); break;
    case BP_O:       celula(18, y, 5, 4, c); celula(19, y + 1, 3, 2, corRosto()); break;
    case BP_O_PEQ:   celula(19, y + 1, 3, 2, c); break;
    case BP_TRISTE:  celula(17, y, 7, 1, c); celula(16, y + 1, 1, 1, c); celula(24, y + 1, 1, 1, c); celula(15, y + 2, 1, 1, c); celula(25, y + 2, 1, 1, c); break;
    case BP_BRAVA:   celula(23, y, 3, 1, c); celula(19, y + 1, 4, 1, c); celula(15, y + 2, 4, 1, c); break;
    case BP_ONDA:    celula(15, y, 2, 1, c); celula(19, y, 2, 1, c); celula(23, y, 2, 1, c); celula(17, y + 1, 2, 1, c); celula(21, y + 1, 2, 1, c); celula(25, y + 1, 1, 1, c); break;
    case BP_RETA:    celula(16, y + 1, 9, 1, c); break;
    default: break;
  }
}

// ---- extras (uma caixa so, apagada e redesenhada a cada quadro animado)
// Z em pixels, desenho 5x5 com unidade de u px (u = 4, 6, 8: pequeno, medio, grande)
void zetaPx(int x, int y, int u, uint16_t cor) {
  preenche(x, y, 5 * u, u, cor);
  for (int k = 0; k < 3; k++) preenche(x + (3 - k) * u, y + (k + 1) * u, u, u, cor);
  preenche(x, y + 4 * u, 5 * u, u, cor);
}
void glifoInterrogacao(int x, int y, uint16_t c) {
  celula(x + 1, y, 3, 1, c); celula(x, y + 1, 1, 1, c); celula(x + 4, y + 1, 1, 2, c);
  celula(x + 3, y + 3, 1, 1, c); celula(x + 2, y + 4, 1, 1, c); celula(x + 2, y + 6, 1, 1, c);
}
void glifoExclamacao(int x, int y, uint16_t c) { celula(x, y, 2, 5, c); celula(x, y + 6, 2, 1, c); }

static Cara extrasCUlt = C_NEUTRO; static int extrasChaveUlt = -1;
void esqueceExtras() { extrasCUlt = C_NEUTRO; extrasChaveUlt = -1; }
void extras(Cara c, int fase) {
  extrasCaraNaTela = c; extrasFaseNaTela = fase;
  Cara& cUlt = extrasCUlt; int& chaveUlt = extrasChaveUlt;
  int chave;                                            // o que de fato muda com a fase, por cara
  switch (c) {
    case C_DORMINDO:  chave = (fase / 4) % 4; break;
    case C_PENSANDO:  chave = (fase / 2) % 4; break;
    case C_ESPERANDO: chave = (fase / 2) % 2; break;
    case C_SUANDO:    chave = fase % 6; break;
    case C_TRISTE:    chave = fase % 8; break;
    default:          chave = 0;
  }
  if (c == cUlt && chave == chaveUlt && (ultimaExtra.w || c == C_NEUTRO)) return;
  cUlt = c; chaveUlt = chave;
  apaga(ultimaExtra);
  // Tudo no espaco ENTRE os olhos (x 13..27), que fica livre.
  switch (c) {
    case C_DORMINDO: {                                // zzz subindo entre os olhos
      int n = (fase / 4) % 4;
      ultimaExtra = {14, 0, 13, 13};
      if (n >= 1) zetaPx(112, 76, 4, COR_ZZZ);      // pequeno, perto dos olhos
      if (n >= 2) zetaPx(140, 40, 6, COR_ZZZ);      // medio
      if (n >= 3) zetaPx(176,  0, 8, COR_ZZZ);      // grande, la em cima
    } break;
    case C_PENSANDO: {                                // pontinhos, como balao de pensamento
      int n = (fase / 2) % 4;
      ultimaExtra = {16, 3, 9, 2};
      for (int k = 0; k < n; k++) celula(17 + k * 3, 4, 1, 1, COR_OLHO);
    } break;
    case C_ESPERANDO:                                 // "?" piscando
      ultimaExtra = {17, 1, 6, 8};
      if ((fase / 2) % 2 == 0) glifoInterrogacao(18, 2, COR_OLHO);
      break;
    case C_SUSTO:                                     // "!" grande
      ultimaExtra = {19, 1, 3, 8};
      glifoExclamacao(19, 1, COR_OLHO);
      break;
    case C_SUANDO: {                                  // gota escorrendo, do lado de fora do olho direito
      int q = fase % 6;
      ultimaExtra = {37, 2, 3, 13};
      celula(38, 2 + q, 1, 2, COR_AZUL); celula(37, 4 + q, 3, 2, COR_AZUL); celula(38, 6 + q, 1, 1, COR_AZUL);
    } break;
    case C_TRISTE: {                                  // lagrima caindo do olho esquerdo
      int q = fase % 8;
      ultimaExtra = {5, 18, 2, 12};
      celula(5, 18 + q, 2, 2, COR_AZUL); celula(5, 20 + q, 1, 1, COR_AZUL);
    } break;
    default: break;
  }
}

// A expressao de cada cara: olhos, boca e cor. fase avanca a cada 250 ms.

void expressao(Cara c, int fase, Expr& x) {
  x = Expr(); x.cor = COR_OLHO;
  Olho& e = x.e; Olho& d = x.d;
  int quica = ((fase / 2) & 1) ? -1 : 0;              // pulinho alternado a cada 500 ms
  switch (c) {
    case C_NEUTRO: break;                                                        // dois retangulos (o Clawd)
    case C_DORMINDO:    e.palpebra = d.palpebra = 10; e.dy = d.dy = 2; break;    // so um traco embaixo + zzz
    case C_PENSANDO:    e.dx = d.dx = 2; e.dy = d.dy = -2; d.palpebra = 4; e.palpebra = 1; break;
    case C_TRABALHANDO: { int dx = (fase & 1) ? 3 : -3; e.dx = d.dx = dx;
                          e.h = d.h = 7; e.dy = d.dy = 2; } break;               // estreitos, varrendo largo
    case C_ESPERANDO:   e.w = d.w = 6; e.h = d.h = 14; x.boca = BP_O_PEQ; break;
    case C_SUSTO:       e.w = d.w = 8; e.h = d.h = 16; e.dy = d.dy = (fase < 2) ? -2 : 0;
                        x.boca = BP_O; break;                                    // pula e fica enorme
    case C_TERMINOU:    d.chev = 1; d.h = 11; break;                              // piscadela "<"
    case C_FELIZ:       e.chev = d.chev = 2; e.w = d.w = 7; e.dy = d.dy = quica;
                        x.boca = BP_SORRISO; break;                              // "^ ^" quicando, sorriso
    case C_EMPOLGADO:   e.chev = d.chev = 1; e.h = d.h = 13; e.dy = d.dy = quica;
                        x.boca = BP_ABERTA; break;                               // "> <" quicando, boca aberta
    case C_BRAVO:       e.diag = d.diag = -1; e.diagN = d.diagN = 5; e.dx = 1; d.dx = -1;
                        { int tr = (fase < 6) ? ((fase & 1) ? 1 : -1) : 0; e.dx += tr; d.dx += tr; }
                        x.cor = COR_BRAVO; x.boca = BP_BRAVA; break;             // vermelho, franzido, tremendo
    case C_PREOCUPADO:  e.diag = d.diag = 1; e.diagN = d.diagN = 5; e.dy = d.dy = 2;
                        x.boca = BP_TRISTE; break;
    case C_TRISTE:      e.h = d.h = 7; e.dy = d.dy = 4; e.dx = -1; d.dx = 1; x.boca = BP_TRISTE; break;
    case C_DESCONFIADO: d.palpebra = 5; e.w = 6; e.h = 12; e.dx = d.dx = -2; x.boca = BP_RETA; break;
    case C_ZONZO:       e.xis = d.xis = true; e.w = d.w = 7; e.h = d.h = 7;
                        e.fundo = d.fundo = (fase & 1) ? 99 : 0; x.boca = BP_ONDA; break;   // X / + girando
    case C_CANSADO:     e.palpebra = d.palpebra = 5; e.dy = d.dy = 1; break;
    case C_SUANDO:      e.palpebra = d.palpebra = 5; e.diag = d.diag = 1; e.diagN = d.diagN = 2; e.dy = d.dy = 1;
                        x.boca = BP_ONDA; break;
  }
}

void desenhaExpr(const Expr& x) {
  exprNaTela = x;
  desenhaOlho(x.e, OLHO_ESQ_X, true,  x.cor, ultimaEsq, formaEsq);
  desenhaOlho(x.d, OLHO_DIR_X, false, x.cor, ultimaDir, formaDir);
  desenhaBoca(x.boca);
}

bool caraAnimada(Cara c) {
  return c == C_TRABALHANDO || c == C_DORMINDO || c == C_PENSANDO || c == C_ESPERANDO || c == C_SUSTO ||
         c == C_FELIZ || c == C_EMPOLGADO || c == C_BRAVO || c == C_ZONZO || c == C_SUANDO || c == C_TRISTE;
}
bool caraPisca(Cara c) { return c == C_NEUTRO || c == C_CANSADO || c == C_ESPERANDO || c == C_PREOCUPADO || c == C_DESCONFIADO || c == C_PENSANDO; }

Cara caraBase() {
  if (!dados.ok || congelado() || dados.n == 0) return C_DORMINDO;   // n == 0: todas as sessoes fechadas
  if (dados.h5 >= 90) return C_SUANDO;
  if (dados.h5 >= 75) return C_CANSADO;
  return C_NEUTRO;
}
Cara caraDesejada() {
  if (evento.ate && (long)(millis() - evento.ate) < 0) return evento.cara;
  if (evento.ate && (evento.cara == C_DESCONFIADO || evento.cara == C_BRAVO)) {
    evento.cara = C_TRABALHANDO; evento.ate = millis() + 120000;
    return C_TRABALHANDO;
  }
  return caraBase();
}
void poeCara(Cara c, unsigned long dur) { evento.cara = c; evento.ate = millis() + dur; caraDesenhada = false; }

void trataEvento(const char* tipo, const char* humor) {
  static int ferramentasSeguidas = 0;
  if (strcmp(tipo, "ferramenta") != 0 && strcmp(tipo, "erro") != 0) ferramentasSeguidas = 0;
  if (consumo.naTela && pagina != 0) mudaPagina(0);    // qualquer evento volta ao rosto na hora
  if (pagina == 6 && !strcmp(tipo, "atencao")) avisoVelha();   // no jogo: so avisa no canto
  if      (!strcmp(tipo, "inicio"))     { poeCara(C_FELIZ, 5000); pedeConsumo(20000, false, false); }
  else if (!strcmp(tipo, "prompt")) {
    if      (!strcmp(humor, "feliz"))      poeCara(C_EMPOLGADO, 5000);
    else if (!strcmp(humor, "preocupado")) poeCara(C_PREOCUPADO, 6000);
    else if (!strcmp(humor, "susto"))      poeCara(C_SUSTO, 5000);
    else                                   poeCara(C_PENSANDO, 120000);
  }
  else if (!strcmp(tipo, "ferramenta")) {
    if (++ferramentasSeguidas == 5) poeCara(C_DESCONFIADO, 4000);
    else if (caraDesejada() != C_DESCONFIADO) poeCara(C_TRABALHANDO, 120000);
  }
  else if (!strcmp(tipo, "erro"))       poeCara(C_BRAVO, 3500);
  else if (!strcmp(tipo, "parou"))      { poeCara(C_TERMINOU, 5000); pedeConsumo(15000, true, false); }
  else if (!strcmp(tipo, "atencao"))    poeCara(C_ESPERANDO, 120000);
  else if (!strcmp(tipo, "compact"))    poeCara(C_ZONZO, 5000);
  else if (!strcmp(tipo, "dormir"))     poeCara(C_DORMINDO, 20000);    // so para teste (claudinho.sh cara dormir)
  else if (!strcmp(tipo, "fim"))        { evento.ate = 0; caraDesenhada = false; }   // quem decide e o contador de sessoes (estado)
}

// Chamado a cada volta do loop na tela 0: troca de cara, animacao, piscada e
// os "olhares" do neutro (de vez em quando olha pro lado, para parecer vivo).
void cuidaRosto() {
  static unsigned long ultimaFase = 0, proximaPiscada = 0, piscadaEm = 0, proximoOlhar = 0, olharAte = 0;
  static int fase = 0, quadroPiscada = -1, olharDx = 0;
  Cara c = caraDesejada();
  unsigned long agora = millis();
  Expr x;

  if (!caraDesenhada || c != caraNaTela) {
    caraNaTela = c; caraDesenhada = true; fase = 0;
    Serial.printf("CARA %d\n", (int)c);
    expressao(c, fase, x); desenhaExpr(x); extras(c, fase);
    proximaPiscada = agora + 2500 + random(3000); quadroPiscada = -1;
    proximoOlhar = agora + 4000 + random(5000); olharAte = 0;
    return;
  }
  if (agora - ultimaFase >= 250) {
    ultimaFase = agora; fase++;
    if (caraAnimada(c)) {
      expressao(c, fase, x);
      if (c == C_DORMINDO || c == C_PENSANDO || c == C_ESPERANDO || c == C_SUANDO || c == C_TRISTE) extras(c, fase);
      else { desenhaExpr(x); if (c == C_SUSTO) extras(c, fase); }
    }
  }
  // neutro: olhadinha pro lado a cada poucos segundos
  if (c == C_NEUTRO && quadroPiscada < 0) {
    if (!olharAte && agora >= proximoOlhar) { olharAte = agora + 700; olharDx = random(2) ? 2 : -2;
      expressao(c, fase, x); x.e.dx = x.d.dx = olharDx; desenhaExpr(x); }
    else if (olharAte && agora >= olharAte) { olharAte = 0; proximoOlhar = agora + 4000 + random(6000);
      expressao(c, fase, x); desenhaExpr(x); }
  }
  if (!caraPisca(c)) return;
  unsigned long dur = (c == C_CANSADO) ? 220 : 45;
  if (quadroPiscada < 0 && agora >= proximaPiscada) { quadroPiscada = 0; piscadaEm = 0; }
  if (quadroPiscada >= 0 && agora - piscadaEm >= dur) {
    piscadaEm = agora;
    expressao(c, fase, x);
    if (olharAte) { x.e.dx = x.d.dx = olharDx; }
    const int PALP[] = {x.e.h * 4 / 10, x.e.h - 1, x.e.h * 4 / 10, 0}; int pp = PALP[quadroPiscada];
    x.e.palpebra = max(x.e.palpebra, pp); x.d.palpebra = max(x.d.palpebra, pp);
    desenhaExpr(x);
    if (++quadroPiscada > 3) { quadroPiscada = -1; proximaPiscada = agora + (c == C_CANSADO ? 1500 : 2500) + random(4000); }
  }
}

// ---------------------------------------------------------------- telas
void desenhaMoldura() {
  cTitulo.limpa(); cRelogio.limpa(); cRodape.limpa();
  if (pagina == 0) {
    limpaTela(corRosto());
    caraDesenhada = false; ultimaEsq = {}; ultimaDir = {}; ultimaExtra = {}; ultimaBoca = {};
    formaEsq = Forma(); formaDir = Forma(); bocaNaTela = BP_NENHUMA; esqueceExtras();
  } else if (pagina >= 6) {
    // os jogos desenham a propria tela
  } else if (pagina >= 3) {
    desenhaPaleta();
  } else if (pagina == 2) {
    limpaTela(COR_FUNDO);
    escreve(0,  40, 320, 30, FONTE_M,  COR_OURO,    COR_FUNDO, 1, "O PC quer me atualizar");
    escreve(0,  90, 320, 38, FONTE_32, COR_TEXTO,   COR_FUNDO, 1, "Toque na tela");
    escreve(0, 130, 320, 38, FONTE_32, COR_TEXTO,   COR_FUNDO, 1, "para permitir");
    escreve(0, 196, 320, 20, FONTE_P,  COR_APAGADO, COR_FUNDO, 1, "(ou aperte BOOT na placa)");
  } else {
    limpaTela(COR_FUNDO);
    preenche(6, b5h.y, 308, 92, COR_BLOCO);
    preenche(6, b7d.y, 308, 92, COR_BLOCO);
    b5h.limpa(); b7d.limpa();
  }
}

void desenhaCabecalho() {
  char t[24];
  snprintf(t, sizeof t, "CLAUDE CODE");
  cTitulo.mostra(t, COR_OURO);
  if (relogioValido()) { horaStr(time(nullptr), t, sizeof t); cRelogio.mostra(t, COR_TEXTO); }
  else cRelogio.mostra("--:--", COR_APAGADO);
}

void desenhaRodape() {
  char t[48], h[16];
  if (servidor == SRV_SEM_WIFI)       { cRodape.mostra("sem wifi", COR_CRITICO); return; }
  if (servidor == SRV_INICIANDO)      { cRodape.mostra("conectando...", COR_APAGADO); return; }
  if (!dados.ok || dados.at == 0) {
    snprintf(t, sizeof t, "aguardando o PC - %s", WiFi.localIP().toString().c_str());
    cRodape.mostra(t, COR_APAGADO); return;
  }
  if (congelado()) {
    diaHoraStr(dados.at, h, sizeof h);
    snprintf(t, sizeof t, "dormindo desde %s", h);
    cRodape.mostra(t, COR_APAGADO);
  } else {
    snprintf(t, sizeof t, "%d terminal%s - %s", dados.n, dados.n == 1 ? "" : "is", WiFi.localIP().toString().c_str());
    cRodape.mostra(t, COR_OK);
  }
}

void desenhaJanela(BlocoJanela& b, const char* nome, int pct, long reseta, bool semana) {
  char t[40];
  b.rotulo.mostra(nome, COR_APAGADO);
  bool vencida = reseta > 0 && relogioValido() && (long)time(nullptr) >= reseta;
  if (dados.ok && vencida) {             // numero velho de uma janela que ja renovou
    b.pct.mostra("0%", COR_TEXTO); b.reseta.mostra("renovou!", COR_OK); b.resta.mostra("", COR_TEXTO);
    b.barra.mostra(0, COR_OURO); return;
  }
  if (dados.ok) {
    // Na exibicao automatica, o numero acima de 75 % sai na cor do alerta; no
    // modo intenso (>= 90 %) pisca a cada segundo (desenha() roda a cada 1 s).
    uint16_t cor = COR_TEXTO;
    if (consumo.naTela && pct >= 75) cor = corPct(pct);
    if (consumo.naTela && consumo.telaIntensa && pct >= 90 && (millis() / 1000) % 2) cor = COR_BLOCO;
    snprintf(t, sizeof t, "%d%%", pct); b.pct.mostra(t, cor);
    if (reseta > 0 && relogioValido()) {
      char h[20];
      if (semana) diaHoraStr(reseta, h, sizeof h); else horaStr(reseta, h, sizeof h);
      snprintf(t, sizeof t, "reseta %s", h); b.reseta.mostra(t, COR_APAGADO);
      restanteStr(reseta, t, sizeof t);      b.resta.mostra(t, COR_TEXTO);
    } else { b.reseta.mostra(pct == 0 ? "janela livre" : "", COR_APAGADO); b.resta.mostra("", COR_TEXTO); }
  } else { b.pct.mostra("--", COR_APAGADO); b.reseta.mostra("aguardando", COR_APAGADO); b.resta.mostra("", COR_TEXTO); }
  b.barra.mostra(dados.ok ? pct : 0, corPct(pct));
}

void desenha() {
  if (pagina == 0 || pagina >= 2) return;   // olhos, manutencao ou paleta: nada a atualizar
  desenhaCabecalho();
  desenhaJanela(b5h, "SESSAO  5 HORAS", dados.h5, dados.h5r, false);
  desenhaJanela(b7d, "SEMANA  7 DIAS",  dados.d7, dados.d7r, true);
  desenhaRodape();
}

void mudaPagina(int p) {
  pagina = p; paginaDesde = millis(); paginaDur = VOLTA_PAGINA_MS;
  consumo.naTela = false; consumo.telaIntensa = false;
  desenhaMoldura();
  desenha();
  if (pagina == 0) cuidaRosto();
}

// ---------------------------------------------------------------- consumo automatico
int faixaDe(int pct) { return pct >= 90 ? 90 : pct >= 75 ? 75 : pct >= 50 ? 50 : 0; }
bool usoIntenso() { return dados.h5 >= 90 || dados.d7 >= 90; }

// Junta pedidos: vale a maior duracao; "so se subiu" so se todos pedirem.
void pedeConsumo(unsigned long dur, bool soSeSubiu, bool intenso) {
  if (!consumo.pendente) { consumo.soSeSubiu = soSeSubiu; consumo.dur = dur; consumo.intenso = intenso; }
  else { consumo.soSeSubiu = consumo.soSeSubiu && soSeSubiu; consumo.dur = max(consumo.dur, dur); consumo.intenso = consumo.intenso || intenso; }
  consumo.pendente = true;
}

// Chamado quando chegam numeros novos (status line): avisa limite cruzado.
void confereLimites() {
  int f5 = faixaDe(dados.h5), f7 = faixaDe(dados.d7);
  if (faixa5 < 0) { faixa5 = f5; faixa7 = f7; mostrado5 = dados.h5; mostrado7 = dados.d7; return; }   // primeiro dado apos ligar: sem alarde
  bool subiu = false;
  if (f5 > faixa5) { registra("consumo: 5 h passou de %d%%", f5); subiu = true; }
  if (f7 > faixa7) { registra("consumo: 7 dias passou de %d%%", f7); subiu = true; }
  faixa5 = f5; faixa7 = f7;                  // tambem desce quando a janela renova
  if (subiu) { bool i = usoIntenso(); pedeConsumo(i ? 30000 : 15000, false, i); if (i) ultimoIntenso = 0; }
}

// Janela que venceu: carinha feliz e zera as referencias.
void confereRenovacao() {
  if (!dados.ok || !relogioValido()) return;
  long agora = time(nullptr);
  bool v5 = dados.h5r > 0 && agora >= dados.h5r, v7 = dados.d7r > 0 && agora >= dados.d7r;
  bool nova = (v5 && !renovou5) || (v7 && !renovou7);
  if (v5 && !renovou5) { mostrado5 = 0; faixa5 = 0; }
  if (v7 && !renovou7) { mostrado7 = 0; faixa7 = 0; }
  renovou5 = v5; renovou7 = v7;
  if (nova && dados.n > 0 && !congelado()) { registra("consumo: janela renovou"); poeCara(C_FELIZ, 5000); }
}

// Atende o pedido pendente quando o Claudinho esta parado no rosto.
void cuidaConsumo() {
  if (!consumo.pendente || pagina != 0) return;
  if (!dados.ok || dados.n == 0 || congelado()) { consumo.pendente = false; return; }
  if (evento.ate && (long)(millis() - evento.ate) < 0) return;      // ainda numa cara de evento
  consumo.pendente = false;
  bool intenso = consumo.intenso;
  if (consumo.soSeSubiu) {
    bool subiu = dados.h5 - mostrado5 >= 5 || dados.d7 - mostrado7 >= 5;
    bool vezIntensa = usoIntenso() && millis() - ultimoIntenso >= INTENSO_INTERVALO_MS;
    if (!subiu && !vezIntensa) return;
    if (vezIntensa) intenso = true;
  }
  unsigned long dur = intenso ? max(consumo.dur, 30000UL) : consumo.dur;
  registra("consumo: mostrando %lu s%s", dur / 1000, intenso ? " (intenso)" : "");
  mudaPagina(1);
  paginaDur = dur; consumo.naTela = true; consumo.telaIntensa = intenso;
  mostrado5 = dados.h5; mostrado7 = dados.d7;
  if (intenso) ultimoIntenso = millis();
}

// ---------------------------------------------------------------- paleta de cor do rosto
// claudinho.sh cor (sem numeros) abre a paleta na tela, para combinar o rosto
// com a cor do filamento. Pagina 3: 6 cores base; pagina 4: 12 variacoes da
// escolhida, sempre no MESMO tom, numa moldura em volta da tela (do mais claro
// e pastel ao mais escuro e vivo, em sentido horario), com o tom tocado grande
// no centro para comparar com o filamento; tocar no centro confirma;
// pagina 5: previa do rosto com Gravar / Voltar (as 24) / Cancelar. Tocar
// numa cor ja avanca; 1 min sem toque cancela (volta a cor de antes).
// Os olhos ficam sempre pretos: a variacao mais escura para antes de some-los.
static const unsigned long PALETA_ESPERA_MS = 60000;
struct CorBase { float h, s, v; };
static const CorBase CORES_BASE[6] = {
  {21, 0.88f, 0.94f},   // laranja Clawd
  {0,  0.85f, 0.90f},   // vermelho
  {48, 0.85f, 0.97f},   // amarelo
  {125, 0.70f, 0.75f},  // verde
  {212, 0.75f, 0.90f},  // azul
  {275, 0.60f, 0.80f},  // roxo
};
static const float VAR_S[4] = {0.50f, 0.70f, 0.85f, 1.0f};   // fracao da saturacao da base
static const float VAR_V[3] = {1.0f, 0.88f, 0.74f};           // brilho
int paletaBase = 0, paletaTom = -1;   // tom da moldura mostrado no centro (-1: a cor base)
uint16_t corAntesPaleta = COR_ROSTO;

uint16_t hsv565(float h, float s, float v) {
  h = fmodf(h + 360.0f, 360.0f);
  float c = v * s, x = c * (1 - fabsf(fmodf(h / 60.0f, 2) - 1)), m = v - c, r, g, b;
  if (h < 60)       { r = c; g = x; b = 0; } else if (h < 120) { r = x; g = c; b = 0; }
  else if (h < 180) { r = 0; g = c; b = x; } else if (h < 240) { r = 0; g = x; b = c; }
  else if (h < 300) { r = x; g = 0; b = c; } else               { r = c; g = 0; b = x; }
  return RGB565((int)((r + m) * 255), (int)((g + m) * 255), (int)((b + m) * 255));
}
uint16_t corBase(int i) { return i == 0 ? COR_ROSTO : hsv565(CORES_BASE[i].h, CORES_BASE[i].s, CORES_BASE[i].v); }
uint16_t corVariacao(int base, int lin, int col) {
  return hsv565(CORES_BASE[base].h, CORES_BASE[base].s * VAR_S[col], VAR_V[lin]);
}

// Moldura de 12 quadrados de 80 x 60: k 0-3 em cima (esq->dir), 4-5 a direita
// (cima->baixo), 6-9 embaixo (dir->esq), 10-11 a esquerda (baixo->cima).
// Centro livre: 160 x 120.
static const int CEL_W = 80, CEL_H = 60, N_TONS = 12;
void posMoldura(int k, int& x, int& y) {
  if (k < 4)       { x = k * CEL_W;         y = 0; }
  else if (k < 6)  { x = 3 * CEL_W;         y = (k - 3) * CEL_H; }
  else if (k < 10) { x = (9 - k) * CEL_W;   y = 3 * CEL_H; }
  else             { x = 0;                 y = (12 - k) * CEL_H; }
}
int tomNaMoldura(int tx, int ty) {        // -1: centro
  for (int k = 0; k < N_TONS; k++) { int x, y; posMoldura(k, x, y);
    if (tx >= x && tx < x + CEL_W && ty >= y && ty < y + CEL_H) return k; }
  return -1;
}
uint16_t corTom(int k) { return k < 0 ? corBase(paletaBase) : corVariacao(paletaBase, k / 4, k % 4); }
void desenhaCentroPaleta() {
  uint16_t c = corTom(paletaTom);
  preenche(CEL_W + 2, CEL_H + 2, 320 - 2 * CEL_W - 4, 240 - 2 * CEL_H - 4, c);
  escreve(CEL_W + 2, 150, 320 - 2 * CEL_W - 4, 18, FONTE_P, COR_OLHO, c, 1, "toque para confirmar");
}

void irPaleta(int p) { mudaPagina(p); paginaDur = PALETA_ESPERA_MS; }

void desenhaPaleta() {
  if (pagina == 3) {                                     // 6 cores base, 3 x 2
    limpaTela(COR_FUNDO);
    for (int i = 0; i < 6; i++) preenche((i % 3) * 106 + 4, (i / 3) * 120 + 4, 100, 112, corBase(i));
  } else if (pagina == 4) {                              // moldura de 12 tons + centro
    limpaTela(COR_FUNDO);
    for (int k = 0; k < N_TONS; k++) { int x, y; posMoldura(k, x, y); preenche(x + 1, y + 1, CEL_W - 2, CEL_H - 2, corTom(k)); }
    desenhaCentroPaleta();
  } else {                                               // previa: rosto + botoes
    limpaTela(corRosto());
    ultimaEsq = {}; ultimaDir = {}; ultimaExtra = {}; ultimaBoca = {};
    formaEsq = Forma(); formaDir = Forma(); bocaNaTela = BP_NENHUMA; esqueceExtras();
    Expr x; expressao(C_NEUTRO, 0, x); desenhaExpr(x);
    preenche(4,   200, 100, 36, COR_OK);      escreve(4,   208, 100, 20, FONTE_P, COR_BRANCO, COR_OK,      1, "Gravar");
    preenche(110, 200, 100, 36, COR_FUNDO);   escreve(110, 208, 100, 20, FONTE_P, COR_BRANCO, COR_FUNDO,   1, "Voltar");
    preenche(216, 200, 100, 36, COR_CRITICO); escreve(216, 208, 100, 20, FONTE_P, COR_BRANCO, COR_CRITICO, 1, "Cancelar");
  }
}

void abrePaleta() { corAntesPaleta = corRostoAtual; registra("cor: paleta aberta"); irPaleta(3); }
void cancelaPaleta(const char* porque) {
  corRostoAtual = corAntesPaleta; registra("cor: cancelada (%s)", porque); mudaPagina(0);
}
void gravaCor(uint16_t cor) {
  corRostoAtual = cor;
  prefs.begin("claudinho", false); prefs.putUShort("cor", cor); prefs.end();
  registra("cor: gravada 0x%04X", cor);
}

// Toque (soltou) em uma das telas da paleta.
void toquePaleta(int tx, int ty) {
  if (pagina == 3) { paletaBase = min(1, ty / 120) * 3 + min(2, tx / 106); paletaTom = -1; irPaleta(4); }
  else if (pagina == 4) {
    int k = tomNaMoldura(tx, ty);
    if (k >= 0) { paletaTom = k; desenhaCentroPaleta(); paginaDesde = millis(); }   // mostra no centro
    else { corRostoAtual = corTom(paletaTom); irPaleta(5); }                         // centro: confirma
  }
  else if (ty >= 192) {
    if (tx < 107)      { gravaCor(corRostoAtual); mudaPagina(0); }
    else if (tx < 213) irPaleta(4);
    else               cancelaPaleta("botao");
  }
}

// ---------------------------------------------------------------- jogo da velha
// claudinho.sh velha (a skill chama quando a pessoa pede) abre o tabuleiro na
// pagina 6. Voce e o X, o Claudinho e o O, e a jogada dele roda aqui mesmo
// (minimax), sem Claude nem token. Cada partida sorteia o quanto ele erra.
// No fim: risca a linha, mostra o rosto reagindo com uma palavra, e comeca
// outra partida sozinha (quem comeca alterna). Passatempo:
// sem placar e sem toque a mais. Sai com 3 toques rapidos no mesmo quadrado
// ou 2 min sem toque. "Esperando voce" do Claude Code vira aviso no canto.
static const unsigned long VELHA_ESPERA_MS = 120000, VELHA_PENSA_MS = 600, VELHA_TRIPLO_MS = 1500;
static const unsigned long VELHA_RISCO_MS = 1200, VELHA_REACAO_MS = 3500;   // linha riscada, depois o rosto
static const int VX = 55, VY = 15, VC = 70;          // tabuleiro 210 x 210, centralizado
int vTab[9];                        // 0 livre, 1 voce (X), 2 Claudinho (O)
int vResultado = 0;                 // 0 jogando; 1 voce ganhou, 2 Claudinho, 3 empate
bool vVoceComeca = true, vReacaoNaTela = false, vAviso = false;
float vErro = 0;                    // chance de o Claudinho jogar ao acaso, sorteada por partida
unsigned long vJogaEm = 0, vFimEm = 0;
int vUltCasa = -1, vToques = 0; unsigned long vUltToque = 0;
static const int LINHAS3[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};

int vVencedor(const int* t) {       // 1 ou 2 = venceu, 3 = empate, 0 = segue
  for (auto& l : LINHAS3) if (t[l[0]] && t[l[0]] == t[l[1]] && t[l[1]] == t[l[2]]) return t[l[0]];
  for (int i = 0; i < 9; i++) if (!t[i]) return 0;
  return 3;
}
int vMinimax(int* t, bool vezDele) {
  int r = vVencedor(t);
  if (r == 2) return 10; if (r == 1) return -10; if (r == 3) return 0;
  int melhor = vezDele ? -100 : 100;
  for (int i = 0; i < 9; i++) if (!t[i]) {
    t[i] = vezDele ? 2 : 1; int v = vMinimax(t, !vezDele); t[i] = 0;
    melhor = vezDele ? max(melhor, v) : min(melhor, v);
  }
  return melhor;
}
int vJogadaDele() {
  int livres[9], n = 0; for (int i = 0; i < 9; i++) if (!vTab[i]) livres[n++] = i;
  if (n == 9) { const int boas[5] = {0, 2, 4, 6, 8}; return boas[random(5)]; }   // tabuleiro vazio: poupa o minimax
  if (random(1000) < vErro * 1000) return livres[random(n)];
  int melhor = -100, casa = livres[0];
  for (int k = 0; k < n; k++) { int i = livres[k]; vTab[i] = 2; int v = vMinimax(vTab, false); vTab[i] = 0;
    if (v > melhor || (v == melhor && random(2))) { melhor = v; casa = i; } }
  return casa;
}

void vCentro(int i, int& cx, int& cy) { cx = VX + (i % 3) * VC + VC / 2; cy = VY + (i / 3) * VC + VC / 2; }
void vDesenhaPeca(int i) {
  int cx, cy; vCentro(i, cx, cy); const int r = 22;
  if (vTab[i] == 1) for (int d = -2; d <= 2; d++) {            // X preto grosso
    nexCmdf("line %d,%d,%d,%d,%u", cx - r + d, cy - r, cx + r + d, cy + r, COR_OLHO);
    nexCmdf("line %d,%d,%d,%d,%u", cx + r + d, cy - r, cx - r + d, cy + r, COR_OLHO);
  } else if (vTab[i] == 2) for (int d = 0; d < 5; d++)         // O branco grosso
    nexCmdf("cir %d,%d,%d,%u", cx, cy, r - d, COR_BRANCO);
}
void vDesenhaAviso() {
  if (vAviso) { escreve(0, 90, VX, 18, FONTE_P, COR_CRITICO, corRosto(), 1, "Claude");
                escreve(0, 110, VX, 18, FONTE_P, COR_CRITICO, corRosto(), 1, "chama!"); }
}
void vDesenhaTabuleiro() {
  limpaTela(corRosto());
  for (int k = 1; k < 3; k++) {                                   // grade preta
    preenche(VX + k * VC - 2, VY, 4, 3 * VC, COR_OLHO);
    preenche(VX, VY + k * VC - 2, 3 * VC, 4, COR_OLHO);
  }
  for (int i = 0; i < 9; i++) vDesenhaPeca(i);
  vDesenhaAviso();
}
void vNovaPartida() {
  for (int& c : vTab) c = 0;
  vResultado = 0; vReacaoNaTela = false;
  const float ERROS[3] = {0.0f, 0.25f, 0.5f}; vErro = ERROS[random(3)];
  vDesenhaTabuleiro();
  vJogaEm = vVoceComeca ? 0 : millis() + VELHA_PENSA_MS;
  registra("velha: nova partida (erro %d%%, %s comeca)", (int)(vErro * 100), vVoceComeca ? "voce" : "Claudinho");
}
void vFimDePartida(int r) {
  vResultado = r; vJogaEm = 0; vFimEm = millis(); vVoceComeca = !vVoceComeca;
  for (auto& l : LINHAS3) if (r != 3 && vTab[l[0]] == r && vTab[l[1]] == r && vTab[l[2]] == r) {   // risca a vitoria
    int x1, y1, x2, y2; vCentro(l[0], x1, y1); vCentro(l[2], x2, y2);
    for (int d = -2; d <= 2; d++) { nexCmdf("line %d,%d,%d,%d,%u", x1 + d, y1, x2 + d, y2, COR_CRITICO); nexCmdf("line %d,%d,%d,%d,%u", x1, y1 + d, x2, y2 + d, COR_CRITICO); }
    break;
  }
  registra("velha: %s", r == 1 ? "voce ganhou" : r == 2 ? "Claudinho ganhou" : "empate");
}
// Rosto reagindo ao resultado, na tela inteira, com uma palavra embaixo.
void vDesenhaReacao() {
  Cara c = vResultado == 1 ? C_TRISTE : vResultado == 2 ? C_EMPOLGADO : C_DESCONFIADO;
  const char* frase = vResultado == 1 ? "You win!" : vResultado == 2 ? "I win!" : "Draw!";
  limpaTela(corRosto());
  ultimaEsq = {}; ultimaDir = {}; ultimaExtra = {}; ultimaBoca = {};
  formaEsq = Forma(); formaDir = Forma(); bocaNaTela = BP_NENHUMA; esqueceExtras();
  Expr x; expressao(c, 0, x); desenhaExpr(x); extras(c, 0);
  escreve(0, 202, 320, 30, FONTE_M, COR_OLHO, corRosto(), 1, frase);
}
void abreVelha() {
  vVoceComeca = true; vAviso = false; vUltCasa = -1; vToques = 0;
  mudaPagina(6); paginaDur = VELHA_ESPERA_MS; vNovaPartida();
}
void avisoVelha() { if (!vAviso) { vAviso = true; if (!vResultado) vDesenhaAviso(); } }
void saiVelha(const char* porque) { registra("velha: saiu (%s)", porque); vJogaEm = 0; mudaPagina(0); }

void toqueVelha(int tx, int ty) {
  paginaDesde = millis();                                          // renova o tempo de inatividade
  if (tx < VX || tx >= VX + 3 * VC || ty < VY || ty >= VY + 3 * VC) return;
  int i = ((ty - VY) / VC) * 3 + (tx - VX) / VC;
  if (i == vUltCasa && millis() - vUltToque < VELHA_TRIPLO_MS) vToques++; else vToques = 1;
  vUltCasa = i; vUltToque = millis();
  if (vToques >= 3) { saiVelha("3 toques"); return; }
  if (vResultado || vJogaEm || vTab[i]) return;                    // fim de partida, vez dele, ou casa ocupada
  vTab[i] = 1; vDesenhaPeca(i);
  int r = vVencedor(vTab);
  if (r) vFimDePartida(r); else vJogaEm = millis() + VELHA_PENSA_MS;
}
void cuidaVelha() {
  if (pagina != 6) return;
  if (vResultado) {                                                // fim: risco -> rosto -> nova partida
    unsigned long t = millis() - vFimEm;
    if (!vReacaoNaTela && t > VELHA_RISCO_MS) { vReacaoNaTela = true; vDesenhaReacao(); }
    else if (t > VELHA_RISCO_MS + VELHA_REACAO_MS) vNovaPartida();
    return;
  }
  if (!vJogaEm || (long)(millis() - vJogaEm) < 0) return;
  vJogaEm = 0;
  int i = vJogadaDele(); vTab[i] = 2; vDesenhaPeca(i);
  int r = vVencedor(vTab);
  if (r) vFimDePartida(r);
}

// ---------------------------------------------------------------- genius
// claudinho.sh genius abre na pagina 7: 4 quadrantes (verde, vermelho,
// amarelo, azul); o Claudinho acende uma sequencia e voce repete tocando; a
// cada acerto ela cresce uma cor e acelera um pouco. Errou: o rosto reage com
// o placar e comeca outra sozinho. Roda na placa, sem token. Sai com 3 toques
// rapidos no mesmo quadrante: toques CERTOS da sequencia nao contam (ela pode
// pedir o mesmo quadrante varias vezes), so os fora de hora (enquanto ele
// mostra, depois de errar, na tela do placar). Ou 2 min sem toque.
static const int G_MAX = 64;
static const unsigned long G_TOQUE_ACESO_MS = 250, G_PAUSA_RODADA_MS = 800, G_REACAO_MS = 3500;
int gSeq[G_MAX], gLen = 0, gIdx = 0, gFase = 0;   // fase 0 mostrando, 1 sua vez, 2 placar
int gAceso = -1, gAcesoJ = -1;       // quadrante aceso pela sequencia / pelo seu toque
unsigned long gProx = 0, gApagaEm = 0, gFimEm = 0;
int gUltQ = -1, gToques = 0; unsigned long gUltToque = 0;
static const float G_TOM[4] = {125, 0, 50, 212};               // verde, vermelho, amarelo, azul

uint16_t gCor(int q, bool aceso) { return hsv565(G_TOM[q], aceso ? 0.75f : 0.85f, aceso ? 1.0f : 0.38f); }
void gQuadrante(int q, bool aceso) {
  preenche((q % 2) * 162, (q / 2) * 122, 158, 118, gCor(q, aceso));
  gCentro();
}
void gCentro() {                                               // rodada no circulo do meio
  char t[8]; snprintf(t, sizeof t, "%d", gLen);
  nexCmdf("cirs 160,120,26,%u", COR_OLHO);
  escreve(136, 108, 48, 26, FONTE_M, COR_BRANCO, COR_OLHO, 1, t);
}
void gDesenhaTudo() {
  limpaTela(COR_OLHO);
  for (int q = 0; q < 4; q++) preenche((q % 2) * 162, (q / 2) * 122, 158, 118, gCor(q, false));
  gCentro();
}
unsigned long gTempoAceso() { return max(250L, 520L - 18L * gLen); }
void gNovaRodada() {
  if (gLen < G_MAX) gSeq[gLen++] = random(4);
  gFase = 0; gIdx = 0; gAceso = -1; gProx = millis() + G_PAUSA_RODADA_MS;
  gCentro();
}
void gNovoJogo() { gLen = 0; gAcesoJ = -1; gDesenhaTudo(); gNovaRodada(); registra("genius: novo jogo"); }
void abreGenius() { gUltQ = -1; gToques = 0; mudaPagina(7); paginaDur = VELHA_ESPERA_MS; gNovoJogo(); }
void saiGenius(const char* porque) { registra("genius: saiu (%s)", porque); mudaPagina(0); }
void gPlacar() {
  int pontos = gLen - 1;
  gFase = 2; gFimEm = millis();
  registra("genius: errou, %d pontos", pontos);
  Cara c = pontos >= 8 ? C_EMPOLGADO : pontos >= 4 ? C_FELIZ : C_DESCONFIADO;
  char t[20]; snprintf(t, sizeof t, "Score: %d", pontos);
  limpaTela(corRosto());
  ultimaEsq = {}; ultimaDir = {}; ultimaExtra = {}; ultimaBoca = {};
  formaEsq = Forma(); formaDir = Forma(); bocaNaTela = BP_NENHUMA; esqueceExtras();
  Expr x; expressao(c, 0, x); desenhaExpr(x); extras(c, 0);
  escreve(0, 202, 320, 30, FONTE_M, COR_OLHO, corRosto(), 1, t);
}
void toqueGenius(int tx, int ty) {
  paginaDesde = millis();
  int q = (ty >= 120 ? 2 : 0) + (tx >= 160 ? 1 : 0);
  bool certo = gFase == 1 && q == gSeq[gIdx];
  if (!certo) {                                                  // so toque fora de hora conta para sair
    if (q == gUltQ && millis() - gUltToque < VELHA_TRIPLO_MS) gToques++; else gToques = 1;
    gUltQ = q; gUltToque = millis();
    if (gToques >= 3) { saiGenius("3 toques"); return; }
    if (gFase == 1) gPlacar();                                   // errou a sequencia
    return;
  }
  gToques = 0; gUltQ = -1;
  if (gAcesoJ >= 0) gQuadrante(gAcesoJ, false);
  gAcesoJ = q; gQuadrante(q, true); gApagaEm = millis() + G_TOQUE_ACESO_MS;
  if (++gIdx >= gLen) gNovaRodada();
}
void cuidaGenius() {
  if (pagina != 7) return;
  unsigned long agora = millis();
  if (gFase == 2) { if (agora - gFimEm > G_REACAO_MS) gNovoJogo(); return; }
  if (gAcesoJ >= 0 && (long)(agora - gApagaEm) >= 0) { gQuadrante(gAcesoJ, false); gAcesoJ = -1; }   // seu toque apaga
  if (gFase == 1) return;
  if ((long)(agora - gProx) < 0) return;                         // fase 0: mostrando a sequencia
  if (gAceso >= 0) { gQuadrante(gAceso, false); gAceso = -1; gProx = agora + 160; gIdx++; return; }
  if (gIdx < gLen) { gAceso = gSeq[gIdx]; gQuadrante(gAceso, true); gProx = agora + gTempoAceso(); return; }
  gFase = 1; gIdx = 0;                                           // sua vez
}

// ---------------------------------------------------------------- manutencao
bool manutLiberada() { return manutAte && (long)(millis() - manutAte) < 0; }
void liberaManutencao(const char* como) {
  manutPedidaEm = 0; manutAte = millis() + MANUT_JANELA_MS;
  registra("manutencao: liberada por %s (%lu s)", como, MANUT_JANELA_MS / 1000);
  limpaTela(COR_FUNDO);
  escreve(0,  80, 320, 38, FONTE_32, COR_OK,    COR_FUNDO, 1, "Liberado!");
  escreve(0, 130, 320, 30, FONTE_M,  COR_TEXTO, COR_FUNDO, 1, "recebendo...");
}
void cuidaManutencao() {
  if (!manutPedidaEm) return;
  if (digitalRead(BOTAO_BOOT) == LOW) { liberaManutencao("botao"); return; }
  if (millis() - manutPedidaEm > MANUT_PEDIDO_MS) { manutPedidaEm = 0; registra("manutencao: ninguem tocou; cancelada"); mudaPagina(0); }
}

// ---------------------------------------------------------------- toque
void leToque() {
  static uint8_t buf[9]; static int n = 0; static unsigned long pressaoEm = 0;
  while (nex.available()) {
    uint8_t b = nex.read();
    if (n == 0 && b != 0x67) { static int outros = 0; if (outros++ < 20) registra("serial: byte 0x%02X", b); continue; }
    buf[n++] = b;
    if (n < 9) continue;
    n = 0;
    if (buf[6] != 0xFF || buf[7] != 0xFF || buf[8] != 0xFF) continue;
    int tx = (buf[1] << 8) | buf[2], ty = (buf[3] << 8) | buf[4];
    registra("toque %s em %d,%d (pagina %d)", buf[5] == 1 ? "press" : "solta", tx, ty, pagina);
    if (buf[5] == 1) { pressaoEm = millis(); continue; }
    if (manutPedidaEm) { liberaManutencao("toque"); continue; }
    if (pagina == 6) { toqueVelha(tx, ty); continue; }
    if (pagina == 7) { toqueGenius(tx, ty); continue; }
    if (pagina >= 3) { toquePaleta(tx, ty); continue; }
    if (millis() - pressaoEm >= TOQUE_LONGO_MS) { brilhoAlto = !brilhoAlto; registra("brilho %s", brilhoAlto ? "alto" : "baixo"); }
    else { mudaPagina(pagina == 0 ? 1 : 0); Serial.printf("-> pagina %d\n", pagina); }
  }
}

// ---------------------------------------------------------------- gravar .tft no Nextion
// Protocolo de upload do Nextion (v1.2): "whmi-wris <tam>,<baud>,1", depois
// blocos de 4096 bytes, cada um confirmado com 0x05 (ou 0x08 + offset para
// pular um trecho que o Nextion ja tem igual).
static uint8_t tftBuf[TFT_BLOCO];

long tftEsperaRetorno(uint32_t timeout) {
  uint32_t t0 = millis();
  while (millis() - t0 < timeout) {
    if (!nex.available()) { delay(1); continue; }
    uint8_t b = nex.read();
    if (b == 0x05) return 0;
    if (b == 0x08) {
      uint8_t o[4]; int n = 0; uint32_t t1 = millis();
      while (n < 4 && millis() - t1 < 1000) { if (nex.available()) o[n++] = nex.read(); }
      if (n < 4) return -1;
      return (long)o[0] | ((long)o[1] << 8) | ((long)o[2] << 16) | ((long)o[3] << 24);
    }
  }
  return -1;
}

bool tftHandshake(long tamanho) {
  char cmd[48]; snprintf(cmd, sizeof cmd, "whmi-wris %ld,%d,1", tamanho, NEXTION_BAUD_RAPIDO);
  // 1) o Nextion normalmente esta em 115200 (pedimos no boot)
  nexCmd(""); nexCmd("sleep=0"); nexCmd("connect"); delay(300);
  while (nex.available()) nex.read();
  nex.print(cmd); nex.write(0xFF); nex.write(0xFF); nex.write(0xFF); nex.flush();
  if (tftEsperaRetorno(3000) == 0) { registra("tft: handshake ok a %d", NEXTION_BAUD_RAPIDO); return true; }
  // 2) depois de um erro ("System Data Error") ele reinicia em 9600
  registra("tft: sem resposta a %d, tentando 9600", NEXTION_BAUD_RAPIDO);
  nex.updateBaudRate(9600); delay(100);
  nexCmd(""); nexCmd("connect"); delay(300);
  while (nex.available()) nex.read();
  nex.print(cmd); nex.write(0xFF); nex.write(0xFF); nex.write(0xFF); nex.flush();
  delay(50);
  nex.updateBaudRate(NEXTION_BAUD_RAPIDO);   // ele troca para o baud pedido no comando
  if (tftEsperaRetorno(5000) == 0) { registra("tft: handshake ok via 9600"); return true; }
  registra("tft: Nextion nao respondeu em nenhum baud");
  return false;
}

// ---------------------------------------------------------------- modo local (sem servidor)
// O PC manda tudo direto para ca. Tudo aqui e do core do ESP32 (WebServer,
// Update, ESPmDNS): nenhuma biblioteca de terceiro.
WebServer web(80);
unsigned long ultimoLocal = 0;
bool localRecente() { return ultimoLocal && millis() - ultimoLocal < 300000UL; }

struct SessaoLocal { char id[12]; long visto; };
SessaoLocal sessoesLocais[8];
static const long SESSAO_TTL_S = 30 * 60;

int contaSessoes() {
  long agora = time(nullptr); int n = 0;
  for (auto& s : sessoesLocais) if (s.id[0] && agora - s.visto < SESSAO_TTL_S) n++;
  return n;
}
void marcaSessao(const char* id) {
  if (!id || !id[0]) return;
  long agora = time(nullptr); SessaoLocal* livre = nullptr;
  for (auto& s : sessoesLocais) {
    if (!strcmp(s.id, id)) { s.visto = agora; return; }
    if (!livre && (!s.id[0] || agora - s.visto >= SESSAO_TTL_S)) livre = &s;
  }
  if (!livre) { livre = &sessoesLocais[0]; for (auto& s : sessoesLocais) if (s.visto < livre->visto) livre = &s; }
  strlcpy(livre->id, id, sizeof livre->id); livre->visto = agora;
}
void tiraSessao(const char* id) { for (auto& s : sessoesLocais) if (id && !strcmp(s.id, id)) s.id[0] = 0; }

bool autorizado() { return cfgToken.length() >= 16 && web.header("Authorization") == String("Bearer ") + cfgToken; }

// Com varios terminais mandando numeros: na mesma janela o uso so cresce; janela antiga
// ainda vigente nao e trocada por valor atrasado de outro terminal.
void mesclaJanela(int& pct, long& reseta, JsonVariant j, long agora) {
  if (j.isNull()) return;
  int np = (int)lroundf(j["usado_pct"].as<float>()); long nr = j["reseta_em"].as<long>();
  if (nr < reseta && reseta > agora) return;
  if (nr == reseta && np < pct) return;
  pct = np; reseta = nr;
}

void recebeuLocal() { ultimoLocal = millis(); servidor = SRV_OK; }

void webEstado() {
  if (!autorizado()) { web.send(401, "text/plain", "segredo invalido\n"); return; }
  JsonDocument doc;
  if (deserializeJson(doc, web.arg("plain"))) { web.send(400, "text/plain", "json invalido\n"); return; }
  if (!relogioValido()) ajustaRelogio(doc["enviado_em"].as<long>());
  long agora = time(nullptr);
  mesclaJanela(dados.h5, dados.h5r, doc["limites"]["cinco_horas"], agora);
  mesclaJanela(dados.d7, dados.d7r, doc["limites"]["sete_dias"], agora);
  if (!doc["contexto"]["usado_pct"].isNull()) dados.ctx = (int)lroundf(doc["contexto"]["usado_pct"].as<float>());
  strlcpy(dados.mod, doc["modelo"] | "", sizeof dados.mod);
  marcaSessao(doc["sessao"] | "");
  dados.n = contaSessoes(); dados.at = agora; dados.ok = true;
  if (dados.h5r > agora) renovou5 = false;
  if (dados.d7r > agora) renovou7 = false;
  confereLimites();
  recebeuLocal();
  web.send(200, "text/plain", "ok\n");
}

void webEvento() {
  if (!autorizado()) { web.send(401, "text/plain", "segredo invalido\n"); return; }
  JsonDocument doc;
  if (deserializeJson(doc, web.arg("plain"))) { web.send(400, "text/plain", "json invalido\n"); return; }
  const char* tipo = doc["tipo"] | ""; const char* humor = doc["humor"] | ""; const char* sessao = doc["sessao"] | "";
  if (!strcmp(tipo, "fim")) tiraSessao(sessao); else marcaSessao(sessao);
  dados.n = contaSessoes();
  if (strcmp(tipo, "fim") != 0) dados.at = time(nullptr);   // atividade = sinal de vida
  recebeuLocal();
  registra("local evento: %s %s (n=%d)", tipo, humor, dados.n);
  trataEvento(tipo, humor);
  web.send(200, "text/plain", "ok\n");
}

void webRaiz() {
  web.send(200, "application/json", "{\"claudinho\":true,\"versao\":\"" VERSAO "\"}\n");
}
void webMini() {
  if (!autorizado()) { web.send(401, "text/plain", "segredo invalido\n"); return; }
  char buf[360];
  snprintf(buf, sizeof buf, "{\"versao\":\"%s\",\"placa\":\"" PLACA_NOME "\",\"h5\":%d,\"h5r\":%ld,\"d7\":%d,\"d7r\":%ld,\"ctx\":%d,\"n\":%d,\"mod\":\"%s\",\"at\":%ld,\"now\":%ld,\"local\":%s,\"rssi\":%d,\"manut\":\"%s\"}\n",
           VERSAO, dados.h5, dados.h5r, dados.d7, dados.d7r, dados.ctx, dados.n, dados.mod, dados.at, (long)time(nullptr), localRecente() ? "true" : "false", (int)WiFi.RSSI(),
           manutPedidaEm ? "pedida" : manutLiberada() ? "liberada" : "");
  web.send(200, "application/json", buf);
}

void webLog() {
  if (!autorizado()) { web.send(401, "text/plain", "segredo invalido\n"); return; }
  String s;
  if (logCheio) s.concat(logBuf + logPos, sizeof logBuf - logPos);
  s.concat(logBuf, logPos);
  web.send(200, "text/plain", s);
}

void webCmd() {
  if (!autorizado()) { web.send(401, "text/plain", "segredo invalido\n"); return; }
  JsonDocument doc;
  if (deserializeJson(doc, web.arg("plain"))) { web.send(400, "text/plain", "json invalido\n"); return; }
  if (doc["cor"].is<JsonArray>()) {
    corRostoAtual = RGB565(doc["cor"][0].as<int>(), doc["cor"][1].as<int>(), doc["cor"][2].as<int>());
    if (doc["salvar"] | false) gravaCor(corRostoAtual);
    if (pagina == 0) mudaPagina(0);
  }
  // {"genius":true}: abre o Genius
  if (doc["genius"] | false) abreGenius();
  // {"velha":true}: abre o jogo da velha
  if (doc["velha"] | false) abreVelha();
  // {"paleta":true}: abre a escolha de cor do rosto na tela
  if (doc["paleta"] | false) abrePaleta();
  // {"manutencao":true}: pede o toque que libera /ota e /tft
  if (doc["manutencao"] | false) {
    manutPedidaEm = millis(); manutAte = 0;
    registra("manutencao: pedida pelo PC; esperando toque");
    mudaPagina(2); paginaDur = MANUT_PEDIDO_MS + 1000;
  }
  // {"consumo":N}: mostra a tela de consumo agora por N segundos
  if (doc["consumo"].is<int>()) {
    mudaPagina(1); paginaDur = constrain(doc["consumo"].as<int>(), 3, 120) * 1000UL; consumo.naTela = true;
  }
  bool reinicia = doc["reiniciar"] | false;
  web.send(200, "text/plain", "ok\n");
  if (reinicia) { delay(300); ESP.restart(); }
}

// ---- firmware por upload: curl -F "f=@arquivo.bin" http://ip/ota
// uploadNegado vale so dentro de um envio; os finais (webOtaFim, webTftFim)
// conferem de novo segredo, liberacao e se um arquivo comecou de verdade:
// um POST vazio chega direto neles e nao pode reiniciar nada.
bool uploadNegado = true, otaIniciado = false, tftIniciado = false;
bool negaFim(bool iniciado) {
  if (!autorizado())     { web.send(401, "text/plain", "segredo invalido\n"); return true; }
  if (!manutLiberada())  { web.send(403, "text/plain", "toque na tela do Claudinho para liberar\n"); return true; }
  if (!iniciado)         { web.send(400, "text/plain", "nenhum arquivo recebido\n"); return true; }
  return false;
}
void webOtaFim() {
  bool iniciado = otaIniciado; otaIniciado = false;
  if (negaFim(iniciado)) return;
  bool ok = !Update.hasError();
  web.send(ok ? 200 : 500, "text/plain", ok ? "ok, reiniciando\n" : "falhou\n");
  registra(ok ? "ota local: ok" : "ota local: falhou");
  delay(500);
  if (ok) ESP.restart(); else mudaPagina(0);
}
void webOtaDados() {
  HTTPUpload& u = web.upload();
  if (u.status == UPLOAD_FILE_START) {
    uploadNegado = !autorizado() || !manutLiberada(); otaIniciado = !uploadNegado;
    if (uploadNegado) return;
    limpaTela(COR_FUNDO);
    escreve(0,  80, 320, 38, FONTE_32, COR_OURO,    COR_FUNDO, 1, "Atualizando...");
    escreve(0, 130, 320, 30, FONTE_M,  COR_APAGADO, COR_FUNDO, 1, "n\xe3o desligue");
    Update.begin(UPDATE_SIZE_UNKNOWN);
  } else if (u.status == UPLOAD_FILE_WRITE) {
    if (!uploadNegado) Update.write(u.buf, u.currentSize);
  } else if (u.status == UPLOAD_FILE_END) {
    if (!uploadNegado) Update.end(true);
  } else if (u.status == UPLOAD_FILE_ABORTED) {
    // Conexao caiu no meio: webOtaFim nao e chamado. Descarta o que foi
    // gravado (o firmware atual continua) e tira o "atualizando..." da tela.
    if (!otaIniciado) return;
    otaIniciado = false;
    Update.abort();
    registra("ota local: envio interrompido; repita");
    mudaPagina(0);
  }
}

// ---- .tft por upload: curl -F "f=@display.tft" "http://ip/tft?tam=<bytes>"
// Os dados chegam do curl e vao para o Nextion em blocos de 4096 com ack.
// Quando o Nextion pede "pulo" (trecho igual ja gravado), os bytes que
// chegarem ate o offset pedido sao descartados.
long tftTam = 0, tftEnviado = 0, tftPular = 0; int tftFill = 0; bool tftOk = false;
bool tftMandaBloco() {
  nex.write(tftBuf, tftFill); nex.flush();
  tftEnviado += tftFill; tftFill = 0;
  long ret = tftEsperaRetorno(10000);
  if (ret < 0) { registra("tft local: sem confirmacao em %ld", tftEnviado); return false; }
  if (ret > tftEnviado) { tftPular = ret - tftEnviado; tftEnviado = ret; registra("tft local: pulo para %ld", ret); }
  return true;
}
void webTftFim() {
  bool iniciado = tftIniciado; tftIniciado = false;
  if (negaFim(iniciado)) return;
  web.send(tftOk ? 200 : 500, "text/plain", tftOk ? "ok, reiniciando\n" : "falhou, reiniciando\n");
  registra("tft local: %s (%ld de %ld)", tftOk ? "concluido" : "falhou", tftEnviado, tftTam);
  delay(3000); ESP.restart();
}
void webTftDados() {
  HTTPUpload& u = web.upload();
  if (u.status == UPLOAD_FILE_START) {
    uploadNegado = !autorizado() || !manutLiberada(); tftOk = false;
    tftTam = web.arg("tam").toInt(); tftEnviado = 0; tftPular = 0; tftFill = 0;
    if (tftTam <= 0) uploadNegado = true;
    tftIniciado = !uploadNegado;
    if (uploadNegado) return;
    limpaTela(COR_FUNDO);
    escreve(0,  86, 320, 30, FONTE_M, COR_OURO,    COR_FUNDO, 1, "Gravando a tela...");
    escreve(0, 130, 320, 30, FONTE_M, COR_APAGADO, COR_FUNDO, 1, "n\xe3o desligue");
    delay(300);
    tftOk = tftHandshake(tftTam);
  } else if (u.status == UPLOAD_FILE_WRITE) {
    if (uploadNegado || !tftOk) return;
    size_t i = 0;
    while (i < u.currentSize) {
      if (tftPular > 0) { size_t d = min((size_t)tftPular, u.currentSize - i); tftPular -= d; i += d; continue; }
      size_t n = min((size_t)(TFT_BLOCO - tftFill), u.currentSize - i);
      memcpy(tftBuf + tftFill, u.buf + i, n); tftFill += n; i += n;
      if (tftFill == TFT_BLOCO && !tftMandaBloco()) { tftOk = false; return; }
    }
  } else if (u.status == UPLOAD_FILE_END) {
    if (!uploadNegado && tftOk && tftFill > 0 && tftEnviado < tftTam) tftOk = tftMandaBloco();
  } else if (u.status == UPLOAD_FILE_ABORTED) {
    // Conexao caiu no meio: webTftFim nao e chamado. O Nextion ficou no modo
    // de gravacao; reinicia como no fim com falha (repetir o envio resolve).
    if (!tftIniciado) return;
    tftIniciado = false;
    registra("tft local: envio interrompido em %ld de %ld; repita", tftEnviado, tftTam);
    delay(3000); ESP.restart();
  }
}

void iniciaLocal() {
  configTzTime(FUSO, NTP_1, NTP_2);          // relogio sem servidor
  MDNS.begin("claudinho");                   // http://claudinho.local (onde houver mDNS)
  const char* cabecalhos[] = {"Authorization"};
  web.collectHeaders(cabecalhos, 1);
  web.on("/", HTTP_GET, webRaiz);
  web.on("/mini.json", HTTP_GET, webMini);
  web.on("/log", HTTP_GET, webLog);
  web.on("/estado", HTTP_POST, webEstado);
  web.on("/evento", HTTP_POST, webEvento);
  web.on("/cmd", HTTP_POST, webCmd);
  web.on("/ota", HTTP_POST, webOtaFim, webOtaDados);
  web.on("/tft", HTTP_POST, webTftFim, webTftDados);
  web.begin();
  registra("local: http://%s/ pronto", WiFi.localIP().toString().c_str());
}

// ---------------------------------------------------------------- configuracao
void carregaConfig() {
  prefs.begin("claudinho", true);
  cfgSsid = prefs.getString("ssid", ""); cfgSenha = prefs.getString("senha", ""); cfgToken = prefs.getString("token", "");
  corRostoAtual = prefs.getUShort("cor", COR_ROSTO);
  prefs.end();
}

String macTexto() { return WiFi.macAddress(); }

void imprimeInfo() {
  JsonDocument d;                      // ArduinoJson escapa SSID com aspas ou barra
  d["versao"] = VERSAO; d["placa"] = PLACA_NOME; d["mac"] = macTexto();
  d["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : String("");
  d["wifi"] = cfgSsid.isEmpty() ? "sem config" : (WiFi.status() == WL_CONNECTED ? "conectado" : "sem conexao");
  d["ssid"] = cfgSsid; d["token"] = cfgToken.length() >= 16;
  Serial.print("CLAUDINHO "); serializeJson(d, Serial); Serial.println();
}

// Linhas pela serial USB: INFO, SCAN, CFG {json}.
void leSerialConfig() {
  static String linha;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c != '\n') { if (linha.length() < 400) linha += c; continue; }
    linha.trim();
    if (linha == "INFO") { if (!conectandoWifi) imprimeInfo(); }
    else if (linha == "SCAN") {
      int n = WiFi.scanNetworks();
      for (int k = 0; k < n; k++) {
        JsonDocument d; d["ssid"] = WiFi.SSID(k); d["rssi"] = WiFi.RSSI(k);
        Serial.print("REDE "); serializeJson(d, Serial); Serial.println();
      }
      Serial.println("SCAN FIM");
    } else if (linha.startsWith("CFG ")) {
      JsonDocument doc;
      if (deserializeJson(doc, linha.substring(4))) Serial.println("CFG ERRO json");
      else {
        prefs.begin("claudinho", false);
        if (doc["ssid"].is<const char*>())  prefs.putString("ssid",  doc["ssid"].as<const char*>());
        if (doc["senha"].is<const char*>()) prefs.putString("senha", doc["senha"].as<const char*>());
        if (doc["token"].is<const char*>()) prefs.putString("token", doc["token"].as<const char*>());
        prefs.end();
        Serial.println("CFG OK");
        Serial.flush(); delay(300); ESP.restart();
      }
    }
    linha = "";
  }
}

void telaTexto(const char* l1, const char* l2, const char* l3) {
  limpaTela(COR_FUNDO);
  escreve(0, 70, 320, 22, FONTE_P, COR_OURO, COR_FUNDO, 1, l1);
  escreve(0, 104, 320, 20, FONTE_P, COR_TEXTO, COR_FUNDO, 1, l2);
  escreve(0, 134, 320, 20, FONTE_P, COR_APAGADO, COR_FUNDO, 1, l3);
}

// ---------------------------------------------------------------- wifi
bool conectaWifi() {
  if (cfgSsid.isEmpty()) return false;
  WiFi.mode(WIFI_STA); WiFi.setSleep(false);
#if WIFI_POTENCIA_REDUZIDA
  WiFi.setTxPower(WIFI_POWER_8_5dBm);   // C3 Super Mini: antena fraca, potencia alta satura e nao conecta
#endif
  // Com mais de um ponto de acesso com o mesmo nome (mesh, repetidor), o
  // padrao e entrar no primeiro que aparece, nao no mais forte: cada reinicio
  // vira um sorteio, e num ponto ruim o Wi-Fi perde pacotes e o OTA falha.
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
  WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
  WiFi.begin(cfgSsid.c_str(), cfgSenha.c_str());
  registra("wifi: conectando a %s", cfgSsid.c_str());
  unsigned long t0 = millis();
  conectandoWifi = true;
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) { delay(250); leSerialConfig(); }
  conectandoWifi = false;
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.setSleep(false);                    // de novo, ja conectado: economia de energia do radio desligada
    wifi_ps_type_t ps = WIFI_PS_NONE; esp_wifi_get_ps(&ps);
    registra("wifi: %s (sinal %d dBm, canal %d, ponto %s, economia %s)", WiFi.localIP().toString().c_str(), WiFi.RSSI(),
             WiFi.channel(), WiFi.BSSIDstr().c_str(), ps == WIFI_PS_NONE ? "desligada" : "LIGADA");
    return true;
  }
  registra("wifi: falhou");
  return false;
}

// ---------------------------------------------------------------- arduino
void setup() {
  Serial.begin(115200);
  pinMode(BOTAO_BOOT, INPUT_PULLUP);
  delay(300);
  Serial.println("\nclaudinho - monitor de tokens  versao " VERSAO);
  setenv("TZ", FUSO, 1); tzset();

  // Nextion acorda a 9600; pede a serial rapida e troca. Quando os dois ligam
  // juntos (ou depois de gravar um .tft) o Nextion leva ~1,5 s para ouvir:
  // espera, e manda o pedido duas vezes por garantia.
  nex.begin(NEXTION_BAUD, SERIAL_8N1, NEXTION_RX, NEXTION_TX);
  delay(1800);
  nexCmd("");
  nexCmdf("baud=%d", NEXTION_BAUD_RAPIDO);
  nex.flush(); delay(150);
  nex.updateBaudRate(NEXTION_BAUD_RAPIDO);
  delay(150);
  nexCmd("");
  // se ja estava em 115200 (ESP reiniciou sozinho), o comando acima foi lixo
  // inofensivo; se ainda estava em 9600, agora troca:
  nex.updateBaudRate(NEXTION_BAUD); nexCmd(""); nexCmdf("baud=%d", NEXTION_BAUD_RAPIDO); nex.flush(); delay(150);
  nex.updateBaudRate(NEXTION_BAUD_RAPIDO); delay(150);
  nexCmd(""); nexCmd("bkcmd=0"); nexCmd("sendxy=1"); nexCmd("dim=100"); nexCmd("thsp=0");
  // A troca de baud acima gera respostas de erro do Nextion (0x1A, 0x00 +
  // FF FF FF) antes do bkcmd=0 valer; descarta para nao virar ruido no log.
  delay(50); while (nex.available()) nex.read();
  mudaPagina(0);

  carregaConfig();
  if (corRostoAtual != COR_ROSTO) mudaPagina(0);    // cor gravada: redesenha o rosto que ja apareceu laranja
  WiFi.mode(WIFI_STA);          // para o MAC e o SCAN funcionarem mesmo sem config
  if (cfgSsid.isEmpty()) {
    String mac = macTexto();
    telaTexto("Ola! Sou o Claudinho.", "Configure pelo Claude Code:", mac.c_str());
    // espera a configuracao pela serial; avisa a cada 5 s que esta aqui
    unsigned long ultimo = 0;
    while (true) { leSerialConfig(); if (millis() - ultimo > 5000) { ultimo = millis(); imprimeInfo(); } delay(20); }
  }
  if (!conectaWifi()) {
    char l2[48]; snprintf(l2, sizeof l2, "rede: %s", cfgSsid.c_str());
    telaTexto("Nao consegui entrar no Wi-Fi", l2, "tentando de novo...");
    servidor = SRV_SEM_WIFI;
  }
  if (cfgToken.length() < 16) registra("aviso: sem token; o PC nao conseguira falar comigo");
  iniciaLocal();
  imprimeInfo();
  mudaPagina(0);
}

void loop() {
  web.handleClient();
  leSerialConfig();
  leToque();
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long ultimaTentativa = 0;
    if (millis() - ultimaTentativa > 10000) { ultimaTentativa = millis(); WiFi.reconnect(); }
    servidor = SRV_SEM_WIFI;
  } else if (servidor == SRV_SEM_WIFI) servidor = SRV_INICIANDO;
  cuidaManutencao();
  if (pagina != 0 && millis() - paginaDesde > paginaDur) {
    if (pagina == 2) manutPedidaEm = 0;
    if (pagina == 6) saiVelha("sem toque");
    else if (pagina == 7) saiGenius("sem toque");
    else if (pagina >= 3) cancelaPaleta("sem toque"); else mudaPagina(0);
  }

  static unsigned long ultimoTick = 0;
  if (millis() - ultimoTick >= 1000) { ultimoTick = millis(); confereRenovacao(); desenha(); }
  cuidaConsumo();
  cuidaVelha();
  cuidaGenius();

  // Brilho: dormindo ha mais de 20 s -> 15 %; acordado -> 100 % (ou o que o
  // toque longo escolheu). Poupa backlight e bateria.
  {
    static unsigned long dormeDesde = 0; static int brilhoNaTela = -1;
    bool dormindo = (pagina == 0 && caraNaTela == C_DORMINDO);   // jogo, paleta e cartao: brilho normal
    if (!dormindo) dormeDesde = 0;
    else if (!dormeDesde) dormeDesde = millis();
    int alvo = (dormindo && millis() - dormeDesde > 20000) ? 15 : (brilhoAlto ? 100 : 15);
    if (alvo != brilhoNaTela) { brilhoNaTela = alvo; nexCmdf("dim=%d", alvo); }
  }

  // Ajustes do Nextion repetidos de tempos em tempos: se ele acordou depois do
  // boot do ESP (ou foi regravado), perdeu o sendxy e o toque some.
  static unsigned long ultimoAjuste = 0;
  if (millis() - ultimoAjuste >= 30000) { ultimoAjuste = millis(); nexCmd("bkcmd=0"); nexCmd("sendxy=1"); nexCmd("thsp=0"); }
  if (pagina == 0) cuidaRosto();
  delay(10);
}
