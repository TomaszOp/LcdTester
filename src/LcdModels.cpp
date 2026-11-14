/**
 * @file LcdModels.cpp
 * @brief Spis modeli: rozdzielczosci, kroki odczytu ID i czestotliwosci SPI.
 *
 * Tu dopisuje sie nowe modele - patrz opis w LcdModels.h.
 */
#include "LcdModels.h"

// ---------------------------------------------------------------------------
// ILI9341 - zawsze 240x320
// ---------------------------------------------------------------------------
/// Rozdzielczosci ILI9341.
static const LcdResolution ILI9341_RES[] = {
  { 240, 320, 0, 0, "2.2\" / 2.4\" / 2.8\" / 3.2\"" },
};

// ---------------------------------------------------------------------------
// ST7789 - pamiec sterownika 240x320, mniejsze matryce z przesunieciem
// ---------------------------------------------------------------------------
/// Rozdzielczosci ST7789 z przesunieciami w pamieci 240x320.
static const LcdResolution ST7789_RES[] = {
  { 240, 320,  0,  0, "2.0\" / 2.4\"" },
  { 240, 240,  0,  0, "1.3\" / 1.54\"" },
  { 240, 280,  0, 20, "1.69\" (zaokraglone rogi)" },
  { 170, 320, 35,  0, "1.9\"" },
  { 172, 320, 34,  0, "1.47\"" },
  { 135, 240, 52, 40, "1.14\" (np. TTGO T-Display)" },
};

// ---------------------------------------------------------------------------
// ST7735 / ST7735S - pamiec sterownika 132x162. "Zakladka" to kolor folii
// ochronnej na nowym module - po niej rozpoznaje sie wariant przesuniecia.
// ---------------------------------------------------------------------------
/// Rozdzielczosci ST7735 z przesunieciami w pamieci 132x162.
static const LcdResolution ST7735_RES[] = {
  { 128, 160,  0,  0, "1.8\" (czerwona/czarna zakladka)" },
  { 128, 160,  2,  1, "1.8\" (zielona zakladka)" },
  { 128, 128,  2,  3, "1.44\"" },
  {  80, 160, 26,  1, "0.96\" (zwykle wymaga inwersji)" },
};

// ---------------------------------------------------------------------------
// ILI9488 - zawsze 320x480. Przez SPI tylko kolor 18-bit (3 bajty na piksel,
// LovyanGFX przelicza sam) - pelna klatka to 460 kB, wiec FPS jest niski.
// ---------------------------------------------------------------------------
/// Rozdzielczosci ILI9488.
static const LcdResolution ILI9488_RES[] = {
  { 320, 480, 0, 0, "3.5\" / 4.0\"" },
};

// ---------------------------------------------------------------------------
// Odczyt ID - kazdy sterownik wg wlasnej dokumentacji
// ---------------------------------------------------------------------------
// ILI9341: ID w rejestrze D3 (RDID4) = 00 93 41. Przez SPI czesc ukladow oddaje go
// tylko metoda D9 (nieudokumentowana, stosowana m.in. przez Adafruit), czesc bezposrednio
// po bajcie dummy. RDDID (04) w ILI9341 zwykle zwraca zera - tylko informacyjnie.
/// Kroki odczytu ID dla ILI9341.
static const LcdIdStep ILI9341_ID[] = {
  { 0xD3, 3, 0, IdReadMethod::IliD9,  "RDID4 metoda D9", true,  { 0x00, 0x93, 0x41 } },
  { 0xD3, 3, 8, IdReadMethod::Direct, "RDID4 bezposrednio", true,  { 0x00, 0x93, 0x41 } },
  { 0x04, 3, 1, IdReadMethod::Direct, "RDDID", false, {} },
  { 0x09, 4, 1, IdReadMethod::Direct, "RDDST status", false, {} },
};

// ST7789: RDDID (04) = 85 85 52 po 1 takcie dummy; RDID1..3 (DA/DB/DC) po jednym bajcie.
/// Kroki odczytu ID dla ST7789.
static const LcdIdStep ST7789_ID[] = {
  { 0x04, 3, 1, IdReadMethod::Direct, "RDDID", true, { 0x85, 0x85, 0x52 } },
  { 0xDA, 1, 0, IdReadMethod::Direct, "RDID1", true, { 0x85 } },
  { 0xDB, 1, 0, IdReadMethod::Direct, "RDID2", true, { 0x85 } },
  { 0xDC, 1, 0, IdReadMethod::Direct, "RDID3", true, { 0x52 } },
  { 0x09, 4, 1, IdReadMethod::Direct, "RDDST status", false, {} },
};

