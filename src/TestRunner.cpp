/**
 * @file TestRunner.cpp
 * @brief Implementacja klasy TestRunner - walidacja parametrow i zadanie testu w tle.
 */
#include "TestRunner.h"

#include "LcdIdReader.h"

/**
 * @brief Sprawdza, czy pin jest polaczony z pamiecia flash.
 *
 * Uzycie takiego pinu zawiesza lub restartuje ESP.
 * @return true dla pinow flash biezacej wersji ESP32.
 */
static bool isFlashPin(int8_t pin) {
#if defined(CONFIG_IDF_TARGET_ESP32C3)
  return pin >= 11 && pin <= 17;
#elif defined(CONFIG_IDF_TARGET_ESP32)
  return pin >= 6 && pin <= 11;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
  return pin >= 26 && pin <= 32;
#else
  return false;
#endif
}

/**
 * @brief Sprawdza, czy pin moze byc wyjsciem dla wyswietlacza.
 * @return true dla -1 (nieuzywany) albo poprawnego GPIO wyjsciowego spoza flash.
 */
static bool validPin(int8_t pin) {
  return pin == -1 || (pin >= 0 && GPIO_IS_VALID_OUTPUT_GPIO(pin) && !isFlashPin(pin));
}

bool TestRunner::start(const LcdTestConfig& config, Mode mode, String& error) {
  if (isRunning()) {
    error = "Test juz trwa";
    return false;
  }
  if (!config.model || config.resolutionIndex >= config.model->resolutionCount) {
    error = "Nieznany model lub rozdzielczosc";
    return false;
  }
  const LcdPins& p = config.pins;
  if (p.sclk < 0 || p.mosi < 0 || p.dc < 0) {
    error = "Piny SCLK, MOSI i DC sa wymagane";
    return false;
  }
  if (!validPin(p.sclk) || !validPin(p.mosi) || !validPin(p.cs) || !validPin(p.dc) ||
      !validPin(p.rst) || !validPin(p.led) || (p.miso >= 0 && (!GPIO_IS_VALID_GPIO(p.miso) || isFlashPin(p.miso)))) {
    error = "Nieprawidlowy numer GPIO (lub pin pamieci flash)";
    return false;
  }
  if (mode == Mode::ReadId && config.pins.miso < 0 && !config.spi3wire) {
    error = "Odczyt ID wymaga pinu MISO albo trybu SPI 3-wire";
    return false;
  }
  if (config.spiFreqHz < 100000 || config.spiFreqHz > 80000000) {
    error = "Czestotliwosc SPI poza zakresem 0.1-80 MHz";
    return false;
  }

  _config = config;
  _mode = mode;
  _backlight.attach(config.pins.led);
  _stopRequested = false;
  _phase = 0;
  memset(_results, 0, sizeof(_results));
  _state = State::Running;
  setMessage("Start wyswietlacza");

  if (xTaskCreate(taskEntry, "lcdTest", 8192, this, 1, &_task) != pdPASS) {
    _state = State::Error;
    error = "Nie udalo sie utworzyc zadania testu";
    return false;
  }
  return true;
}

void TestRunner::stop() {
  if (isRunning()) _stopRequested = true;
}

TestRunner::Status TestRunner::status() const {
  Status s;
  s.state = _state;
  s.phase = _phase;
  s.phaseElapsedMs = isRunning() && _phase > 0 ? millis() - _phaseStartMs : 0;
  memcpy(s.results, _results, sizeof(_results));
  strlcpy(s.message, _message, sizeof(s.message));
  strlcpy(s.idInfo, _idInfo, sizeof(s.idInfo));
  return s;
}

void TestRunner::taskEntry(void* arg) {
  static_cast<TestRunner*>(arg)->run();
  vTaskDelete(nullptr);
}

