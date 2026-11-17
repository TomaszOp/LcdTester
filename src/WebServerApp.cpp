/**
 * @file WebServerApp.cpp
 * @brief Implementacja klasy WebServerApp - obsluga adresow strony i API JSON.
 */
#include "WebServerApp.h"

#include "BoardPins.h"
#include "LcdModels.h"
#include "TouchModels.h"
#include "web/IndexHtml.h"

/**
 * @brief Przygotowuje tekst do wstawienia w JSON (cudzyslow, backslash, nowa linia).
 * @param s Tekst zrodlowy.
 * @return Tekst z sekwencjami ucieczki.
 */
static String jsonEscape(const char* s) {
  String out;
  for (; *s; s++) {
    if (*s == '\n') {
      out += "\\n";
      continue;
    }
    if (*s == '"' || *s == '\\') out += '\\';
    out += *s;
  }
  return out;
}

WebServerApp::WebServerApp(TestRunner& runner, uint16_t port) : _server(port), _runner(runner) {}

void WebServerApp::begin() {
  _server.on("/", HTTP_GET, [this] { handleIndex(); });
  _server.on("/api/config", HTTP_GET, [this] { handleConfig(); });
  _server.on("/api/start", HTTP_POST, [this] { handleStart(TestRunner::Mode::FullTest); });
  _server.on("/api/pattern", HTTP_POST, [this] { handleStart(TestRunner::Mode::Pattern); });
  _server.on("/api/readid", HTTP_POST, [this] { handleStart(TestRunner::Mode::ReadId); });
  _server.on("/api/touch", HTTP_POST, [this] { handleStart(TestRunner::Mode::TouchTest); });
  _server.on("/api/stop", HTTP_POST, [this] { handleStop(); });
  _server.on("/api/status", HTTP_GET, [this] { handleStatus(); });
  _server.on("/api/brightness", HTTP_POST, [this] { handleBrightness(); });
  _server.onNotFound([this] { handleNotFound(); });
  _server.begin();
  Serial.println("[WWW] Serwer uruchomiony");
}

void WebServerApp::loop() {
  _server.handleClient();
}

