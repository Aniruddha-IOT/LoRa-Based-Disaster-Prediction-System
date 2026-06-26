#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WebServer.h>

// =====================================
// LORA PINS
// =====================================
#define LORA_SS      5
#define LORA_RST     14
#define LORA_DIO0    26

// =====================================
// LED PINS
// =====================================
#define RED_LED       32
#define BLUE_LED      33
#define YELLOW_LED    27

// =====================================
// BUZZER PIN
// =====================================
#define BUZZER        25

// =====================================
// WIFI HOTSPOT
// =====================================
const char* ssid = "ESP32_DISASTER";
const char* password = "12345678";

// =====================================
// WEB SERVER
// =====================================
WebServer server(80);

// =====================================
// VARIABLES
// =====================================
String temperature = "0";
String humidity = "0";
String gas = "0";
String water = "0";

String statusText = "WAITING";

// =====================================
// WEBPAGE FUNCTION
// =====================================
String webpage() {

  String page = "";

  page += "<!DOCTYPE html><html>";

  page += "<head>";

  page += "<meta http-equiv='refresh' content='1'>";

  page += "<meta name='viewport' content='width=device-width, initial-scale=1'>";

  page += "<title>AI Disaster Monitoring</title>";

  page += "<style>";

  page += "body{";
  page += "font-family:Arial;";
  page += "background:#f2f2f2;";
  page += "text-align:center;";
  page += "padding:20px;";
  page += "}";

  page += ".card{";
  page += "background:white;";
  page += "margin:15px;";
  page += "padding:20px;";
  page += "border-radius:15px;";
  page += "box-shadow:0px 0px 10px gray;";
  page += "}";

  page += "h1{color:red;}";

  page += "</style>";

  page += "</head>";

  page += "<body>";

  page += "<h1>AI Disaster Monitoring System</h1>";

  page += "<div class='card'>";
  page += "<h2>Temperature</h2>";
  page += "<h3>" + temperature + " °C</h3>";
  page += "</div>";

  page += "<div class='card'>";
  page += "<h2>Humidity</h2>";
  page += "<h3>" + humidity + " %</h3>";
  page += "</div>";

  page += "<div class='card'>";
  page += "<h2>Gas Value</h2>";
  page += "<h3>" + gas + "</h3>";
  page += "</div>";

  page += "<div class='card'>";
  page += "<h2>Water Level</h2>";
  page += "<h3>" + water + "</h3>";
  page += "</div>";

  page += "<div class='card'>";
  page += "<h2>Status</h2>";
  page += "<h2>" + statusText + "</h2>";
  page += "</div>";

  page += "</body></html>";

  return page;
}

// =====================================
// ROOT PAGE
// =====================================
void handleRoot() {

  server.send(
    200,
    "text/html",
    webpage()
  );
}

// =====================================
// APP DATA
// =====================================
void handleData() {

  String data =
    temperature + "," +
    humidity + "," +
    gas + "," +
    water + "," +
    statusText;

  server.send(
    200,
    "text/plain",
    data
  );
}

// =====================================
// SETUP
// =====================================
void setup() {

  Serial.begin(115200);

  // =====================================
  // OUTPUT PINS
  // =====================================

  pinMode(RED_LED, OUTPUT);

  pinMode(BLUE_LED, OUTPUT);

  pinMode(YELLOW_LED, OUTPUT);

  pinMode(BUZZER, OUTPUT);

  digitalWrite(RED_LED, LOW);

  digitalWrite(BLUE_LED, LOW);

  digitalWrite(YELLOW_LED, LOW);

  digitalWrite(BUZZER, LOW);

  // =====================================
  // WIFI HOTSPOT
  // =====================================

  WiFi.softAP(ssid, password);

  Serial.println();
  Serial.println("WiFi Started");
  Serial.println(WiFi.softAPIP());

  // =====================================
  // WEB SERVER
  // =====================================

  server.on("/", handleRoot);

  server.on("/data", handleData);

  server.begin();

  // =====================================
  // START SPI
  // =====================================

  SPI.begin(18, 19, 23, 5);

  // =====================================
  // START LORA
  // =====================================

  LoRa.setPins(
    LORA_SS,
    LORA_RST,
    LORA_DIO0
  );

  if (!LoRa.begin(433E6)) {

    Serial.println("LoRa Init Failed!");

    while (1);
  }

  LoRa.setSyncWord(0xF3);

  Serial.println("LoRa Receiver Started");
}

// =====================================
// LOOP
// =====================================
void loop() {

  server.handleClient();

  int packetSize = LoRa.parsePacket();

  if (packetSize) {

    String received = "";

    while (LoRa.available()) {

      received += (char)LoRa.read();
    }

    Serial.println(received);

    // =====================================
    // SPLIT DATA
    // =====================================

    int p1 = received.indexOf(',');

    int p2 = received.indexOf(',', p1 + 1);

    int p3 = received.indexOf(',', p2 + 1);

    int p4 = received.indexOf(',', p3 + 1);

    if (p1 > 0 &&
        p2 > 0 &&
        p3 > 0 &&
        p4 > 0) {

      temperature =
        received.substring(0, p1);

      humidity =
        received.substring(p1 + 1, p2);

      gas =
        received.substring(p2 + 1, p3);

      water =
        received.substring(p3 + 1, p4);

      statusText =
        received.substring(p4 + 1);

      // =====================================
      // RESET OUTPUTS
      // =====================================

      digitalWrite(RED_LED, LOW);

      digitalWrite(BLUE_LED, LOW);

      digitalWrite(YELLOW_LED, LOW);

      digitalWrite(BUZZER, LOW);

      // =====================================
      // FIRE ALERT
      // =====================================

      if (statusText.indexOf("FIRE") >= 0) {

        digitalWrite(RED_LED, HIGH);

        digitalWrite(BUZZER, HIGH);
      }

      // =====================================
      // FLOOD ALERT
      // =====================================

      if (statusText.indexOf("FLOOD") >= 0) {

        digitalWrite(BLUE_LED, HIGH);

        digitalWrite(BUZZER, HIGH);
      }

      // =====================================
      // GAS ALERT
      // =====================================

      if (statusText.indexOf("GAS") >= 0) {

        digitalWrite(YELLOW_LED, HIGH);

        digitalWrite(BUZZER, HIGH);
      }

      // =====================================
      // SERIAL MONITOR
      // =====================================

      Serial.println("====================");

      Serial.print("Temperature: ");
      Serial.println(temperature);

      Serial.print("Humidity: ");
      Serial.println(humidity);

      Serial.print("Gas: ");
      Serial.println(gas);

      Serial.print("Water: ");
      Serial.println(water);

      Serial.print("Status: ");
      Serial.println(statusText);

      Serial.println("====================");
    }
  }
}