/**
 * @file SoftXpt2046.cpp
 * @brief Implementacja programowego sterownika XPT2046.
 */
#include "SoftXpt2046.h"

#include <algorithm>

// Komendy XPT2046: bit7 = start, bity 6-4 = kanal, bity 1-0 = zasilanie
// (01 = przetwornik wlaczony miedzy pomiarami, 00 = uspienie z aktywnym T_IRQ)
static constexpr uint8_t CMD_X = 0xD1;
static constexpr uint8_t CMD_Y = 0x91;
static constexpr uint8_t CMD_Z1 = 0xB1;
static constexpr uint8_t CMD_Z2 = 0xC1;
static constexpr uint8_t CMD_POWER_DOWN = 0x80;
static constexpr uint8_t SAMPLES = 5;            ///< Pomiarow na jeden punkt (mediana).
static constexpr uint32_t HALF_CLOCK_US = 1;     ///< Pol okresu zegara (ok. 250-500 kHz).

SoftXpt2046::SoftXpt2046() {
  _cfg.x_min = 0;
  _cfg.x_max = 4095;
  _cfg.y_min = 0;
  _cfg.y_max = 4095;
  _cfg.bus_shared = false;
}

bool SoftXpt2046::init() {
  _inited = false;
  if (_cfg.pin_sclk < 0 || _cfg.pin_mosi < 0 || _cfg.pin_miso < 0 || _cfg.pin_cs < 0) return false;
  pinMode(_cfg.pin_cs, OUTPUT);
  digitalWrite(_cfg.pin_cs, HIGH);
  pinMode(_cfg.pin_sclk, OUTPUT);
  digitalWrite(_cfg.pin_sclk, LOW);
  pinMode(_cfg.pin_mosi, OUTPUT);
  pinMode(_cfg.pin_miso, INPUT);
  if (_cfg.pin_int >= 0) pinMode(_cfg.pin_int, INPUT_PULLUP);
  _inited = true;
  return true;
}

uint16_t SoftXpt2046::transfer(uint8_t command) {
  // 8 bitow komendy, potem 16 taktow odczytu (12 bitow wyniku + 4 zera)
  for (int8_t bit = 7; bit >= 0; bit--) {
    digitalWrite(_cfg.pin_mosi, (command >> bit) & 1);
    delayMicroseconds(HALF_CLOCK_US);
    digitalWrite(_cfg.pin_sclk, HIGH);
    delayMicroseconds(HALF_CLOCK_US);
    digitalWrite(_cfg.pin_sclk, LOW);
  }
  digitalWrite(_cfg.pin_mosi, LOW);
  uint16_t value = 0;
  for (uint8_t i = 0; i < 16; i++) {
    delayMicroseconds(HALF_CLOCK_US);
    digitalWrite(_cfg.pin_sclk, HIGH);
    delayMicroseconds(HALF_CLOCK_US);
    value = (value << 1) | digitalRead(_cfg.pin_miso);
    digitalWrite(_cfg.pin_sclk, LOW);
  }
  return value >> 3;
}

uint_fast8_t SoftXpt2046::getTouchRaw(lgfx::touch_point_t* tp, uint_fast8_t count) {
  if (!_inited || count == 0) return 0;
  tp->size = 0;
  // T_IRQ w stanie wysokim = brak dotyku (oszczedza pomiar)
  if (_cfg.pin_int >= 0 && digitalRead(_cfg.pin_int)) return 0;

  uint16_t xs[SAMPLES], ys[SAMPLES];
  int z = 0;
  digitalWrite(_cfg.pin_cs, LOW);
  const int z1 = transfer(CMD_Z1);
  const int z2 = transfer(CMD_Z2);
  z = z1 + 4095 - z2;
  if (z >= PRESSURE_THRESHOLD) {
    transfer(CMD_X);  // pierwszy pomiar po przelaczeniu kanalu bywa zaszumiony
    for (uint8_t i = 0; i < SAMPLES; i++) {
      xs[i] = transfer(CMD_X);
      ys[i] = transfer(CMD_Y);
    }
  }
  transfer(CMD_POWER_DOWN);
  digitalWrite(_cfg.pin_cs, HIGH);

  if (z < PRESSURE_THRESHOLD) return 0;
  std::sort(xs, xs + SAMPLES);
  std::sort(ys, ys + SAMPLES);
  tp->x = xs[SAMPLES / 2];
  tp->y = ys[SAMPLES / 2];
  tp->size = z;
  tp->id = 0;
  return 1;
}
