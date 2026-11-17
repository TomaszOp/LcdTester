/**
 * @file TouchXpt2046.h
 * @brief Sterownik dotyku XPT2046 na wspolnej magistrali SPI z pomiarem nacisku.
 */
#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

/**
 * @brief XPT2046 na sprzetowej magistrali SPI wspolnej z wyswietlaczem, z prawdziwym pomiarem nacisku.
 *
 * Zamiennik lgfx::Touch_XPT2046. Sterownik LovyanGFX uznaje odczyt za dotyk, gdy
 * wspolrzedne mieszcza sie w zakresie, prawie nie sprawdzajac nacisku - bez pinu
 * T_IRQ szum na liniach daje dotkniecia "fantomowe". Ta klasa liczy nacisk ze
 * standardowego wzoru XPT2046: Z = Z1 + 4095 - Z2, i zglasza dotyk tylko, gdy
 * Z przekracza PRESSURE_THRESHOLD w co najmniej MIN_VALID z SAMPLES pomiarow.
 *
 * Transmisja jak w LovyanGFX (lgfx::spi na hoscie wyswietlacza, pin_cs, freq).
 */
class TouchXpt2046 : public lgfx::ITouch {
 public:
  static constexpr int PRESSURE_THRESHOLD = 150;  ///< Minimalny nacisk Z uznawany za dotkniecie.
  static constexpr uint8_t SAMPLES = 7;           ///< Pomiarow w jednym odczycie.
  static constexpr uint8_t MIN_VALID = 3;         ///< Ile pomiarow musi miec nacisk powyzej progu.

  /// Ostatni pojedynczy pomiar (bez filtrowania) - do diagnostyki na stronie.
  struct Sample {
    int x = 0;   ///< X z przetwornika (0..4095).
    int y = 0;   ///< Y z przetwornika (0..4095).
    int z1 = 0;  ///< Pomiar Z1.
    int z2 = 0;  ///< Pomiar Z2.
    int z = 0;   ///< Nacisk Z1 + 4095 - Z2.
  };

  TouchXpt2046();

  /// @return Srodkowy pomiar z ostatniego odczytu, takze gdy nie uznano go za dotyk.
  Sample lastSample() const { return _last; }

  /// Ustawia CS i inicjalizuje magistrale (wspolna z wyswietlaczem). @return true przy powodzeniu.
  bool init() override;
  /// Nic nie robi - kontroler budzi sie przy kazdym pomiarze.
  void wakeup() override {}
  /// Nic nie robi - po pomiarze kontroler przechodzi w uspienie sam.
  void sleep() override {}

  /**
   * @brief Odczyt surowego punktu dotyku (mediana pomiarow z naciskiem powyzej progu).
   * @param[out] tp Punkt: x, y w jednostkach przetwornika (0..4095), size = nacisk Z.
   * @param count Liczba punktow do odczytu (XPT2046 obsluguje 1).
   * @return 1 przy dotknieciu, 0 gdy brak dotyku albo za slaby nacisk.
   */
  uint_fast8_t getTouchRaw(lgfx::touch_point_t* tp, uint_fast8_t count) override;

 private:
  bool _inited = false;  ///< Czy init() sie powiodlo.
  Sample _last;          ///< Ostatni pomiar do diagnostyki.
};
