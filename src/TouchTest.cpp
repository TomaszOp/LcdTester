/**
 * @file TouchTest.cpp
 * @brief Implementacja testu i kalibracji dotyku.
 */
#include "TouchTest.h"

#include <algorithm>

namespace TouchTest {

static constexpr uint8_t MAX_SAMPLES = 15;        ///< Probki zbierane podczas jednego dotkniecia.
static constexpr uint32_t PRESS_SAMPLE_MS = 200;  ///< Czas zbierania probek po dotknieciu.
static constexpr uint32_t RELEASE_MS = 120;       ///< Tyle ms bez dotyku = puszczenie.

/// @return true, gdy kontroler zglasza dotkniecie (surowe wspolrzedne w x, y).
static bool readRaw(LcdDevice& lcd, int16_t& x, int16_t& y) {
  lgfx::touch_point_t tp;
  if (lcd.getTouchRaw(&tp, 1) == 0) return false;
  x = tp.x;
  y = tp.y;
  return true;
}

/**
 * @brief Czeka na puszczenie ekranu (brak dotyku przez RELEASE_MS).
 * @return false przy przerwaniu testu albo po timeoutMs.
 */
static bool waitRelease(LcdDevice& lcd, uint32_t timeoutMs, const volatile bool& stop) {
  const uint32_t start = millis();
  uint32_t freeSince = millis();
  int16_t x, y;
  while (!stop && millis() - start < timeoutMs) {
    if (readRaw(lcd, x, y)) {
      freeSince = millis();
    } else if (millis() - freeSince >= RELEASE_MS) {
      return true;
    }
    delay(10);
  }
  return false;
}

/**
 * @brief Czeka na jedno dotkniecie i zwraca mediane surowych odczytow.
 * @param[out] rx Surowe X.
 * @param[out] ry Surowe Y.
 * @return false przy przerwaniu albo braku dotkniecia przez TAP_TIMEOUT_MS.
 */
static bool waitTap(LcdDevice& lcd, int16_t& rx, int16_t& ry, const volatile bool& stop) {
  if (!waitRelease(lcd, TAP_TIMEOUT_MS, stop)) return false;

  const uint32_t start = millis();
  int16_t x, y;
  while (!stop && !readRaw(lcd, x, y)) {
    if (millis() - start > TAP_TIMEOUT_MS) return false;
    delay(10);
  }
  if (stop) return false;

  // Zbieramy probki przez chwile i bierzemy mediane - odrzuca skoki na poczatku nacisku
  int16_t xs[MAX_SAMPLES], ys[MAX_SAMPLES];
  uint8_t n = 0;
  const uint32_t pressStart = millis();
  while (n < MAX_SAMPLES && millis() - pressStart < PRESS_SAMPLE_MS) {
    if (readRaw(lcd, x, y)) {
      xs[n] = x;
      ys[n] = y;
      n++;
    }
    delay(10);
  }
  if (n == 0) return false;
  std::sort(xs, xs + n);
  std::sort(ys, ys + n);
  rx = xs[n / 2];
  ry = ys[n / 2];
  waitRelease(lcd, TAP_TIMEOUT_MS, stop);
  return true;
}

/// Przelicza surowy odczyt na piksele wg kalibracji.
static void applyCalibration(const Calibration& c, int16_t rx, int16_t ry, int16_t& x, int16_t& y) {
  x = lroundf(c.ax * rx + c.bx * ry + c.cx);
  y = lroundf(c.ay * rx + c.by * ry + c.cy);
}

/// Wyznacznik macierzy 3x3.
static double det3(const double m[3][3]) {
  return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
         m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
         m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

/**
 * @brief Rozwiazuje uklad 3x3 (wzory Cramera).
 * @return false, gdy uklad jest osobliwy (np. wszystkie odczyty takie same).
 */
static bool solve3(const double m[3][3], const double r[3], double out[3]) {
  const double d = det3(m);
  if (fabs(d) < 1e-9) return false;
  for (int col = 0; col < 3; col++) {
    double t[3][3];
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++) t[i][j] = (j == col) ? r[i] : m[i][j];
    out[col] = det3(t) / d;
  }
  return true;
}

/**
 * @brief Kalibracja metoda najmniejszych kwadratow: piksel = a*rx + b*ry + c.
 *
 * Przeksztalcenie afiniczne obejmuje skale, przesuniecie, odwrocenie osi
 * i zamiane X z Y, wiec dziala przy dowolnym obrocie ekranu i dotyku.
 */
static Calibration computeCalibration(const TestPoint* points, uint8_t n) {
  double m[3][3] = {};
  double rhsX[3] = {}, rhsY[3] = {};
  for (uint8_t i = 0; i < n; i++) {
    const double v[3] = { (double)points[i].calib.rawX, (double)points[i].calib.rawY, 1.0 };
    for (int a = 0; a < 3; a++) {
      for (int b = 0; b < 3; b++) m[a][b] += v[a] * v[b];
      rhsX[a] += v[a] * points[i].targetX;
      rhsY[a] += v[a] * points[i].targetY;
    }
  }
  Calibration c;
  double kx[3], ky[3];
  if (!solve3(m, rhsX, kx) || !solve3(m, rhsY, ky)) return c;
  c.ax = kx[0]; c.bx = kx[1]; c.cx = kx[2];
  c.ay = ky[0]; c.by = ky[1]; c.cy = ky[2];
  c.valid = true;
  return c;
}

/**
 * @brief Surowy odczyt odpowiadajacy pikselowi (odwrotnosc kalibracji).
 * @return false, gdy kalibracja jest nieodwracalna.
 */
static bool inverseCalibration(const Calibration& c, float x, float y, int32_t& rx, int32_t& ry) {
  const float d = c.ax * c.by - c.bx * c.ay;
  if (fabsf(d) < 1e-9f) return false;
  const float px = x - c.cx, py = y - c.cy;
  rx = lroundf((c.by * px - c.bx * py) / d);
  ry = lroundf((c.ax * py - c.ay * px) / d);
  return true;
}

/// Opis kalibracji do skopiowania: wspolczynniki, zamiana osi i parametry LovyanGFX.
static void describeCalibration(Result& r, int w, int h, uint8_t rotation) {
  const Calibration& c = r.calibration;
  const bool swapXY = fabsf(c.bx) > fabsf(c.ax);
  const float sx = swapXY ? c.bx : c.ax;
  const float sy = swapXY ? c.ay : c.by;
  size_t pos = snprintf(r.calibrationText, sizeof(r.calibrationText),
                        "x = %.5f*rx + %.5f*ry + %.1f\n"
                        "y = %.5f*rx + %.5f*ry + %.1f\n"
                        "Zamiana osi X/Y: %s, odwrocenie X: %s, Y: %s\n",
                        c.ax, c.bx, c.cx, c.ay, c.by, c.cy, swapXY ? "tak" : "nie",
                        sx < 0 ? "tak" : "nie", sy < 0 ? "tak" : "nie");

  // setTouchCalibrate() w LovyanGFX: surowe X,Y w rogach ekranu: LG, LD, PG, PD
  const float corners[4][2] = { { 0, 0 }, { 0, (float)h - 1 }, { (float)w - 1, 0 }, { (float)w - 1, (float)h - 1 } };
  int32_t params[8];
  for (int i = 0; i < 4; i++) {
    if (!inverseCalibration(c, corners[i][0], corners[i][1], params[i * 2], params[i * 2 + 1])) return;
  }
  pos += snprintf(r.calibrationText + pos, sizeof(r.calibrationText) - pos,
                  "LovyanGFX (obrot %u): uint16_t cal[8] = { %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld };",
                  rotation, (long)constrain(params[0], 0, 65535), (long)constrain(params[1], 0, 65535),
                  (long)constrain(params[2], 0, 65535), (long)constrain(params[3], 0, 65535),
                  (long)constrain(params[4], 0, 65535), (long)constrain(params[5], 0, 65535),
                  (long)constrain(params[6], 0, 65535), (long)constrain(params[7], 0, 65535));
}

/// Krzyzyk celu z kolkiem.
static void drawTarget(LcdDevice& lcd, int x, int y, uint16_t color) {
  lcd.drawFastHLine(x - 10, y, 21, color);
  lcd.drawFastVLine(x, y - 10, 21, color);
  lcd.drawCircle(x, y, 6, color);
}

/// Ekran z poleceniem dla uzytkownika i jednym celem.
static void drawPrompt(LcdDevice& lcd, const char* title, const char* line, int x, int y) {
  lcd.fillScreen(TFT_BLACK);
  lcd.setFont(&fonts::Font2);
  lcd.setTextDatum(textdatum_t::middle_center);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.drawString(title, lcd.width() / 2, lcd.height() / 2 - 10);
  lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
  lcd.drawString(line, lcd.width() / 2, lcd.height() / 2 + 10);
  drawTarget(lcd, x, y, TFT_WHITE);
}

/// Ekran wyniku weryfikacji: wzorcowy i sklikany prostokat oraz bledy we wszystkich punktach.
static void drawResult(LcdDevice& lcd, const Result& r) {
  lcd.fillScreen(TFT_BLACK);
  // Wzorzec (szary) i prostokat z dotkniec weryfikacji (zielony)
  for (uint8_t i = 0; i < CORNER_COUNT; i++) {
    const TestPoint& a = r.points[i];
    const TestPoint& b = r.points[(i + 1) % CORNER_COUNT];
    lcd.drawLine(a.targetX, a.targetY, b.targetX, b.targetY, TFT_DARKGREY);
    if (a.verify.done && b.verify.done) lcd.drawLine(a.verify.x, a.verify.y, b.verify.x, b.verify.y, TFT_GREEN);
  }
  // Kazdy punkt: cel (zielony = OK, czerwony = poza progiem), dotkniecie (zolte), linia bledu
  for (uint8_t i = 0; i < POINT_COUNT; i++) {
    const TestPoint& p = r.points[i];
    drawTarget(lcd, p.targetX, p.targetY, p.ok ? TFT_GREEN : TFT_RED);
    if (!p.verify.done) continue;
    lcd.drawLine(p.targetX, p.targetY, p.verify.x, p.verify.y, TFT_RED);
    lcd.fillCircle(p.verify.x, p.verify.y, 3, TFT_YELLOW);
  }
  lcd.setFont(&fonts::Font2);
  lcd.setTextDatum(textdatum_t::bottom_center);
  lcd.setTextColor(r.ok ? TFT_GREEN : TFT_RED, TFT_BLACK);
  lcd.drawString(String(r.ok ? "OK" : "ZLE") + "  maks. blad " + String(r.maxError, 1) + " px",
                 lcd.width() / 2, lcd.height() - 2);
}

/**
 * @brief Prosi kolejno o dotkniecie wszystkich punktow i zapisuje odczyty.
 * @param verify false = faza kalibracji (zapis do calib), true = weryfikacja (zapis do verify).
 * @return false przy braku dotyku albo przerwaniu.
 */
static bool collectTaps(LcdDevice& lcd, Result& result, bool verify, const volatile bool& stop,
                        const MessageFn& message) {
  const char* phase = verify ? "Weryfikacja" : "Kalibracja";
  char buf[64];
  for (uint8_t i = 0; i < POINT_COUNT; i++) {
    TestPoint& p = result.points[i];
    Tap& tap = verify ? p.verify : p.calib;
    const char* kind = i < CORNER_COUNT ? "rog" : "punkt losowy";
    snprintf(buf, sizeof(buf), "%s: dotknij %s (%u/%u)", phase, kind, i + 1, POINT_COUNT);
    message(buf);
    snprintf(buf, sizeof(buf), "Dotknij krzyzyk %u/%u", i + 1, POINT_COUNT);
    drawPrompt(lcd, phase, buf, p.targetX, p.targetY);
    if (!waitTap(lcd, tap.rawX, tap.rawY, stop)) {
      message(stop ? "Test dotyku przerwany" : "Brak odpowiedzi dotyku (15 s bez dotkniecia)");
      return false;
    }
    tap.done = true;
    if (verify) {
      applyCalibration(result.calibration, tap.rawX, tap.rawY, tap.x, tap.y);
      tap.error = hypotf(tap.x - p.targetX, tap.y - p.targetY);
      p.ok = tap.error <= result.threshold;
      result.maxError = max(result.maxError, tap.error);
      // Krotki podglad: gdzie trafiono
      lcd.drawLine(p.targetX, p.targetY, tap.x, tap.y, TFT_RED);
      lcd.fillCircle(tap.x, tap.y, 3, p.ok ? TFT_GREEN : TFT_RED);
      delay(400);
    }
  }
  return true;
}

bool run(LcdDevice& lcd, const volatile bool& stopRequested, Result& result, const MessageFn& message) {
  result = Result();
  result.active = true;
  const int w = lcd.width();
  const int h = lcd.height();
  const int mx = w / 10;
  const int my = h / 10;
  result.threshold = max(8.0f, min(w, h) / 25.0f);
  char buf[96];

  // Punkty testu: 4 rogi prostokata (~10% od krawedzi), potem 4 losowe wewnatrz niego
  const int16_t cornerXY[CORNER_COUNT][2] = {
    { (int16_t)mx, (int16_t)my }, { (int16_t)(w - 1 - mx), (int16_t)my },
    { (int16_t)(w - 1 - mx), (int16_t)(h - 1 - my) }, { (int16_t)mx, (int16_t)(h - 1 - my) },
  };
  for (uint8_t i = 0; i < POINT_COUNT; i++) {
    TestPoint& p = result.points[i];
    if (i < CORNER_COUNT) {
      p.targetX = cornerXY[i][0];
      p.targetY = cornerXY[i][1];
    } else {
      p.targetX = random(mx, w - mx);
      p.targetY = random(my, h - my);
    }
  }

  // 1. Kalibracja ze wszystkich 8 punktow
  if (!collectTaps(lcd, result, false, stopRequested, message)) return false;
  result.calibration = computeCalibration(result.points, POINT_COUNT);
  if (!result.calibration.valid) {
    message("Kalibracja niemozliwa - odczyty dotyku sa takie same (sprawdz polaczenia)");
    return false;
  }
  // Blad dopasowania w punktach kalibracji - pokazuje, jak dobrze kalibracja opisuje dotyk
  for (auto& p : result.points) {
    applyCalibration(result.calibration, p.calib.rawX, p.calib.rawY, p.calib.x, p.calib.y);
    p.calib.error = hypotf(p.calib.x - p.targetX, p.calib.y - p.targetY);
  }
  describeCalibration(result, w, h, lcd.getRotation());

  // 2. Weryfikacja na tych samych 8 punktach
  lcd.fillScreen(TFT_BLACK);
  lcd.setFont(&fonts::Font2);
  lcd.setTextDatum(textdatum_t::middle_center);
  lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  lcd.drawString("Kalibracja gotowa", w / 2, h / 2 - 10);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.drawString("Teraz weryfikacja - te same punkty", w / 2, h / 2 + 10);
  message("Kalibracja gotowa - za chwile weryfikacja tych samych punktow");
  for (int i = 0; i < 20 && !stopRequested; i++) delay(100);
  if (!collectTaps(lcd, result, true, stopRequested, message)) return false;
  result.verified = true;
  result.ok = true;
  for (const auto& p : result.points) result.ok = result.ok && p.ok;

  // 3. Wynik
  drawResult(lcd, result);
  snprintf(buf, sizeof(buf), "Wynik: %s, maks. blad %.1f px - za chwile rysowanie",
           result.ok ? "OK" : "ZLE", result.maxError);
  message(buf);
  for (int i = 0; i < 30 && !stopRequested; i++) delay(100);

  // 4. Rysowanie palcem
  lcd.fillScreen(TFT_BLACK);
  lcd.setFont(&fonts::Font2);
  lcd.setTextDatum(textdatum_t::top_center);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.drawString("Rysuj palcem (10 s)", w / 2, 2);
  message("Rysowanie palcem (10 s)");
  const uint32_t drawStart = millis();
  uint8_t hue = 0;
  int16_t lastX = -1, lastY = -1;
  while (!stopRequested && millis() - drawStart < DRAW_DURATION_MS) {
    int16_t rx, ry, x, y;
    if (readRaw(lcd, rx, ry)) {
      applyCalibration(result.calibration, rx, ry, x, y);
      const uint16_t color = lcd.color565(128 + 127 * sinf(hue * 0.05f), 128 + 127 * sinf(hue * 0.05f + 2.1f),
                                          128 + 127 * sinf(hue * 0.05f + 4.2f));
      if (lastX >= 0) lcd.drawLine(lastX, lastY, x, y, color);
      lcd.fillCircle(x, y, 2, color);
      lastX = x;
      lastY = y;
      hue++;
    } else {
      lastX = -1;
    }
    delay(10);
  }

  // 5. Ponownie wynik - zostaje na ekranie
  drawResult(lcd, result);
  uint8_t okCount = 0;
  for (const auto& p : result.points) okCount += p.ok;
  snprintf(buf, sizeof(buf), "Test dotyku: %s, %u/%u punktow OK, maks. blad %.1f px (prog %.0f px)",
           result.ok ? "OK" : "ZLE", okCount, POINT_COUNT, result.maxError, result.threshold);
  message(buf);
  return !stopRequested;
}

}  // namespace TouchTest
