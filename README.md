# 🔥 Alat Pendeteksi Kebocoran Gas
### Berbasis Arduino Uno & Sensor MQ-5

> Proyek tugas sekolah — SMKN 2 Surakarta | Teknik Elektronika | TA. 2025/2026

---

## 📋 Deskripsi

Alat pendeteksi kebocoran gas otomatis yang mampu mendeteksi gas LPG, gas alam, dan karbon monoksida dari kompor maupun tabung gas. Sistem menggunakan **4 level peringatan bertahap** yang ditampilkan melalui LED, buzzer, dan LCD I2C secara real-time.

---

## ⚡ Fitur

- ✅ Deteksi gas LPG, metana, dan CO menggunakan sensor MQ-5
- ✅ Sistem **4 level peringatan** (Aman → Menengah → Tinggi → Kritis)
- ✅ LED & buzzer dengan pola kedip berbeda tiap level
- ✅ Mode AMAN: kedip **random** (tidak monoton)
- ✅ LCD I2C 16x2 menampilkan status + bar progress sensor
- ✅ True random seed dari noise pin floating
- ✅ Non-blocking (tidak pakai `delay()` di main loop)
- ✅ Serial Monitor untuk kalibrasi dan monitoring real-time

---

## 🔧 Komponen

| No | Komponen | Jumlah |
|----|----------|--------|
| 1 | Arduino Uno | 1 |
| 2 | Sensor MQ-5 | 1 |
| 3 | LCD I2C 16x2 | 1 |
| 4 | LED Merah | 1 |
| 5 | Buzzer | 1 |
| 6 | Resistor 220Ω | 1 |
| 7 | Kabel Jumper | secukupnya |
| 8 | Breadboard | 1 |

---

## 🔌 Wiring

```
Komponen        →   Arduino Uno
─────────────────────────────────
MQ-5 VCC        →   5V
MQ-5 GND        →   GND
MQ-5 AOUT       →   A0

LED (+)         →   Pin 7  (via Resistor 220Ω)
LED (-)         →   GND

Buzzer (+)      →   Pin 8
Buzzer (-)      →   GND

LCD I2C SDA     →   A4
LCD I2C SCL     →   A5
LCD I2C VCC     →   5V
LCD I2C GND     →   GND
```

---

## 📊 Sistem 4 Level Peringatan

| Level | Nilai ADC | Pola LED & Buzzer | Keterangan |
|-------|-----------|-------------------|------------|
| 🟢 AMAN | < 150 | Kedip random (500ms ON, OFF acak) | Tidak ada gas |
| 🟡 MENENGAH | 150 – 299 | Kedip tiap 400ms | Gas mulai terdeteksi |
| 🟠 TINGGI | 300 – 449 | Kedip tiap 180ms | Konsentrasi berbahaya |
| 🔴 KRITIS | ≥ 450 | Kedip panik 60ms | Evakuasi segera! |

---

## 📦 Library yang Dibutuhkan

```bash
arduino-cli lib install "LiquidCrystal I2C"
```

## 🖥️ Contoh Output Serial Monitor

```
============================================
 PENDETEKSI GAS v3.0 - LCD + 4 LEVEL
============================================
[5s]  Gas: 94  / 1023  |  Level: AMAN     (random)
[10s] Gas: 178 / 1023  |  Level: AMAN     (random)
[15s] Gas: 352 / 1023  |  Level: MENENGAH (sedang)   !!!
[20s] Gas: 615 / 1023  |  Level: KRITIS   (panik!)   !!!
[25s] Gas: 94  / 1023  |  Level: AMAN     (random)
```
---

## 👤 Author

**Bagus**
Tahun Ajaran 2025/2026

---

## 📄 Lisensi

Proyek ini dibuat untuk keperluan tugas sekolah. Bebas digunakan sebagai referensi pembelajaran.
