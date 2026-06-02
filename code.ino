/*
 * ПРОЕКТ: УМЕН ТЕРМОСТАТ ЗА ПОДОВО ОТОПЛЕНИЕ
 * ВЕРСИЯ: 7.0 - С РЕАЛНО ВРЕМЕ ОТ OPENWEATHERMAP
 * ПЛАТФОРМА: ESP32-C3
 */

#include <SPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "time.h"

#include "FreeSansBold32pt7b.h"
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

// =============================================
//   НАСТРОЙКИ
// =============================================
const char* ssid          = "ИМЕ_НА_МРЕЖАТА";
const char* password      = "ПАРОЛА";
const char* weatherApiKey = "ТВОЯ_КЛЮЧ_ТУК";

const char* ntpServer          = "pool.ntp.org";
const long  gmtOffset_sec      = 7200;
const int   daylightOffset_sec = 3600;

#define TFT_CS        7
#define TFT_RST       3
#define TFT_DC        2
#define TFT_MOSI      6
#define TFT_SCLK      4
#define ONE_WIRE_BUS  5
#define RELAY_PIN     10

// =============================================
//   ОБЕКТИ
// =============================================
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

GFXcanvas16 clockCanvas(70, 28);
GFXcanvas16 statusCanvas(55, 26);
GFXcanvas16 tempCanvas(220, 85);

// =============================================
//   ПРОМЕНЛИВИ
// =============================================
unsigned long lastClockUpdate   = 0;
unsigned long lastTempUpdate    = 0;
unsigned long lastWeatherUpdate = 0;

float  lastTemp       = -999.0;
bool   relayState     = false;
bool   isWiFiConnected = false;
String lastDisplayedDate = "";

String weatherCity    = "";
String weatherDesc    = "";
float  weatherTemp    = 0;
float  weatherTempMin = 0;
float  weatherTempMax = 0;
int    weatherId      = 800;
float  locationLat    = 0;
float  locationLon    = 0;

// =============================================
//   ИКОНКИ 16x16
// =============================================
const uint8_t iconSun[] = {
  0b00010000, 0b00001000,
  0b00000000, 0b00000000,
  0b00111000, 0b00011100,
  0b01111100, 0b00111110,
  0b11111110, 0b01111111,
  0b11111110, 0b01111111,
  0b01111100, 0b00111110,
  0b00111000, 0b00011100,
  0b00000000, 0b00000000,
  0b00010000, 0b00001000,
  0b00000000, 0b00000000,
  0b10000001, 0b10000001,
  0b01000010, 0b01000010,
  0b00000000, 0b00000000,
  0b01000010, 0b01000010,
  0b10000001, 0b10000001,
};

const uint8_t iconCloud[] = {
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
  0b00001111, 0b00000000,
  0b00011111, 0b10000000,
  0b00111111, 0b11000000,
  0b01111001, 0b11100000,
  0b11110000, 0b11110000,
  0b11100000, 0b01111000,
  0b11000000, 0b00111100,
  0b11111111, 0b11111110,
  0b01111111, 0b11111100,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
};

const uint8_t iconRain[] = {
  0b00000000, 0b00000000,
  0b00001111, 0b00000000,
  0b00011111, 0b10000000,
  0b00111111, 0b11000000,
  0b01111001, 0b11100000,
  0b11110000, 0b11110000,
  0b11111111, 0b11111110,
  0b01111111, 0b11111100,
  0b00000000, 0b00000000,
  0b01001001, 0b00100100,
  0b01001001, 0b00100100,
  0b00100100, 0b10010010,
  0b00100100, 0b10010010,
  0b00010010, 0b01001001,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
};

const uint8_t iconSnow[] = {
  0b00000100, 0b00100000,
  0b00001010, 0b01010000,
  0b00000100, 0b00100000,
  0b00100100, 0b00100100,
  0b00010000, 0b00001000,
  0b11111111, 0b11111111,
  0b00010000, 0b00001000,
  0b00100100, 0b00100100,
  0b00000100, 0b00100000,
  0b00001010, 0b01010000,
  0b00000100, 0b00100000,
  0b00000000, 0b00000000,
  0b00100100, 0b00100100,
  0b01010010, 0b01001010,
  0b00100100, 0b00100100,
  0b00000000, 0b00000000,
};

