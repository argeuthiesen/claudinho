// Teste da placa E32R28T (ESP32 + tela 2,8" + toque resistivo XPT2046).
// Mostra faixas vermelha / verde / azul e o nome do controlador em uso; cada
// toque desenha um ponto e escreve as coordenadas. Serve para confirmar o
// controlador da tela (ILI9341 ou ST7789), as cores e a calibracao do toque.
// Pinos: ficha da LCDWiki (E32R28T_E32N28T_Specification_V1.0, secao 4.2).
#define LGFX_USE_V1
#define CONTROLADOR_ST7789   // confirmado na placa (a ficha diz ILI9341)
#include <LovyanGFX.hpp>

#ifndef CONTROLADOR_ST7789
  #define PAINEL lgfx::Panel_ILI9341
  #define NOME_PAINEL "ILI9341"
#else
  #define PAINEL lgfx::Panel_ST7789
  #define NOME_PAINEL "ST7789"
#endif

class Tela : public lgfx::LGFX_Device {
  PAINEL painel; lgfx::Bus_SPI bus; lgfx::Light_PWM luz; lgfx::Touch_XPT2046 toque;
public:
  Tela() {
    { auto c = bus.config();
      c.spi_host = HSPI_HOST; c.spi_mode = 0; c.freq_write = 40000000; c.freq_read = 16000000;
      c.pin_sclk = 14; c.pin_mosi = 13; c.pin_miso = 12; c.pin_dc = 2;
      bus.config(c); painel.setBus(&bus); }
    { auto c = painel.config();
      c.pin_cs = 15; c.pin_rst = -1; c.pin_busy = -1;      // reset ligado ao EN do ESP32
      c.panel_width = 240; c.panel_height = 320; c.readable = true; c.bus_shared = false;
      painel.config(c); }
    { auto c = luz.config(); c.pin_bl = 21; c.invert = false; c.freq = 44100; c.pwm_channel = 7;
      luz.config(c); painel.setLight(&luz); }
    { auto c = toque.config();
      c.spi_host = VSPI_HOST; c.freq = 1000000;
      c.pin_sclk = 25; c.pin_mosi = 32; c.pin_miso = 39; c.pin_cs = 33; c.pin_int = 36;
      c.x_min = 300; c.x_max = 3900; c.y_min = 200; c.y_max = 3700;  // aproximado; o teste mostra o bruto
      c.bus_shared = false; c.offset_rotation = 0;
      toque.config(c); painel.setTouch(&toque); }
    setPanel(&painel);
  }
};
Tela tft;

// Mostra as 8 orientacoes (0-3 normais, 4-7 espelhadas), 4 s cada, em loop.
void mostra(int r) {
  tft.setRotation(r);
  int w = tft.width(), h = tft.height();
  tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, w, h, TFT_WHITE); tft.drawRect(2, 2, w - 4, h - 4, TFT_WHITE);
  tft.fillRect(6, 6, w / 3 - 4, 20, TFT_RED); tft.fillRect(6 + w / 3, 6, w / 3 - 4, 20, TFT_GREEN); tft.fillRect(6 + 2 * w / 3, 6, w / 3 - 12, 20, TFT_BLUE);
  tft.setTextColor(TFT_YELLOW); tft.setTextSize(2); tft.setCursor(10, 34); tft.print("TOPO ^");
  tft.setTextColor(TFT_WHITE); tft.setTextSize(8); tft.setCursor(w / 2 - 20, h / 2 - 30); tft.print(r);
  tft.setTextSize(2); tft.setCursor(10, h - 26); tft.printf("%dx%d %s", w, h, r >= 4 ? "espelho" : "");
}

// calibragem do toque medida nesta placa (argeu, 2026-10-07), na rotacao 1
static uint16_t CAL_TOQUE[8] = {3591, 282, 3629, 3507, 424, 367, 396, 3646};
static const int ROTACAO = 1;   // a certa nesta placa com ST7789 (confirmada pelo argeu): deitada, sem espelho

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setBrightness(255);
  tft.setRotation(ROTACAO);
  // calibragem: toque na ponta de cada seta, uma por vez (4 cantos)
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE); tft.setTextSize(2);
  tft.setCursor(30, 100); tft.print("Toque na ponta de");
  tft.setCursor(30, 124); tft.print("cada seta, uma a uma");
  uint16_t cal[8];
  tft.calibrateTouch(cal, TFT_YELLOW, TFT_BLACK, 20);
  tft.setTouchCalibrate(cal);
  char t[80]; snprintf(t, sizeof t, "cal %u,%u,%u,%u,%u,%u,%u,%u", cal[0], cal[1], cal[2], cal[3], cal[4], cal[5], cal[6], cal[7]);
  Serial.println(t);
  mostra(ROTACAO);
  tft.setTextSize(1); tft.setTextColor(TFT_WHITE); tft.setCursor(10, 60); tft.print("toque na tela: ponto amarelo onde tocou");
  tft.setTextColor(TFT_CYAN); tft.setCursor(10, 200); tft.print(t);
}

void loop() {
  lgfx::touch_point_t p;
  if (tft.getTouch(&p)) {
    tft.fillCircle(p.x, p.y, 4, TFT_YELLOW);
    tft.fillRect(10, 78, 200, 12, TFT_BLACK);
    tft.setTextSize(1); tft.setTextColor(TFT_YELLOW); tft.setCursor(10, 80); tft.printf("toque x=%d y=%d", p.x, p.y);
    Serial.printf("toque %d,%d\n", p.x, p.y);
    delay(30);
  }
}
