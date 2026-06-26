#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>

// =====================================
// EDGE IMPULSE
// =====================================
#include <piushpak_h7-project-1_inferencing.h>

// =====================================
// LORA PINS
// =====================================
#define SS    5
#define RST   14
#define DIO0  26

// =====================================
// DHT11
// =====================================
#define DHTPIN   4
#define DHTTYPE  DHT11

DHT dht(DHTPIN, DHTTYPE);

// =====================================
// SENSOR PINS
// =====================================
#define MQ2_PIN     34
#define WATER_PIN   35

// =====================================
// THRESHOLD VALUES
// =====================================
#define TEMP_THRESHOLD     35
#define GAS_THRESHOLD      600
#define WATER_THRESHOLD    200

// =====================================
// SERIAL TIMER
// =====================================
unsigned long lastSerialPrint = 0;

// =====================================
// SETUP
// =====================================
void setup() {

  Serial.begin(115200);

  delay(2000);

  dht.begin();

  Serial.println();
  Serial.println("=================================");
  Serial.println("AI DISASTER TRANSMITTER");
  Serial.println("=================================");

  // =====================================
  // LORA
  // =====================================

  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {

    Serial.println("LoRa initialization failed!");

    while (1);
  }

  LoRa.setSyncWord(0xF3);

  Serial.println("LoRa Sender Ready!");
}

// =====================================
// LOOP
// =====================================
void loop() {

  // =====================================
  // SENSOR READINGS
  // =====================================

  float temperature = dht.readTemperature();

  float humidity = dht.readHumidity();

  int gasValue = analogRead(MQ2_PIN);

  int waterValue = analogRead(WATER_PIN);

  // =====================================
  // DHT CHECK
  // =====================================

  if (isnan(temperature) || isnan(humidity)) {

    Serial.println("DHT11 Read Failed!");

    delay(1000);

    return;
  }

  // =====================================
  // EDGE IMPULSE FEATURES
  // =====================================

  float features[4];

  features[0] = temperature;
  features[1] = humidity;
  features[2] = gasValue;
  features[3] = waterValue;

  signal_t signal;

  int err = numpy::signal_from_buffer(
              features,
              4,
              &signal
            );

  if (err != 0) {

    Serial.println("Signal Error");

    return;
  }

  // =====================================
  // RUN AI MODEL
  // =====================================

  ei_impulse_result_t result = { 0 };

  EI_IMPULSE_ERROR res =
    run_classifier(
      &signal,
      &result,
      false
    );

  if (res != EI_IMPULSE_OK) {

    Serial.println("AI Classifier Failed");

    return;
  }

  // =====================================
  // MULTI DISASTER DETECTION
  // =====================================

  String aiPrediction = "";

  bool disasterDetected = false;

  // =====================================
  // FIRE
  // =====================================

  if (temperature > TEMP_THRESHOLD) {

    aiPrediction += "FIRE,";

    disasterDetected = true;
  }

  // =====================================
  // GAS
  // =====================================

  if (gasValue > GAS_THRESHOLD) {

    aiPrediction += "GAS,";

    disasterDetected = true;
  }

  // =====================================
  // FLOOD
  // =====================================

  if (waterValue > WATER_THRESHOLD) {

    aiPrediction += "FLOOD,";

    disasterDetected = true;
  }

  // =====================================
  // NO DISASTER
  // =====================================

  if (!disasterDetected) {

    aiPrediction = "NO DISASTER";
  }

  // =====================================
  // CREATE PACKET
  // =====================================

  String data =
    String(temperature, 2) + "," +
    String(humidity, 2) + "," +
    String(gasValue) + "," +
    String(waterValue) + "," +
    aiPrediction;

  // =====================================
  // SEND LORA
  // =====================================

  LoRa.beginPacket();

  LoRa.print(data);

  LoRa.endPacket();

  // =====================================
  // SLOW SERIAL OUTPUT
  // =====================================

  if (millis() - lastSerialPrint > 2000) {

    lastSerialPrint = millis();

    Serial.println();
    Serial.println("=================================");

    Serial.print("Temperature: ");
    Serial.println(temperature);

    Serial.print("Humidity: ");
    Serial.println(humidity);

    Serial.print("Gas Value: ");
    Serial.println(gasValue);

    Serial.print("Water Level: ");
    Serial.println(waterValue);

    Serial.print("AI Prediction: ");
    Serial.println(aiPrediction);

    Serial.println("LoRa Packet Sent!");

    Serial.println("=================================");
    Serial.println();
  }

  // =====================================
  // FAST RESPONSE
  // =====================================

  delay(200);
}