// ST7735/ST7735S: RDDID (04) = 7C 89 F0 po 1 takcie dummy; RDID1..3 (DA/DB/DC) po jednym bajcie.
/// Kroki odczytu ID dla ST7735 / ST7735S.
static const LcdIdStep ST7735_ID[] = {
  { 0x04, 3, 1, IdReadMethod::Direct, "RDDID", true, { 0x7C, 0x89, 0xF0 } },
  { 0xDA, 1, 0, IdReadMethod::Direct, "RDID1", true, { 0x7C } },
  { 0xDB, 1, 0, IdReadMethod::Direct, "RDID2", true, { 0x89 } },
  { 0xDC, 1, 0, IdReadMethod::Direct, "RDID3", true, { 0xF0 } },
  { 0x09, 4, 1, IdReadMethod::Direct, "RDDST status", false, {} },
};

// ILI9488: ID w RDID4 (D3) = 00 94 88, po bajcie dummy. Uwaga: w wielu modulach
// ILI9488 linia SDO nie przechodzi w stan wysokiej impedancji - nie dzielic jej z innymi.
/// Kroki odczytu ID dla ILI9488.
static const LcdIdStep ILI9488_ID[] = {
  { 0xD3, 3, 8, IdReadMethod::Direct, "RDID4 bezposrednio", true,  { 0x00, 0x94, 0x88 } },
  { 0xD3, 3, 0, IdReadMethod::IliD9,  "RDID4 metoda D9", true,  { 0x00, 0x94, 0x88 } },
  { 0x04, 3, 1, IdReadMethod::Direct, "RDDID", false, {} },
  { 0xDA, 1, 0, IdReadMethod::Direct, "RDID1", false, {} },
  { 0xDB, 1, 0, IdReadMethod::Direct, "RDID2", false, {} },
  { 0xDC, 1, 0, IdReadMethod::Direct, "RDID3", false, {} },
  { 0x09, 4, 1, IdReadMethod::Direct, "RDDST status", false, {} },
};

/// Liczba elementow tablicy statycznej.
#define COUNT(a) (sizeof(a) / sizeof((a)[0]))

const LcdModel LCD_MODELS[] = {
  { "ili9341", "ILI9341", LcdDriver::ILI9341, false, false, 40000000, 240, 320,
    ILI9341_RES, COUNT(ILI9341_RES), ILI9341_ID, COUNT(ILI9341_ID) },
  { "st7789",  "ST7789",  LcdDriver::ST7789,  true,  false, 80000000, 240, 320,
    ST7789_RES, COUNT(ST7789_RES), ST7789_ID, COUNT(ST7789_ID) },
  // ST7735 wg dokumentacji do ok. 15 MHz, w praktyce dziala do ~27 MHz
  { "st7735",  "ST7735",  LcdDriver::ST7735,  false, false, 26666667, 132, 162,
    ST7735_RES, COUNT(ST7735_RES), ST7735_ID, COUNT(ST7735_ID) },
  // ILI9488 wg dokumentacji zapis do 20 MHz, w praktyce czesto dziala wiecej
  { "ili9488", "ILI9488", LcdDriver::ILI9488, false, false, 20000000, 320, 480,
    ILI9488_RES, COUNT(ILI9488_RES), ILI9488_ID, COUNT(ILI9488_ID) },
};
const uint8_t LCD_MODEL_COUNT = sizeof(LCD_MODELS) / sizeof(LCD_MODELS[0]);

const uint32_t SPI_FREQUENCIES[] = {
  1000000, 5000000, 10000000, 20000000, 26666667, 40000000, 80000000,
};
const uint8_t SPI_FREQUENCY_COUNT = sizeof(SPI_FREQUENCIES) / sizeof(SPI_FREQUENCIES[0]);

const LcdModel* findLcdModel(const String& id) {
  for (uint8_t i = 0; i < LCD_MODEL_COUNT; i++) {
    if (id.equalsIgnoreCase(LCD_MODELS[i].id)) return &LCD_MODELS[i];
  }
  return nullptr;
}
