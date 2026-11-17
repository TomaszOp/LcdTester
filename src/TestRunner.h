/**
 * @file TestRunner.h
 * @brief Uruchamianie testow wyswietlacza w tle.
 */
#pragma once

#include "Backlight.h"
#include "LcdDevice.h"
#include "LcdTests.h"
#include "TouchTest.h"

/**
 * @brief Uruchamia test, obraz testowy albo odczyt ID w osobnym zadaniu FreeRTOS.
 *
 * Dzieki temu serwer WWW odpowiada w trakcie testu: mozna sledzic postep,
 * zmieniac jasnosc i przerwac test. Klasa sprawdza parametry, przechowuje stan
 * i wyniki do odczytu przez status() oraz steruje podswietleniem (Backlight).
 * Naraz moze dzialac tylko jeden test.
 */
class TestRunner {
 public:
  /// Stan biezacego lub ostatniego testu.
  enum class State : uint8_t {
    Idle,     ///< Nic jeszcze nie uruchomiono.
    Running,  ///< Test trwa.
    Done,     ///< Test zakonczony poprawnie (przy odczycie ID: ID zgodne).
    Stopped,  ///< Test przerwany przez uzytkownika.
    Error,    ///< Blad inicjalizacji, brak odpowiedzi albo niezgodne ID.
  };

  /// Rodzaj zadania do wykonania.
  enum class Mode : uint8_t {
    FullTest,  ///< Trzy fazy po PHASE_DURATION_MS.
    Pattern,   ///< Tylko start LCD i jeden statyczny obraz testowy.
    ReadId,    ///< Sam odczyt ID sterownika (LcdIdReader, bez LovyanGFX).
    TouchTest, ///< Test i kalibracja dotyku (TouchTest).
    TouchRaw,  ///< Podglad surowych odczytow dotyku przez TOUCH_RAW_MS (diagnostyka).
  };

  static constexpr uint32_t PHASE_DURATION_MS = 10000;  ///< Czas jednej fazy testu.
  static constexpr uint8_t PHASE_COUNT = 3;             ///< Liczba faz testu.
  static constexpr uint32_t TOUCH_RAW_MS = 30000;       ///< Czas podgladu surowych odczytow dotyku.

  /// Migawka stanu do wyslania na strone (kopia, bezpieczna miedzy zadaniami).
  struct Status {
    State state;                                 ///< Stan testu.
    uint8_t phase;                               ///< 0 = start, 1..3 = fazy testu.
    uint32_t phaseElapsedMs;                     ///< Czas od poczatku biezacej fazy.
    LcdTests::PhaseResult results[PHASE_COUNT];  ///< Wyniki faz.
    char message[96];                            ///< Komunikat dla uzytkownika.
    char idInfo[320];                            ///< Rejestry z ostatniego odczytu ID.
  };

  /**
   * @brief Sprawdza parametry i uruchamia zadanie w tle.
   * @param config Parametry testu.
   * @param mode Rodzaj zadania.
   * @param[out] error Opis bledu, gdy zwrocono false.
   * @return false, gdy test juz trwa albo parametry sa bledne (np. pin pamieci flash).
   */
  bool start(const LcdTestConfig& config, Mode mode, String& error);

  /// Prosi o przerwanie trwajacego testu (zakonczy sie po biezacej operacji).
  void stop();

  /// @return true, gdy test trwa.
  bool isRunning() const { return _state == State::Running; }

  /// @return Kopia biezacego stanu i wynikow.
  Status status() const;

  /// @return Kopia wynikow ostatniego testu dotyku (aktualizowana w trakcie testu).
  TouchTest::Result touchResult() const { return _touch; }

  /**
   * @brief Ustawia jasnosc podswietlenia; dziala w kazdej chwili, takze w trakcie testu.
   * @param percent Jasnosc 0..100 %.
   */
  void setBrightness(uint8_t percent) { _backlight.setLevel(percent); }

  /// @return Biezaca jasnosc podswietlenia w %.
  uint8_t brightness() const { return _backlight.level(); }

  /// @return Nazwa stanu uzywana w API ("idle", "running", ...).
  static const char* stateName(State s);

  /// @return Nazwa fazy dla uzytkownika (0 = "Start").
  static const char* phaseName(uint8_t phase);

 private:
  /// Punkt wejscia zadania FreeRTOS.
  static void taskEntry(void* arg);
  /// Wykonuje zadanie wg _mode i ustawia stan koncowy.
  void run();
  /// Trzy fazy testu z ekranem startowym i koncowym.
  State runPhases();
  /// Start LCD i obraz testowy.
  State runPattern();
  /// Odczyt ID sposobem wybranego modelu.
  State runReadId();
  /// Test i kalibracja dotyku.
  State runTouch();
  /// Podglad surowych odczytow dotyku (X, Y, Z1, Z2, nacisk) - diagnostyka.
  State runTouchRaw();
  /// Ustawia komunikat widoczny na stronie.
  void setMessage(const String& msg);

  LcdTestConfig _config;                        ///< Parametry biezacego testu.
  Mode _mode = Mode::FullTest;                  ///< Rodzaj biezacego zadania.
  Backlight _backlight;                         ///< Podswietlenie (pin LED).
  volatile State _state = State::Idle;          ///< Stan testu.
  volatile uint8_t _phase = 0;                  ///< Biezaca faza.
  volatile uint32_t _phaseStartMs = 0;          ///< millis() na poczatku fazy.
  volatile bool _stopRequested = false;         ///< Flaga przerwania sprawdzana przez testy.
  LcdTests::PhaseResult _results[PHASE_COUNT];  ///< Wyniki faz.
  char _message[96] = "Gotowy";                 ///< Komunikat dla uzytkownika.
  char _idInfo[320] = "";                       ///< Rejestry z ostatniego odczytu ID.
  TouchTest::Result _touch;                     ///< Wyniki ostatniego testu dotyku.
  TaskHandle_t _task = nullptr;                 ///< Uchwyt zadania FreeRTOS.
};
