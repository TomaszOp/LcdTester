/**
 * @file TouchTest.h
 * @brief Test i kalibracja dotyku.
 *
 * Wyswietlacz (model, rozdzielczosc, obrot) pochodzi z ustawien sekcji
 * "Wyswietlacz" - test dotyku nie ma osobnego wyboru ekranu.
 */
#pragma once

#include <functional>

#include "LcdDevice.h"

/**
 * @brief Test dotyku: kalibracja z 8 punktow, weryfikacja na tych samych punktach, rysowanie.
 *
 * Punkty to 4 rogi prostokata (~10% od krawedzi) i 4 punkty losowe. Kalibracja
 * jest liczona ze wszystkich 8, a weryfikacja prosi ponownie o te same 8 punktow
 * i sprawdza, czy po kalibracji dotkniecia trafiaja w cele.
 */
namespace TouchTest {

constexpr uint8_t CORNER_COUNT = 4;                          ///< Rogi prostokata.
constexpr uint8_t RANDOM_COUNT = 4;                          ///< Punkty losowe.
constexpr uint8_t POINT_COUNT = CORNER_COUNT + RANDOM_COUNT; ///< Wszystkie punkty (rogi, potem losowe).
constexpr uint32_t TAP_TIMEOUT_MS = 15000;                   ///< Czas oczekiwania na dotkniecie.
constexpr uint32_t DRAW_DURATION_MS = 10000;                 ///< Czas rysowania palcem na koniec.

/// Jedno dotkniecie punktu: surowy odczyt i polozenie po kalibracji.
struct Tap {
  int16_t rawX = 0;   ///< Surowy odczyt X z kontrolera.
  int16_t rawY = 0;   ///< Surowy odczyt Y z kontrolera.
  int16_t x = 0;      ///< X po kalibracji, w pikselach.
  int16_t y = 0;      ///< Y po kalibracji, w pikselach.
  float error = 0;    ///< Odleglosc od celu w pikselach.
  bool done = false;  ///< Czy punkt zostal dotkniety.
};

/// Punkt testu: cel na ekranie i dotkniecia z obu faz.
struct TestPoint {
  int16_t targetX = 0;  ///< Cel X w pikselach.
  int16_t targetY = 0;  ///< Cel Y w pikselach.
  Tap calib;            ///< Dotkniecie w fazie kalibracji (blad = dopasowanie kalibracji).
  Tap verify;           ///< Dotkniecie w fazie weryfikacji (blad = dokladnosc po kalibracji).
  bool ok = false;      ///< Czy blad weryfikacji miesci sie w progu.
};

/// Wspolczynniki kalibracji: piksel = a*surowyX + b*surowyY + c (osobno dla X i Y).
struct Calibration {
  float ax = 0, bx = 0, cx = 0;  ///< x = ax*rx + bx*ry + cx.
  float ay = 0, by = 0, cy = 0;  ///< y = ay*rx + by*ry + cy.
  bool valid = false;            ///< Czy obliczono kalibracje.
};

/// Pelny wynik testu dotyku.
struct Result {
  bool active = false;             ///< Czy test dotyku byl uruchamiany.
  TestPoint points[POINT_COUNT];   ///< Punkty: najpierw 4 rogi, potem 4 losowe.
  Calibration calibration;         ///< Kalibracja obliczona z 8 punktow.
  float maxError = 0;              ///< Najwiekszy blad weryfikacji.
  float threshold = 0;             ///< Dopuszczalny blad w pikselach.
  bool verified = false;           ///< Czy weryfikacja zostala zakonczona.
  bool ok = false;                 ///< Czy wszystkie punkty weryfikacji miescily sie w progu.
  char calibrationText[256] = "";  ///< Wspolczynniki do skopiowania (tekst).
};

/// Funkcja ustawiajaca komunikat na stronie.
using MessageFn = std::function<void(const char*)>;

/**
 * @brief Przeprowadza caly test dotyku.
 *
 * Kolejno: kalibracja (4 rogi + 4 losowe punkty), obliczenie kalibracji z 8 punktow,
 * weryfikacja na tych samych 8 punktach, ekran wyniku, 10 s rysowania palcem
 * i ponownie ekran wyniku.
 * @param lcd Wyswietlacz z uruchomionym dotykiem (LcdDevice::beginTouch()).
 * @param stopRequested Flaga przerwania testu.
 * @param[out] result Wyniki, uzupelniane na biezaco (widoczne na stronie w trakcie).
 * @param message Funkcja ustawiajaca komunikat dla uzytkownika.
 * @return true po przejsciu calego testu; false przy braku dotyku (timeout) albo przerwaniu.
 */
bool run(LcdDevice& lcd, const volatile bool& stopRequested, Result& result, const MessageFn& message);

}  // namespace TouchTest
