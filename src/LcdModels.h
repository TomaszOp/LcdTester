/**
 * @file LcdModels.h
 * @brief Spis obslugiwanych wyswietlaczy: modele, rozdzielczosci i sposob odczytu ID.
 *
 * Nowy model: dodaj sterownik do ::LcdDriver, obsluz go w LcdDevice.cpp
 * i dopisz wpis do tablicy ::LCD_MODELS w LcdModels.cpp (z krokami odczytu ID).
 */
#pragma once

#include <Arduino.h>

/// Sterownik wyswietlacza - decyduje, ktorego panelu LovyanGFX uzyc.
enum class LcdDriver : uint8_t {
  ILI9341,  ///< ILI9341, 240x320.
  ST7789,   ///< ST7789, pamiec 240x320, rozne mniejsze matryce.
  ST7735,   ///< ST7735 / ST7735S, pamiec 132x162.
  ILI9488,  ///< ILI9488, 320x480, przez SPI kolor 18-bit.
};

/**
 * @brief Jedna rozdzielczosc matrycy dostepna dla danego sterownika.
 *
 * Pamiec obrazu sterownika bywa wieksza od matrycy - wtedy widoczny obszar
 * zaczyna sie w niej z przesunieciem (offsetX, offsetY).
 */
struct LcdResolution {
  uint16_t width;    ///< Szerokosc matrycy w pikselach (przy obrocie 0).
  uint16_t height;   ///< Wysokosc matrycy w pikselach (przy obrocie 0).
  int16_t offsetX;   ///< Przesuniecie poziome widocznego obszaru w pamieci sterownika.
  int16_t offsetY;   ///< Przesuniecie pionowe widocznego obszaru w pamieci sterownika.
  const char* note;  ///< Opis dla uzytkownika (przekatna, wariant modulu).
};

/// Sposob odczytu jednego rejestru identyfikacyjnego.
enum class IdReadMethod : uint8_t {
  Direct,  ///< Komenda, potem LcdIdStep::dummyBits taktow zegara, potem dane.
  IliD9,   ///< ILI934x: kazdy bajt rejestru osobno, po komendzie D9 z argumentem 0x10 + indeks.
};

/**
 * @brief Jeden krok odczytu ID: ktory rejestr, jak go czytac i czego oczekiwac.
 *
 * Kazdy model ma wlasna liste krokow (LcdModel::idSteps), bo producenci
 * udostepniaja ID w roznych rejestrach i z roznymi taktami "dummy".
 */
struct LcdIdStep {
  uint8_t cmd;           ///< Kod komendy odczytu (np. 0x04 RDDID, 0xD3 RDID4).
  uint8_t len;           ///< Liczba bajtow danych do odczytu (max 4).
  uint8_t dummyBits;     ///< Takty zegara "dummy" przed danymi wg dokumentacji sterownika.
  IdReadMethod method;   ///< Sposob odczytu.
  const char* name;      ///< Nazwa rejestru pokazywana w wynikach.
  bool checkExpected;    ///< true = wynik porownywany z expected; false = tylko informacyjnie.
  uint8_t expected[4];   ///< Wartosc oczekiwana dla oryginalnego sterownika.
};

/**
 * @brief Opis jednego modelu wyswietlacza.
 *
 * Zawiera wszystko, czego potrzebuja: strona WWW (nazwy, rozdzielczosci,
 * ustawienia domyslne), LcdDevice (sterownik, pamiec, przesuniecia)
 * i LcdIdReader (kroki odczytu ID).
 */
struct LcdModel {
  const char* id;                    ///< Identyfikator uzywany w API (np. "ili9341").
  const char* name;                  ///< Nazwa wyswietlana na stronie.
  LcdDriver driver;                  ///< Sterownik - wybor panelu LovyanGFX.
  bool invertDefault;                ///< Domyslna inwersja kolorow (wiekszosc ST7789 jej wymaga).
  bool swapRbDefault;                ///< Domyslna zamiana czerwony<->niebieski wzgledem LovyanGFX.
  uint32_t maxFreqHz;                ///< Zalecana czestotliwosc zapisu SPI, domyslna na stronie.
  uint16_t memoryWidth;              ///< Szerokosc pamieci obrazu w sterowniku.
  uint16_t memoryHeight;             ///< Wysokosc pamieci obrazu w sterowniku.
  const LcdResolution* resolutions;  ///< Dostepne rozdzielczosci.
  uint8_t resolutionCount;           ///< Liczba elementow w resolutions.
  const LcdIdStep* idSteps;          ///< Kroki odczytu ID wlasciwe dla tego sterownika.
  uint8_t idStepCount;               ///< Liczba elementow w idSteps.
};

/// Wszystkie obslugiwane modele (definicja w LcdModels.cpp).
extern const LcdModel LCD_MODELS[];
/// Liczba elementow w ::LCD_MODELS.
extern const uint8_t LCD_MODEL_COUNT;

/// Czestotliwosci SPI (Hz) do wyboru na stronie.
extern const uint32_t SPI_FREQUENCIES[];
/// Liczba elementow w ::SPI_FREQUENCIES.
extern const uint8_t SPI_FREQUENCY_COUNT;

/**
 * @brief Wyszukuje model po identyfikatorze z API.
 * @param id Identyfikator modelu (wielkosc liter bez znaczenia), np. "st7789".
 * @return Wskaznik na model albo nullptr, gdy nie ma takiego modelu.
 */
const LcdModel* findLcdModel(const String& id);
