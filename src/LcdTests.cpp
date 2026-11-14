/**
 * @file LcdTests.cpp
 * @brief Implementacja faz testu, obrazu testowego i ekranow pomocniczych.
 */
#include "LcdTests.h"

namespace LcdTests {

/**
 * @brief Tworzy wynik fazy z liczby operacji i czasu startu.
 * @param frames Liczba klatek lub operacji.
 * @param startMs millis() na poczatku fazy.
 */
static PhaseResult makeResult(uint32_t frames, uint32_t startMs) {
  PhaseResult r;
  r.frames = frames;
  uint32_t elapsed = millis() - startMs;
  r.fps = elapsed ? frames * 1000.0f / elapsed : 0;
  return r;
}

/**
 * @brief Odstep po paczce danych (fazy 1-2).
 *
 * Bez odstepu tylko co jakis czas oddaje procesor innym zadaniom (WiFi, WWW).
 * @param delayMs Odstep w ms, 0 = brak.
 * @param ops Liczba wykonanych operacji.
 * @param yieldEvery Co ile operacji oddac procesor, gdy delayMs == 0.
 */
static void packetPause(uint16_t delayMs, uint32_t ops, uint32_t yieldEvery) {
  if (delayMs) {
    delay(delayMs);
  } else if (ops % yieldEvery == 0) {
    delay(1);
  }
}

/**
 * @brief Limit FPS (faza 3): czeka do konca okresu klatki.
 * @param fpsLimit Maks. klatek na sekunde, 0 = bez limitu (tylko krotka pauza).
 * @param frameStartMs millis() na poczatku klatki.
 * @param stopRequested Przerywa czekanie, gdy test zostaje zatrzymany.
 */
static void frameLimit(uint8_t fpsLimit, uint32_t frameStartMs, const volatile bool& stopRequested) {
  if (!fpsLimit) {
    delay(1);
    return;
  }
  const uint32_t period = 1000 / fpsLimit;
  uint32_t elapsed;
  while (!stopRequested && (elapsed = millis() - frameStartMs) < period) {
    delay(min<uint32_t>(10, period - elapsed));
  }
}

/// @return Losowy kolor RGB565.
static uint16_t randomColor(LcdDevice& lcd) {
  return lcd.color565(random(256), random(256), random(256));
}

// ---------------------------------------------------------------------------
// Ekran startowy
// ---------------------------------------------------------------------------
void drawIntro(LcdDevice& lcd, const LcdTestConfig& config) {
  const LcdResolution& res = config.resolution();
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextDatum(textdatum_t::top_left);
  lcd.setFont(&fonts::Font2);
  int y = 4;
  lcd.drawString("LcdTester", 4, y);  y += 20;
  lcd.drawString(config.model->name, 4, y);  y += 18;
  lcd.drawString(String(res.width) + "x" + String(res.height) + " rot " + String(config.rotation), 4, y);  y += 18;
  lcd.drawString("SPI " + String(config.spiFreqHz / 1000000.0f, 1) + " MHz", 4, y);
  // Ramka i narozniki pokazuja, czy obszar i przesuniecie sa poprawne
  lcd.drawRect(0, 0, lcd.width(), lcd.height(), TFT_RED);
  lcd.fillRect(0, 0, 6, 6, TFT_GREEN);
  lcd.fillRect(lcd.width() - 6, lcd.height() - 6, 6, 6, TFT_BLUE);
  delay(1500);
}

// ---------------------------------------------------------------------------
// Faza 1: proste ksztalty
// ---------------------------------------------------------------------------
PhaseResult shapes(LcdDevice& lcd, uint32_t durationMs, uint16_t packetDelayMs,
                   const volatile bool& stopRequested) {
  const uint32_t start = millis();
  const int w = lcd.width();
  const int h = lcd.height();
  uint32_t ops = 0;

  // Pelne kolory: jesli czerwony jest niebieski -> zaznacz "Zamien czerwony i niebieski",
  // jesli czarny jest bialy -> zla inwersja
  static const uint16_t solid[] = { TFT_RED, TFT_GREEN, TFT_BLUE, TFT_WHITE, TFT_BLACK };
  static const char* solidNames[] = { "RED", "GREEN", "BLUE", "WHITE", "BLACK" };
  lcd.setFont(&fonts::Font4);
  lcd.setTextDatum(textdatum_t::middle_center);
  for (uint8_t i = 0; i < 5 && !stopRequested; i++) {
    lcd.fillScreen(solid[i]);
    lcd.setTextColor(solid[i] == TFT_WHITE ? TFT_BLACK : TFT_WHITE);
    lcd.drawString(solidNames[i], w / 2, h / 2);
    ops++;
    delay(400);
  }

  // Pasy kolorow (gradienty)
  if (!stopRequested) {
    for (int y = 0; y < h; y++) {
      uint8_t v = y * 255 / (h - 1);
      int band = w / 4;
      lcd.drawFastHLine(0, y, band, lcd.color565(v, 0, 0));
      lcd.drawFastHLine(band, y, band, lcd.color565(0, v, 0));
      lcd.drawFastHLine(band * 2, y, band, lcd.color565(0, 0, v));
      lcd.drawFastHLine(band * 3, y, w - band * 3, lcd.color565(v, v, v));
    }
    ops++;
    delay(800);
  }

  // Losowe kwadraty, kola i trojkaty
  lcd.fillScreen(TFT_BLACK);
  while (!stopRequested && millis() - start < durationMs) {
    int size = random(8, min(w, h) / 2);
    int x = random(-size / 2, w);
    int y = random(-size / 2, h);
    switch (ops % 4) {
      case 0: lcd.fillRect(x, y, size, size, randomColor(lcd)); break;
      case 1: lcd.drawRect(x, y, size, size, randomColor(lcd)); break;
      case 2: lcd.fillCircle(x, y, size / 2, randomColor(lcd)); break;
      case 3: lcd.fillTriangle(x, y, x + size, y, x + size / 2, y + size, randomColor(lcd)); break;
    }
    ops++;
    if (ops % 300 == 0) lcd.fillScreen(TFT_BLACK);
    packetPause(packetDelayMs, ops, 20);
  }
  return makeResult(ops, start);
}

// ---------------------------------------------------------------------------
// Faza 2: napisy
// ---------------------------------------------------------------------------
PhaseResult text(LcdDevice& lcd, uint32_t durationMs, uint16_t packetDelayMs,
                 const volatile bool& stopRequested) {
  const uint32_t start = millis();
  const int w = lcd.width();
  const int h = lcd.height();
  uint32_t ops = 0;

  static const lgfx::IFont* fontList[] = {
    &fonts::Font0, &fonts::Font2, &fonts::Font4,
    &fonts::FreeSans9pt7b, &fonts::FreeSansBold12pt7b, &fonts::FreeMonoBold9pt7b,
  };
  static const char* words[] = {
    "Hello", "LCD", "Test", "ESP32", "SPI", "0123456789", "ABCDEF", "LovyanGFX", "OK!",
  };
  const uint8_t fontCount = sizeof(fontList) / sizeof(fontList[0]);
  const uint8_t wordCount = sizeof(words) / sizeof(words[0]);

  lcd.fillScreen(TFT_BLACK);
  lcd.setTextDatum(textdatum_t::top_left);

  // Napisy w naroznikach i na srodku - sprawdzenie krawedzi ekranu
  lcd.setFont(&fonts::Font2);
  lcd.setTextColor(TFT_YELLOW);
  lcd.drawString("TL", 0, 0);
  lcd.setTextDatum(textdatum_t::top_right);
  lcd.drawString("TR", w, 0);
  lcd.setTextDatum(textdatum_t::bottom_left);
  lcd.drawString("BL", 0, h);
  lcd.setTextDatum(textdatum_t::bottom_right);
  lcd.drawString("BR", w, h);
  lcd.setTextDatum(textdatum_t::middle_center);
  lcd.setFont(&fonts::Font4);
  lcd.setTextColor(TFT_CYAN);
  lcd.drawString("CENTER", w / 2, h / 2);
  delay(1500);

  lcd.setTextDatum(textdatum_t::top_left);
  lcd.fillScreen(TFT_BLACK);
  while (!stopRequested && millis() - start < durationMs) {
    lcd.setFont(fontList[random(fontCount)]);
    lcd.setTextSize(random(10) == 0 ? 2 : 1);
    if (random(3) == 0) {
      lcd.setTextColor(randomColor(lcd), randomColor(lcd));  // z tlem
    } else {
      lcd.setTextColor(randomColor(lcd));                    // przezroczyste
    }
    lcd.drawString(words[random(wordCount)], random(-20, w), random(-10, h));
    ops++;
    if (ops % 150 == 0) lcd.fillScreen(TFT_BLACK);
    packetPause(packetDelayMs, ops, 10);
  }
  lcd.setTextSize(1);
  return makeResult(ops, start);
}

// ---------------------------------------------------------------------------
// Faza 3: animacje
// Caly ekran rysowany jest w pasach do bufora (sprite) i wysylany po SPI.
// Pelnoekranowy bufor 320x240x16bit (150 kB) nie zmiescilby sie w RAM C3 obok WiFi.
// ---------------------------------------------------------------------------
/// Kula w animacji odbijajacych sie kul.
struct Ball {
  float x;         ///< Polozenie srodka X.
  float y;         ///< Polozenie srodka Y.
  float vx;        ///< Predkosc X w pikselach na klatke.
  float vy;        ///< Predkosc Y w pikselach na klatke.
  int r;           ///< Promien.
  uint16_t color;  ///< Kolor RGB565.
};

/**
 * @brief Dwa bufory pasow ekranu uzywane na zmiane w animacjach.
 *
 * pushSprite() startuje transfer DMA i wraca od razu, a DMA dalej czyta bufor.
 * Rysowanie w nim przed koncem transferu daje artefakty (fragmenty linii
 * z poprzedniej klatki). Dlatego sa dwa bufory: w jednym rysujemy, drugi jest
 * w tym czasie wysylany. Gdy brakuje RAM na drugi, uzywany jest jeden z czekaniem.
 */
class BandBuffers {
 public:
  /// @param lcd Wyswietlacz, do ktorego beda wysylane pasy.
  explicit BandBuffers(LcdDevice& lcd) : _lcd(lcd), _sprites{ LGFX_Sprite(&lcd), LGFX_Sprite(&lcd) } {}
  /// Czeka na koniec transferu i zwalnia bufory.
  ~BandBuffers() {
    _lcd.waitDMA();
    for (auto& s : _sprites) s.deleteSprite();
  }

