/**
 * @file BoardPins.h
 * @brief Domyslne piny wyswietlacza dla poszczegolnych wersji ESP32.
 *
 * Wybor wersji odbywa sie przy kompilacji (CONFIG_IDF_TARGET_*), wiec ten sam kod
 * dziala na ESP32-C3, ESP32-S3 i klasycznym ESP32. Piny mozna nadpisac na stronie
 * WWW przed uruchomieniem testu.
 */
#pragma once

#include <Arduino.h>

/**
 * @brief Przypisanie pinow GPIO do sygnalow wyswietlacza SPI.
 *
 * Wartosc -1 oznacza, ze sygnal nie jest podlaczony.
 */
struct LcdPins {
  int8_t sclk;  ///< Zegar SPI (na module: SCK / SCL / CLK).
  int8_t mosi;  ///< Dane do wyswietlacza (na module: SDI / SDA / MOSI / DIN).
  int8_t miso;  ///< Dane z wyswietlacza (na module: SDO / MISO); potrzebne do odczytu ID.
  int8_t cs;    ///< Wybor ukladu (Chip Select); -1 gdy CS na stale podlaczony do masy.
  int8_t dc;    ///< Komenda / dane (na module: DC / RS / A0).
  int8_t rst;   ///< Reset sprzetowy (na module: RESET / RES / RST).
  int8_t led;   ///< Podswietlenie (na module: LED / BL / BLK), sterowane PWM.
};

#if defined(CONFIG_IDF_TARGET_ESP32C3)
// SCLK 6, MOSI 7, MISO 2, CS 10 = natywne piny SPI2 (IO MUX).
// Omijamy GPIO 8/9 (strapping, LED), 11-17 (flash), 18/19 (USB), 20/21 (UART0)
/// Domyslne piny dla ESP32-C3: SCLK, MOSI, MISO, CS, DC, RST, LED.
static constexpr LcdPins DEFAULT_LCD_PINS = { 6, 7, 2, 10, 3, 1, 4 };
/// Nazwa ukladu wyswietlana na stronie i w monitorze portu.
#define BOARD_NAME "ESP32-C3"

#elif defined(CONFIG_IDF_TARGET_ESP32S3)
/// Domyslne piny dla ESP32-S3: SCLK, MOSI, MISO, CS, DC, RST, LED.
static constexpr LcdPins DEFAULT_LCD_PINS = { 12, 11, 13, 10, 9, 8, 7 };
/// Nazwa ukladu wyswietlana na stronie i w monitorze portu.
#define BOARD_NAME "ESP32-S3"

#elif defined(CONFIG_IDF_TARGET_ESP32)
// Natywne piny VSPI
/// Domyslne piny dla ESP32: SCLK, MOSI, MISO, CS, DC, RST, LED.
static constexpr LcdPins DEFAULT_LCD_PINS = { 18, 23, 19, 5, 2, 4, 15 };
/// Nazwa ukladu wyswietlana na stronie i w monitorze portu.
#define BOARD_NAME "ESP32"

#else
#error "Brak domyslnych pinow dla tej wersji ESP32 - dodaj je w BoardPins.h"
#endif