void WebServerApp::handleIndex() {
  _server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void WebServerApp::handleConfig() {
  const LcdPins& p = DEFAULT_LCD_PINS;
  String json;
  json.reserve(1536);
  json += "{\"board\":\"" BOARD_NAME "\",";
  json += "\"pins\":{\"sclk\":" + String(p.sclk) + ",\"mosi\":" + String(p.mosi) +
          ",\"miso\":" + String(p.miso) + ",\"cs\":" + String(p.cs) + ",\"dc\":" + String(p.dc) +
          ",\"rst\":" + String(p.rst) + ",\"led\":" + String(p.led) + "},";
  json += "\"phaseSeconds\":" + String(TestRunner::PHASE_DURATION_MS / 1000) + ",";

  const TouchPins& t = DEFAULT_TOUCH_PINS;
  json += "\"touchPins\":{\"tcs\":" + String(t.cs) + ",\"tirq\":" + String(t.irq) +
          ",\"tsclk\":" + String(t.sclk) + ",\"tmosi\":" + String(t.mosi) + ",\"tmiso\":" + String(t.miso) +
          ",\"tsda\":" + String(t.sda) + ",\"tscl\":" + String(t.scl) + ",\"trst\":" + String(t.rst) + "},";
  json += "\"touchModels\":[";
  for (uint8_t i = 0; i < TOUCH_MODEL_COUNT; i++) {
    const TouchModel& m = TOUCH_MODELS[i];
    if (i) json += ',';
    json += "{\"id\":\"" + jsonEscape(m.id) + "\",\"name\":\"" + jsonEscape(m.name) +
            "\",\"bus\":\"" + (m.bus == TouchBus::I2C ? "i2c" : "spi") + "\",\"addr\":" + String(m.i2cAddr) +
            ",\"separate\":" + (m.separateBusSupported ? "true" : "false") + "}";
  }
  json += "],";

  json += "\"frequencies\":[";
  for (uint8_t i = 0; i < SPI_FREQUENCY_COUNT; i++) {
    if (i) json += ',';
    json += String(SPI_FREQUENCIES[i]);
  }
  json += "],\"models\":[";
  for (uint8_t m = 0; m < LCD_MODEL_COUNT; m++) {
    const LcdModel& model = LCD_MODELS[m];
    if (m) json += ',';
    json += "{\"id\":\"" + jsonEscape(model.id) + "\",\"name\":\"" + jsonEscape(model.name) +
            "\",\"invert\":" + (model.invertDefault ? "true" : "false") +
            ",\"swapRb\":" + (model.swapRbDefault ? "true" : "false") +
            ",\"defaultFreq\":" + String(model.defaultFreqHz) +
            ",\"maxFreq\":" + String(model.maxFreqHz) + ",\"resolutions\":[";
    for (uint8_t r = 0; r < model.resolutionCount; r++) {
      const LcdResolution& res = model.resolutions[r];
      if (r) json += ',';
      json += "{\"w\":" + String(res.width) + ",\"h\":" + String(res.height) +
              ",\"note\":\"" + jsonEscape(res.note) + "\"}";
    }
    json += "]}";
  }
  json += "]}";
  sendJson(200, json);
}

void WebServerApp::handleStart(TestRunner::Mode mode) {
  LcdTestConfig config;
  config.model = findLcdModel(_server.arg("model"));
  if (!config.model) {
    sendError(400, "Nieznany model");
    return;
  }
  config.resolutionIndex = _server.arg("res").toInt();
  config.spiFreqHz = _server.hasArg("freq") ? _server.arg("freq").toInt() : config.model->defaultFreqHz;
  config.spiMode = constrain(_server.arg("mode").toInt(), 0, 3);
  config.rotation = constrain(_server.arg("rot").toInt(), 0, 7);
  config.invert = _server.hasArg("invert") ? _server.arg("invert") == "1" : config.model->invertDefault;
  config.spi3wire = _server.arg("spi3wire") == "1";
  config.packetDelayMs = constrain(_server.arg("delay").toInt(), 0, 100);  // puste = 0
  config.fpsLimit = constrain(_server.arg("fps").toInt(), 0, 60);          // puste = 0
  config.swapRB = _server.hasArg("swaprb") ? _server.arg("swaprb") == "1" : config.model->swapRbDefault;

  const LcdPins& d = DEFAULT_LCD_PINS;
  config.pins.sclk = argPin("sclk", d.sclk);
  config.pins.mosi = argPin("mosi", d.mosi);
  config.pins.miso = argPin("miso", d.miso);
  config.pins.cs = argPin("cs", d.cs);
  config.pins.dc = argPin("dc", d.dc);
  config.pins.rst = argPin("rst", d.rst);
  config.pins.led = argPin("led", d.led);

  // Dotyk - uzywany tylko w tescie dotyku; wyswietlacz bierze ustawienia z powyzszych pol
  config.touch.model = findTouchModel(_server.arg("touch"));
  if (config.touch.model) {
    const TouchPins& td = DEFAULT_TOUCH_PINS;
    config.touch.separateBus = _server.arg("tsep") == "1";
    config.touch.i2cAddr = _server.hasArg("taddr") && !_server.arg("taddr").isEmpty()
                               ? strtol(_server.arg("taddr").c_str(), nullptr, 0)
                               : config.touch.model->i2cAddr;
    config.touch.pins.cs = argPin("tcs", td.cs);
    config.touch.pins.irq = argPin("tirq", td.irq);
    config.touch.pins.sclk = argPin("tsclk", td.sclk);
    config.touch.pins.mosi = argPin("tmosi", td.mosi);
    config.touch.pins.miso = argPin("tmiso", td.miso);
    config.touch.pins.sda = argPin("tsda", td.sda);
    config.touch.pins.scl = argPin("tscl", td.scl);
    config.touch.pins.rst = argPin("trst", td.rst);
  }

  if (_server.hasArg("brightness")) {
    _runner.setBrightness(constrain(_server.arg("brightness").toInt(), 0, 100));
  }

  String error;
  if (!_runner.start(config, mode, error)) {
    sendError(_runner.isRunning() ? 409 : 400, error);
    return;
  }
  sendJson(200, "{\"ok\":true}");
}

void WebServerApp::handleStop() {
  _runner.stop();
  sendJson(200, "{\"ok\":true}");
}

void WebServerApp::handleBrightness() {
  if (!_server.hasArg("value")) {
    sendError(400, "Brak parametru value (0-100)");
    return;
  }
  _runner.setBrightness(constrain(_server.arg("value").toInt(), 0, 100));
  sendJson(200, "{\"ok\":true,\"brightness\":" + String(_runner.brightness()) + "}");
}

void WebServerApp::handleStatus() {
  TestRunner::Status s = _runner.status();
  String json;
  json.reserve(2200);
  json += "{\"state\":\"" + String(TestRunner::stateName(s.state)) + "\",";
  json += "\"phase\":" + String(s.phase) + ",";
  json += "\"phaseName\":\"" + String(TestRunner::phaseName(s.phase)) + "\",";
  json += "\"phaseElapsedMs\":" + String(s.phaseElapsedMs) + ",";
  json += "\"message\":\"" + jsonEscape(s.message) + "\",";
  json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"brightness\":" + String(_runner.brightness()) + ",";
  json += "\"idInfo\":\"" + jsonEscape(s.idInfo) + "\",";
  appendTouchJson(json);
  json += "\"results\":[";
  for (uint8_t i = 0; i < TestRunner::PHASE_COUNT; i++) {
    if (i) json += ',';
    json += "{\"frames\":" + String(s.results[i].frames) + ",\"fps\":" + String(s.results[i].fps, 1) + "}";
  }
  json += "]}";
  sendJson(200, json);
}

void WebServerApp::handleNotFound() {
  sendError(404, "Nie znaleziono");
}

void WebServerApp::appendTouchJson(String& json) {
  const TouchTest::Result t = _runner.touchResult();
  if (!t.active) return;
  // Dla kazdego punktu: cel, dotkniecie z kalibracji (c*) i z weryfikacji (v*)
  json += "\"touch\":{\"points\":[";
  for (uint8_t i = 0; i < TouchTest::POINT_COUNT; i++) {
    const TouchTest::TestPoint& p = t.points[i];
    if (i) json += ',';
    json += "{\"tx\":" + String(p.targetX) + ",\"ty\":" + String(p.targetY) +
            ",\"cx\":" + String(p.calib.x) + ",\"cy\":" + String(p.calib.y) +
            ",\"cerr\":" + String(p.calib.error, 1) + ",\"cdone\":" + (p.calib.done ? "true" : "false") +
            ",\"vx\":" + String(p.verify.x) + ",\"vy\":" + String(p.verify.y) +
            ",\"verr\":" + String(p.verify.error, 1) + ",\"vdone\":" + (p.verify.done ? "true" : "false") +
            ",\"ok\":" + (p.ok ? "true" : "false") + "}";
  }
  json += "],\"corners\":" + String(TouchTest::CORNER_COUNT) +
          ",\"calibrated\":" + (t.calibration.valid ? "true" : "false") +
          ",\"verified\":" + (t.verified ? "true" : "false") +
          ",\"maxError\":" + String(t.maxError, 1) + ",\"threshold\":" + String(t.threshold, 0) +
          ",\"ok\":" + (t.ok ? "true" : "false") + ",\"calibration\":\"" + jsonEscape(t.calibrationText) +
          "\"},";
}

int8_t WebServerApp::argPin(const char* name, int8_t fallback) {
  if (!_server.hasArg(name) || _server.arg(name).isEmpty()) return fallback;
  return _server.arg(name).toInt();
}

void WebServerApp::sendJson(int code, const String& json) {
  _server.send(code, "application/json", json);
}

void WebServerApp::sendError(int code, const String& message) {
  sendJson(code, "{\"ok\":false,\"error\":\"" + jsonEscape(message.c_str()) + "\"}");
}
