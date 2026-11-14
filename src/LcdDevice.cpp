/**
 * @file LcdDevice.cpp
 * @brief Implementacja klasy LcdDevice - konfiguracja szyny SPI i panelu LovyanGFX.
 */
#include "LcdDevice.h"

LcdDevice::LcdDevice(const LcdTestConfig& config) {
  const LcdResolution& res = config.resolution();

  {  // Szyna SPI
    auto cfg = _bus.config();
    cfg.spi_host = SPI2_HOST;
    cfg.spi_mode = config.spiMode;
    cfg.freq_write = config.spiFreqHz;
    cfg.freq_read = 16000000;
    cfg.spi_3wire = false;
    cfg.use_lock = true;
    cfg.dma_channel = SPI_DMA_CH_AUTO;
    cfg.pin_sclk = config.pins.sclk;
    cfg.pin_mosi = config.pins.mosi;
    cfg.pin_miso = config.pins.miso;
    cfg.pin_dc = config.pins.dc;
    _bus.config(cfg);
  }

  switch (config.model->driver) {
    case LcdDriver::ILI9341: _panel = &_panelIli9341; break;
    case LcdDriver::ST7789:  _panel = &_panelSt7789;  break;
    case LcdDriver::ST7735:  _panel = &_panelSt7735;  break;
    case LcdDriver::ILI9488: _panel = &_panelIli9488; break;
  }
  _panel->setBus(&_bus);

  {  // Panel
    auto cfg = _panel->config();
    cfg.pin_cs = config.pins.cs;
    cfg.pin_rst = config.pins.rst;
    cfg.pin_busy = -1;
    cfg.memory_width = config.model->memoryWidth;
    cfg.memory_height = config.model->memoryHeight;
    cfg.panel_width = res.width;
    cfg.panel_height = res.height;
    cfg.offset_x = res.offsetX;
    cfg.offset_y = res.offsetY;
    cfg.offset_rotation = 0;
    cfg.readable = config.pins.miso >= 0;
    cfg.invert = config.invert;
    // false = domyslna kolejnosc LovyanGFX dla danego panelu
    cfg.rgb_order = config.swapRB;
    cfg.dlen_16bit = false;
    cfg.bus_shared = false;
    _panel->config(cfg);
  }

  setPanel(_panel);
}

LcdDevice::~LcdDevice() {
  _bus.release();
}

bool LcdDevice::begin() {
  // Podswietlenie (pin LED) obsluguje osobno klasa Backlight
  return init();
}
