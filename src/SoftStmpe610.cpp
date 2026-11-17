/**
 * @file SoftStmpe610.cpp
 * @brief Implementacja programowego sterownika STMPE610.
 */
#include "SoftStmpe610.h"

// Rejestry i wartosci jak w Touch_STMPE610 z LovyanGFX
static constexpr uint8_t REG_SYS_CTRL1 = 0x03;
static constexpr uint8_t REG_SYS_CTRL2 = 0x04;
static constexpr uint8_t REG_INT_CTRL = 0x09;
static constexpr uint8_t REG_INT_EN = 0x0A;
static constexpr uint8_t REG_INT_STA = 0x0B;
static constexpr uint8_t REG_ADC_CTRL1 = 0x20;
static constexpr uint8_t REG_ADC_CTRL2 = 0x21;
static constexpr uint8_t REG_TSC_CTRL = 0x40;
static constexpr uint8_t REG_TSC_CFG = 0x41;
static constexpr uint8_t REG_FIFO_TH = 0x4A;
static constexpr uint8_t REG_FIFO_STA = 0x4B;
static constexpr uint8_t REG_TSC_FRACTION_Z = 0x56;
static constexpr uint8_t REG_TSC_I_DRIVE = 0x58;
static constexpr uint8_t REG_TSC_DATA = 0xD7;  ///< Dane XYZ z kolejki FIFO (bez autoinkrementacji).

static constexpr uint8_t FIFO_STA_RESET = 0x01;
static constexpr uint8_t FIFO_STA_EMPTY = 0x20;
static constexpr uint8_t TSC_CTRL_TOUCHED = 0x80;
static constexpr uint16_t CHIP_VERSION = 0x0811;
static constexpr uint32_t HALF_CLOCK_US = 1;   ///< Pol okresu zegara (ok. 250-500 kHz).
static constexpr uint8_t MAX_FIFO_READS = 32;  ///< Ograniczenie oprozniania kolejki.

SoftStmpe610::SoftStmpe610() {
  _cfg.x_min = 0;
  _cfg.x_max = 4095;
  _cfg.y_min = 0;
  _cfg.y_max = 4095;
  _cfg.bus_shared = false;
}

uint8_t SoftStmpe610::transfer(uint8_t value) {
  uint8_t in = 0;
  for (int8_t bit = 7; bit >= 0; bit--) {
    if (_spiMode == 0) {
      // Tryb 0: dane ustawiane przed zboczem narastajacym, odczyt na narastajacym
      digitalWrite(_cfg.pin_mosi, (value >> bit) & 1);
      delayMicroseconds(HALF_CLOCK_US);
      digitalWrite(_cfg.pin_sclk, HIGH);
      in = (in << 1) | digitalRead(_cfg.pin_miso);
      delayMicroseconds(HALF_CLOCK_US);
      digitalWrite(_cfg.pin_sclk, LOW);
    } else {
      // Tryb 1: dane ustawiane na zboczu narastajacym, odczyt na opadajacym
      digitalWrite(_cfg.pin_sclk, HIGH);
      digitalWrite(_cfg.pin_mosi, (value >> bit) & 1);
      delayMicroseconds(HALF_CLOCK_US);
      digitalWrite(_cfg.pin_sclk, LOW);
      in = (in << 1) | digitalRead(_cfg.pin_miso);
      delayMicroseconds(HALF_CLOCK_US);
    }
  }
  return in;
}

uint8_t SoftStmpe610::readRegister(uint8_t reg) {
  digitalWrite(_cfg.pin_cs, LOW);
  transfer(0x80 | reg);
  const uint8_t value = transfer(0);
  digitalWrite(_cfg.pin_cs, HIGH);
  return value;
}

void SoftStmpe610::writeRegister(uint8_t reg, uint8_t value) {
  digitalWrite(_cfg.pin_cs, LOW);
  transfer(reg);
  transfer(value);
  digitalWrite(_cfg.pin_cs, HIGH);
}

uint16_t SoftStmpe610::readVersion() {
  return (readRegister(0) << 8) | readRegister(1);
}

bool SoftStmpe610::init() {
  _inited = false;
  if (_cfg.pin_sclk < 0 || _cfg.pin_mosi < 0 || _cfg.pin_miso < 0 || _cfg.pin_cs < 0) return false;
  pinMode(_cfg.pin_cs, OUTPUT);
  digitalWrite(_cfg.pin_cs, HIGH);
  pinMode(_cfg.pin_sclk, OUTPUT);
  digitalWrite(_cfg.pin_sclk, LOW);
  pinMode(_cfg.pin_mosi, OUTPUT);
  pinMode(_cfg.pin_miso, INPUT);
  if (_cfg.pin_int >= 0) pinMode(_cfg.pin_int, INPUT);

  _spiMode = 0;
  if (readVersion() != CHIP_VERSION) {
    _spiMode = 1;
    if (readVersion() != CHIP_VERSION) return false;
  }

  writeRegister(REG_SYS_CTRL1, 0x02);  // reset programowy
  delay(10);
  for (uint8_t i = 0; i < 65; i++) readRegister(i);
  writeRegister(REG_SYS_CTRL2, 0x00);         // zegary wlaczone
  writeRegister(REG_TSC_CTRL, 0x01);          // pomiar XYZ, kontroler dotyku wlaczony
  writeRegister(REG_INT_EN, 0x01);            // przerwanie przy dotknieciu
  writeRegister(REG_ADC_CTRL1, 0x00 | (0x6 << 4));  // 10 bit, 96 taktow na pomiar
  writeRegister(REG_ADC_CTRL2, 0x02);         // 6,5 MHz
  writeRegister(REG_TSC_CFG, 0xC0 | 0x10 | 0x02);   // 8 probek, opoznienie 100 us, ustalanie 500 us
  writeRegister(REG_TSC_FRACTION_Z, 0x06);
  writeRegister(REG_FIFO_TH, 1);
  writeRegister(REG_FIFO_STA, FIFO_STA_RESET);
  writeRegister(REG_FIFO_STA, 0);
  writeRegister(REG_TSC_I_DRIVE, 0x01);       // 50 mA
  writeRegister(REG_INT_STA, 0xFF);           // skasowanie przerwan
  writeRegister(REG_INT_CTRL, 0x04 | 0x01);   // przerwanie aktywne stanem wysokim
  _inited = true;
  return true;
}

uint_fast8_t SoftStmpe610::getTouchRaw(lgfx::touch_point_t* tp, uint_fast8_t count) {
  if (!_inited || count == 0) return 0;
  tp->size = 0;
  const bool touched = readRegister(REG_TSC_CTRL) & TSC_CTRL_TOUCHED;

  // Oprozniamy kolejke FIFO - ostatnia probka jest najswiezsza
  bool haveSample = false;
  uint8_t d[4] = {};
  for (uint8_t i = 0; i < MAX_FIFO_READS && !(readRegister(REG_FIFO_STA) & FIFO_STA_EMPTY); i++) {
    for (uint8_t b = 0; b < 4; b++) d[b] = readRegister(REG_TSC_DATA);
    haveSample = true;
  }
  writeRegister(REG_INT_STA, 0xFF);

  if (!touched || !haveSample) return 0;
  tp->x = (d[0] << 4) | (d[1] >> 4);
  tp->y = ((d[1] & 0x0F) << 8) | d[2];
  tp->size = d[3] ? d[3] : 1;
  tp->id = 0;
  return 1;
}
