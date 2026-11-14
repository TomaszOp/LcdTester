/**
 * @file Backlight.h
 * @brief Sterowanie podswietleniem wyswietlacza.
 */
#pragma once

#include <Arduino.h>

/**
 * @brief Programowe przyciemnianie ekranu sygnalem PWM (LEDC) na pinie LED wyswietlacza.
 *
 * Jasnosc jest pamietana takze bez podlaczonego pinu i stosowana przy attach().
 * 100 % daje pelny, staly stan wysoki.
 */
class Backlight {
 public:
  static constexpr uint32_t PWM_FREQ_HZ = 5000;      ///< Czestotliwosc PWM (bez migotania).
  static constexpr uint8_t PWM_RESOLUTION_BITS = 8;  ///< Rozdzielczosc PWM.

  /**
   * @brief Podlacza PWM do pinu i ustawia zapamietana jasnosc; odlacza poprzedni pin.
   * @param pin Numer GPIO albo -1, gdy modul nie ma podlaczonego pinu LED.
   */
  void attach(int8_t pin);

  /**
   * @brief Ustawia jasnosc; dziala od razu, takze w trakcie testu.
   * @param percent Jasnosc 0..100 % (wieksze wartosci sa obcinane do 100).
   */
  void setLevel(uint8_t percent);

  /// @return Jasnosc w %.
  uint8_t level() const { return _percent; }

  /// @return Podlaczony pin albo -1.
  int8_t pin() const { return _pin; }

 private:
  /// Zapisuje wypelnienie PWM odpowiadajace _percent.
  void apply();

  int8_t _pin = -1;        ///< Pin LED albo -1.
  uint8_t _percent = 100;  ///< Jasnosc w %.
};
