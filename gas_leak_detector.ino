#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// pin
int mq5 = A0;
int led = 7;
int buzzer = 8;

// batas gas
int batas1 = 150; // aman
int batas2 = 300; // menengah
int batas3 = 450; // tinggi

int nilaiGas = 0;

// untuk timing kedip
unsigned long prevMillis = 0;
bool nyala = false;

void setup() {
  Serial.begin(9600);
  pinMode(led, OUTPUT);
  pinMode(buzzer, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Detektor Gas");
  lcd.setCursor(0, 1);
  lcd.print("SMKN 2 SKA");
  delay(2000);
  lcd.clear();

  // warmup sensor
  lcd.setCursor(0, 0);
  lcd.print("Warming up...");
  for (int i = 20; i > 0; i--) {
    lcd.setCursor(0, 1);
    lcd.print("Sisa: ");
    lcd.print(i);
    lcd.print(" detik  ");
    digitalWrite(led, HIGH);
    delay(500);
    digitalWrite(led, LOW);
    delay(500);
  }
  lcd.clear();
}

void loop() {
  nilaiGas = bacaSensor();
  tampilLCD();
  kedipLedBuzzer();

  Serial.print("Gas: ");
  Serial.print(nilaiGas);
  Serial.print(" | Status: ");
  Serial.println(getStatus());
  delay(150);
}

int bacaSensor() {
  long total = 0;
  for (int i = 0; i < 5; i++) {
    total += analogRead(mq5);
    delay(2);
  }
  return total / 5;
}

String getStatus() {
  if (nilaiGas < batas1) return "AMAN";
  if (nilaiGas < batas2) return "MENENGAH";
  if (nilaiGas < batas3) return "TINGGI";
  return "KRITIS";
}

void tampilLCD() {
  lcd.setCursor(0, 0);
  if (nilaiGas < batas1) {
    lcd.print("Status: AMAN    ");
  } else if (nilaiGas < batas2) {
    lcd.print("Status: WASPADA ");
  } else if (nilaiGas < batas3) {
    lcd.print("Status: BAHAYA! ");
  } else {
    lcd.print("!! KRITIS !!    ");
  }

  lcd.setCursor(0, 1);
  lcd.print("Gas: ");
  lcd.print(nilaiGas);
  lcd.print("      ");
}

void kedipLedBuzzer() {
  unsigned long sekarang = millis();
  int interval;

  if (nilaiGas < batas1) {
    // aman: random
    if (nyala && sekarang - prevMillis >= 500) {
      nyala = false;
      prevMillis = sekarang;
    } else if (!nyala && sekarang - prevMillis >= (long)random(500, 10000)) {
      nyala = true;
      prevMillis = sekarang;
    }
  } else if (nilaiGas < batas2) {
    interval = 400;
    if (sekarang - prevMillis >= interval) {
      nyala = !nyala;
      prevMillis = sekarang;
    }
  } else if (nilaiGas < batas3) {
    interval = 180;
    if (sekarang - prevMillis >= interval) {
      nyala = !nyala;
      prevMillis = sekarang;
    }
  } else {
    interval = 60;
    if (sekarang - prevMillis >= interval) {
      nyala = !nyala;
      prevMillis = sekarang;
    }
  }

  if (nyala) {
    digitalWrite(led, HIGH);
    tone(buzzer, 1000);
  } else {
    digitalWrite(led, LOW);
    noTone(buzzer);
  }
}