  /**
   * @brief Tworzy bufory o szerokosci ekranu; wysokosc pasa 40, 20, 10 linii - ile sie zmiesci.
   * @param w Szerokosc ekranu w pikselach.
   * @return false, gdy brak RAM nawet na jeden bufor.
   */
  bool create(int w) {
    for (_height = 40; _height >= 8; _height /= 2) {
      _sprites[0].setColorDepth(16);
      if (!_sprites[0].createSprite(w, _height)) continue;
      _sprites[1].setColorDepth(16);
      _count = _sprites[1].createSprite(w, _height) ? 2 : 1;
      return true;
    }
    return false;
  }

  /// @return Wysokosc pasa w liniach.
  int height() const { return _height; }

  /// @return Bufor gotowy do rysowania (zaden transfer DMA go juz nie czyta).
  LGFX_Sprite& acquire() {
    if (_count == 1) _lcd.waitDMA();
    return _sprites[_next];
  }

  /**
   * @brief Wysyla bufor z acquire(); czeka tylko na koniec poprzedniego transferu.
   * @param y Wspolrzedna Y gornej krawedzi pasa na ekranie.
   */
  void push(int y) {
    _lcd.waitDMA();
    _sprites[_next].pushSprite(0, y);
    _next = (_next + 1) % _count;
  }

