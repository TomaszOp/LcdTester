/**
 * @file LcdDevice.h
 * @brief Parametry testu i wyswietlacz LovyanGFX konfigurowany w czasie dzialania.
 */
#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include "BoardPins.h"
#include "LcdModels.h"

/**
 * @brief Komplet parametrow jednego testu, wybieranych na stronie WWW.
 */
struct LcdTestConfig {
  const LcdModel* model = nullptr;  ///< Wybrany model z ::LCD_MODELS.
  uint8_t resolutionIndex = 0;      ///< Indeks w LcdModel::resolutions.
  uint32_t spiFreqHz = 40000000;    ///< Czestotliwosc zapisu SPI w Hz.
  uint8_t spiMode = 0;              ///< Tryb SPI 0..3 (CPOL/CPHA).
  uint8_t rotation = 0;             ///< Obrot ekranu 0..3 (co 90 stopni).
  bool invert = false;              ///< Inwersja kolorow.
  bool swapRB = false;              ///< Zamiana czerwony<->niebieski (rgb_order w LovyanGFX).
  bool spi3wire = false;            ///< Tylko odczyt ID: odpowiedz linia SDA/MOSI (moduly bez MISO).
  LcdPins pins = DEFAULT_LCD_PINS;  ///< Piny GPIO wyswietlacza.
  uint16_t packetDelayMs = 0;       ///< Fazy 1-2: odstep po kazdej paczce w ms, 0 = brak.
  uint8_t fpsLimit = 0;             ///< Faza 3: limit pelnych klatek na sekunde, 0 = brak.

  /// @return Wybrana rozdzielczosc (wymaga ustawionego model).
  const LcdResolution& resolution() const {
    return model->resolutions[resolutionIndex];
  }
};

/**
 * @brief Wyswietlacz SPI oparty na LovyanGFX, konfigurowany z LcdTestConfig.
 *
 * W odroznieniu od typowej konfiguracji LovyanGFX (stala klasa LGFX z pinami
 * wpisanymi w kodzie) model, piny i szybkosc sa ustawiane w czasie dzialania.
 * Dla kazdego testu tworzony jest nowy obiekt, a destruktor zwalnia szyne SPI,
 * dzieki czemu kolejny test moze uzyc innych ustawien bez restartu ESP.
 *
 * Start wyswietlacza (sekwencja inicjalizacji) wykonuje sam LovyanGFX
 * wlasciwym panelem: Panel_ILI9341, Panel_ST7789, Panel_ST7735S, Panel_ILI9488.
 * Podswietleniem steruje osobno klasa Backlight.
 */
class LcdDevice : public lgfx::LGFX_Device {
 public:
  /**
   * @brief Konfiguruje szyne SPI i panel; nie wysyla jeszcze nic do wyswietlacza.
   * @param config Parametry testu (model, rozdzielczosc, piny, SPI, kolory).
   */
  explicit LcdDevice(const LcdTestConfig& config);

  /// Zwalnia szyne SPI. Obraz na wyswietlaczu pozostaje.
  ~LcdDevice();

  /**
   * @brief Uruchamia wyswietlacz (reset i sekwencja inicjalizacji LovyanGFX).
   * @return true, gdy inicjalizacja sie powiodla.
   */
  bool begin();

 private:
  lgfx::Bus_SPI _bus;                    ///< Szyna SPI (SPI2_HOST, DMA).
  lgfx::Panel_ILI9341 _panelIli9341;     ///< Panel dla LcdDriver::ILI9341.
  lgfx::Panel_ST7789 _panelSt7789;       ///< Panel dla LcdDriver::ST7789.
  lgfx::Panel_ST7735S _panelSt7735;      ///< Panel dla LcdDriver::ST7735.
  lgfx::Panel_ILI9488 _panelIli9488;     ///< Panel dla LcdDriver::ILI9488.
  lgfx::Panel_Device* _panel = nullptr;  ///< Panel wybrany dla biezacego modelu.
};
