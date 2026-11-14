/**
 * @file LcdTests.h
 * @brief Fazy testu wyswietlacza i ekrany pomocnicze.
 *
 * Kazdy model przechodzi ten sam zestaw faz. Kazda faza trwa durationMs
 * i konczy sie wczesniej, gdy stopRequested == true.
 */
#pragma once

#include "LcdDevice.h"

/// Funkcje rysujace testy na wyswietlaczu.
namespace LcdTests {

/// Wynik jednej fazy testu.
struct PhaseResult {
  uint32_t frames = 0;  ///< Liczba narysowanych klatek (faza 3) albo operacji (fazy 1-2).
  float fps = 0;        ///< Klatki albo operacje na sekunde.
};

/**
 * @brief Faza 1: pelne kolory, gradienty i losowe kwadraty, kola, trojkaty.
 *
 * Pelne kolory z podpisami sluza do kontroli inwersji i kolejnosci R/B.
 * @param lcd Uruchomiony wyswietlacz.
 * @param durationMs Czas trwania fazy w ms.
 * @param packetDelayMs Odstep po kazdym ksztalcie w ms, 0 = bez odstepu.
 * @param stopRequested Flaga przerwania testu.
 * @return Liczba narysowanych ksztaltow i ksztalty na sekunde.
 */
PhaseResult shapes(LcdDevice& lcd, uint32_t durationMs, uint16_t packetDelayMs,
                   const volatile bool& stopRequested);

/**
 * @brief Faza 2: napisy w naroznikach i losowe napisy roznymi czcionkami i kolorami.
 * @param lcd Uruchomiony wyswietlacz.
 * @param durationMs Czas trwania fazy w ms.
 * @param packetDelayMs Odstep po kazdym napisie w ms, 0 = bez odstepu.
 * @param stopRequested Flaga przerwania testu.
 * @return Liczba narysowanych napisow i napisy na sekunde.
 */
PhaseResult text(LcdDevice& lcd, uint32_t durationMs, uint16_t packetDelayMs,
                 const volatile bool& stopRequested);

/**
 * @brief Faza 3: animacje pelnoekranowe (odbijajace sie kule, plazma) z pomiarem FPS.
 *
 * Klatka jest rysowana pasami w dwoch buforach na zmiane i wysylana przez DMA.
 * @param lcd Uruchomiony wyswietlacz.
 * @param durationMs Czas trwania fazy w ms (polowa kule, polowa plazma).
 * @param fpsLimit Maksymalna liczba pelnych klatek na sekunde, 0 = bez limitu.
 * @param stopRequested Flaga przerwania testu.
 * @return Liczba pelnych klatek i FPS.
 */
PhaseResult animation(LcdDevice& lcd, uint32_t durationMs, uint8_t fpsLimit,
                      const volatile bool& stopRequested);

/**
 * @brief Ekran poczatkowy z parametrami testu, ramka i znacznikami naroznikow.
 * @param lcd Uruchomiony wyswietlacz.
 * @param config Parametry testu do wypisania.
 */
void drawIntro(LcdDevice& lcd, const LcdTestConfig& config);

/**
 * @brief Statyczny obraz testowy: paski kolorow, gradienty, siatka, kolo, ramka, narozniki.
 *
 * Rysowany w buforze calego ekranu i wysylany jednym przeslaniem, gdy starczy RAM.
 * @param lcd Uruchomiony wyswietlacz.
 * @param config Parametry testu (podpis na obrazie).
 * @return Glebia koloru bufora (16 lub 8) albo 0, gdy rysowano bezposrednio na ekranie.
 */
uint8_t testPattern(LcdDevice& lcd, const LcdTestConfig& config);

/**
 * @brief Ekran koncowy z wynikami wszystkich faz.
 * @param lcd Uruchomiony wyswietlacz.
 * @param results Wyniki trzech faz.
 */
void drawSummary(LcdDevice& lcd, const PhaseResult results[3]);

}  // namespace LcdTests