 private:
  LcdDevice& _lcd;          ///< Wyswietlacz docelowy.
  LGFX_Sprite _sprites[2];  ///< Bufory pasow.
  uint8_t _count = 0;       ///< Liczba utworzonych buforow (1 lub 2).
  uint8_t _next = 0;        ///< Indeks bufora do nastepnego uzycia.
  int _height = 0;          ///< Wysokosc pasa w liniach.
};

/**
 * @brief Animacja odbijajacych sie kul na siatce, z licznikiem FPS na ekranie.
 * @param lcd Wyswietlacz.
 * @param bands Bufory pasow.
 * @param fpsLimit Limit klatek na sekunde, 0 = brak.
 * @param untilMs millis(), do ktorego trwa animacja.
 * @param stopRequested Flaga przerwania.
 * @return Liczba pelnych klatek.
 */
static uint32_t animateBalls(LcdDevice& lcd, BandBuffers& bands, uint8_t fpsLimit,
                             uint32_t untilMs, const volatile bool& stopRequested) {
  const int w = lcd.width();
  const int h = lcd.height();
  const int ballCount = 14;
  Ball balls[ballCount];
  for (auto& b : balls) {
    b.r = random(6, max(8, min(w, h) / 8));
    b.x = random(b.r, w - b.r);
    b.y = random(b.r, h - b.r);
    b.vx = random(-60, 61) / 10.0f;
    b.vy = random(-60, 61) / 10.0f;
    b.color = lcd.color565(random(64, 256), random(64, 256), random(64, 256));
  }

  const int bandH = bands.height();
  uint32_t frames = 0;
  const uint32_t start = millis();
  while (!stopRequested && millis() < untilMs) {
    const uint32_t frameStart = millis();
    for (auto& b : balls) {
      b.x += b.vx;
      b.y += b.vy;
      if (b.x < b.r || b.x > w - b.r) b.vx = -b.vx;
      if (b.y < b.r || b.y > h - b.r) b.vy = -b.vy;
    }
    lcd.startWrite();
    for (int y0 = 0; y0 < h; y0 += bandH) {
      LGFX_Sprite& band = bands.acquire();
      band.fillScreen(TFT_NAVY);
      for (int gx = 0; gx < w; gx += 20) band.drawFastVLine(gx, 0, bandH, TFT_DARKGREY);
      for (auto& b : balls) {
        if (b.y + b.r >= y0 && b.y - b.r < y0 + bandH) {
          band.fillCircle(b.x, b.y - y0, b.r, b.color);
        }
      }
      if (y0 == 0) {
        uint32_t el = millis() - start;
        band.setFont(&fonts::Font2);
        band.setTextColor(TFT_WHITE);
        band.setCursor(2, 2);
        band.printf("FPS %.1f", el ? frames * 1000.0f / el : 0.0f);
      }
      bands.push(y0);
    }
    lcd.endWrite();
    frames++;
    frameLimit(fpsLimit, frameStart, stopRequested);
  }
  return frames;
}

/**
 * @brief Animacja "plazmy" - kazdy piksel liczony osobno, obciaza CPU i SPI.
 * @param lcd Wyswietlacz.
 * @param bands Bufory pasow.
 * @param fpsLimit Limit klatek na sekunde, 0 = brak.
 * @param untilMs millis(), do ktorego trwa animacja.
 * @param stopRequested Flaga przerwania.
 * @return Liczba pelnych klatek.
 */
static uint32_t animatePlasma(LcdDevice& lcd, BandBuffers& bands, uint8_t fpsLimit,
                              uint32_t untilMs, const volatile bool& stopRequested) {
  const int w = lcd.width();
  const int h = lcd.height();

  int8_t sinTab[256];
  for (int i = 0; i < 256; i++) sinTab[i] = (int8_t)(sinf(i * 2 * PI / 256) * 63);
  uint16_t palette[256];
  for (int i = 0; i < 256; i++) {
    uint8_t r = 128 + 127 * sinf(i * 2 * PI / 256);
    uint8_t g = 128 + 127 * sinf(i * 2 * PI / 256 + 2.094f);
    uint8_t b = 128 + 127 * sinf(i * 2 * PI / 256 + 4.189f);
    palette[i] = lgfx::swap565(r, g, b);  // bufor sprite'a trzyma kolory z zamienionymi bajtami
  }

  const int bandH = bands.height();
  uint32_t frames = 0;
  uint8_t t = 0;
  while (!stopRequested && millis() < untilMs) {
    const uint32_t frameStart = millis();
    lcd.startWrite();
    for (int y0 = 0; y0 < h; y0 += bandH) {
      uint16_t* buf = (uint16_t*)bands.acquire().getBuffer();
      int lines = min(bandH, h - y0);
      for (int dy = 0; dy < lines; dy++) {
        int y = y0 + dy;
        int8_t sy = sinTab[(uint8_t)(y * 2 + t)];
        for (int x = 0; x < w; x++) {
          int v = sinTab[(uint8_t)(x * 2 - t)] + sy + sinTab[(uint8_t)(x + y + t * 2)];
          buf[dy * w + x] = palette[(uint8_t)(v + t)];
        }
      }
      bands.push(y0);
    }
    lcd.endWrite();
    frames++;
    t += 3;
    frameLimit(fpsLimit, frameStart, stopRequested);
  }
  return frames;
}

PhaseResult animation(LcdDevice& lcd, uint32_t durationMs, uint8_t fpsLimit,
                      const volatile bool& stopRequested) {
  const uint32_t start = millis();
  BandBuffers bands(lcd);
  if (!bands.create(lcd.width())) {
    lcd.fillScreen(TFT_RED);
    return makeResult(0, start);
  }

  uint32_t frames = 0;
  frames += animateBalls(lcd, bands, fpsLimit, start + durationMs / 2, stopRequested);
  frames += animatePlasma(lcd, bands, fpsLimit, start + durationMs, stopRequested);
  return makeResult(frames, start);
}

// ---------------------------------------------------------------------------
// Obraz testowy
// ---------------------------------------------------------------------------
/**
 * @brief Rysuje obraz testowy na dowolnej powierzchni LovyanGFX (ekran albo bufor).
 * @param g Powierzchnia do rysowania.
 * @param config Parametry testu (podpis na obrazie).
 */
static void drawPattern(lgfx::LGFXBase& g, const LcdTestConfig& config) {
  const int w = g.width();
  const int h = g.height();
  g.fillScreen(TFT_BLACK);

  // Paski kolorow (gorna polowa)
  static const uint16_t bars[] = {
    TFT_WHITE, TFT_YELLOW, TFT_CYAN, TFT_GREEN, TFT_MAGENTA, TFT_RED, TFT_BLUE, TFT_BLACK,
  };
  const int barsH = h / 2;
  for (int i = 0; i < 8; i++) {
    int x0 = w * i / 8;
    g.fillRect(x0, 0, w * (i + 1) / 8 - x0, barsH, bars[i]);
  }

  // Gradienty R, G, B, szary
  const int gradH = max(6, h / 16);
  for (int x = 0; x < w; x++) {
    uint8_t v = x * 255 / (w - 1);
    g.drawFastVLine(x, barsH, gradH, g.color565(v, 0, 0));
    g.drawFastVLine(x, barsH + gradH, gradH, g.color565(0, v, 0));
    g.drawFastVLine(x, barsH + gradH * 2, gradH, g.color565(0, 0, v));
    g.drawFastVLine(x, barsH + gradH * 3, gradH, g.color565(v, v, v));
  }

  // Siatka co 20 px w dolnej czesci
  const int gridTop = barsH + gradH * 4;
  for (int x = 0; x < w; x += 20) g.drawFastVLine(x, gridTop, h - gridTop, TFT_DARKGREY);
  for (int y = gridTop; y < h; y += 20) g.drawFastHLine(0, y, w, TFT_DARKGREY);

  // Kolo i krzyz na srodku - kolo musi byc okragle (kontrola proporcji)
  const int cx = w / 2;
  const int cy = h / 2;
  const int r = min(w, h) / 4;
  g.drawCircle(cx, cy, r, TFT_WHITE);
  g.drawFastHLine(cx - r, cy, r * 2 + 1, TFT_WHITE);
  g.drawFastVLine(cx, cy - r, r * 2 + 1, TFT_WHITE);

  // Ramka 1 px po samej krawedzi i znaczniki naroznikow
  g.drawRect(0, 0, w, h, TFT_WHITE);
  const int m = 10;
  g.fillRect(1, 1, m, m, TFT_RED);
  g.fillRect(w - m - 1, 1, m, m, TFT_GREEN);
  g.fillRect(1, h - m - 1, m, m, TFT_BLUE);
  g.fillRect(w - m - 1, h - m - 1, m, m, TFT_WHITE);

  // Opis
  const LcdResolution& res = config.resolution();
  g.setFont(&fonts::Font2);
  g.setTextDatum(textdatum_t::bottom_center);
  g.setTextColor(TFT_WHITE, TFT_BLACK);
  g.drawString(String(config.model->name) + " " + res.width + "x" + res.height, cx, h - 22);
  g.drawString("SPI " + String(config.spiFreqHz / 1e6f, 1) + " MHz  rot " + config.rotation, cx, h - 4);
}

uint8_t testPattern(LcdDevice& lcd, const LcdTestConfig& config) {
  LGFX_Sprite frame(&lcd);
  frame.setPsram(false);
  // 16 bit = 150 kB dla 240x320; gdy brak RAM, 8 bit (75 kB) z mniejsza liczba kolorow
  for (uint8_t depth : { 16, 8 }) {
    frame.setColorDepth(depth);
    if (frame.createSprite(lcd.width(), lcd.height())) {
      drawPattern(frame, config);
      frame.pushSprite(0, 0);  // jedno przeslanie calego ekranu
      frame.deleteSprite();
      return depth;
    }
  }
  drawPattern(lcd, config);
  return 0;
}

// ---------------------------------------------------------------------------
// Ekran koncowy
// ---------------------------------------------------------------------------
void drawSummary(LcdDevice& lcd, const PhaseResult results[3]) {
  static const char* names[] = { "Ksztalty", "Napisy", "Animacje" };
  static const char* units[] = { "op/s", "nap/s", "FPS" };
  lcd.fillScreen(TFT_BLACK);
  lcd.setFont(&fonts::Font2);
  lcd.setTextDatum(textdatum_t::top_left);
  lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  lcd.drawString("Test zakonczony", 4, 4);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  for (int i = 0; i < 3; i++) {
    lcd.drawString(String(names[i]) + ": " + String(results[i].fps, 1) + " " + units[i], 4, 28 + i * 18);
  }
}

}  // namespace LcdTests
