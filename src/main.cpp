/**
 * @file main.cpp
 * @brief Punkt wejscia programu LcdTester.
 *
 * Tworzy trzy glowne obiekty: polaczenie WiFi, uruchamianie testow i serwer WWW.
 * setup() laczy z siecia i startuje serwer, loop() obsluguje WiFi i zapytania HTTP;
 * same testy dzialaja w osobnym zadaniu FreeRTOS (TestRunner).
 */
#include <Arduino.h>

#include "TestRunner.h"
#include "WebServerApp.h"
#include "WifiConnection.h"

static WifiConnection wifi;                ///< Polaczenie z siecia.
static TestRunner testRunner;              ///< Uruchamianie testow w tle.
static WebServerApp webServer(testRunner); ///< Panel WWW i API.

/// Start: port szeregowy, WiFi i serwer WWW.
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== LcdTester (" BOARD_NAME ") ===");

  wifi.begin();
  webServer.begin();
  Serial.printf("Panel: http://%s  lub  http://%s\n",
                wifi.ip().toString().c_str(), wifi.hostname().c_str());
}

/// Petla glowna: utrzymanie WiFi i obsluga zapytan HTTP.
void loop() {
  wifi.loop();
  webServer.loop();
  delay(2);
}
