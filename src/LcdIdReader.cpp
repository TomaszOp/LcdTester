/**
 * @file LcdIdReader.cpp
 * @brief Implementacja klasy LcdIdReader - programowy (bit-bang) odczyt ID sterownika.
 */
#include "LcdIdReader.h"

/// Pol okresu zegara bit-bang w us (ok. 100 kHz - bezpiecznie dla kazdego sterownika).
static constexpr uint32_t HALF_CLOCK_US = 5;

/**
 * @brief Charakterystyczna para kolejnych bajtow w odpowiedzi sterownika.
 *
 * Sluzy do podpowiedzi, co naprawde siedzi w module, gdy ID nie zgadza sie
 * z wybranym modelem.
 */
struct Signature {
  uint8_t a;         ///< Pierwszy bajt pary.
  uint8_t b;         ///< Drugi bajt pary.
  const char* name;  ///< Nazwa rozpoznanego sterownika.
};

/// Znane sygnatury sterownikow.
static const Signature SIGNATURES[] = {
  { 0x93, 0x41, "ILI9341" },
  { 0x93, 0x42, "ILI9342" },
  { 0x93, 0x40, "ILI9340" },
  { 0x94, 0x86, "ILI9486" },
  { 0x94, 0x88, "ILI9488" },
  { 0x94, 0x81, "ILI9481" },
  { 0x77, 0x96, "ST7796" },
  { 0x9A, 0x01, "GC9A01" },
  { 0x85, 0x85, "ST7789" },
  { 0x7C, 0x89, "ST7735" },
};

LcdIdReader::LcdIdReader(const LcdPins& pins, bool threeWire)
    : _pins(pins), _threeWire(threeWire), _pinIn(threeWire ? pins.mosi : pins.miso) {}

void LcdIdReader::begin() {
  pinMode(_pins.sclk, OUTPUT);
  digitalWrite(_pins.sclk, LOW);
  pinMode(_pins.mosi, OUTPUT);
  digitalWrite(_pins.mosi, LOW);
  if (!_threeWire && _pins.miso >= 0) pinMode(_pins.miso, INPUT_PULLUP);
  pinMode(_pins.dc, OUTPUT);
  digitalWrite(_pins.dc, HIGH);
  if (_pins.cs >= 0) {
    pinMode(_pins.cs, OUTPUT);
    digitalWrite(_pins.cs, HIGH);
  }
}

void LcdIdReader::end() {
  if (_pins.cs >= 0) digitalWrite(_pins.cs, HIGH);
  pinMode(_pins.mosi, OUTPUT);
}

void LcdIdReader::hardwareReset() {
  if (_pins.rst >= 0) {
    pinMode(_pins.rst, OUTPUT);
    digitalWrite(_pins.rst, HIGH);
    delay(5);
    digitalWrite(_pins.rst, LOW);
    delay(20);
    digitalWrite(_pins.rst, HIGH);
    delay(150);
  }
  writeCommand(0x01);  // programowy reset - na wypadek braku pinu RST
  delay(150);
}

void LcdIdReader::writeBits(uint8_t value) {
  for (int8_t bit = 7; bit >= 0; bit--) {
    digitalWrite(_pins.mosi, (value >> bit) & 1);
    delayMicroseconds(HALF_CLOCK_US);
    digitalWrite(_pins.sclk, HIGH);
    delayMicroseconds(HALF_CLOCK_US);
    digitalWrite(_pins.sclk, LOW);
  }
}

uint8_t LcdIdReader::readBit() {
  digitalWrite(_pins.sclk, HIGH);
  delayMicroseconds(HALF_CLOCK_US);
  uint8_t v = digitalRead(_pinIn);
  digitalWrite(_pins.sclk, LOW);
  delayMicroseconds(HALF_CLOCK_US);
  return v;
}

void LcdIdReader::writeCommand(uint8_t cmd) {
  if (_pins.cs >= 0) digitalWrite(_pins.cs, LOW);
  digitalWrite(_pins.dc, LOW);
  writeBits(cmd);
  digitalWrite(_pins.dc, HIGH);
  if (_pins.cs >= 0) digitalWrite(_pins.cs, HIGH);
}

void LcdIdReader::writeData(uint8_t data) {
  if (_pins.cs >= 0) digitalWrite(_pins.cs, LOW);
  digitalWrite(_pins.dc, HIGH);
  writeBits(data);
  if (_pins.cs >= 0) digitalWrite(_pins.cs, HIGH);
}

void LcdIdReader::readBits(uint8_t cmd, uint8_t* bits, uint8_t nBits) {
  if (_pins.cs >= 0) digitalWrite(_pins.cs, LOW);
  digitalWrite(_pins.dc, LOW);
  writeBits(cmd);
  digitalWrite(_pins.dc, HIGH);
  if (_threeWire) pinMode(_pins.mosi, INPUT_PULLUP);
  delayMicroseconds(HALF_CLOCK_US);

  for (uint8_t i = 0; i < nBits; i++) bits[i] = readBit();

  if (_threeWire) pinMode(_pins.mosi, OUTPUT);
  if (_pins.cs >= 0) digitalWrite(_pins.cs, HIGH);
}

/**
 * @brief Sklada bajty z tablicy bitow.
 * @param bits Bity (0/1), po jednym w bajcie, od najstarszego.
 * @param offset Indeks bitu, od ktorego zaczyna sie pierwszy bajt.
 * @param len Liczba bajtow do zlozenia.
 * @param[out] out Wynikowe bajty.
 */
