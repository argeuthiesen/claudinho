// Configuracao de compilacao do Claudinho. Nada pessoal aqui: Wi-Fi e segredo
// sao gravados na placa na primeira vez (ver o cabecalho do claudinho.ino).

#define VERSAO "1.8.2"

// Serial do Nextion. A placa e detectada na compilacao.
#if CONFIG_IDF_TARGET_ESP32C3
  // ESP32-C3 Super Mini: pinos marcados TX e RX na placa. 5V vem do pino "5V".
  #define PLACA_NOME  "esp32c3"
  #define NEXTION_TX  21   // C3 TX -> fio RX (amarelo) do Nextion
  #define NEXTION_RX  20   // C3 RX <- fio TX (azul)    do Nextion
  #define WIFI_POTENCIA_REDUZIDA 1   // a antena do Super Mini conecta melhor com menos potencia
  #define BOTAO_BOOT  9    // botao BOOT da placa: alternativa ao toque para liberar atualizacao
#else
  // ESP32-S3 DevKitC-1
  #define PLACA_NOME  "esp32s3"
  #define NEXTION_TX  17
  #define NEXTION_RX  18
  #define WIFI_POTENCIA_REDUZIDA 0
  #define BOTAO_BOOT  0
#endif
#define NEXTION_BAUD        9600     // de fabrica
#define NEXTION_BAUD_RAPIDO 115200   // pedido no boot (nao persiste no Nextion)
#define TFT_BLOCO           4096

#define VOLTA_PAGINA_MS   15000   // tela de numeros volta sozinha
#define CONGELADO_APOS_S  180     // 3 min sem noticia do PC -> dorme
#define TOQUE_LONGO_MS    1500    // segurar -> alterna brilho
#define MANUT_PEDIDO_MS   60000   // quanto a tela espera o toque que libera uma atualizacao
#define MANUT_JANELA_MS   120000  // depois do toque, quanto tempo /ota e /tft ficam liberados

// Fuso (POSIX TZ) e relogio por NTP.
#define FUSO  "<-03>3"            // Brasilia; ex.: Lisboa "WET0WEST,M3.5.0/1,M10.5.0"
#define NTP_1 "pool.ntp.org"
#define NTP_2 "time.google.com"
