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
#define API_KEY "xc"
#define DATABASE_URL "https://smart-shopping-cart-264b0-default-rtdb.firebaseio.com/"

#define USER_EMAIL "smartcart@gmail.com"
#define USER_PASSWORD "smartcart123"

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

  if(uid == lastUID) return;
  lastUID = uid;

  Serial.println("Scanned UID: " + uid);

  fetchProduct(uid);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(1500);
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

void fetchProduct(String uid){

  String name = "";
  float price = 0;

  // 🔍 RFID → Product mapping
  if(uid == "27D11300"){
    name = "Milk";
    price = 40;
  }
  else if(uid == "9C86AA00"){
    name = "Bread";
    price = 25;
  }
  else if(uid == "AA389604"){
    name = "Rice";
    price = 60;
  }
  else if(uid == "105BE93B"){
    name = "Soap";
    price = 30;
  }
  else{
    Serial.println("Unknown Product");

    u8g2.clearBuffer();
    u8g2.drawStr(10,30,"Unknown Product");
    u8g2.sendBuffer();

    beep(500);
    return;
  }

  // ✅ Add to total
  totalBill += price;

  // 🔥 Firebase paths
  String basePath = "/cart/" + uid;
  String namePath = basePath + "/name";
  String pricePath = basePath + "/price";
  String qtyPath = basePath + "/qty";

  // 🔄 Get current quantity
  int qty = 0;
  if(Firebase.RTDB.getInt(&fbdo, qtyPath)){
    qty = fbdo.intData();
  }

  qty++; // increase count

  // 🔥 Update Firebase
  Firebase.RTDB.setString(&fbdo, namePath, name);
  Firebase.RTDB.setFloat(&fbdo, pricePath, price);
  Firebase.RTDB.setInt(&fbdo, qtyPath, qty);

  Serial.println("Product: " + name);
  Serial.println("Qty: " + String(qty));
  Serial.println("Total: " + String(totalBill));

  // 🔔 Beep
  beep(200);

  // OLED DISPLAY
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);

  u8g2.drawStr(0,12,"Product:");
  u8g2.drawStr(0,28,name.c_str());

  String q = "Qty: " + String(qty);
  String t = "Total: Rs " + String(totalBill);

  u8g2.drawStr(0,46,q.c_str());
  u8g2.drawStr(0,62,t.c_str());

  // 🔔 Budget warning
  if(totalBill > budget){
    beep(1000);

    u8g2.clearBuffer();
    u8g2.drawStr(10,30,"BUDGET EXCEEDED!");
    u8g2.sendBuffer();

    delay(2000);
  }

  u8g2.sendBuffer();
}
