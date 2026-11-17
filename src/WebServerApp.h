/**
 * @file WebServerApp.h
 * @brief Serwer WWW z panelem testera i API JSON.
 */
#pragma once

#include <WebServer.h>

#include "TestRunner.h"

/**
 * @brief Serwer WWW: udostepnia strone panelu i proste API JSON do sterowania testami.
 *
 * Parametry testu przychodza z formularza strony, sa zamieniane na LcdTestConfig
 * i przekazywane do TestRunner. Odpowiedzi JSON sa skladane recznie (bez bibliotek).
 *
 * | Metoda | Adres | Dzialanie |
 * |---|---|---|
 * | GET  | /                | strona panelu (src/web/IndexHtml.h) |
 * | GET  | /api/config      | modele, rozdzielczosci, czestotliwosci, domyslne piny |
 * | POST | /api/start       | pelny test (3 fazy) |
 * | POST | /api/pattern     | start LCD i jeden statyczny obraz testowy |
 * | POST | /api/readid      | odczyt ID sposobem wybranego modelu, bez LovyanGFX (MISO lub spi3wire=1) |
 * | POST | /api/touch       | test i kalibracja dotyku (wyswietlacz z tych samych parametrow) |
 * | POST | /api/touchraw    | 30 s podgladu surowych odczytow dotyku (diagnostyka) |
 * | POST | /api/stop        | przerwanie testu |
 * | GET  | /api/status      | stan testu i wyniki |
 * | POST | /api/brightness  | jasnosc podswietlenia, parametr value=0..100 |
 */
class WebServerApp {
 public:
  /**
   * @param runner Obiekt uruchamiajacy testy.
   * @param port Port HTTP.
   */
  explicit WebServerApp(TestRunner& runner, uint16_t port = 80);

  /// Rejestruje adresy i uruchamia serwer.
  void begin();

  /// Obsluguje zapytania; wywolywane w loop().
  void loop();

 private:
  /// GET / - strona panelu.
  void handleIndex();
  /// GET /api/config - dane do formularza strony.
  void handleConfig();
  /**
   * @brief POST /api/start, /api/pattern, /api/readid, /api/touch - odczyt parametrow i start.
   * @param mode Rodzaj zadania dla TestRunner.
   */
  void handleStart(TestRunner::Mode mode);
  /// POST /api/stop - przerwanie testu.
  void handleStop();
  /// GET /api/status - stan, wyniki, komunikat, wolna pamiec, jasnosc.
  void handleStatus();
  /// POST /api/brightness - zmiana jasnosci.
  void handleBrightness();
  /// Odpowiedz 404 dla nieznanych adresow.
  void handleNotFound();
  /// Dopisuje do JSON statusu wyniki testu dotyku (pole "touch"), jesli test byl uruchamiany.
  void appendTouchJson(String& json);

  /**
   * @brief Odczytuje numer pinu z parametru zapytania.
   * @param name Nazwa parametru (np. "sclk").
   * @param fallback Wartosc, gdy parametru brak albo jest pusty.
   * @return Numer GPIO albo -1.
   */
  int8_t argPin(const char* name, int8_t fallback);
  /// Wysyla odpowiedz JSON z podanym kodem HTTP.
  void sendJson(int code, const String& json);
  /// Wysyla blad w postaci {"ok":false,"error":"..."}.
  void sendError(int code, const String& message);

  WebServer _server;     ///< Serwer HTTP z rdzenia Arduino-ESP32.
  TestRunner& _runner;   ///< Obiekt uruchamiajacy testy.
};
