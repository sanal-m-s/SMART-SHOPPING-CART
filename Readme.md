# 🛒 Smart Shopping Cart (ESP32 + RFID + Firebase)

## 📌 Overview

This project is an IoT-based smart shopping cart that automatically detects products using RFID and updates the cart in real-time using Firebase.

---

## ⚙️ Features

* RFID-based product detection
* Real-time Firebase integration
* OLED display for product info
* Buzzer alerts
* Budget tracking system
* Android app integration

---

## 🧰 Hardware Used

* ESP32
* MFRC522 RFID Module
* SSD1306 OLED Display
* Buzzer

---

## 🔥 Technologies Used

* Arduino (ESP32)
* Firebase Realtime Database
* Android App

---

## 🚀 How It Works

1. Scan RFID tag
2. ESP32 reads UID
3. Product mapped
4. Data sent to Firebase
5. Android app displays cart

---

## ⚠️ Setup Instructions

1. Add WiFi credentials
2. Add Firebase API key
3. Enable Firebase Authentication
4. Upload code to ESP32

---

## 📱 Future Improvements

* Remove item feature
* Payment integration
* Barcode scanning
* AI recommendations

---

## 👩‍💻 Author

Srishti Campus Project

## Connection diagrem

/*
| S. No | Component              | Pin on Component | Connected to ESP32 Pin |
| ----: | ---------------------- | ---------------- | ---------------------- |
|     1 | RFID Reader (RC522)    | SDA (SS)         | GPIO 5                 |
|     2 | RFID Reader (RC522)    | SCK              | GPIO 18                |
|     3 | RFID Reader (RC522)    | MOSI             | GPIO 23                |
|     4 | RFID Reader (RC522)    | MISO             | GPIO 19                |
|     5 | RFID Reader (RC522)    | RST              | GPIO 27                |
|     6 | RFID Reader (RC522)    | VCC              | 3.3V                   |
|     7 | RFID Reader (RC522)    | GND              | GND                    |
|     8 | OLED Display (SSD1306) | SDA              | GPIO 21                |
|     9 | OLED Display (SSD1306) | SCL              | GPIO 22                |
|    10 | OLED Display (SSD1306) | VCC              | 3.3V                   |
|    11 | OLED Display (SSD1306) | GND              | GND                    |
|    12 | Buzzer                 | Positive (+)     | GPIO 26                |
|    13 | Buzzer                 | Negative (–)     | GND                    |
|    14 | ESP32                  | USB Port         | Power Supply           |
*/
