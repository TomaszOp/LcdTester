/**
 * @file SoftStmpe610.h
 * @brief Sterownik dotyku STMPE610 na osobnej, programowej magistrali SPI.
 */
#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

/**
 * @brief STMPE610 obslugiwany programowo (bit-bang) na wlasnych pinach.
 *
 * Odpowiednik SoftXpt2046 dla kontrolera STMPE610 - uzywany, gdy dotyk ma osobna
 * magistrale (T_CLK, T_DIN, T_DO, T_CS). ESP32-C3 ma tylko jeden wolny sprzetowy
 * kontroler SPI, zajety przez wyswietlacz, dlatego magistrala jest programowa.
 *
 * Sekwencja startu jest taka sama jak w sterowniku LovyanGFX (Touch_STMPE610):
 * sprawdzenie wersji ukladu (0x0811) w trybie SPI 0, a w razie braku odpowiedzi
 * w trybie 1, reset i konfiguracja przetwornika. Piny pochodza z config():
 * pin_sclk, pin_mosi, pin_miso, pin_cs i opcjonalnie pin_int.
 */
class SoftStmpe610 : public lgfx::ITouch {
 public:
  SoftStmpe610();

  /**
   * @brief Ustawia piny, wykrywa tryb SPI i konfiguruje kontroler.
   * @return false, gdy brakuje pinow albo uklad nie zwrocil wersji 0x0811.
   */
  bool init() override;
  /// Nic nie robi - kontroler pracuje ciagle.
  void wakeup() override {}
  /// Nic nie robi - kontroler pracuje ciagle.
  void sleep() override {}

  /**
   * @brief Odczyt surowego punktu dotyku (ostatnia probka z kolejki FIFO).
   * @param[out] tp Punkt: x, y w jednostkach przetwornika (0..4095), size = nacisk.
   * @param count Liczba punktow do odczytu (STMPE610 obsluguje 1).
   * @return 1 przy dotknieciu, 0 gdy brak dotyku.
   */
  uint_fast8_t getTouchRaw(lgfx::touch_point_t* tp, uint_fast8_t count) override;

 private:
  /// Wysyla i jednoczesnie odbiera jeden bajt w biezacym trybie SPI.
  uint8_t transfer(uint8_t value);
  /// Odczyt rejestru 8-bitowego.
  uint8_t readRegister(uint8_t reg);
  /// Zapis rejestru 8-bitowego.
  void writeRegister(uint8_t reg, uint8_t value);
  /// @return Wersja ukladu (0x0811 dla STMPE610).
  uint16_t readVersion();

  uint8_t _spiMode = 0;  ///< Wykryty tryb SPI (0 albo 1).
  bool _inited = false;  ///< Czy init() sie powiodlo.
};
