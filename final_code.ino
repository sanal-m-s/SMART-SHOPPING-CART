/*************************************************
 * ESP32 Smart Energy Meter
 * OLED + Blynk + Relay + Email Alert + Bill Prediction
 *************************************************/

// ----------- BLYNK -----------------
#define BLYNK_TEMPLATE_ID "TMPL3Hilcw39d"
#define BLYNK_TEMPLATE_NAME "Smart Energy Meter"
#define BLYNK_AUTH_TOKEN "7K7mYz60cO6_FY5QpkAtXNZ5r4-9E1Iy"
#define BLYNK_PRINT Serial

// ----------- LIBRARIES -------------
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "EmonLib.h"
#include <Preferences.h>

// ----------- WIFI ------------------
char ssid[] = "SANAL";
char pass[] = "8547931827";

// ----------- PINS ------------------
#define VOLTAGE_PIN 35
#define CURRENT_PIN 34
#define RELAY_PIN   33
#define RELAY_ACTIVE_LOW true

// ----------- OLED ------------------
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ----------- OBJECTS ---------------
EnergyMonitor emon;
Preferences prefs;
BlynkTimer timer;

// ----------- CALIBRATION -----------
float vCalibration = 125.0;
float currCalibration = 60.6;

// ----------- BILL SETTINGS ---------
const float RUPEES_PER_KWH = 6.0;

// ----------- VARIABLES -------------
float kWh = 0.0;
float billAmount = 0.0;
float predictedBill = 0.0;

unsigned long startTime;
unsigned long lastMillis = 0;
unsigned long lastSaveMs = 0;

const unsigned long SAVE_INTERVAL_MS = 60000;

bool screenToggle = false;
bool relayState = false;
bool emailSent = false;

// ----------- RELAY CONTROL ---------
void setRelay(bool on)
{
  relayState = on;

  if (RELAY_ACTIVE_LOW)
    digitalWrite(RELAY_PIN, on ? LOW : HIGH);
  else
    digitalWrite(RELAY_PIN, on ? HIGH : LOW);

  Blynk.virtualWrite(V4, relayState ? 1 : 0);
}

// ----------- BILL CALCULATION ------
float calculateBill(float unitsKWh)
{
  return unitsKWh * RUPEES_PER_KWH;
}

// ----------- OLED DISPLAY ----------
void drawOLED(float vrms, float irms, float pwrW, float kwh, float bill)
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);

  if (!screenToggle)
  {
    char line[30];

    u8g2.drawStr(0,12," Smart Energy Meter");

    sprintf(line," V: %.1f V",vrms);
    u8g2.drawStr(0,28,line);

    sprintf(line," I: %.2f A",irms);
    u8g2.drawStr(0,40,line);

    sprintf(line," P: %.0f W",pwrW);
    u8g2.drawStr(0,52,line);

    sprintf(line," E: %.3f kWh",kwh);
    u8g2.drawStr(0,64,line);
  }
  else
  {
    char line[30];

    u8g2.drawStr(0,12," Billing");

    sprintf(line," Bill: Rs %.2f",bill);
    u8g2.drawStr(0,32,line);

    sprintf(line," Rate: Rs %.2f/kWh",RUPEES_PER_KWH);
    u8g2.drawStr(0,44,line);

    u8g2.drawStr(0,64, relayState ? " Relay: ON" : "Relay: OFF");
  }

  u8g2.sendBuffer();
  screenToggle = !screenToggle;
}

// ----------- MAIN MEASUREMENT ------
void measureAndSend()
{
  emon.calcVI(20,2000);

  float vrms = emon.Vrms;
  float irms = emon.Irms;
  float powerW = emon.apparentPower;

  unsigned long now = millis();
  unsigned long dt = now - lastMillis;
  lastMillis = now;

  // Energy calculation
  kWh += powerW * (dt / 3600000000.0);

  billAmount = calculateBill(kWh);

  // -------- PREDICT 30 DAY BILL --------
  float daysPassed = (millis() - startTime) / 86400000.0;

  if(daysPassed >= 1)
  {
    predictedBill = billAmount * (30.0 / daysPassed);
  }
  else
  {
    predictedBill = billAmount * 30.0;
  }

  // -------- EMAIL ALERT --------
  if(billAmount > 200 && !emailSent)
  {
    Blynk.logEvent("bill_alert","Electricity bill exceeded Rs 200");
    emailSent = true;
  }

  // -------- SAVE ENERGY --------
  if(now - lastSaveMs >= SAVE_INTERVAL_MS)
  {
    prefs.putFloat("kwh",kWh);
    lastSaveMs = now;
  }

  // -------- OLED --------
  drawOLED(vrms,irms,powerW,kWh,billAmount);

  // -------- SERIAL --------
  Serial.print("Voltage: ");
  Serial.print(vrms);

  Serial.print(" Current: ");
  Serial.print(irms);

  Serial.print(" Power: ");
  Serial.print(powerW);

  Serial.print(" kWh: ");
  Serial.print(kWh);

  Serial.print(" Bill: ");
  Serial.print(billAmount);

  Serial.print(" Predicted: ");
  Serial.println(predictedBill);

  // -------- BLYNK SEND --------
  Blynk.virtualWrite(V0,vrms);
  Blynk.virtualWrite(V1,irms);
  Blynk.virtualWrite(V2,powerW);
  Blynk.virtualWrite(V3,kWh);
  Blynk.virtualWrite(V5,billAmount);
  Blynk.virtualWrite(V7,predictedBill);
}

// -------- RELAY BUTTON --------
BLYNK_WRITE(V4)
{
  int v = param.asInt();
  setRelay(v == 1);
}

// -------- RESET ENERGY --------
BLYNK_WRITE(V6)
{
  if(param.asInt()==1)
  {
    kWh = 0;
    billAmount = 0;
    predictedBill = 0;

    prefs.putFloat("kwh",kWh);

    emailSent = false;
    startTime = millis();
  }
}

// -------- CONNECTED --------
BLYNK_CONNECTED()
{
  Blynk.syncVirtual(V4);
}

// -------- SETUP --------------
void setup()
{
  Serial.begin(115200);

  pinMode(RELAY_PIN,OUTPUT);

  if(RELAY_ACTIVE_LOW)
    digitalWrite(RELAY_PIN,HIGH);
  else
    digitalWrite(RELAY_PIN,LOW);

  prefs.begin("energy",false);
  kWh = prefs.getFloat("kwh",0);

  Wire.begin(21,22);
  u8g2.begin();

  analogReadResolution(12);
  analogSetPinAttenuation(VOLTAGE_PIN,ADC_11db);
  analogSetPinAttenuation(CURRENT_PIN,ADC_11db);

  Blynk.begin(BLYNK_AUTH_TOKEN,ssid,pass);

  emon.voltage(VOLTAGE_PIN,vCalibration,1.7);
  emon.current(CURRENT_PIN,currCalibration);

  startTime = millis();

  lastMillis = millis();
  lastSaveMs = millis();

  timer.setInterval(5000L,measureAndSend);
}

// -------- LOOP --------------
void loop()
{
  Blynk.run();
  timer.run();
}