void TestRunner::run() {
  const LcdResolution& res = _config.resolution();
  Serial.printf("[TEST] %s %ux%u, SPI %.1f MHz, rot %u\n", _config.model->name,
                res.width, res.height, _config.spiFreqHz / 1e6f, _config.rotation);

  State finalState;
  switch (_mode) {
    case Mode::Pattern: finalState = runPattern(); break;
    case Mode::ReadId:  finalState = runReadId();  break;
    default:            finalState = runPhases();  break;
  }
  // Najpierw zwalniamy uchwyt zadania, potem zmieniamy stan - wtedy mozna od razu
  // uruchomic kolejny test
  _task = nullptr;
  _state = finalState;
}

TestRunner::State TestRunner::runPattern() {
  LcdDevice lcd(_config);
  if (!lcd.begin()) {
    setMessage("Blad inicjalizacji wyswietlacza");
    return State::Error;
  }
  lcd.setRotation(_config.rotation);
  setMessage("Rysowanie obrazu testowego");
  const uint32_t start = millis();
  uint8_t depth = LcdTests::testPattern(lcd, _config);
  const uint32_t elapsed = millis() - start;
  String msg = "Obraz testowy wyswietlony (" + String(elapsed) + " ms, ";
  msg += depth ? "bufor " + String(depth) + " bit, jedno przeslanie)" : "rysowany bezposrednio - brak RAM na bufor)";
  setMessage(msg);
  Serial.printf("[TEST] %s\n", _message);
  return State::Done;
}

TestRunner::State TestRunner::runReadId() {
  // Bez LovyanGFX - surowy odczyt rejestrow sposobem wlasciwym dla wybranego modelu
  setMessage("Odczyt ID");
  LcdIdReader reader(_config.pins, _config.spi3wire);
  LcdIdReader::Result id = reader.read(*_config.model);
  strlcpy(_idInfo, id.details, sizeof(_idInfo));
  setMessage(id.summary);
  Serial.printf("[ID] %s\n%s", id.summary, id.details);
  // Niezgodnosc i brak odpowiedzi pokazujemy na stronie na czerwono
  return id.verdict == LcdIdReader::Verdict::Match ? State::Done : State::Error;
}

TestRunner::State TestRunner::runPhases() {
  // Wyswietlacz zostaje z ekranem koncowym; destruktor LcdDevice zwalnia tylko szyne SPI
  LcdDevice lcd(_config);
  if (!lcd.begin()) {
    setMessage("Blad inicjalizacji wyswietlacza");
    return State::Error;
  }
  lcd.setRotation(_config.rotation);
  LcdTests::drawIntro(lcd, _config);

  for (uint8_t i = 0; i < PHASE_COUNT && !_stopRequested; i++) {
    _phase = i + 1;
    _phaseStartMs = millis();
    setMessage(String("Faza ") + _phase + ": " + phaseName(_phase));
    switch (i) {
      case 0: _results[i] = LcdTests::shapes(lcd, PHASE_DURATION_MS, _config.packetDelayMs, _stopRequested); break;
      case 1: _results[i] = LcdTests::text(lcd, PHASE_DURATION_MS, _config.packetDelayMs, _stopRequested); break;
      case 2: _results[i] = LcdTests::animation(lcd, PHASE_DURATION_MS, _config.fpsLimit, _stopRequested); break;
    }
    Serial.printf("[TEST] Faza %u (%s): %lu klatek, %.1f/s\n", _phase, phaseName(_phase),
                  (unsigned long)_results[i].frames, _results[i].fps);
  }

  if (_stopRequested) {
    lcd.fillScreen(TFT_BLACK);
    setMessage("Test przerwany");
    return State::Stopped;
  }
  LcdTests::drawSummary(lcd, _results);
  setMessage("Test zakonczony");
  return State::Done;
}

void TestRunner::setMessage(const String& msg) {
  strlcpy(_message, msg.c_str(), sizeof(_message));
}

const char* TestRunner::stateName(State s) {
  switch (s) {
    case State::Idle:    return "idle";
    case State::Running: return "running";
    case State::Done:    return "done";
    case State::Stopped: return "stopped";
    case State::Error:   return "error";
  }
  return "?";
}

const char* TestRunner::phaseName(uint8_t phase) {
  switch (phase) {
    case 1: return "Ksztalty";
    case 2: return "Napisy";
    case 3: return "Animacje";
  }
  return "Start";
}
