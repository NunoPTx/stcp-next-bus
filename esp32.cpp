#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* WIFI_SSID = "NULL"; // DEFINE THIS
const char* WIFI_PASS = "NULL"; // DEFINE THIS

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define OLED_I2C_ADDR 0x3C
#define I2C_SDA 8
#define I2C_SCL 9

#define REFRESH_MS 10000

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

struct BusArrival {
  String line;
  String text;
};

BusArrival displayLines[4];
int totalLinesCount = 0;

void addLine(const String& line, const String& text) {
  if (totalLinesCount >= 4) return;
  displayLines[totalLinesCount].line = line;
  displayLines[totalLinesCount].text = text;
  totalLinesCount++;
}

String formatMinutes(JsonVariant v) {
  if (v.isNull()) return "- min";
  float m = v.as<float>();
  return (m < 1.0f) ? "0 min" : String((int)m) + " min";
}

void fetchStopData(const String& stopCode, const String& line1, const String& line2 = "") {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(5000);
  http.begin(client, "https://stcp-proxy.onrender.com/?stop=" + stopCode);

  JsonDocument doc;
  bool ok = false;

  if (http.GET() == HTTP_CODE_OK) {
    String payload = http.getString();
    ok = (deserializeJson(doc, payload) == DeserializationError::Ok) &&
         doc["arrivals"].is<JsonArray>();
  }
  http.end();

  const String wanted[2] = {line1, line2};
  for (int w = 0; w < 2; w++) {
    if (wanted[w].length() == 0) continue;

    String text = ok ? "- min" : "NO DATA";
    if (ok) {
      for (JsonObject item : doc["arrivals"].as<JsonArray>()) {
        String line = item["route_short_name"] | "";
        line.trim();
        if (line.equalsIgnoreCase(wanted[w])) {
          JsonVariant v = item["arrival_minutes"];
          text = formatMinutes(v);
          break;
        }
      }
    }
    addLine(wanted[w], text);
  }
}

void renderUI() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  const int lineY[4] = {0, 8, 16, 24};

  for (int i = 0; i < totalLinesCount; i++) {
    int y = lineY[i];

    display.setCursor(1, y);
    display.print(displayLines[i].line);

    int16_t bx, by;
    uint16_t bw, bh;
    display.getTextBounds(displayLines[i].text, 0, y, &bx, &by, &bw, &bh);
    display.setCursor(SCREEN_WIDTH - 1 - bw, y);
    display.print(displayLines[i].text);
  }

  display.display();
}

void showMessage(const char* msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(1, 12);
  display.print(msg);
  display.display();
}

void setup() {
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    for (;;);
  }

  showMessage("CONNECTING...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
  }

  if (WiFi.status() != WL_CONNECTED) {
    showMessage("WIFI FAILED");
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    showMessage("NO WIFI");
    delay(5000);
    return;
  }

  totalLinesCount = 0;
  fetchStopData("ID1", "LINE1", "LINE2");
  fetchStopData("ID2", "LINE1");
  fetchStopData("ID3", "LINE1");
  renderUI();

  delay(REFRESH_MS);
}
