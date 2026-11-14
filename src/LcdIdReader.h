/**
 * @file LcdIdReader.h
 * @brief Odczyt identyfikatora sterownika LCD niezalezny od LovyanGFX.
 */
#pragma once

#include <Arduino.h>

#include "BoardPins.h"
#include "LcdModels.h"

/**
 * @brief Odczytuje ID sterownika wyswietlacza i porownuje je z wybranym modelem.
 *
 * Sposob odczytu (komendy, takty dummy, metoda D9) pochodzi z wybranego modelu
 * (LcdModel::idSteps), a wynik jest porownywany z ID oryginalnego sterownika.
 * Tak wychodza "podrobki", np. modul z napisem ILI9341 z innym ukladem w srodku.
 *
 * Wyswietlacz nie jest inicjalizowany - tylko reset sprzetowy i programowy.
 * Piny sterowane sa programowo (bit-bang, ok. 100 kHz), bez sprzetowego SPI.
 * Wymaga pinu MISO (SDO modulu) albo trybu 3-wire, w ktorym sterownik
 * odpowiada ta sama linia SDA/MOSI.
 */
class LcdIdReader {
 public:
  /// Werdykt odczytu.
  enum class Verdict : uint8_t {
    Match,       ///< ID zgodne z wybranym modelem.
    Mismatch,    ///< Sterownik odpowiada, ale ID jest inne.
    NoResponse,  ///< Same 00 albo FF - brak komunikacji.
  };

  /// Wynik odczytu.
  struct Result {
    Verdict verdict;     ///< Werdykt.
    char summary[96];    ///< Werdykt dla uzytkownika, np. "NIEZGODNY z ILI9341 - prawdopodobnie ST7789".
    char details[320];   ///< Odczytane rejestry z wartosciami oczekiwanymi, po jednym w linii.
  };

  /**
   * @param pins Piny wyswietlacza.
   * @param threeWire true = odpowiedz czytana z linii MOSI (moduly bez MISO).
   */
  LcdIdReader(const LcdPins& pins, bool threeWire);

  /**
   * @brief Resetuje sterownik i odczytuje rejestry wg krokow z modelu.
   * @param model Model, ktorego sposobem czytac i z ktorym porownywac.
   * @return Werdykt i szczegoly odczytu.
   */
  Result read(const LcdModel& model);

 private:
  static constexpr uint8_t MAX_LEN = 4;                     ///< Maks. bajtow w jednym rejestrze.
  static constexpr uint8_t MAX_BITS = 9 + MAX_LEN * 8 + 1;  ///< Maks. bitow z taktami dummy.

  /// Ustawia kierunki pinow i stany spoczynkowe.
  void begin();
  /// Konczy odczyt: CS w gore, MOSI z powrotem jako wyjscie.
  void end();
  /// Reset sprzetowy (RST) i programowy (komenda 0x01).
  void hardwareReset();
  /// Wysyla komende (DC = 0).
  void writeCommand(uint8_t cmd);
  /// Wysyla bajt danych (DC = 1).
  void writeData(uint8_t data);
  /**
   * @brief Wysyla komende i czyta podana liczbe bitow (CS trzymany przez caly odczyt).
   * @param cmd Komenda odczytu.
   * @param[out] bits Odczytane bity (0/1), po jednym w bajcie.
   * @param nBits Liczba bitow do odczytu (z taktami dummy).
   */
  void readBits(uint8_t cmd, uint8_t* bits, uint8_t nBits);
  /**
   * @brief Odczytuje rejestr wg kroku w dwoch wyrownaniach.
   * @param step Krok odczytu z modelu.
   * @param[out] primary Bajty wyrownane wg dokumentacji (LcdIdStep::dummyBits).
   * @param[out] alternate Bajty przesuniete o 1 bit (klony czesto roznia sie taktem dummy).
   */
  void readStep(const LcdIdStep& step, uint8_t* primary, uint8_t* alternate);
  /// Wysyla 8 bitow, od najstarszego.
  void writeBits(uint8_t value);
  /// Jeden takt zegara z odczytem bitu.
  uint8_t readBit();

  LcdPins _pins;    ///< Piny wyswietlacza.
  bool _threeWire;  ///< Odczyt przez MOSI zamiast MISO.
  int8_t _pinIn;    ///< Pin, z ktorego czytane sa dane (MISO albo MOSI).
};