const uint8_t iconThunder[] = {
  0b00000000, 0b00000000,
  0b00001111, 0b00000000,
  0b00011111, 0b10000000,
  0b00111111, 0b11000000,
  0b11111111, 0b11111110,
  0b01111111, 0b11111100,
  0b00000000, 0b00000000,
  0b00001111, 0b00000000,
  0b00000111, 0b10000000,
  0b00001111, 0b00000000,
  0b00000111, 0b10000000,
  0b00000011, 0b11000000,
  0b00000001, 0b10000000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
};

const uint8_t iconFog[] = {
  0b00000000, 0b00000000,
  0b11111111, 0b11111110,
  0b00000000, 0b00000000,
  0b01111111, 0b11111100,
  0b00000000, 0b00000000,
  0b11111111, 0b11111110,
  0b00000000, 0b00000000,
  0b01111111, 0b11111100,
  0b00000000, 0b00000000,
  0b11111111, 0b11111110,
  0b00000000, 0b00000000,
  0b01111111, 0b11111100,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
  0b00000000, 0b00000000,
};

// =============================================
//   ИКОНА ПО WEATHER ID
// =============================================
void drawWeatherIcon(int x, int y, int wid) {
  const uint8_t* icon;
  uint16_t color;
  if      (wid >= 200 && wid < 300) { icon = iconThunder; color = 0xFFE0; }
  else if (wid >= 300 && wid < 600) { icon = iconRain;    color = 0x001F; }
  else if (wid >= 600 && wid < 700) { icon = iconSnow;    color = 0xAFFF; }
  else if (wid >= 700 && wid < 800) { icon = iconFog;     color = 0x8410; }
  else if (wid == 800)              { icon = iconSun;     color = 0xFFE0; }
  else                              { icon = iconCloud;   color = 0x8410; }
  tft.fillRect(x, y, 16, 16, ST77XX_WHITE);
  tft.drawBitmap(x, y, icon, 16, 16, color);
}

// =============================================
//   ЛОКАЦИЯ ПО IP
// =============================================
bool getLocationByIP() {
  HTTPClient http;
  http.begin("http://ip-api.com/json/");
  if (http.GET() != 200) { http.end(); return false; }
  DynamicJsonDocument doc(512);
  deserializeJson(doc, http.getString());
  http.end();
  weatherCity = doc["city"].as<String>();
  locationLat = doc["lat"].as<float>();
  locationLon = doc["lon"].as<float>();
  return true;
}

// =============================================
//   ВРЕМЕТО ОТ OPENWEATHERMAP
// =============================================
bool getWeather() {
  if (locationLat == 0 && locationLon == 0) return false;
  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/weather?lat=" +
               String(locationLat, 4) + "&lon=" + String(locationLon, 4) +
               "&appid=" + String(weatherApiKey) +
               "&units=metric&lang=en";
  http.begin(url);
  if (http.GET() != 200) { http.end(); return false; }
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, http.getString());
  http.end();
  weatherId      = doc["weather"][0]["id"].as<int>();
  weatherDesc    = doc["weather"][0]["description"].as<String>();
  weatherDesc[0] = toupper(weatherDesc[0]);
  weatherTemp    = doc["main"]["temp"].as<float>();
  weatherTempMin = doc["main"]["temp_min"].as<float>();
  weatherTempMax = doc["main"]["temp_max"].as<float>();
  return true;
}

// =============================================
//   ДОЛЕН ПАНЕЛ
// =============================================
void updateWeatherPanel() {
  tft.fillRect(3, 170, 314, 67, ST77XX_WHITE);
  drawWeatherIcon(10, 178, weatherId);
  tft.setFont(&FreeSansBold9pt7b);
  tft.setTextColor(0x18C3);
  tft.setCursor(32, 190);
  tft.print(weatherCity);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(ST77XX_BLACK);
  tft.setCursor(32, 205);
  tft.print(weatherDesc);
  tft.setTextColor(0x4208);
  tft.setCursor(32, 220);
  tft.print("Min: ");
  tft.print(weatherTempMin, 1);
  tft.print(" C   Max: ");
  tft.print(weatherTempMax, 1);
  tft.print(" C");
  tft.setFont(&FreeSansBold9pt7b);
  tft.setTextColor(ST77XX_ORANGE);
  tft.setCursor(32, 235);
  tft.print("Outside: ");
  tft.print(weatherTemp, 1);
  tft.print(" C");
}

