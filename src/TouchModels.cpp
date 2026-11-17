/**
 * @file TouchModels.cpp
 * @brief Spis kontrolerow dotyku - tu dopisuje sie nowe kontrolery.
 */
#include "TouchModels.h"

// Kolejnosc pol: id, nazwa, kontroler, szyna, adres I2C, osobna magistrala SPI.
// Osobna magistrala jest realizowana programowo (bit-bang: SoftXpt2046, SoftStmpe610),
// bo ESP32-C3 ma jeden wolny sprzetowy kontroler SPI, zajety przez wyswietlacz.
const TouchModel TOUCH_MODELS[] = {
  { "xpt2046",  "XPT2046 (SPI)",               TouchController::XPT2046,  TouchBus::SPI, 0x00, true },
  { "stmpe610", "STMPE610 (SPI)",              TouchController::STMPE610, TouchBus::SPI, 0x00, true },
  { "ft6x36",   "FT6206 / FT6336 (I2C)",       TouchController::FT6x36,   TouchBus::I2C, 0x38, false },
  { "cst816s",  "CST816S (I2C)",               TouchController::CST816S,  TouchBus::I2C, 0x15, false },
  { "gt911",    "GT911 (I2C)",                 TouchController::GT911,    TouchBus::I2C, 0x5D, false },
};
const uint8_t TOUCH_MODEL_COUNT = sizeof(TOUCH_MODELS) / sizeof(TOUCH_MODELS[0]);

const TouchModel* findTouchModel(const String& id) {
  for (uint8_t i = 0; i < TOUCH_MODEL_COUNT; i++) {
    if (id.equalsIgnoreCase(TOUCH_MODELS[i].id)) return &TOUCH_MODELS[i];
  }
  return nullptr;
}
