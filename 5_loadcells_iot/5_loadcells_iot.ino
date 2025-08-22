//  read this ! : this code isn't work you need to create your own get_units_kg function

#include "HX711.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ------------------- PIN DEFINITIONS -------------------
#define LOADCELL_DOUT1 32
#define LOADCELL_SCK1 33
#define LOADCELL_DOUT2 25
#define LOADCELL_SCK2 26
#define LOADCELL_DOUT3 13
#define LOADCELL_SCK3 27
#define LOADCELL_DOUT4 14
#define LOADCELL_SCK4 12
#define LOADCELL_DOUT5 4
#define LOADCELL_SCK5 2

#define ONE_WIRE_BUS 15   // DS18B20 pin
#define BUTTON_PIN 5     // Button pin (use pull-down or pull-up)

// ------------------- WIFI -------------------
#define WIFI_SSID "Ok"
#define WIFI_PASSWORD "q12345678"

// ------------------- TELEGRAM -------------------
String BOT_TOKEN = "7991014450:AAGjCEUHqhbV-E9Q2YQP-ZdgmakDjq52550";
String CHAT_ID   = "7625451518";

// ------------------- THINGSBOARD -------------------
const char* tb_host = "demo.thingsboard.io";
String tb_token = "TKbxEl5Rfi6MgqqYBuIQ";

//offset 8388607
// ------------------- CALIBRATION FACTORS -------------------
// Each scale should have its own calibration + offset from your calibration sketch
#define CALIBRATION_FACTOR1 2379861.00
#define OFFSET1 8388607

#define CALIBRATION_FACTOR2 2379861.00
#define OFFSET2 8391000

#define CALIBRATION_FACTOR3 2379861.00
#define OFFSET3 8389000

#define CALIBRATION_FACTOR4 2379861.00
#define OFFSET4 8390500

#define CALIBRATION_FACTOR5 2379861.00
#define OFFSET5 8388000


// ------------------- OBJECTS -------------------
HX711 scale1, scale2, scale3, scale4, scale5;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// ------------------- VARIABLES -------------------
unsigned long lastSendTB = 0;  // Timer for ThingsBoard
const long intervalTB = 5000;  // 5 seconds

void setup() {
  Serial.begin(115200);

  // Init button
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Init HX711 scales
  scale1.begin(LOADCELL_DOUT1, LOADCELL_SCK1);
  scale1.set_scale(CALIBRATION_FACTOR1);
  scale1.set_offset(OFFSET1);

  scale2.begin(LOADCELL_DOUT2, LOADCELL_SCK2);
  scale2.set_scale(CALIBRATION_FACTOR2);
  scale2.set_offset(OFFSET2);

  scale3.begin(LOADCELL_DOUT3, LOADCELL_SCK3);
  scale3.set_scale(CALIBRATION_FACTOR3);
  scale3.set_offset(OFFSET3);

  scale4.begin(LOADCELL_DOUT4, LOADCELL_SCK4);
  scale4.set_scale(CALIBRATION_FACTOR4);
  scale4.set_offset(OFFSET4);

  scale5.begin(LOADCELL_DOUT5, LOADCELL_SCK5);
  scale5.set_scale(CALIBRATION_FACTOR5);
  scale5.set_offset(OFFSET5);

  // Init DS18B20
  sensors.begin();

  // Connect WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void loop() {
  // Read sensors
  float w1 = scale1.is_ready() ? scale1.get_units() : 0;
  float w2 = scale2.is_ready() ? scale2.get_units() : 0;
  float w3 = scale3.is_ready() ? scale3.get_units() : 0;
  float w4 = scale4.is_ready() ? scale4.get_units() : 0;
  float w5 = scale5.is_ready() ? scale5.get_units() : 0;

  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  // Print locally
  Serial.printf("Weights: %.2f, %.2f, %.2f, %.2f, %.2f | Temp: %.2f °C\n",
                w1, w2, w3, w4, w5, tempC);

  // ------------------- SEND TO THINGSBOARD EVERY 5s -------------------
  if (millis() - lastSendTB >= intervalTB) {
    lastSendTB = millis();
    sendToThingsBoard(w1, w2, w3, w4, w5, tempC);
  }

  Serial.println(digitalRead(BUTTON_PIN));
  // ------------------- SEND TO TELEGRAM IF BUTTON PRESSED -------------------
  if (digitalRead(BUTTON_PIN) == LOW) { // button pressed
    Serial.println("Press !!");
    sendToTelegram(w1, w2, w3, w4, w5, tempC);
    delay(1000); // debounce
  }
}

// ------------------- FUNCTIONS -------------------
void sendToTelegram(float w1, float w2, float w3, float w4, float w5, float temp) {
  String message = "Load Cells:\n"
                   "1: " + String(w1) + " g\n" +
                   "2: " + String(w2) + " g\n" +
                   "3: " + String(w3) + " g\n" +
                   "4: " + String(w4) + " g\n" +
                   "5: " + String(w5) + " g\n" +
                   "Temp: " + String(temp) + " °C";

  sendMessage(message);  // use the helper function
}

void sendMessage(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure secureClient;
    secureClient.setInsecure();  // allow insecure TLS

    HTTPClient http;
    if (http.begin(secureClient, "https://api.telegram.org/bot" + BOT_TOKEN + "/sendMessage")) {
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      String postData = "chat_id=" + CHAT_ID + "&text=" + message;
      int httpResponseCode = http.POST(postData);

      if (httpResponseCode > 0) {
        Serial.println("Telegram message sent. Code: " + String(httpResponseCode));
      } else {
        Serial.println("Telegram send failed. Code: " + String(httpResponseCode));
      }

      http.end();
    } else {
      Serial.println("Failed to connect to Telegram API");
    }
  }
}


void sendToThingsBoard(float w1, float w2, float w3, float w4, float w5, float temp) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    if (client.connect(tb_host, 80)) {
      String payload = "{";
      payload += "\"weight1\":" + String(w1) + ",";
      payload += "\"weight2\":" + String(w2) + ",";
      payload += "\"weight3\":" + String(w3) + ",";
      payload += "\"weight4\":" + String(w4) + ",";
      payload += "\"weight5\":" + String(w5) + ",";
      payload += "\"temperature\":" + String(temp);
      payload += "}";

      String request = String("POST /api/v1/") + tb_token + "/telemetry HTTP/1.1\r\n";
      request += "Host: " + String(tb_host) + "\r\n";
      request += "Content-Type: application/json\r\n";
      request += "Content-Length: " + String(payload.length()) + "\r\n\r\n";
      request += payload;

      client.print(request);
      delay(10);
      Serial.println("Data sent to ThingsBoard: " + payload);
    }
  }
}
