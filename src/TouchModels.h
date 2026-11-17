/**
 * @file TouchModels.h
 * @brief Spis obslugiwanych kontrolerow dotyku.
 *
 * Kontroler wybiera sie recznie na stronie - program nie wykrywa modelu.
 * Nowy kontroler: dodaj go do ::TouchController, obsluz w LcdDevice.cpp
 * i dopisz wpis do tablicy ::TOUCH_MODELS w TouchModels.cpp.
 */
#pragma once

#include <Arduino.h>

/// Kontroler dotyku - decyduje, ktorego sterownika dotyku uzyc.
enum class TouchController : uint8_t {
  XPT2046,   ///< Rezystancyjny, SPI (najczesciej spotykany przy ILI9341).
  STMPE610,  ///< Rezystancyjny, SPI.
  FT6x36,    ///< Pojemnosciowy FT6206 / FT6236 / FT6336, I2C.
  CST816S,   ///< Pojemnosciowy CST816S (czesto przy GC9A01 i ST7789), I2C.
  GT911,     ///< Pojemnosciowy GT911 (wieksze ekrany), I2C.
};

/// Szyna, po ktorej komunikuje sie kontroler.
enum class TouchBus : uint8_t {
  SPI,  ///< SPI - domyslnie wspolna z wyswietlaczem, osobny pin CS.
  I2C,  ///< I2C - piny SDA, SCL i adres.
};

/// Opis jednego kontrolera dotyku.
struct TouchModel {
  const char* id;            ///< Identyfikator uzywany w API (np. "xpt2046").
  const char* name;          ///< Nazwa wyswietlana na stronie.
  TouchController controller;  ///< Kontroler - wybor sterownika.
  TouchBus bus;              ///< Szyna komunikacji.
  uint8_t i2cAddr;           ///< Domyslny adres I2C (0 dla SPI).
  bool separateBusSupported; ///< Czy mozna go podlaczyc osobna magistrala SPI (programowa).
};

/// Wszystkie obslugiwane kontrolery dotyku (definicja w TouchModels.cpp).
extern const TouchModel TOUCH_MODELS[];
/// Liczba elementow w ::TOUCH_MODELS.
extern const uint8_t TOUCH_MODEL_COUNT;

/// Czestotliwosc SPI dotyku przy wspolnej magistrali (kontrolery dotyku sa wolne).
constexpr uint32_t TOUCH_SPI_FREQ_HZ = 2500000;

/**
 * @brief Wyszukuje kontroler po identyfikatorze z API.
 * @param id Identyfikator (wielkosc liter bez znaczenia); pusty albo "none" = brak dotyku.
 * @return Wskaznik na kontroler albo nullptr.
 */
const TouchModel* findTouchModel(const String& id);
