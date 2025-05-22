#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>
#include <ESP8266.h>
#include <ArduinoJson.h>

#define DHTPIN 9
#define DHTTYPE DHT11
#define ONE_WIRE_BUS 10
#define TdsSensorPin A1
#define pHSense A0

#define VREF 5.0
#define SCOUNT 30

DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define ESP Serial1

const char* SSID = "MGP-RODRIGO";
const char* PASSWORD = "rodrigo2918";

int analogBuffer[SCOUNT];
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;

float waterTemp = 0, envTemperature = 0, envHumidity = 0, waterPh = 0, tdsValue = 0;

void setup() {
  Serial.begin(9600);
  ESP.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.print("Initializing...");

  dht.begin();
  sensors.begin();

  pinMode(TdsSensorPin, INPUT);

  ESP.println("AT");
  delay(1000);
  if (ESP.find("OK")) {
    lcd.setCursor(0, 1);
    lcd.print("ESP Connected");
  }

  connect_wifi();
}

void loop() {
  lcd.clear();
  show_dht();
  delay(2000);
  lcd.clear();
  show_ds18b20();
  delay(2000);
  lcd.clear();
  show_ph();
  delay(2000);
  lcd.clear();
  show_tds();
  delay(2000);

  send_water_quality();
  delay(1000);
  send_env_quality();
  delay(2000);
}

void show_ds18b20() {
  sensors.requestTemperatures();
  waterTemp = sensors.getTempCByIndex(0);
  lcd.setCursor(0, 0);
  lcd.print("Water Temp:");
  lcd.setCursor(0, 1);
  lcd.print(waterTemp);
  lcd.print(" C");
}

void show_dht() {
  envTemperature = dht.readTemperature();
  envHumidity = dht.readHumidity();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(envTemperature);
  lcd.print(" C");
  lcd.setCursor(0, 1);
  lcd.print("Humidity: ");
  lcd.print(envHumidity);
  lcd.print(" %");
}

float ph(float voltage) {
  return 7 + ((2.5 - voltage) / 0.18);
}

void show_ph() {
  long measurings = 0;
  for (int i = 0; i < 20; i++) {
    measurings += analogRead(pHSense);
    delay(10);
  }
  float voltage = 5.0 * measurings / 1024.0 / 20.0;
  waterPh = ph(voltage);
  lcd.setCursor(0, 0);
  lcd.print("pH: ");
  lcd.print(waterPh);
}

void show_tds() {
  for (int i = 0; i < SCOUNT; i++) {
    analogBuffer[i] = analogRead(TdsSensorPin);
    delay(30);
  }

  memcpy(analogBufferTemp, analogBuffer, sizeof(analogBuffer));
  float averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * VREF / 1024.0;
  float compensationCoefficient = 1.0 + 0.02 * (waterTemp - 25.0);
  float compensationVoltage = averageVoltage / compensationCoefficient;

  tdsValue = (133.42 * pow(compensationVoltage, 3)
              - 255.86 * pow(compensationVoltage, 2)
              + 857.39 * compensationVoltage)
             * 0.5;

  lcd.setCursor(0, 0);
  lcd.print("TDS: ");
  lcd.setCursor(0, 1);
  lcd.print(tdsValue);
  lcd.print(" ppm");
}

int getMedianNum(int bArray[], int iFilterLen) {
  for (int i = 0; i < iFilterLen - 1; i++) {
    for (int j = 0; j < iFilterLen - i - 1; j++) {
      if (bArray[j] > bArray[j + 1]) {
        int temp = bArray[j];
        bArray[j] = bArray[j + 1];
        bArray[j + 1] = temp;
      }
    }
  }
  if (iFilterLen % 2 == 0)
    return (bArray[iFilterLen / 2] + bArray[iFilterLen / 2 - 1]) / 2;
  else
    return bArray[iFilterLen / 2];
}

void send_water_quality() {
  StaticJsonDocument<256> doc;
  doc["tds"] = tdsValue;
  doc["temperature"] = waterTemp;
  doc["ph"] = waterPh;

  char payload[256];
  serializeJson(doc, payload);
  doc.clear();

  send_http("POST", "/waterQuality", payload);
}

void send_env_quality() {
  StaticJsonDocument<128> doc;
  doc["temperature"] = envTemperature;
  doc["humidity"] = envHumidity;

  char payload[128];
  serializeJson(doc, payload);

  send_http("POST", "/environmentQuality", payload);
}

void send_http(String method, String path, String body) {
  checkWifi();
  ESP.println("AT+CIPCLOSE");
  delay(1000);
  while (ESP.available()) ESP.read();  // Limpa buffer
  ESP.println("AT+CIPSTART=\"TCP\",\"192.168.0.65\",8080");
  delay(5000);
  String response = "";
  unsigned long timeout = millis();
  while (millis() - timeout < 5000) {
    while (ESP.available()) {
      char c = ESP.read();
      Serial.print(c);
      response += c;
      timeout = millis();  // Reinicia o timeout a cada dado recebido
    }
  }
  Serial.println(response);
  response.toUpperCase();
  if (response.indexOf("OK") == -1 && response.indexOf("ALREADY CONNECTED") == -1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("TCP Conn Failed");
    return;
  }

  String request = method + " " + path + " HTTP/1.1\r\n" + "Host: 192.168.0.65\r\n" + "Content-Type: application/json\r\n" + "Content-Length: " + body.length() + "\r\n\r\n" + body;

  Serial.println(method);
  Serial.println(path);
  Serial.println(body);

  ESP.print("AT+CIPSEND=");
  ESP.println(request.length());
  unsigned long startSend = millis();
  while (!ESP.find(">")) {
    if (millis() - startSend > 5000) {  // 5 segundos
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Send Fail >");
      return;
    }
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sending data");
  Serial.println("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  Serial.println("Enviando: ");
  Serial.println(request);
  ESP.print(request);
  String sendResponse = "";
  unsigned long startResponse = millis();
  while (millis() - startResponse < 5000) {
    while (ESP.available()) {
      char c = ESP.read();
      sendResponse += c;
    }

    if (sendResponse.indexOf("SEND OK") != -1) {
      break;
    }
  }

  if (sendResponse.indexOf("SEND OK") == -1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Send Error");
    Serial.println("Erro ao enviar:");
    Serial.println(sendResponse);
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sent OK");
  ESP.println("AT+CIPCLOSE");
  while (ESP.available()) ESP.read();
}

void connect_wifi() {
  while (ESP.available()) ESP.read();
  ESP.println("AT+RST");
  while (ESP.available()) ESP.read();
  delay(1000);
  ESP.println("AT+CWMODE=1");
  while (ESP.available()) ESP.read();
  delay(1000);
  ESP.print("AT+CWJAP=\"");
  ESP.print(SSID);
  ESP.print("\",\"");
  ESP.print(PASSWORD);
  ESP.println("\"");
  while (ESP.available()) ESP.read();
  delay(8000);
  ESP.println("AT+CIPMODE=0");
  while (ESP.available()) ESP.read();
  delay(1000);
  ESP.println("AT+CIPMUX=0");
  while (ESP.available()) ESP.read();
  delay(1000);
}

void checkWifi() {
  while (ESP.available()) ESP.read();
  ESP.println("AT+CWJAP?");
  delay(1000);
  String resposta = ESP.readString();
  lcd.clear();
  if (resposta.indexOf(SSID) >= 0) {
    lcd.setCursor(0, 0);
    lcd.print("WiFi: OK");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("WiFi: Reconnect");
    connect_wifi();
  }
  while (ESP.available()) ESP.read();
}
