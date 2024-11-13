#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Konfigurasi DHT
#define DHTPIN 25
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Pin LED dan Relay
#define LED_MERAH 27
#define LED_HIJAU 26
#define LED_KUNING 12
#define RELAY_PIN 15
#define BUZZER 23

// Konfigurasi WiFi dan MQTT
const char* ssid = "Wokwi-GUEST";             // Ganti dengan nama WiFi Anda
const char* password = "";                     // Ganti dengan password WiFi Anda
const char* mqtt_server = "broker.hivemq.com"; // Ganti dengan alamat broker MQTT

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Pengaturan WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // Pengaturan MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Pengaturan pin
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);
  pinMode(LED_KUNING, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ArduinoClient")) {
      Serial.println("Connected to MQTT");
      client.subscribe("relay/control");  // Subscribe ke topik untuk kontrol relay
    } else {
      delay(5000);
    }
  }
}

// Fungsi untuk menerima pesan dari MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // Kontrol relay berdasarkan pesan yang diterima
  if (String(topic) == "relay/control") {
    if (message == "{\"relay\": \"ON\"}") {
      digitalWrite(RELAY_PIN, HIGH);  // Nyalakan relay
      Serial.println("Relay status: ON");
    } else if (message == "{\"relay\": \"OFF\"}") {
      digitalWrite(RELAY_PIN, LOW);   // Matikan relay
      Serial.println("Relay status: OFF");
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Baca suhu dan kelembapan dari sensor DHT
  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();
  String relayStatus = (digitalRead(RELAY_PIN) == HIGH) ? "ON" : "OFF";

  if (!isnan(suhu) && !isnan(kelembapan)) {
    Serial.print("Suhu: ");
    Serial.print(suhu);
    Serial.print(" *C, Kelembapan: ");
    Serial.print(kelembapan);
    Serial.print(" %, Relay: ");
    Serial.println(relayStatus);

    // Publikasikan data suhu, kelembapan, dan status relay ke satu topik MQTT
    char message[100];
    snprintf(message, 100, "{\"temperature\": %.2f, \"humidity\": %.2f, \"relay\": \"%s\"}", suhu, kelembapan, relayStatus.c_str());
    client.publish("sensor/data", message);

    // Logika untuk LED dan Buzzer berdasarkan suhu
    if (suhu > 35) {
      digitalWrite(LED_MERAH, HIGH);
      digitalWrite(BUZZER, HIGH);
      digitalWrite(LED_KUNING, LOW);
      digitalWrite(LED_HIJAU, LOW);
      if (digitalRead(RELAY_PIN) == LOW) {
        digitalWrite(RELAY_PIN, HIGH); // Nyalakan relay
      }
    } else if (suhu >= 30 && suhu <= 35) {
      digitalWrite(LED_KUNING, HIGH);
      digitalWrite(LED_MERAH, LOW);
      digitalWrite(BUZZER, LOW);
      digitalWrite(LED_HIJAU, LOW);
      digitalWrite(RELAY_PIN, LOW); // Matikan relay
    } else {
      digitalWrite(LED_HIJAU, HIGH);
      digitalWrite(LED_MERAH, LOW);
      digitalWrite(LED_KUNING, LOW);
      digitalWrite(BUZZER, LOW);
      digitalWrite(RELAY_PIN, LOW); // Matikan relay
    }
  } else {
    Serial.println("Failed to read from DHT sensor!");
  }

  delay(5000); // Interval kirim data setiap 5 detik
}
