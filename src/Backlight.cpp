/**
 * @file Backlight.cpp
 * @brief Implementacja klasy Backlight (PWM podswietlenia).
 */
#include "Backlight.h"

void Backlight::attach(int8_t pin) {
  if (pin == _pin) {
    apply();
    return;
  }
  if (_pin >= 0) ledcDetach(_pin);
  _pin = pin;
  if (_pin < 0) return;
  if (!ledcAttach(_pin, PWM_FREQ_HZ, PWM_RESOLUTION_BITS)) {
    Serial.printf("[LED] Nie mozna uruchomic PWM na GPIO %d\n", _pin);
    _pin = -1;
    return;
  }
  apply();
}

void Backlight::setLevel(uint8_t percent) {
  _percent = min<uint8_t>(percent, 100);
  apply();
}

void Backlight::apply() {
  if (_pin < 0) return;
  const uint32_t maxDuty = (1 << PWM_RESOLUTION_BITS) - 1;
  // Przy maksymalnym wypelnieniu rdzen Arduino daje staly stan wysoki
  ledcWrite(_pin, maxDuty * _percent / 100);
}
