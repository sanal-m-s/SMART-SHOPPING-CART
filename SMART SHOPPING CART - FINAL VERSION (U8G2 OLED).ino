/*
SMART SHOPPING CART

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

/*
 SMART SHOPPING CART - FINAL VERSION (U8G2 OLED)
*/

#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <U8g2lib.h>

// ================= OLED =================
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ================= BUZZER =================
#define BUZZER_PIN 26

// ================= WIFI =================
#define WIFI_SSID "SANAL"
#define WIFI_PASSWORD "8547931827"

// ================= FIREBASE =================
#define API_KEY "AIzaSyDjtLzC1gTg6hjnpNdsD7H1iq2tvyjUwPI"
#define DATABASE_URL "https://smart-shopping-cart-264b0-default-rtdb.firebaseio.com/"

#define USER_EMAIL "smartcart@gmail.com"
#define USER_PASSWORD "smartcart123"

// ================= RFID =================
#define SS_PIN 5
#define RST_PIN 27
MFRC522 rfid(SS_PIN, RST_PIN);

// ================= FIREBASE OBJECTS =================
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ================= VARIABLES =================
float totalBill = 0;
float budget = 500;
String lastUID = "";

// ==================================================

void setup() {

  Serial.begin(115200);

  SPI.begin();
  rfid.PCD_Init();

  pinMode(BUZZER_PIN, OUTPUT);

  // OLED Init
  Wire.begin(21, 22);
  u8g2.begin();

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(15, 30, "Smart Cart Ready");
  u8g2.sendBuffer();

  connectWiFi();
  connectFirebase();
}

// ==================================================

void loop() {

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = getUID();

  if (uid == lastUID) return;  // Prevent duplicate scan
  lastUID = uid;

  Serial.println("Scanned UID: " + uid);

  fetchProduct(uid);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(2000);
}

// ==================================================

void connectWiFi() {

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(10, 30, "Connecting WiFi...");
  u8g2.sendBuffer();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");

  u8g2.clearBuffer();
  u8g2.drawStr(20, 30, "WiFi Connected!");
  u8g2.sendBuffer();
  delay(1000);
}

// ==================================================

void connectFirebase() {

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Firebase Connected");
}

// ==================================================

String getUID() {

  String uid = "";

  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }

  uid.toUpperCase();
  return uid;
}

// ==================================================

void fetchProduct(String uid) {

  String pricePath = "/products/" + uid + "/price";
  String namePath  = "/products/" + uid + "/name";

  if (Firebase.RTDB.getFloat(&fbdo, pricePath)) {

    float price = fbdo.floatData();
    totalBill += price;

    Firebase.RTDB.getString(&fbdo, namePath);
    String name = fbdo.stringData();

    Serial.println("Product: " + name);
    Serial.println("Price: " + String(price));
    Serial.println("Total: " + String(totalBill));

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);

    u8g2.drawStr(0, 10, "Product:");
    u8g2.drawStr(0, 25, name.c_str());

    String priceText = "Price: Rs " + String(price);
    String totalText = "Total: Rs " + String(totalBill);

    u8g2.drawStr(0, 45, priceText.c_str());
    u8g2.drawStr(0, 60, totalText.c_str());

    if (totalBill > budget) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(1000);
      digitalWrite(BUZZER_PIN, LOW);
    }

    u8g2.sendBuffer();

  } else {

    Serial.println("Product Not Found");
    Serial.println(fbdo.errorReason());

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(10, 30, "Product Not Found");
    u8g2.sendBuffer();
  }
}
