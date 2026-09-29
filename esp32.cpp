#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* WIFI_SSID = "NULL";
const char* WIFI_PASS = "NULL";

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
  if (WiFi.status() != WL_CONNECTED || totalLinesCount >= 4) return;

  HTTPClient http;
  http.begin("https://stcp-proxy.onrender.com/?stop=" + stopCode);
  http.setTimeout(4000);

  if (http.GET() == HTTP_CODE_OK) {
    JsonDocument doc;
    if (deserializeJson(doc, http.getStream()) == DeserializationError::Ok) {
      JsonArray arrivals = doc["arrivals"].as<JsonArray>();

      bool foundLine1 = false;
      bool foundLine2 = false;

      for (JsonObject item : arrivals) {
        if (totalLinesCount >= 4) break;

        String line = item["route_short_name"].as<String>();
        line.trim();

        if (!foundLine1 && line.equalsIgnoreCase(line1)) {
          int mins = item["arrival_minutes"].as<int>();
          displayLines[totalLinesCount].line = line;
          displayLines[totalLinesCount].text = String(mins) + " min";
          totalLinesCount++;
          foundLine1 = true;
        } 
        else if (line2.length() > 0 && !foundLine2 && line.equalsIgnoreCase(line2)) {
          int mins = item["arrival_minutes"].as<int>();
          displayLines[totalLinesCount].line = line;
          displayLines[totalLinesCount].text = String(mins) + " min";
          totalLinesCount++;
          foundLine2 = true;
        }
      }
    }
  }
  http.end();
}

void renderUI() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // 5x7 glyph height = 7px; 8px row pitch leaves the spare pixel below
  // each character row (not centered), filling the 32px screen exactly.
  const int lineY[4] = {0, 8, 16, 24};

  for (int i = 0; i < totalLinesCount && i < 4; i++) {
    int y = lineY[i];

    // Line number: flush left, 1px from border
    display.setCursor(1, y);
    display.print(displayLines[i].line);

    // "XY min": flush right, 1px from border (exact pixel width, not an estimate)
    int16_t bx, by;
    uint16_t bw, bh;
    display.getTextBounds(displayLines[i].text, 0, y, &bx, &by, &bw, &bh);
    int xRight = SCREEN_WIDTH - 1 - bw;
    display.setCursor(xRight, y);
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

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
  }

  totalLinesCount = 0;

  fetchStopData("ALX2", "207", "504");
  fetchStopData("ALX1", "209");
  fetchStopData("PLM2", "204");

  renderUI();

  delay(15000);
}