/**
 * @file WifiConnection.cpp
 * @brief Implementacja klasy WifiConnection - WiFi, punkt dostepowy i mDNS.
 */
#include "WifiConnection.h"

#include <ESPmDNS.h>
#include <WiFi.h>

#include "Secret.h"

// Puste haslo = siec otwarta. Niepuste haslo AP musi miec min. 8 znakow (WPA2),
// inaczej softAP() nie wystartuje - sprawdzamy to juz przy kompilacji.
static_assert(sizeof(AP_PASSWORD) == 1 || sizeof(AP_PASSWORD) > 8,
              "AP_PASSWORD w Secret.h musi byc pusty (AP otwarty) albo miec min. 8 znakow");

/**
 * @brief Zamienia puste haslo na nullptr - jawny sygnal "siec otwarta" dla rdzenia WiFi.
 * @param password Haslo z Secret.h.
 * @return password albo nullptr, gdy jest puste.
 */
static const char* passwordOrNull(const char* password) {
  return password[0] ? password : nullptr;
}

void WifiConnection::begin(uint32_t timeoutMs) {
  if (WIFI_SSID[0] == '\0') {
    Serial.println("[WIFI] Brak WIFI_SSID - uruchamiam punkt dostepowy");
    startAccessPoint();
    startMdns();
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(WIFI_HOSTNAME);
  WiFi.begin(WIFI_SSID, passwordOrNull(WIFI_PASSWORD));
  Serial.printf("[WIFI] Laczenie z %s (%s)", WIFI_SSID,
                WIFI_PASSWORD[0] ? "zabezpieczona" : "siec otwarta");

  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    _apMode = false;
    Serial.printf("[WIFI] Polaczono, IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("[WIFI] Brak polaczenia - uruchamiam punkt dostepowy");
    startAccessPoint();
  }
  startMdns();
}

void WifiConnection::loop() {
  if (_apMode || WiFi.status() == WL_CONNECTED) return;
  if (millis() - _lastReconnectMs < 10000) return;
  _lastReconnectMs = millis();
  Serial.println("[WIFI] Utracono polaczenie, ponawiam");
  WiFi.reconnect();
}

IPAddress WifiConnection::ip() const {
  return _apMode ? WiFi.softAPIP() : WiFi.localIP();
}

String WifiConnection::hostname() const {
  return String(WIFI_HOSTNAME) + ".local";
}

void WifiConnection::startAccessPoint() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  _apMode = true;
  if (!WiFi.softAP(AP_SSID, passwordOrNull(AP_PASSWORD))) {
    Serial.println("[WIFI] Nie udalo sie uruchomic punktu dostepowego");
    return;
  }
  Serial.printf("[WIFI] AP \"%s\" (%s), IP: %s\n", AP_SSID,
                AP_PASSWORD[0] ? "z haslem" : "otwarty, bez hasla",
                WiFi.softAPIP().toString().c_str());
}

void WifiConnection::startMdns() {
  if (MDNS.begin(WIFI_HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("[WIFI] mDNS: http://%s\n", hostname().c_str());
  }
}
