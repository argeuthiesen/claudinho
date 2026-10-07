// Tela da placa E32R28T (ESP32 com tela ST7789 de 2,8" e toque XPT2046).
// O resto do firmware fala "Nextion" (fill, xstr, line, cir, cirs, dim): aqui
// esses comandos viram desenho direto na tela, com as mesmas fontes (em
// fontes_e32.h, com bordas suaves) e as mesmas coordenadas. O toque gera os
// mesmos eventos do Nextion (apertou / soltou, x, y). So entra na compilacao
// com PLACA_E32R28T. Pinos: ficha da LCDWiki; controlador e calibragem do
// toque conferidos na placa (a ficha diz ILI9341, mas e ST7789).
#pragma once
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "fontes_e32.h"

class TelaE32 : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 painel; lgfx::Bus_SPI bus; lgfx::Light_PWM luz; lgfx::Touch_XPT2046 toque;
public:
  TelaE32() {
    { auto c = bus.config();
      c.spi_host = HSPI_HOST; c.spi_mode = 0; c.freq_write = 40000000; c.freq_read = 16000000;
      c.pin_sclk = 14; c.pin_mosi = 13; c.pin_miso = 12; c.pin_dc = 2;
      bus.config(c); painel.setBus(&bus); }
    { auto c = painel.config();
      c.pin_cs = 15; c.pin_rst = -1; c.pin_busy = -1;          // reset ligado ao EN do ESP32
      c.panel_width = 240; c.panel_height = 320; c.readable = true; c.bus_shared = false;
      painel.config(c); }
    { auto c = luz.config(); c.pin_bl = 21; c.invert = false; c.freq = 44100; c.pwm_channel = 7;
      luz.config(c); painel.setLight(&luz); }
    { auto c = toque.config();
      c.spi_host = VSPI_HOST; c.freq = 1000000;
      c.pin_sclk = 25; c.pin_mosi = 32; c.pin_miso = 39; c.pin_cs = 33; c.pin_int = 36;
      c.bus_shared = false; c.offset_rotation = 0;
      toque.config(c); painel.setTouch(&toque); }
    setPanel(&painel);
  }
};
static TelaE32 e32;
static int e32Fonte = -1;

inline void e32Inicia() {
  static uint16_t cal[8] = {3591, 282, 3629, 3507, 424, 367, 396, 3646};   // medida na placa, rotacao 1
  e32.init();
  e32.setRotation(1);                      // deitada, 320 x 240, sem espelho
  e32.setTouchCalibrate(cal);
  e32.setAttribute(lgfx::utf8_switch, 0);  // os textos sao ISO-8859-1: cada byte e um caractere
  e32.setBrightness(255);
  e32.fillScreen(0);
}

inline void e32UsaFonte(int f) {
  if (f < 0 || f > 5 || !FONTES_VLW[f]) f = 0;
  if (f == e32Fonte) return;
  e32.loadFont(FONTES_VLW[f]); e32Fonte = f;
}

// xstr x,y,w,h,fonte,cor,fundo,alinhaX,alinhaY,fundoCheio,"texto"
inline void e32Xstr(const char* c) {
  int x, y, w, h, f, ax, ay, cheio; unsigned cor, fundo;
  if (sscanf(c, "xstr %d,%d,%d,%d,%d,%u,%u,%d,%d,%d", &x, &y, &w, &h, &f, &cor, &fundo, &ax, &ay, &cheio) != 10) return;
  const char* a = strchr(c, '"'); const char* b = strrchr(c, '"');
  if (!a || b <= a) return;
  char t[128]; size_t n = min((size_t)(b - a - 1), sizeof t - 1); memcpy(t, a + 1, n); t[n] = 0;
  if (cheio == 1) e32.fillRect(x, y, w, h, (uint16_t)fundo);
  e32UsaFonte(f);
  e32.setTextColor((uint16_t)cor, (uint16_t)fundo);
  e32.setTextDatum(ax == 0 ? (ay == 1 ? middle_left : top_left) : ax == 2 ? (ay == 1 ? middle_right : top_right)
                                                                           : (ay == 1 ? middle_center : top_center));
  int tx = ax == 0 ? x : ax == 2 ? x + w : x + w / 2, ty = ay == 1 ? y + h / 2 : y;
  e32.setClipRect(x, y, w, h);
  e32.drawString(t, tx, ty);
  e32.clearClipRect();
}

// Os comandos do Nextion que o firmware usa; o resto (bkcmd, sendxy, thsp,
// baud, sleep, connect) nao tem sentido aqui e e ignorado.
inline void e32Cmd(const char* c) {
  int a, b, d, e; unsigned cor;
  if (!strncmp(c, "fill ", 5)) { if (sscanf(c + 5, "%d,%d,%d,%d,%u", &a, &b, &d, &e, &cor) == 5) e32.fillRect(a, b, d, e, (uint16_t)cor); }
  else if (!strncmp(c, "xstr ", 5)) e32Xstr(c);
  else if (!strncmp(c, "line ", 5)) { if (sscanf(c + 5, "%d,%d,%d,%d,%u", &a, &b, &d, &e, &cor) == 5) e32.drawLine(a, b, d, e, (uint16_t)cor); }
  else if (!strncmp(c, "cirs ", 5)) { if (sscanf(c + 5, "%d,%d,%d,%u", &a, &b, &d, &cor) == 4) e32.fillCircle(a, b, d, (uint16_t)cor); }
  else if (!strncmp(c, "cir ", 4))  { if (sscanf(c + 4, "%d,%d,%d,%u", &a, &b, &d, &cor) == 4) e32.drawCircle(a, b, d, (uint16_t)cor); }
  else if (!strncmp(c, "dim=", 4))  { int v = constrain(atoi(c + 4), 0, 100); e32.setBrightness(v * 255 / 100); }
}

// Toque: devolve true quando o estado muda (apertou = true / soltou = false),
// com a posicao em x, y. Le a cada 15 ms, no maximo.
inline bool e32Toque(bool& apertou, int& x, int& y) {
  static bool tocando = false; static int ux = 0, uy = 0; static unsigned long ult = 0;
  if (millis() - ult < 15) return false;
  ult = millis();
  lgfx::touch_point_t p;
  bool agora = e32.getTouch(&p) > 0;
  if (agora) { ux = constrain(p.x, 0, 319); uy = constrain(p.y, 0, 239); }
  if (agora == tocando) return false;
  tocando = agora; apertou = agora; x = ux; y = uy;
  return true;
}
