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
#define WIFI_SSID "wifi name"
#define WIFI_PASSWORD "password"

// ================= FIREBASE =================
#define API_KEY "your api here"
#define DATABASE_URL "firebase project url"

#define USER_EMAIL "email id"
#define USER_PASSWORD "password"

// ================= RFID =================
#define SS_PIN 5
#define RST_PIN 27
MFRC522 rfid(SS_PIN, RST_PIN);

// ================= FIREBASE =================
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ================= VARIABLES =================
float totalBill = 0;
float budget = 500;
String lastUID = "";
unsigned long lastScanTime = 0;   // ✅ NEW

// ==================================================

void beep(int timeMs){
  digitalWrite(BUZZER_PIN, HIGH);
  delay(timeMs);
  digitalWrite(BUZZER_PIN, LOW);
}

// ==================================================

void setup() {

  Serial.begin(115200);

  SPI.begin();
  rfid.PCD_Init();

  pinMode(BUZZER_PIN, OUTPUT);

  // OLED
  Wire.begin(21,22);
  u8g2.begin();

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(15,30,"Smart Cart Ready");
  u8g2.sendBuffer();

  connectWiFi();
  connectFirebase();
}

// ==================================================

void loop() {

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = getUID();

  // ✅ FIX: allow same product after 2 seconds
  if(uid == lastUID && millis() - lastScanTime < 2000) return;

  lastUID = uid;
  lastScanTime = millis();

  Serial.println("Scanned UID: " + uid);

  processProduct(uid);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(500);  // smoother scanning
}

// ==================================================

void connectWiFi(){

  u8g2.clearBuffer();
  u8g2.drawStr(10,30,"Connecting WiFi");
  u8g2.sendBuffer();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while(WiFi.status()!=WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi Connected");

  u8g2.clearBuffer();
  u8g2.drawStr(20,30,"WiFi Connected");
  u8g2.sendBuffer();

  delay(1000);
}

// ==================================================

void connectFirebase(){

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.begin(&config,&auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Firebase Connected");
}

// ==================================================

String getUID(){

  String uid="";

  for(byte i=0;i<rfid.uid.size;i++){
    if(rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i],HEX);
  }

  uid.toUpperCase();
  return uid;
}

// ==================================================

void processProduct(String uid){

  String name = "";
  float price = 0;
  String link = "";

  if(uid == "27D11300"){
    name = "Milk";
    price = 40;
    link = "https://images.unsplash.com/photo-1563636619-e9143da7973b";
  }
  else if(uid == "9C86AA00"){
    name = "Bread";
    price = 25;
    link = "https://images.unsplash.com/photo-1608198093002-ad4e005484ec";
  }
  else if(uid == "AA389604"){
    name = "Rice";
    price = 60;
    link = "https://images.unsplash.com/photo-1586201375761-83865001e31c";
  }
  else if(uid == "105BE93B"){
    name = "Soap";
    price = 30;
    link = "https://images.unsplash.com/photo-1581578731548-c64695cc6952";
  }
  else{
    Serial.println("Unknown Product");

    u8g2.clearBuffer();
    u8g2.drawStr(10,30,"Unknown Product");
    u8g2.sendBuffer();

    beep(500);
    return;
  }

  // ================= FIREBASE =================
  String basePath = "/cart/" + uid;
  String qtyPath = basePath + "/qty";

  int qty = 0;

  if(Firebase.RTDB.getInt(&fbdo, qtyPath)){
    qty = fbdo.intData();
  }

  qty++;

  FirebaseJson json;
  json.set("name", name);
  json.set("price", price);
  json.set("qty", qty);
  json.set("link", link);

  if(Firebase.RTDB.setJSON(&fbdo, basePath, &json)){
    Serial.println("Firebase Updated");
  } else {
    Serial.println("Firebase Error: " + fbdo.errorReason());
  }

  // ================= BILL =================
  totalBill += price;

  Serial.println("Product: " + name);
  Serial.println("Qty: " + String(qty));
  Serial.println("Total: " + String(totalBill));

  // ================= BUZZER =================
  beep(200);

  // ================= OLED =================
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);

  u8g2.drawStr(0,12,"Product:");
  u8g2.drawStr(0,28,name.c_str());

  String q = "Qty: " + String(qty);
  String t = "Total: Rs " + String(totalBill);

  u8g2.drawStr(0,46,q.c_str());
  u8g2.drawStr(0,62,t.c_str());

  u8g2.sendBuffer();

  // ================= BUDGET ALERT =================
  if(totalBill > budget){
    beep(1000);

    u8g2.clearBuffer();
    u8g2.drawStr(5,30,"BUDGET EXCEEDED!");
    u8g2.sendBuffer();

    delay(2000);
  }
}