// =============================================
//   ПОМОЩНИ ФУНКЦИИ
// =============================================
void drawRoundedCorners() {
  int r = 14;
  for (int x = 0; x < r; x++) {
    for (int y = 0; y < r; y++) {
      if ((r-x)*(r-x) + (r-y)*(r-y) > r*r) {
        tft.drawPixel(x,     y,     ST77XX_BLACK);
        tft.drawPixel(319-x, y,     ST77XX_BLACK);
        tft.drawPixel(x,     239-y, ST77XX_BLACK);
        tft.drawPixel(319-x, 239-y, ST77XX_BLACK);
      }
    }
  }
  tft.drawFastHLine(0,   0, 4, ST77XX_BLACK);
  tft.drawFastVLine(0,   0, 4, ST77XX_BLACK);
  tft.drawFastHLine(316, 0, 4, ST77XX_BLACK);
  tft.drawFastVLine(319, 0, 4, ST77XX_BLACK);
}

void drawWiFiIcon(int x, int y, bool connected) {
  tft.fillRect(x, y, 24, 18, ST77XX_WHITE);
  uint16_t color = connected ? ST77XX_BLACK : 0xC618;
  tft.drawFastHLine(x+4,  y+1,  16, color);
  tft.drawFastHLine(x+2,  y+2,  2,  color);
  tft.drawFastHLine(x+20, y+2,  2,  color);
  tft.drawPixel(x+1,  y+3, color);
  tft.drawPixel(x+22, y+3, color);
  tft.drawPixel(x,    y+4, color);
  tft.drawPixel(x+23, y+4, color);
  tft.drawFastHLine(x+7,  y+5,  10, color);
  tft.drawFastHLine(x+5,  y+6,  2,  color);
  tft.drawFastHLine(x+17, y+6,  2,  color);
  tft.drawPixel(x+4,  y+7, color);
  tft.drawPixel(x+19, y+7, color);
  tft.drawFastHLine(x+9,  y+9,  6,  color);
  tft.drawFastHLine(x+8,  y+10, 2,  color);
  tft.drawFastHLine(x+14, y+10, 2,  color);
  tft.fillRect(x+11, y+13, 2, 2, color);
  if (!connected) {
    tft.drawLine(x+2,  y+1,  x+21, y+15, ST77XX_RED);
    tft.drawLine(x+21, y+1,  x+2,  y+15, ST77XX_RED);
  }
}

void updateStatusButton(bool state) {
  statusCanvas.fillScreen(ST77XX_WHITE);
  if (state) {
    statusCanvas.fillRoundRect(0, 0, 55, 26, 6, 0x03E0);
    statusCanvas.setFont(&FreeSansBold9pt7b);
    statusCanvas.setTextColor(ST77XX_WHITE);
    statusCanvas.setCursor(13, 18);
    statusCanvas.print("ON");
  } else {
    statusCanvas.fillRoundRect(0, 0, 55, 26, 6, 0xF800);
    statusCanvas.setFont(&FreeSansBold9pt7b);
    statusCanvas.setTextColor(ST77XX_WHITE);
    statusCanvas.setCursor(10, 18);
    statusCanvas.print("OFF");
  }
  tft.drawRGBBitmap(215, 13, statusCanvas.getBuffer(), statusCanvas.width(), statusCanvas.height());
}

void drawThermometerIcon(int x, int y) {
  tft.fillRect(x-5, y-10, 25, 10, ST77XX_WHITE);
  uint16_t glassColor = 0x4208;
  uint16_t fluidColor = 0xF800;
  tft.drawCircle(x+8,    y+40, 9, glassColor);
  tft.drawRoundRect(x+4, y,    9, 36, 4, glassColor);
  tft.fillRect(x+5,      y+32, 7, 4, ST77XX_WHITE);
  tft.fillCircle(x+8,    y+40, 6, fluidColor);
  tft.fillRect(x+7,      y+15, 3, 22, fluidColor);
  tft.fillRect(x+7,      y+3,  3, 12, 0x7FFF);
}

// =============================================
//   SETUP
// =============================================
void setup(void) {
  tft.init(240, 320);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_WHITE);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  sensors.begin();

  tft.drawRoundRect(0, 0, 320, 240, 15, 0x18C3);
  tft.drawRoundRect(2, 2, 316, 236, 13, 0x18C3);
  tft.drawFastHLine(15,  48, 290, 0xD6BA);
  tft.drawFastHLine(15, 168, 290, 0xD6BA);

  drawRoundedCorners();
  drawWiFiIcon(282, 16, false);
  updateStatusButton(relayState);
  drawThermometerIcon(23, 90);

  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(0x18C3);
  tft.setCursor(20, 72);
  tft.print("Connecting to Wi-Fi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 25) {
    delay(1000);
    attempts++;
    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(20 + ((attempts % 15) * 12), 75 + ((attempts / 15) * 15));
    tft.print(".");
  }

  tft.fillRect(18, 55, 290, 50, ST77XX_WHITE);

  if (WiFi.status() == WL_CONNECTED) {
    isWiFiConnected = true;
    drawWiFiIcon(282, 16, true);
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(0x03E0);
    tft.setCursor(20, 72);
    tft.print("Getting location...");

    if (getLocationByIP()) {
      tft.fillRect(18, 55, 290, 25, ST77XX_WHITE);
      tft.setTextColor(0x03E0);
      tft.setCursor(20, 72);
      tft.print("Getting weather...");
      getWeather();
    }
    tft.fillRect(18, 55, 290, 25, ST77XX_WHITE);
  } else {
    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(20, 72);
    tft.print("Offline Mode (No WiFi)");
  }

  drawThermometerIcon(23, 90);
  updateWeatherPanel();
  drawRoundedCorners();
}

