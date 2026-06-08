/*
 * ПРОЕКТ: УМЕН IoT ТЕРМОСТАТ ЗА ПОДОВО ОТОПЛЕНИЕ
 * ВЕРСИЯ: 9.2 - НЕБЛОКИРАЩ МРЕЖЕВИ СТАРТ
 * ПЛАТФОРМА: ESP32-C3
 */

#define BLYNK_TEMPLATE_ID "TMPL4oR-sch3w"
#define BLYNK_TEMPLATE_NAME "Floor Heating"
#define BLYNK_AUTH_TOKEN "********" // Сложи твоя BLYNK API тук

#include <SPI.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "time.h"

#include "FreeSansBold32pt7b.h"
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

// НАСТРОЙКИ НА МРЕЖАТА
const char* ssid          = "ИМЕ_НА_МРЕЖАТА"; // Сложи името на твоята мрежа тук
const char* password      = "ПАРОЛА"; // Сложи паролата на твоята мрежа тук
const char* weatherApiKey = "ТВОЯ_КЛЮЧ_ТУК"; // Сложи твоя OpenWeatherMap API тук

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

#define BTN_MANUAL      0   
#define BTN_EMERGENCY   1   

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

GFXcanvas16 clockCanvas(140, 42);
GFXcanvas16 statusCanvas(55, 26);
GFXcanvas16 tempCanvas(220, 70);

unsigned long lastClockUpdate   = 0;
unsigned long lastTempUpdate    = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long lastBtnManual      = 0;
unsigned long lastBtnEmergency  = 0;

float   lastTemp        = -999.0;
bool    relayState      = false;
bool    isWiFiConnected = false;
bool    manualOff       = false; 
bool    emergencyOff    = false; 
String lastDisplayedDate = "";

float targetTemp = 25.0; 

String weatherCity    = "";
String weatherDesc    = "";
float  weatherTemp    = 0;
float  weatherTempMin = 0;
float  weatherTempMax = 0;
int    weatherId      = 800;
float  locationLat    = 0;
float  locationLon    = 0;

