#include <WiFi.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>

#include "api_calls.h"

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

#define TS_MINX 200
#define TS_MINY 300
#define TS_MAXX 3800
#define TS_MAXY 3800

SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

TFT_eSPI tft = TFT_eSPI();

const char* ssid = "Livebox-ACEA";
const char* password = "StephaneGwenael2002";
const char* serverName = "http://192.168.1.27:8080/stats";

float oldCpuTemp = 50;
float oldGpuTemp = 50;
float oldCpuLoad = 1;
float oldGpuLoad = 1;
float oldRamLoad = 1;

// JsonDocument token;

void setup() {
  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(mySpi);
  ts.setRotation(1);

  tft.init();
  tft.setRotation(1);
  tft.invertDisplay(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);

  WiFi.begin(ssid, password);
  switch (WiFi.waitForConnectResult(10000)) {
    case WL_CONNECTED:
      {
        tft.drawString("Connected to: " + String(ssid), 5, 0, 2);
        tft.drawString("Device local IP address: " + WiFi.localIP().toString(), 5, 15, 2);
        WiFi.setAutoReconnect(true);
        break;
      }
    case WL_DISCONNECTED:
      {
        tft.drawString("Connection failed", 5, 0, 2);
        break;
      }
  }

  delay(5000);
  tft.fillScreen(TFT_BLACK);
  drawFrame(10, 10, 145, 105, "Ryzen 5 7600X", 0xE8E4);
  drawFrame(165, 10, 145, 105, "RTX 4060", 0x75C0);
  drawFrame(10, 125, 125, 105, "FURY BEAST", 0xC658);
}

void loop() {
  // if (ts.tirqTouched() && ts.touched()) {
  //   TS_Point p = ts.getPoint();
  //   drawCircle(p);
  // }

  JsonDocument stats;
  const char* rawStats = getStats(serverName).c_str();
  deserializeJson(stats, rawStats);

  drawTemperatureArc(16, 31, 82, stats["cpu"]["temperature"], oldCpuTemp, "");
  drawLoadGraph(100, 32, 48, 80, stats["cpu"]["utilization"], oldCpuLoad);
  delay(200);

  drawTemperatureArc(171, 31, 82, stats["gpu"]["temperature"], oldGpuTemp, "");
  drawLoadGraph(255, 32, 48, 80, stats["gpu"]["utilization"], oldGpuLoad);
  delay(200);

  drawTemperatureArc(16, 146, 82, stats["ram"]["utilization"], oldRamLoad, "%");
  drawRamLoad(105, 147, stats["ram"]["utilization"]);

  oldCpuTemp = stats["cpu"]["temperature"];
  oldGpuTemp = stats["gpu"]["temperature"];
  oldCpuLoad = stats["cpu"]["utilization"];
  oldGpuLoad = stats["gpu"]["utilization"];
  oldRamLoad = stats["ram"]["utilization"];

  delay(2000);
}

void drawCircle(TS_Point p) {
  p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
  p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
  tft.drawCircle(p.x, p.y, 30, TFT_RED);
}

void drawFrame(int posX, int posY, int width, int height, String name, uint color) {
  TFT_eSprite sprite = TFT_eSprite(&tft);
  sprite.createSprite(width, height);
  sprite.drawRoundRect(0, 0, width, height, 10, color);
  sprite.drawString(name, 8, 6, 2);
  sprite.pushSprite(posX, posY);
  sprite.deleteSprite();
}

void drawTemperatureArc(int posX, int posY, int size, float value, float oldValue, String unit) {
  if (value == 0 || oldValue == 0) {
    return;
  }

  int fillColor = TFT_GREEN;
  if (value > 65) {
    fillColor = TFT_ORANGE;
    if (value > 85) {
      fillColor = TFT_RED;
    }
  }

  TFT_eSprite sprite = TFT_eSprite(&tft);
  sprite.createSprite(size, size);

  String strValue = String(value);
  int oldFillValue = round(oldValue * 3.6);
  int fillValue = round(value * 3.6);

  int step = 1;
  if (oldFillValue > fillValue) {
    step = -1;
  }

  float stepValue = (value - oldValue) / abs(fillValue - oldFillValue);

  while (oldFillValue != fillValue) {
    oldFillValue += step;
    oldValue += stepValue;
    sprite.fillSprite(TFT_BLACK);
    sprite.drawArc(size / 2, size / 2, (size - 10) / 2, (size - 24) / 2, 45, oldFillValue, fillColor, TFT_BLACK);
    sprite.drawFloat(oldValue, 1, size / 2 - tft.textWidth(strValue, 2) / 2 - 4 * 1.5, size / 2 - tft.fontHeight(2) / 2 - 4 / 2, 4);
    sprite.drawString(unit, 82 / 2 - tft.textWidth(unit) / 2 - 4 * 1.5, 54, 4);
    sprite.pushSprite(posX, posY);
    delay(20);
  }
  sprite.deleteSprite();
}

void drawLoadGraph(int posX, int posY, int width, int height, float value, float oldValue) {
  TFT_eSprite sprite = TFT_eSprite(&tft);
  sprite.createSprite(width, height);

  String strValue = String(value);
  int oldFillValue = round(oldValue * 0.46);
  int fillValue = round(value * 0.46);

  int step = 1;
  if (oldFillValue > fillValue) {
    step = -1;
  }

  float stepValue = (value - oldValue) / abs(fillValue - oldFillValue);

  while (oldFillValue != fillValue) {
    oldFillValue += step;
    oldValue += stepValue;
    sprite.fillSprite(TFT_BLACK);
    sprite.drawRect(width / 2 - 10, 4, 20, 46, 0xB115);
    sprite.fillRect(width / 2 - 10, 4 + 46 - oldFillValue, 20, oldFillValue, 0xB115);
    sprite.drawFloat(oldValue, 1, width / 2 - tft.textWidth(strValue, 2) / 2, 60, 2);
    sprite.drawString("%", width / 2 + tft.textWidth(strValue, 2) / 2 - 3, 60, 2);
    sprite.pushSprite(posX, posY);
    delay(20);
  }
  sprite.deleteSprite();
}

void drawRamLoad(int posX, int posY, float ramLoad) {
  TFT_eSprite sprite = TFT_eSprite(&tft);
  sprite.createSprite(19, 75);
  sprite.fillSprite(TFT_BLACK);

  int used = round(ramLoad * 32 / 100);
  int target = used - 32;

  int startX = 14;
  int startY = 70;
  int x = startX;
  int y = startY;

  for (int i = 0; i < 32; i++) {
    if (used > 0) {
      sprite.fillRect(x, y, 5, 5, 0xB115);
    } else {
      sprite.drawRect(x, y, 5, 5, 0xB115);
    }

    x -= 7;

    if (x < 0) {
      x = startX;
      y -= 7;
    }

    used -= 1;
  }
  sprite.pushSprite(posX, posY);
  sprite.deleteSprite();
}
