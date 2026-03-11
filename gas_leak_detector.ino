/**
 * ============================================================
 * PROJECT   : Alat Pendeteksi Kebocoran Gas
 * BOARD     : Arduino Uno
 * SENSOR    : MQ-5 (LPG, Gas Alam, Karbon Monoksida)
 * DESKRIPSI : Sistem deteksi gas 4 level + LCD I2C 16x2:
 *             AMAN     → LED + buzzer kedip random, LCD hijau
 *             MENENGAH → LED + buzzer sedang (400ms)
 *             TINGGI   → LED + buzzer cepat (180ms)
 *             KRITIS   → LED + buzzer panik (60ms)
 * AUTHOR    : Bagus @ SMKN 2 Surakarta
 * VERSION   : 3.0.0
 *
 * ============================================================
 * WIRING:
 *   MQ-5 VCC    → Arduino 5V
 *   MQ-5 GND    → Arduino GND
 *   MQ-5 AOUT   → Arduino A0
 *
 *   LED (+)     → Arduino Pin 7 → Resistor 220Ω → GND
 *   Buzzer (+)  → Arduino Pin 8
 *   Buzzer (-)  → Arduino GND
 *
 *   LCD I2C VCC → Arduino 5V
 *   LCD I2C GND → Arduino GND
 *   LCD I2C SDA → Arduino A4
 *   LCD I2C SCL → Arduino A5
 *
 * LIBRARY YANG DIBUTUHKAN:
 *   - LiquidCrystal_I2C
 *   Install: arduino-cli lib install "LiquidCrystal I2C"
 * ============================================================
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ============================================================
// LCD I2C — alamat default 0x27, coba 0x3F jika tidak tampil
// Parameter: (alamat, kolom, baris)
// ============================================================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ============================================================
// PIN DEFINITIONS
// ============================================================
const int PIN_MQ5    = A0;
const int PIN_LED    = 7;
const int PIN_BUZZER = 8;

// ============================================================
// THRESHOLD LEVEL GAS (nilai ADC 0-1023)
// ============================================================
const int BATAS_AMAN      = 150;   // < 150        = AMAN
const int BATAS_MENENGAH  = 300;   // 150 - 299    = MENENGAH
const int BATAS_TINGGI    = 450;   // 300 - 449    = TINGGI
                                   // 450+          = KRITIS

// ============================================================
// KONFIGURASI POLA KEDIP PER LEVEL
// Format: { durasi_ON (ms), durasi_OFF (ms), frekuensi (Hz) }
// ============================================================
//                           ON    OFF   FREQ
const int POLA_AMAN[3]     = {  0,   0,  800 };  // ON=500ms tetap, OFF=RANDOM
const int POLA_MENENGAH[3] = {400, 400,  900 };  // Sedang
const int POLA_TINGGI[3]   = {180, 180, 1000 };  // Cepat
const int POLA_KRITIS[3]   = { 60,  60, 1200 };  // Panik!

// ============================================================
// INTERVAL
// ============================================================
const unsigned long INTERVAL_BACA = 150UL;
const unsigned long INTERVAL_LCD  = 500UL;  // Update LCD tiap 500ms

// ============================================================
// ENUM LEVEL
// ============================================================
enum Level { AMAN, MENENGAH, TINGGI, KRITIS };

// ============================================================
// VARIABEL SISTEM
// ============================================================
int   nilaiSensor   = 0;
Level levelSekarang = AMAN;
Level levelSebelumnya = AMAN;  // Untuk deteksi perubahan level di LCD
unsigned long lastSample  = 0;
unsigned long lastLCD     = 0;

// State untuk pola AMAN random
bool          amanFaseOn    = false;
unsigned long amanFaseMulai = 0UL;
unsigned long amanFaseDur   = 2000UL;

// ============================================================
// CUSTOM CHARACTER LCD — ikon gas bocor
// ============================================================
byte ikonGas[8] = {
  0b00100,
  0b01110,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000,
  0b00000
};

byte ikonOke[8] = {
  0b00000,
  0b00001,
  0b00011,
  0b10110,
  0b11100,
  0b01000,
  0b00000,
  0b00000
};

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(9600);

  // True random seed
  unsigned long seed = 0;
  for (int i = 0; i < 32; i++) {
    seed ^= (unsigned long)analogRead(A1) << (i % 16);
    seed ^= (unsigned long)analogRead(A2) << ((i + 3) % 16);
    seed += millis();
    seed *= 1664525UL;
    seed += 1013904223UL;
    delayMicroseconds(13);
  }
  randomSeed(seed);

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  noTone(PIN_BUZZER);

  // Init LCD
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, ikonGas);
  lcd.createChar(1, ikonOke);

  // Layar splash startup
  lcd.setCursor(0, 0);
  lcd.print("  DETEKTOR GAS  ");
  lcd.setCursor(0, 1);
  lcd.print(" SMKN 2 SKA v3.0");
  delay(2000);
  lcd.clear();

  Serial.println("============================================");
  Serial.println(" PENDETEKSI GAS v3.0 - LCD + 4 LEVEL");
  Serial.println("============================================");
  Serial.println(" Pemanasan sensor... tunggu 20 detik");

  // Tampilkan pemanasan di LCD juga
  lcd.setCursor(0, 0);
  lcd.print("Pemanasan sensor");

  pemanasanSensor(20);

  // Mulai dari fase OFF random
  amanFaseOn  = false;
  amanFaseDur = (unsigned long)random(500, 10001);

  lcd.clear();
  Serial.println(" Sensor siap! Sistem aktif.");
  Serial.println("============================================");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  unsigned long sekarang = millis();

  // Baca sensor
  if (sekarang - lastSample >= INTERVAL_BACA) {
    lastSample    = sekarang;
    nilaiSensor   = bacaSensor();
    levelSekarang = nentukanLevel(nilaiSensor);
    tampilkanSerial(sekarang);
  }

  // Update LCD
  if (sekarang - lastLCD >= INTERVAL_LCD) {
    lastLCD = sekarang;
    updateLCD();
  }

  // Jalankan pola LED + buzzer
  jalankanPola(sekarang, levelSekarang);
}

// ============================================================
// FUNGSI: Baca sensor (rata-rata 5x)
// ============================================================
int bacaSensor() {
  long total = 0;
  for (int i = 0; i < 5; i++) {
    total += analogRead(PIN_MQ5);
    delay(2);
  }
  return (int)(total / 5);
}

// ============================================================
// FUNGSI: Tentukan level
// ============================================================
Level nentukanLevel(int nilai) {
  if (nilai < BATAS_AMAN)     return AMAN;
  if (nilai < BATAS_MENENGAH) return MENENGAH;
  if (nilai < BATAS_TINGGI)   return TINGGI;
  return KRITIS;
}

// ============================================================
// FUNGSI: Update tampilan LCD
// Baris 0: Status level + ikon
// Baris 1: Nilai sensor (bar progress)
// ============================================================
void updateLCD() {
  // Baris 0 — Status
  lcd.setCursor(0, 0);
  switch (levelSekarang) {
    case AMAN:
      lcd.write(1);  // ikon centang
      lcd.print(" AMAN           ");
      break;
    case MENENGAH:
      lcd.write(0);  // ikon gas
      lcd.print(" GAS MENENGAH   ");
      break;
    case TINGGI:
      lcd.write(0);
      lcd.print(" GAS TINGGI!    ");
      break;
    case KRITIS:
      lcd.write(0);
      lcd.print(" !! KRITIS !!   ");
      break;
  }

  // Baris 1 — Nilai sensor + bar visual
  lcd.setCursor(0, 1);
  lcd.print("Gas:");

  // Tampilkan nilai (3 digit, spasi jika kurang)
  if (nilaiSensor < 100) lcd.print(" ");
  if (nilaiSensor < 10)  lcd.print(" ");
  lcd.print(nilaiSensor);
  lcd.print(" ");

  // Bar progress: 10 karakter, scale 0-1023 → 0-10
  int barPanjang = map(nilaiSensor, 0, 1023, 0, 10);
  for (int i = 0; i < 10; i++) {
    if (i < barPanjang) lcd.print("|");
    else                lcd.print(" ");
  }
}

// ============================================================
// FUNGSI: Jalankan pola LED + buzzer (non-blocking)
// ============================================================
void jalankanPola(unsigned long sekarang, Level level) {

  // MODE AMAN: ON selalu 500ms, OFF random 500ms - 30 detik
  if (level == AMAN) {
    if (sekarang - amanFaseMulai >= amanFaseDur) {
      amanFaseOn    = !amanFaseOn;
      amanFaseMulai = sekarang;

      if (amanFaseOn) {
        amanFaseDur = 500UL;  // ON tetap 500ms
      } else {
        amanFaseDur = (unsigned long)random(500, 30001);  // OFF random
      }
    }

    if (amanFaseOn) {
      digitalWrite(PIN_LED, HIGH);
      tone(PIN_BUZZER, POLA_AMAN[2]);
    } else {
      digitalWrite(PIN_LED, LOW);
      noTone(PIN_BUZZER);
    }
    return;
  }

  // MODE BAHAYA: MENENGAH / TINGGI / KRITIS
  const int* pola;
  switch (level) {
    case MENENGAH: pola = POLA_MENENGAH; break;
    case TINGGI:   pola = POLA_TINGGI;   break;
    case KRITIS:   pola = POLA_KRITIS;   break;
    default:       pola = POLA_MENENGAH; break;
  }

  unsigned long periode = (unsigned long)(pola[0] + pola[1]);
  unsigned long fase    = sekarang % periode;

  if (fase < (unsigned long)pola[0]) {
    digitalWrite(PIN_LED, HIGH);
    tone(PIN_BUZZER, pola[2]);
  } else {
    digitalWrite(PIN_LED, LOW);
    noTone(PIN_BUZZER);
  }
}

// ============================================================
// FUNGSI: Tampilkan status ke Serial Monitor
// ============================================================
void tampilkanSerial(unsigned long sekarang) {
  Serial.print("[");
  Serial.print(sekarang / 1000);
  Serial.print("s] Gas: ");
  Serial.print(nilaiSensor);
  Serial.print(" / 1023  |  Level: ");

  switch (levelSekarang) {
    case AMAN:     Serial.print("AMAN     (random)");       break;
    case MENENGAH: Serial.print("MENENGAH (sedang)  !!!");  break;
    case TINGGI:   Serial.print("TINGGI   (cepat)   !!!");  break;
    case KRITIS:   Serial.print("KRITIS   (panik!)  !!!");  break;
  }

  Serial.println();
}

// ============================================================
// FUNGSI: Pemanasan sensor saat startup
// ============================================================
void pemanasanSensor(int detik) {
  for (int i = detik; i > 0; i--) {
    Serial.print(" Sisa: ");
    Serial.print(i);
    Serial.println(" detik...");

    // Update countdown di LCD baris 1
    lcd.setCursor(0, 1);
    lcd.print("Sisa: ");
    lcd.print(i);
    lcd.print(" detik   ");

    digitalWrite(PIN_LED, HIGH);
    delay(300);
    digitalWrite(PIN_LED, LOW);
    delay(700);
  }
}