// ИКОНКИ
const uint8_t iconSun[] = { 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x03, 0x18, 0xc0, 0x03, 0xff, 0xc0, 0x01, 0xff, 0x80, 0x0f, 0xff, 0xf0, 0x1f, 0x7e, 0xf8, 0x3e, 0x3c, 0x7c, 0x3c, 0x00, 0x3c, 0x7c, 0x00, 0x3e, 0xff, 0x00, 0xff, 0xff, 0x00, 0xff, 0x7c, 0x00, 0x3e, 0x3c, 0x00, 0x3c, 0x3e, 0x3c, 0x7c, 0x1f, 0x7e, 0xf8, 0x0f, 0xff, 0xf0, 0x01, 0xff, 0x80, 0x03, 0xff, 0xc0, 0x03, 0x18, 0xc0, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t iconCloud[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0xff, 0x80, 0x01, 0xff, 0xc0, 0x03, 0xe3, 0xe0, 0x07, 0x80, 0xf0, 0x0f, 0x00, 0x78, 0x1e, 0x00, 0x3c, 0x3c, 0x00, 0x3e, 0x38, 0x00, 0x1e, 0x70, 0x00, 0x0e, 0x60, 0x0f, 0x8e, 0xe0, 0x3f, 0xc7, 0xc0, 0x7f, 0xe3, 0xc0, 0xff, 0xe3, 0xc1, 0xff, 0xe3, 0xc1, 0xf0, 0x63, 0xc0, 0x00, 0x03, 0x7f, 0xff, 0xff, 0x7f, 0xff, 0xff, 0x3f, 0xff, 0xfe, 0x00, 0x00, 0x00 };
const uint8_t iconRain[] = { 0x00, 0x7c, 0x00, 0x01, 0xff, 0x80, 0x03, 0xff, 0xc0, 0x07, 0x81, 0xe0, 0x0e, 0x00, 0xf0, 0x1c, 0x00, 0x70, 0x18, 0x07, 0x38, 0x38, 0x1f, 0xbc, 0x30, 0x3f, 0x9c, 0x30, 0x7f, 0x8c, 0x30, 0x7f, 0x8c, 0x30, 0x7f, 0xce, 0x18, 0x33, 0xee, 0x1c, 0x00, 0x7e, 0x0f, 0xff, 0xfc, 0x07, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x24, 0x49, 0x12, 0x24, 0x49, 0x12, 0x48, 0x92, 0x24, 0x48, 0x92, 0x24, 0x12, 0x24, 0x48, 0x12, 0x24, 0x48, 0x00, 0x00, 0x00 };
const uint8_t iconSnow[] = { 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x14, 0x18, 0x28, 0x0c, 0x18, 0x30, 0x03, 0x18, 0xc0, 0x00, 0xff, 0x00, 0x39, 0x18, 0x9c, 0x1c, 0x18, 0x38, 0x00, 0x18, 0x00, 0x7f, 0xff, 0xfe, 0x7f, 0xff, 0xfe, 0x00, 0x18, 0x00, 0x1c, 0x18, 0x38, 0x39, 0x18, 0x9c, 0x00, 0xff, 0x00, 0x03, 0x18, 0xc0, 0x0c, 0x18, 0x30, 0x14, 0x18, 0x28, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t iconThunder[] = { 0x00, 0x7c, 0x00, 0x01, 0xff, 0x80, 0x03, 0xff, 0xc0, 0x07, 0x81, 0xe0, 0x0e, 0x00, 0xf0, 0x1c, 0x00, 0x70, 0x18, 0x07, 0x38, 0x38, 0x1f, 0xbc, 0x30, 0x3f, 0x9c, 0x30, 0x7f, 0x8c, 0x30, 0x7f, 0xce, 0x18, 0x33, 0xee, 0x1c, 0x00, 0x7e, 0x0f, 0xff, 0xfc, 0x07, 0xff, 0xf0, 0x00, 0x1c, 0x00, 0x00, 0x38, 0x00, 0x00, 0x70, 0x00, 0x00, 0xfe, 0x00, 0x01, 0xfc, 0x00, 0x00, 0x38, 0x00, 0x00, 0x70, 0x00, 0x00, 0xe0, 0x00, 0x01, 0xc0, 0x00 };
const uint8_t iconFog[] = { 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfc, 0x3f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xf0, 0x0f, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xfe, 0x7f, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xf8, 0x1f, 0xff, 0xf8, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x07, 0xff, 0xe0, 0x07, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfc, 0x3f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

void drawWeatherIcon(int x, int y, int wid) {
  const uint8_t* icon; uint16_t color;
  if      (wid >= 200 && wid < 300) { icon = iconThunder; color = 0x02CD; }
  else if (wid >= 300 && wid < 600) { icon = iconRain;    color = 0x001F; }
  else if (wid >= 600 && wid < 700) { icon = iconSnow;    color = 0x001F; }
  else if (wid >= 700 && wid < 800) { icon = iconFog;     color = 0x8410; }
  else if (wid == 800)              { icon = iconSun;     color = ST77XX_ORANGE; }
  else                              { icon = iconCloud;   color = 0x8410; }
  tft.fillRect(x, y, 24, 24, ST77XX_WHITE); tft.drawBitmap(x, y, icon, 24, 24, color);
}

void updateStatusButton() {
  statusCanvas.fillScreen(ST77XX_WHITE);
  if (emergencyOff) {
    statusCanvas.fillRoundRect(0, 0, 55, 26, 6, 0x8410); statusCanvas.setFont(&FreeSansBold9pt7b);
    statusCanvas.setTextColor(ST77XX_WHITE); statusCanvas.setCursor(4, 18); statusCanvas.print("EMRG");
  } else if (manualOff) {
    statusCanvas.fillRoundRect(0, 0, 55, 26, 6, ST77XX_ORANGE); statusCanvas.setFont(&FreeSansBold9pt7b);
    statusCanvas.setTextColor(ST77XX_WHITE); statusCanvas.setCursor(4, 18); statusCanvas.print("MAN");
  } else if (relayState) {
    statusCanvas.fillRoundRect(0, 0, 55, 26, 6, 0x03E0); statusCanvas.setFont(&FreeSansBold9pt7b);
    statusCanvas.setTextColor(ST77XX_WHITE); statusCanvas.setCursor(13, 18); statusCanvas.print("ON");
  } else {
    statusCanvas.fillRoundRect(0, 0, 55, 26, 6, 0xF800); statusCanvas.setFont(&FreeSansBold9pt7b);
    statusCanvas.setTextColor(ST77XX_WHITE); statusCanvas.setCursor(10, 18); statusCanvas.print("OFF");
  }
  tft.drawRGBBitmap(215, 13, statusCanvas.getBuffer(), statusCanvas.width(), statusCanvas.height());
}

BLYNK_WRITE(V1) { int remoteState = param.asInt(); if (!emergencyOff) { manualOff = (remoteState == 0); updateStatusButton(); lastTemp = -999.0; } }
BLYNK_WRITE(V2) { targetTemp = param.asFloat(); lastTemp = -999.0; }
BLYNK_CONNECTED() { Blynk.syncVirtual(V1); Blynk.syncVirtual(V2); }

void thermostatLogic(float currentTemp) {
  if (emergencyOff || manualOff) {
    digitalWrite(RELAY_PIN, LOW); if(relayState) { relayState = false; updateStatusButton(); } return;
  }
  if (currentTemp < (targetTemp - 0.5) && !relayState) {
    relayState = true; digitalWrite(RELAY_PIN, HIGH); updateStatusButton();
  } else if (currentTemp > (targetTemp + 0.5) && relayState) {
    relayState = false; digitalWrite(RELAY_PIN, LOW); updateStatusButton();
  }
}

void checkButtons() {
  unsigned long now = millis();
  if (digitalRead(BTN_MANUAL) == LOW && now - lastBtnManual > 400) {
    lastBtnManual = now; if (emergencyOff) return;
    manualOff = !manualOff; Blynk.virtualWrite(V1, manualOff ? 0 : 1); updateStatusButton(); lastTemp = -999.0;
  }
  if (digitalRead(BTN_EMERGENCY) == LOW && now - lastBtnEmergency > 400) {
    lastBtnEmergency = now; emergencyOff = !emergencyOff;
    if (emergencyOff) { manualOff = false; Blynk.virtualWrite(V1, 0); } else { Blynk.virtualWrite(V1, 1); }
    updateStatusButton(); lastTemp = -999.0;
  }
}

bool getLocationByIP() {
  HTTPClient http; http.begin("http://ip-api.com/json/"); if (http.GET() != 200) { http.end(); return false; }
  DynamicJsonDocument doc(512); deserializeJson(doc, http.getString()); http.end();
  weatherCity = doc["city"].as<String>(); locationLat = doc["lat"].as<float>(); locationLon = doc["lon"].as<float>();
  return true;
}

bool getWeather() {
  if (locationLat == 0 && locationLon == 0) return false;
  HTTPClient http; String url = "http://api.openweathermap.org/data/2.5/weather?lat=" + String(locationLat, 4) + "&lon=" + String(locationLon, 4) + "&appid=" + String(weatherApiKey) + "&units=metric&lang=en";
  http.begin(url); if (http.GET() != 200) { http.end(); return false; }
  DynamicJsonDocument doc(1024); deserializeJson(doc, http.getString()); http.end();
  weatherId = doc["weather"][0]["id"].as<int>(); weatherDesc = doc["weather"][0]["description"].as<String>();
  weatherDesc[0] = toupper(weatherDesc[0]); weatherTemp = doc["main"]["temp"].as<float>();
  weatherTempMin = doc["main"]["temp_min"].as<float>(); weatherTempMax = doc["main"]["temp_max"].as<float>();
  return true;
}

void updateWeatherPanel() {
  tft.fillRect(3, 170, 314, 67, ST77XX_WHITE); drawWeatherIcon(8, 174, weatherId);
  tft.setFont(&FreeSansBold9pt7b); tft.setTextColor(0x18C3); tft.setCursor(42, 190); tft.print(weatherCity);
  tft.setFont(&FreeSans9pt7b); tft.setTextColor(ST77XX_BLACK); tft.setCursor(42, 208); tft.print(weatherDesc);
  tft.setFont(&FreeSansBold12pt7b); tft.setTextColor(ST77XX_ORANGE); tft.setCursor(42, 233); tft.print("Outside: "); tft.print(weatherTemp, 1); tft.print(" C");
}

void drawRoundedCorners() {
  int r = 14;
  for (int x = 0; x < r; x++) {
    for (int y = 0; y < r; y++) {
      if ((r-x)*(r-x) + (r-y)*(r-y) > r*r) {
        tft.drawPixel(x, y, ST77XX_BLACK); tft.drawPixel(319-x, y, ST77XX_BLACK);
        tft.drawPixel(x, 239-y, ST77XX_BLACK); tft.drawPixel(319-x, 239-y, ST77XX_BLACK);
      }
    }
  }
  tft.drawFastHLine(0, 0, 4, ST77XX_BLACK); tft.drawFastVLine(0, 0, 4, ST77XX_BLACK);
  tft.drawFastHLine(316, 0, 4, ST77XX_BLACK); tft.drawFastVLine(319, 0, 4, ST77XX_BLACK);
}

void drawWiFiIcon(int x, int y, bool connected) {
  tft.fillRect(x, y, 24, 18, ST77XX_WHITE); uint16_t color = connected ? ST77XX_BLACK : 0xC618;
  tft.drawFastHLine(x+4, y+1, 16, color); tft.drawFastHLine(x+2, y+2, 2, color); tft.drawFastHLine(x+20, y+2, 2, color);
  tft.drawPixel(x+1, y+3, color); tft.drawPixel(x+22, y+3, color); tft.drawPixel(x, y+4, color); tft.drawPixel(x+23, y+4, color);
  tft.drawFastHLine(x+7, y+5, 10, color); tft.drawFastHLine(x+5, y+6, 2, color); tft.drawFastHLine(x+17, y+6, 2, color);
  tft.drawPixel(x+4, y+7, color); tft.drawPixel(x+19, y+7, color); tft.drawFastHLine(x+9, y+9, 6, color);
  tft.drawFastHLine(x+8, y+10, 2, color); tft.drawFastHLine(x+14, y+10, 2, color); tft.fillRect(x+11, y+13, 2, 2, color);
  if (!connected) { tft.drawLine(x+2, y+1, x+21, y+15, ST77XX_RED); tft.drawLine(x+21, y+1, x+2, y+15, ST77XX_RED); }
}

void drawThermometerIcon(int x, int y) {
  tft.fillRect(x-5, y-10, 25, 10, ST77XX_WHITE); uint16_t glassColor = 0x4208; uint16_t fluidColor = 0xF800;
  tft.drawCircle(x+8, y+40, 9, glassColor); tft.drawRoundRect(x+4, y, 9, 36, 4, glassColor);
  tft.fillRect(x+5, y+32, 7, 4, ST77XX_WHITE); tft.fillCircle(x+8, y+40, 6, fluidColor);
  tft.fillRect(x+7, y+15, 3, 22, fluidColor); tft.fillRect(x+7, y+3, 3, 12, 0x7FFF);
}

// =============================================
//   SETUP (БЕЗ ЗАКЛЕСУВАНЕ)
// =============================================
void setup(void) {
  Serial.begin(115200); 
  
  // Даваме твърди 3 секунди на Windows да отвори порта преди изобщо да почнем!
  delay(3000); 
  Serial.println("\n=== МИКРОПРОЦЕСОРЪТ СТАРТИРА УСПЕШНО ===");

  tft.init(240, 320);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_WHITE);

  pinMode(RELAY_PIN,     OUTPUT);
  pinMode(BTN_MANUAL,    INPUT_PULLUP);
  pinMode(BTN_EMERGENCY, INPUT_PULLUP);
  digitalWrite(RELAY_PIN, LOW);
  sensors.begin();

  tft.drawRoundRect(0, 0, 320, 240, 15, 0x18C3);
  tft.drawRoundRect(2, 2, 316, 236, 13, 0x18C3);
  tft.drawFastHLine(15,  48, 290, 0xD6BA);
  tft.drawFastHLine(15, 168, 290, 0xD6BA);

  drawRoundedCorners();
  drawWiFiIcon(282, 16, false);
  updateStatusButton();
  drawThermometerIcon(23, 100);

  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(0x18C3);
  tft.setCursor(20, 72);
  tft.print("Connecting via Blynk...");

  Serial.println("[МРЕЖА] Стартирам Wi-Fi мениджър...");
  
  Serial.print("[Wi-Fi] Свързвам се към рутер: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // Проверяваме връзката точно 15 пъти. Ако няма рутер - продължаваме напред, за да работи термостата!
  int wifiTimeout = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTimeout < 15) {
    delay(500);
    Serial.print(".");
    wifiTimeout++;
  }
  Serial.println("");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[Wi-Fi] УСПЕШНО СВЪРЗВАНЕ!");
    Serial.print("[Wi-Fi] IP Адрес: ");
    Serial.println(WiFi.localIP());
    
    isWiFiConnected = true;
    drawWiFiIcon(282, 16, true);
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    // НЕБЛОКИРАЩ СТАРТ: Казваме на Blynk само токена, без да го пускаме да забива рутера
    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk.connect(3000);  // 3000ms timeout - неблокиращо
    Serial.println("[BLYNK] connect() извикан");
    Serial.print("[BLYNK] Свързан: ");
    Serial.println(Blynk.connected() ? "ДА" : "НЕ");

    Serial.println("[API] Вземам локация...");
    if (getLocationByIP()) {
      Serial.println("[API] Локацията е намерена. Вземам времето...");
      getWeather();
    }
  } else {
    Serial.println("[ГРЕШКА] Рутерът не отговори! Системата преминава в АВТОНОМЕН режим.");
  }

  // Почистваме екрана от сервизните съобщения
  tft.fillRect(18, 55, 290, 40, ST77XX_WHITE);
  drawThermometerIcon(23, 100);
  updateWeatherPanel();
  drawRoundedCorners();
  
  Serial.println("=== ИНИЦИАЛИЗАЦИЯТА ЗАВЪРШИ ===");
}

// =============================================
//   LOOP
// =============================================
void loop() {
  static unsigned long lastBlynkCheck = 0;
  if (millis() - lastBlynkCheck > 5000) {
    lastBlynkCheck = millis();
    Serial.print("[BLYNK] Статус: ");
    Serial.println(Blynk.connected() ? "ONLINE" : "OFFLINE");
  }
  
  
  // Проверяваме Blynk само ако реално имаме Wi-Fi връзка
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.run();
    if (!isWiFiConnected) {
      isWiFiConnected = true;
      drawWiFiIcon(282, 16, true);
      Blynk.connect(3000);  // Реконект при възстановена WiFi връзка
    }
  } else {
    if (isWiFiConnected) {
      isWiFiConnected = false;
      drawWiFiIcon(282, 16, false);
    }
  }
  
  unsigned long currentMillis = millis();
  checkButtons();

  // 1. ОБНОВЯВАНЕ НА ЧАСОВНИКА (Само при наличен Wi-Fi)
  if (currentMillis - lastClockUpdate >= 1000) {
    lastClockUpdate = currentMillis;
    struct tm timeinfo;
    if (isWiFiConnected && getLocalTime(&timeinfo)) {
      clockCanvas.fillScreen(ST77XX_WHITE);
      clockCanvas.setFont(&FreeSansBold24pt7b);
      clockCanvas.setTextColor(ST77XX_BLACK);
      clockCanvas.setCursor(0, 36);
      char timeHourMin[6];
      strftime(timeHourMin, sizeof(timeHourMin), "%H:%M", &timeinfo);
      clockCanvas.print(timeHourMin);
      tft.drawRGBBitmap(15, 6, clockCanvas.getBuffer(), clockCanvas.width(), clockCanvas.height());

      char dateBuffer[40];
      strftime(dateBuffer, sizeof(dateBuffer), "%A, %B %d", &timeinfo);
      String currentDate = String(dateBuffer);
      if (currentDate != lastDisplayedDate) {
        tft.fillRect(20, 55, 250, 22, ST77XX_WHITE);
        tft.setFont(&FreeSansBold12pt7b);
        tft.setTextColor(0x4208);
        tft.setCursor(20, 70);
        tft.print(currentDate);
        lastDisplayedDate = currentDate;
      }
    }
  }

  // 2. ТЕРМОСТАТ И СЕНЗОР (Работи ВИНАГИ, дори без интернет!)
  if (currentMillis - lastTempUpdate >= 2000) {
    lastTempUpdate = currentMillis;
    sensors.requestTemperatures();
    float currentTemp = sensors.getTempCByIndex(0);

    if (currentTemp != DEVICE_DISCONNECTED_C) {
      thermostatLogic(currentTemp);

      if (isWiFiConnected) {
        Blynk.virtualWrite(V4, currentTemp);        
        Blynk.virtualWrite(V3, relayState ? 1 : 0); 
      }

      if (currentTemp != lastTemp) {
        tempCanvas.fillScreen(ST77XX_WHITE);
        tempCanvas.setFont(&FreeSansBold32pt7b);
        tempCanvas.setTextColor(0x02CD);
        tempCanvas.setCursor(0, 65);
        char tempStr[10];
        dtostrf(currentTemp, 4, 1, tempStr);
        String finalTempText = String(tempStr) + " C";
        finalTempText.trim();
        tempCanvas.print(finalTempText);
        tft.drawRGBBitmap(56, 85, tempCanvas.getBuffer(), tempCanvas.width(), tempCanvas.height());
        lastTemp = currentTemp;
      }
    } else {
      tft.fillRect(56, 85, 220, 70, ST77XX_WHITE);
      tft.setFont(&FreeSans9pt7b);
      tft.setTextColor(ST77XX_RED);
      tft.setCursor(55, 125);
      tft.print("Sensor Error!");
      digitalWrite(RELAY_PIN, LOW);
      relayState = false;
      if (isWiFiConnected) Blynk.virtualWrite(V3, 0);
      lastTemp = -999.0;
    }
  }

  // 3. ОБНОВЯВАНЕ НА ВРЕМЕТО
  if (currentMillis - lastWeatherUpdate >= 1800000 || lastWeatherUpdate == 0) {
    lastWeatherUpdate = currentMillis;
    if (isWiFiConnected && getWeather()) {
      updateWeatherPanel();
      drawRoundedCorners();
    }
  }
}
