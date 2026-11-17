/**
 * @file SoftXpt2046.h
 * @brief Sterownik dotyku XPT2046 na osobnej, programowej magistrali SPI.
 */
#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

/**
 * @brief XPT2046 obslugiwany programowo (bit-bang) na wlasnych pinach.
 *
 * Uzywany, gdy dotyk ma osobna magistrale (piny T_CLK, T_DIN, T_DO, T_CS).
 * ESP32-C3 ma tylko jeden wolny sprzetowy kontroler SPI, zajety przez wyswietlacz,
 * dlatego osobna magistrala jest realizowana programowo. XPT2046 pracuje
 * wolno (do ok. 2 MHz), wiec programowe SPI w zupelnosci wystarcza.
 *
 * Implementuje interfejs lgfx::ITouch, wiec LovyanGFX traktuje go jak
 * kazdy inny sterownik dotyku. Piny pochodza z config(): pin_sclk, pin_mosi,
 * pin_miso, pin_cs i opcjonalnie pin_int (T_IRQ).
 */
class SoftXpt2046 : public lgfx::ITouch {
 public:
  /// Minimalny nacisk (Z) uznawany za dotkniecie.
  static constexpr int PRESSURE_THRESHOLD = 300;

  SoftXpt2046();

  /// Ustawia kierunki pinow. @return true, gdy wymagane piny sa ustawione.
  bool init() override;
  /// Nic nie robi - kontroler budzi sie przy kazdym pomiarze.
  void wakeup() override {}
  /// Nic nie robi - po pomiarze kontroler przechodzi w uspienie sam.
  void sleep() override {}

  /**
   * @brief Odczyt surowego punktu dotyku (mediana kilku pomiarow).
   * @param[out] tp Punkt: x, y w jednostkach przetwornika (0..4095), size = nacisk.
   * @param count Liczba punktow do odczytu (XPT2046 obsluguje 1).
   * @return 1 przy dotknieciu, 0 gdy brak dotyku.
   */
  uint_fast8_t getTouchRaw(lgfx::touch_point_t* tp, uint_fast8_t count) override;

 private:
  /// Wysyla komende i zwraca 12-bitowy wynik przetwornika.
  uint16_t transfer(uint8_t command);

  bool _inited = false;  ///< Czy init() sie powiodlo.
};
