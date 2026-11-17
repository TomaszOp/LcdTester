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
    case LcdDriver::GC9A01A: _panel = &_panelGc9a01;  break;
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
    // Dotyk SPI na wspolnej magistrali wymaga zwalniania szyny miedzy operacjami
    cfg.bus_shared = config.touch.model && config.touch.model->bus == TouchBus::SPI &&
                     !config.touch.separateBus;
    _panel->config(cfg);
  }

  setPanel(_panel);
  setupTouch(config.touch);
}

void LcdDevice::setupTouch(const TouchConfig& touch) {
  if (!touch.model) return;
  const TouchPins& p = touch.pins;

  switch (touch.model->controller) {
    case TouchController::XPT2046:
      _touch = touch.separateBus ? static_cast<lgfx::ITouch*>(&_touchSoftXpt2046)
                                 : static_cast<lgfx::ITouch*>(&_touchXpt2046);
      break;
    case TouchController::STMPE610:
      _touch = touch.separateBus ? static_cast<lgfx::ITouch*>(&_touchSoftStmpe610)
                                 : static_cast<lgfx::ITouch*>(&_touchStmpe610);
      break;
    case TouchController::FT6x36:   _touch = &_touchFt6x36;   break;
    case TouchController::CST816S:  _touch = &_touchCst816s;  break;
    case TouchController::GT911:    _touch = &_touchGt911;    break;
  }

  auto cfg = _touch->config();
  cfg.pin_int = p.irq;
  cfg.offset_rotation = 0;  // kalibracja liczona jest z surowych odczytow
  if (touch.model->bus == TouchBus::SPI) {
    const auto& bus = _bus.config();
    cfg.spi_host = SPI2_HOST;
    cfg.freq = TOUCH_SPI_FREQ_HZ;
    cfg.pin_cs = p.cs;
    cfg.bus_shared = !touch.separateBus;
    cfg.pin_sclk = touch.separateBus ? p.sclk : bus.pin_sclk;
    cfg.pin_mosi = touch.separateBus ? p.mosi : bus.pin_mosi;
    cfg.pin_miso = touch.separateBus ? p.miso : bus.pin_miso;
  } else {
    _touchI2c = true;
    cfg.i2c_port = 0;
    cfg.i2c_addr = touch.i2cAddr;
    cfg.pin_sda = p.sda;
    cfg.pin_scl = p.scl;
    cfg.pin_rst = p.rst;
    cfg.pin_cs = -1;
    cfg.freq = 400000;
    cfg.bus_shared = false;
  }
  _touch->config(cfg);
}

LcdDevice::~LcdDevice() {
  if (_touchI2c) lgfx::i2c::release(0);
  _bus.release();
}

bool LcdDevice::beginTouch() {
  if (!_touch) return false;
  // Dotyk podlaczamy po starcie wyswietlacza (tak jak robi to LGFX_Device::init)
  _panel->setTouch(_touch);
  return _panel->initTouch();
}

bool LcdDevice::begin() {
  // Podswietlenie (pin LED) obsluguje osobno klasa Backlight
  return init();
}
