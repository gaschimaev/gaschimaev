#include <DHT.h>
#include <PubSubClient.h>
#include <WiFi.h>

// Inisialisasi PIN dan MQTT
#define DHT_PIN 8
#define RED_LED_PIN 12
#define YELLOW_LED_PIN 10
#define GREEN_LED_PIN 5
#define RELAY_PIN 7
#define BUZZER_PIN 9

#define DHT_TYPE DHT22

const char* ssid = "SSID_WIFI_ANDA";
const char* password = "PASSWORD_WIFI_ANDA";
const char* mqtt_server = "BROKER_MQTT_ANDA";

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHT_PIN, DHT_TYPE);

// Fungsi untuk menghubungkan ke WiFi
void setup_wifi() {
  delay(10);
  Serial.print("Menghubungkan ke WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Tersambung");
}

// Fungsi untuk menghubungkan ke MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT...");
    if (client.connect("Hidroponik_Client")) {
      Serial.println("Tersambung");
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void setup() {
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  Serial.begin(115200);
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  // Membaca suhu dan kelembapan
  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();
  
  // Validasi pembacaan
  if (isnan(suhu) || isnan(kelembapan)) {
    Serial.println("Gagal membaca sensor DHT!");
    return;
  }

  // Logika kontrol suhu
  if (suhu > 35) {
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, HIGH);
  } else if (suhu >= 30 && suhu <= 35) {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  } else {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, LOW);
  }

  // Menyalakan Pompa
  digitalWrite(RELAY_PIN, HIGH); // Pompa ON

  // Mengirim data ke MQTT
  char suhu_str[8];
  char kelembapan_str[8];
  dtostrf(suhu, 6, 2, suhu_str);
  dtostrf(kelembapan, 6, 2, kelembapan_str);

  client.publish("hidroponik/suhu", suhu_str);
  client.publish("hidroponik/kelembapan", kelembapan_str);
  
  delay(2000);
}
