#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* WIFI_SSID = "Vodafone-Beni";
const char* WIFI_PASS = "Beni2@19";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
#define OLED_I2C_ADDR 0x3C

#define I2C_SDA 8
#define I2C_SCL 9

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

struct BusArrival {
  String line;
  String text;
};

BusArrival displayLines[4];
int totalLinesCount = 0;

void fetchStopData(const String& stopCode, const String& line1, const String& line2 = "") {
  HTTPClient http;
  http.begin("https://stcp-proxy.onrender.com/?stop=" + stopCode);
  http.setTimeout(4000);
  bool dataOk = false;
  
  if (http.GET() == HTTP_CODE_OK) {
    JsonDocument doc;
    if (deserializeJson(doc, http.getStream()) == DeserializationError::Ok && doc["arrivals"].is<JsonArray>()) {
      dataOk = true;
      JsonArray arrivals = doc["arrivals"].as<JsonArray>();

      bool foundLine1 = false;
      bool foundLine2 = false;

      for (JsonObject item : arrivals) {
        if (totalLinesCount >= 4) break;

        String line = item["route_short_name"].as<String>();
        line.trim();

        if (!foundLine1 && line.equalsIgnoreCase(line1)) {
          JsonVariant v = item["arrival_minutes"];
          displayLines[totalLinesCount].line = line;
          if (!v.is<float>()) displayLines[totalLinesCount].text = "- min";
          else if (v.as<float>() < 1.0f) displayLines[totalLinesCount].text = "0 min";
          else displayLines[totalLinesCount].text = String((int)v.as<float>()) + " min";
          totalLinesCount++;
          foundLine1 = true;
        }
        else if (line2.length() > 0 && !foundLine2 && line.equalsIgnoreCase(line2)) {
          JsonVariant v = item["arrival_minutes"];
          displayLines[totalLinesCount].line = line;
          if (!v.is<float>()) displayLines[totalLinesCount].text = "- min";
          else if (v.as<float>() < 1.0f) displayLines[totalLinesCount].text = "0 min";
          else displayLines[totalLinesCount].text = String((int)v.as<float>()) + " min";
          totalLinesCount++;
          foundLine2 = true;
        }
      }
      if (!foundLine1 && totalLinesCount < 4) {
        displayLines[totalLinesCount].line = line1;
        displayLines[totalLinesCount].text = "- min";
        totalLinesCount++;
      }
      if (line2.length() > 0 && !foundLine2 && totalLinesCount < 4) {
        displayLines[totalLinesCount].line = line2;
        displayLines[totalLinesCount].text = "- min";
        totalLinesCount++;
      }
    }
  }
  http.end();

  if (!dataOk) {
    if (totalLinesCount < 4) {
      displayLines[totalLinesCount].line = line1;
      displayLines[totalLinesCount].text = "NO DATA";
      totalLinesCount++;
    }
    if (line2.length() > 0 && totalLinesCount < 4) {
      displayLines[totalLinesCount].line = line2;
      displayLines[totalLinesCount].text = "NO DATA";
      totalLinesCount++;
    }
  }
}

void renderUI() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  const int lineY[4] = {0, 8, 16, 24};

  for (int i = 0; i < totalLinesCount && i < 4; i++) {
    int y = lineY[i];

    display.setCursor(1, y);
    display.print(displayLines[i].line);

  uint16_t bw, bh;
  int16_t bx, by;
  display.getTextBounds(displayLines[i].text, 0, y, &bx, &by, &bw, &bh);
  display.setCursor(SCREEN_WIDTH - 1 - bw, y);
  display.print(displayLines[i].text);
  }

  display.display();
}

void setup() {
  Serial.begin(115200);

  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(1, 12);
  display.print("CONNECTING");
  display.display();

  int wifiAttempts = 0;
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    delay(500);
    wifiAttempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    display.clearDisplay();
    display.setCursor(1, 12);
    display.print("WIFI FAILED");
    display.display();
  }

}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 12);
    display.print("NO WIFI CONNECTION");
    display.display();
    delay(5000);
    return;
  }

  totalLinesCount = 0;
  fetchStopData("ALX2", "207", "504");
  fetchStopData("ALX1", "209");
  fetchStopData("PLM2", "204");
  renderUI();
  delay(15000);
}