// =============================================
//   LOOP
// =============================================
void loop() {
  unsigned long currentMillis = millis();

  if (WiFi.status() == WL_CONNECTED) {
    if (!isWiFiConnected) {
      isWiFiConnected = true;
      drawWiFiIcon(282, 16, true);
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      getLocationByIP();
    }
  } else {
    if (isWiFiConnected) {
      isWiFiConnected = false;
      drawWiFiIcon(282, 16, false);
    }
  }

  if (currentMillis - lastClockUpdate >= 1000) {
    lastClockUpdate = currentMillis;
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      clockCanvas.fillScreen(ST77XX_WHITE);
      clockCanvas.setFont(&FreeSansBold12pt7b);
      clockCanvas.setTextColor(ST77XX_BLACK);
      clockCanvas.setCursor(0, 22);
      char timeHourMin[6];
      strftime(timeHourMin, sizeof(timeHourMin), "%H:%M", &timeinfo);
      clockCanvas.print(timeHourMin);
      tft.drawRGBBitmap(20, 13, clockCanvas.getBuffer(), clockCanvas.width(), clockCanvas.height());

      char dateBuffer[40];
      strftime(dateBuffer, sizeof(dateBuffer), "%A, %B %d", &timeinfo);
      String currentDate = String(dateBuffer);
      if (currentDate != lastDisplayedDate) {
        tft.fillRect(20, 55, 250, 22, ST77XX_WHITE);
        tft.setFont(&FreeSans9pt7b);
        tft.setTextColor(0x4208);
        tft.setCursor(20, 72);
        tft.print(currentDate);
        lastDisplayedDate = currentDate;
      }
    }
  }

  if (currentMillis - lastTempUpdate >= 2000) {
    lastTempUpdate = currentMillis;
    sensors.requestTemperatures();
    float currentTemp = sensors.getTempCByIndex(0);

    if (currentTemp != DEVICE_DISCONNECTED_C) {
      if (currentTemp < 26.0 && !relayState) {
        relayState = true;
        digitalWrite(RELAY_PIN, HIGH);
        updateStatusButton(relayState);
      } else if (currentTemp > 28.0 && relayState) {
        relayState = false;
        digitalWrite(RELAY_PIN, LOW);
        updateStatusButton(relayState);
      }

      if (currentTemp != lastTemp) {
        tempCanvas.fillScreen(ST77XX_WHITE);
        tempCanvas.setFont(&FreeSansBold32pt7b);
        tempCanvas.setTextSize(1);
        tempCanvas.setTextColor(0x02CD);
        tempCanvas.setCursor(0, 58);
        char tempStr[10];
        dtostrf(currentTemp, 4, 1, tempStr);
        String finalTempText = String(tempStr) + " C";
        finalTempText.trim();
        tempCanvas.print(finalTempText);
        tft.drawRGBBitmap(56, 85, tempCanvas.getBuffer(), tempCanvas.width(), tempCanvas.height());
        lastTemp = currentTemp;
      }
    } else {
      tft.fillRect(50, 85, 220, 85, ST77XX_WHITE);
      tft.setFont(&FreeSans9pt7b);
      tft.setTextColor(ST77XX_RED);
      tft.setCursor(55, 125);
      tft.print("Sensor Error!");
      digitalWrite(RELAY_PIN, LOW);
      lastTemp = -999.0;
    }
  }

  // Времето на всеки 30 минути
  if (currentMillis - lastWeatherUpdate >= 1800000 || lastWeatherUpdate == 0) {
    lastWeatherUpdate = currentMillis;
    if (isWiFiConnected && getWeather()) {
      updateWeatherPanel();
      drawRoundedCorners();
    }
  }
}
