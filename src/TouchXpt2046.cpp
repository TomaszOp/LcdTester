/**
 * @file TouchXpt2046.cpp
 * @brief Implementacja sterownika XPT2046 z pomiarem nacisku.
 */
#include "TouchXpt2046.h"

#include <algorithm>

// Komendy XPT2046 (przetwornik wlaczony miedzy pomiarami): Y, Z1, X, Z2; na koniec uspienie
static constexpr uint8_t CMD_Y = 0x91;
static constexpr uint8_t CMD_Z1 = 0xB1;
static constexpr uint8_t CMD_X = 0xD1;
static constexpr uint8_t CMD_Z2 = 0xC1;
static constexpr uint8_t CMD_POWER_DOWN = 0x80;
static constexpr size_t GROUP = 8;  ///< Bajty na jeden pomiar: 4 komendy co 2 bajty.

TouchXpt2046::TouchXpt2046() {
  _cfg.freq = 1000000;
  _cfg.x_min = 0;
  _cfg.x_max = 4095;
  _cfg.y_min = 0;
  _cfg.y_max = 4095;
}

bool TouchXpt2046::init() {
  _inited = false;
  if (_cfg.pin_cs < 0) return false;
  lgfx::gpio_hi(_cfg.pin_cs);
  lgfx::pinMode(_cfg.pin_cs, lgfx::pin_mode_t::output);
  if (lgfx::spi::init(_cfg.spi_host, _cfg.pin_sclk, _cfg.pin_miso, _cfg.pin_mosi).has_error()) return false;
  if (_cfg.pin_int >= 0) lgfx::pinMode(_cfg.pin_int, lgfx::pin_mode_t::input_pullup);
  _inited = true;
  return true;
}

uint_fast8_t TouchXpt2046::getTouchRaw(lgfx::touch_point_t* tp, uint_fast8_t count) {
  if (!_inited || count == 0) return 0;
  tp->size = 0;
  // T_IRQ (jesli podlaczony) w stanie wysokim = brak dotyku
  if (_cfg.pin_int >= 0 && lgfx::gpio_in(_cfg.pin_int)) return 0;

  // Odpowiedz na komende przychodzi w 2 kolejnych bajtach, wiec wynik komendy z bajtu i
  // jest w bajtach i+1, i+2 (dla Z2 ostatniej grupy - w dodatkowym bajcie na koncu)
  uint8_t data[SAMPLES * GROUP + 1];
  memset(data, 0, sizeof(data));
  for (uint8_t s = 0; s < SAMPLES; s++) {
    uint8_t* d = &data[s * GROUP];
    d[0] = CMD_Y;
    d[2] = CMD_Z1;
    d[4] = CMD_X;
    d[6] = CMD_Z2;
  }
  data[SAMPLES * GROUP] = CMD_POWER_DOWN;

  lgfx::spi::beginTransaction(_cfg.spi_host, _cfg.freq, 0);
  lgfx::gpio_lo(_cfg.pin_cs);
  lgfx::spi::readBytes(_cfg.spi_host, data, sizeof(data));
  lgfx::gpio_hi(_cfg.pin_cs);
  lgfx::spi::endTransaction(_cfg.spi_host);

  int16_t xs[SAMPLES], ys[SAMPLES], zs[SAMPLES];
  uint8_t n = 0;
  for (uint8_t s = 0; s < SAMPLES; s++) {
    const uint8_t* d = &data[s * GROUP];
    const int y = (d[1] << 8 | d[2]) >> 3;
    const int z1 = (d[3] << 8 | d[4]) >> 3;
    const int x = (d[5] << 8 | d[6]) >> 3;
    const int z2 = (d[7] << 8 | d[8]) >> 3;
    const int z = z1 + 4095 - z2;  // nacisk: im mocniej, tym wieksze Z1 i mniejsze Z2
    if (s == SAMPLES / 2) _last = { x, y, z1, z2, z };
    if (z >= PRESSURE_THRESHOLD && x > 0 && x < 4095 && y > 0 && y < 4095) {
      xs[n] = x;
      ys[n] = y;
      zs[n] = z;
      n++;
    }
  }
  if (n < MIN_VALID) return 0;

  std::sort(xs, xs + n);
  std::sort(ys, ys + n);
  std::sort(zs, zs + n);
  tp->x = xs[n / 2];
  tp->y = ys[n / 2];
  tp->size = zs[n / 2];
  tp->id = 0;
  return 1;
}