static void bitsToBytes(const uint8_t* bits, uint8_t offset, uint8_t len, uint8_t* out) {
  for (uint8_t b = 0; b < len; b++) {
    uint8_t v = 0;
    for (uint8_t i = 0; i < 8; i++) v = (v << 1) | bits[offset + b * 8 + i];
    out[b] = v;
  }
}

void LcdIdReader::readStep(const LcdIdStep& step, uint8_t* primary, uint8_t* alternate) {
  const uint8_t len = min<uint8_t>(step.len, MAX_LEN);
  // Klony czesto roznia sie o 1 takt dummy - drugie wyrownanie liczymy na zapas
  const uint8_t offset = step.dummyBits;
  const uint8_t altOffset = (offset % 2 == 0) ? offset + 1 : offset - 1;

  if (step.method == IdReadMethod::IliD9) {
    for (uint8_t i = 0; i < len; i++) {
      uint8_t bits[MAX_BITS];
      writeCommand(0xD9);
      writeData(0x10 + i + 1);
      readBits(step.cmd, bits, 8 + 1);
      bitsToBytes(bits, 0, 1, &primary[i]);
      bitsToBytes(bits, 1, 1, &alternate[i]);
    }
    return;
  }

  uint8_t bits[MAX_BITS];
  readBits(step.cmd, bits, max(offset, altOffset) + len * 8);
  bitsToBytes(bits, offset, len, primary);
  bitsToBytes(bits, altOffset, len, alternate);
}

/**
 * @brief Szuka znanej sygnatury w odczytanych bajtach.
 * @return Nazwa sterownika albo nullptr.
 */
static const char* findSignature(const uint8_t* bytes, uint8_t len) {
  for (uint8_t i = 0; i + 1 < len; i++) {
    for (const auto& sig : SIGNATURES) {
      if (bytes[i] == sig.a && bytes[i + 1] == sig.b) return sig.name;
    }
  }
  return nullptr;
}

/**
 * @brief Dopisuje bajty jako tekst szesnastkowy (" 7C 89 F0") do bufora.
 * @return Nowa pozycja konca tekstu (nie wieksza niz size - 1).
 */
static size_t appendHex(char* buf, size_t size, size_t pos, const uint8_t* bytes, uint8_t len) {
  for (uint8_t i = 0; i < len && pos < size - 1; i++) {
    pos += snprintf(buf + pos, size - pos, " %02X", bytes[i]);
  }
  return min(pos, size - 1);
}

LcdIdReader::Result LcdIdReader::read(const LcdModel& model) {
  Result result = {};
  begin();
  hardwareReset();

  bool allZero = true, allFF = true;
  const char* matchedStep = nullptr;
  bool matchedShifted = false;
  const char* signature = nullptr;
  char* d = result.details;
  const size_t dsize = sizeof(result.details);
  size_t pos = snprintf(d, dsize, "Sposob odczytu: %s\n", model.name);

  for (uint8_t s = 0; s < model.idStepCount; s++) {
    const LcdIdStep& step = model.idSteps[s];
    const uint8_t len = min<uint8_t>(step.len, MAX_LEN);
    uint8_t primary[MAX_LEN], alternate[MAX_LEN];
    readStep(step, primary, alternate);

    for (uint8_t i = 0; i < len; i++) {
      if (primary[i] != 0x00) allZero = false;
      if (primary[i] != 0xFF) allFF = false;
    }
    if (!signature) signature = findSignature(primary, len);
    if (!signature) signature = findSignature(alternate, len);

    const char* mark = "";
    if (step.checkExpected) {
      if (memcmp(primary, step.expected, len) == 0) {
        mark = " OK";
        if (!matchedStep) matchedStep = step.name;
      } else if (memcmp(alternate, step.expected, len) == 0) {
        mark = " OK (przesuniete o 1 bit)";
        if (!matchedStep) {
          matchedStep = step.name;
          matchedShifted = true;
        }
      } else {
        mark = " ROZNE";
      }
    }

    pos += snprintf(d + pos, dsize - pos, "%02X %s:", step.cmd, step.name);
    pos = appendHex(d, dsize, pos, primary, len);
    if (step.checkExpected) {
      pos += snprintf(d + pos, dsize - pos, " (oczek.");
      pos = appendHex(d, dsize, pos, step.expected, len);
      pos += snprintf(d + pos, dsize - pos, ")%s", mark);
    }
    pos += snprintf(d + pos, dsize - pos, "\n");
    pos = min(pos, dsize - 1);
  }
  end();

  if (matchedStep) {
    result.verdict = Verdict::Match;
    snprintf(result.summary, sizeof(result.summary), "Zgodny z %s (%s%s)", model.name, matchedStep,
             matchedShifted ? ", przesuniecie 1 bit" : "");
  } else if (allZero || allFF) {
    result.verdict = Verdict::NoResponse;
    snprintf(result.summary, sizeof(result.summary), "Brak odpowiedzi (same %s) - sprawdz %s",
             allFF ? "FF" : "00", _threeWire ? "SDA/MOSI" : "MISO/SDO");
  } else {
    result.verdict = Verdict::Mismatch;
    if (signature && strcmp(signature, model.name) != 0) {
      snprintf(result.summary, sizeof(result.summary), "NIEZGODNY z %s - prawdopodobnie %s",
               model.name, signature);
    } else {
      snprintf(result.summary, sizeof(result.summary), "NIEZGODNY z %s - nieznany sterownik",
               model.name);
    }
  }
  return result;
}
