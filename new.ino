/*************************************************
 * ESP32 Smart Energy Meter (OLED + Blynk + Preferences)
 *************************************************/

// ----------- BLYNK IOT DEFINITIONS -------------
#define BLYNK_TEMPLATE_ID "TMPL3Hilcw39d"
#define BLYNK_TEMPLATE_NAME "Smart Energy Meter"
#define BLYNK_AUTH_TOKEN "7K7mYz60cO6_FY5QpkAtXNZ5r4-9E1Iy"
#define BLYNK_PRINT Serial

// ------------- LIBRARIES -------------------
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "EmonLib.h"
#include <Preferences.h>

// ------------- WIFI ------------------------
char ssid[] = "SANAL";
char pass[] = "8547931827";

// ------------- PIN DEFINITIONS -------------
#define VOLTAGE_PIN 35
#define CURRENT_PIN 34
#define RELAY_PIN   33

// >>> MOST RELAY MODULES ARE ACTIVE-LOW <<<
// If your relay turns ON when GPIO is LOW, keep true.
// If your relay turns ON when GPIO is HIGH, set false.
#define RELAY_ACTIVE_LOW true

// ------------- OLED ------------------------
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ------------- OBJECTS ---------------------
EnergyMonitor emon;
Preferences prefs;
BlynkTimer timer;

// ------------- CALIBRATION -----------------
// float vCalibration = 220.0;
// float vCalibration = 40.0;
float vCalibration = 125.0;
float currCalibration = 60.6;

// ------------- BILL SETTINGS ---------------
const float RUPEES_PER_KWH = 6.0;

// ------------- VARIABLES -------------------
float kWh = 0.0;
float billAmount = 0.0;

unsigned long lastMillis = 0;
unsigned long lastSaveMs = 0;
const unsigned long SAVE_INTERVAL_MS = 60000;

bool screenToggle = false;
bool relayState = false; // false=OFF, true=ON

// ---------- helper: set relay ----------
void setRelay(bool on) {
  relayState = on;

  if (RELAY_ACTIVE_LOW) {
    digitalWrite(RELAY_PIN, on ? LOW : HIGH);
  } else {
    digitalWrite(RELAY_PIN, on ? HIGH : LOW);
  }

  // update Blynk button to match real state
  Blynk.virtualWrite(V4, relayState ? 1 : 0);

  Serial.print("Relay is now: ");
  Serial.println(relayState ? "ON" : "OFF");
}

// ---------- BILL ----------
float calculateBill(float unitsKWh) {
  return unitsKWh * RUPEES_PER_KWH;
}

// ---------- OLED DRAW ----------
static void drawOLED(float vrms, float irms, float pwrW, float kwh, float bill, bool relayOn) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);

  if (!screenToggle) {
    u8g2.drawStr(0, 12, "Smart Energy Meter");

    char line[28];
    snprintf(line, sizeof(line), "  V: %.1f V", vrms);
    u8g2.drawStr(0, 28, line);

    snprintf(line, sizeof(line), "  I: %.2f A", irms);
    u8g2.drawStr(0, 40, line);

    snprintf(line, sizeof(line), "  P: %.0f W", pwrW);
    u8g2.drawStr(0, 52, line);

    snprintf(line, sizeof(line), "  E: %.3f kWh", kwh);
    u8g2.drawStr(0, 64, line);
  } else {
    u8g2.drawStr(0, 12, "  Billing / Relay");

    char line[28];
    snprintf(line, sizeof(line), "  Bill: Rs %.2f", bill);
    u8g2.drawStr(0, 32, line);

    snprintf(line, sizeof(line), "  Rate: Rs %.2f/kWh", RUPEES_PER_KWH);
    u8g2.drawStr(0, 44, line);

    u8g2.drawStr(0, 64, relayOn ? "  Relay: ON" : "  Relay: OFF");
  }

  u8g2.sendBuffer();
  screenToggle = !screenToggle;
}

// ---------- MEASURE + SEND ----------
void measureAndSend() {
  emon.calcVI(20, 2000);

  float vrms = emon.Vrms;
  float irms = emon.Irms;
  float powerW = emon.apparentPower;

  unsigned long now = millis();
  unsigned long dt = now - lastMillis;
  lastMillis = now;

  kWh += powerW * (dt / 3600000000.0);
  billAmount = calculateBill(kWh);

  if (now - lastSaveMs >= SAVE_INTERVAL_MS) {
    prefs.putFloat("kwh", kWh);
    lastSaveMs = now;
  }

  drawOLED(vrms, irms, powerW, kWh, billAmount, relayState);

  Serial.print("Vrms: "); Serial.print(vrms, 2); Serial.print(" V");
  Serial.print("\tIrms: "); Serial.print(irms, 3); Serial.print(" A");
  Serial.print("\tP: "); Serial.print(powerW, 1); Serial.print(" W");
  Serial.print("\tkWh: "); Serial.print(kWh, 5);
  Serial.print("\tBill: "); Serial.println(billAmount, 2);

  Blynk.virtualWrite(V0, vrms);
  Blynk.virtualWrite(V1, irms);
  Blynk.virtualWrite(V2, powerW);
  Blynk.virtualWrite(V3, kWh);
  Blynk.virtualWrite(V5, billAmount);
}

// ---------- BLYNK RELAY ----------
BLYNK_WRITE(V4) {
  int v = param.asInt();     // 0 or 1 from app
  setRelay(v == 1);
}

// ---------- BLYNK RESET ENERGY ----------
BLYNK_WRITE(V6) {
  if (param.asInt() == 1) {
    kWh = 0.0;
    billAmount = 0.0;
    prefs.putFloat("kwh", kWh);
    Serial.println("Energy reset (V6).");
  }
}

// When device connects, sync widget state -> device,
// then push back actual relay state to app.
BLYNK_CONNECTED() {
  Blynk.syncVirtual(V4);
  Blynk.virtualWrite(V4, relayState ? 1 : 0);
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);

  // Set relay OFF at boot
  if (RELAY_ACTIVE_LOW) {
    digitalWrite(RELAY_PIN, HIGH);  // OFF
  } else {
    digitalWrite(RELAY_PIN, LOW);   // OFF
  }
  relayState = false;

  prefs.begin("energy", false);
  kWh = prefs.getFloat("kwh", 0.0);

  Wire.begin(21, 22);

  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(12, 28, "Smart Energy Meter");
  u8g2.drawStr(30, 44, "Starting...");
  u8g2.sendBuffer();
  delay(1200);

  analogReadResolution(12);
  analogSetPinAttenuation(VOLTAGE_PIN, ADC_11db);
  analogSetPinAttenuation(CURRENT_PIN, ADC_11db);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  emon.voltage(VOLTAGE_PIN, vCalibration, 1.7);
  emon.current(CURRENT_PIN, currCalibration);

  lastMillis = millis();
  lastSaveMs = millis();

  timer.setInterval(5000L, measureAndSend);
}

void loop() {
  Blynk.run();
  timer.run();
}
