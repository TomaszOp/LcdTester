/**
 * @file WifiConnection.h
 * @brief Polaczenie z siecia WiFi.
 */
#pragma once

#include <Arduino.h>

/**
 * @brief Laczy ESP z WiFi na podstawie danych z Secret.h.
 *
 * Gdy siec jest niedostepna (albo WIFI_SSID jest pusty), uruchamia wlasny
 * punkt dostepowy (AP), zeby strona testera byla zawsze osiagalna.
 * Rejestruje tez nazwe w sieci przez mDNS (np. http://lcdtester.local).
 * Puste haslo oznacza siec otwarta - zarowno dla WiFi, jak i dla AP.
 */
class WifiConnection {
 public:
  /**
   * @brief Laczy sie z siecia, a w razie porazki startuje punkt dostepowy.
   * @param timeoutMs Maksymalny czas oczekiwania na polaczenie w ms.
   */
  void begin(uint32_t timeoutMs = 15000);

  /// Wywolywane w loop() - co 10 s ponawia polaczenie po jego zerwaniu.
  void loop();

  /// @return true, gdy dziala punkt dostepowy zamiast polaczenia z siecia.
  bool isAccessPoint() const { return _apMode; }

  /// @return Adres IP strony (w sieci albo punktu dostepowego).
  IPAddress ip() const;

  /// @return Nazwa mDNS, np. "lcdtester.local".
  String hostname() const;

 private:
  /// Uruchamia punkt dostepowy AP_SSID / AP_PASSWORD.
  void startAccessPoint();
  /// Rejestruje nazwe WIFI_HOSTNAME i usluge HTTP w mDNS.
  void startMdns();

  bool _apMode = false;          ///< true = dziala punkt dostepowy.
  uint32_t _lastReconnectMs = 0; ///< millis() ostatniej proby ponownego polaczenia.
};
