#include <WiFi.h>
#include <WiFiUdp.h>
#include "esp_wifi.h"

const char* AP_SSID = "ESP32_CSI_LINK";
const char* AP_PASS = "csi_secure_pass123";
const int   WIFI_CHANNEL = 6;

WiFiUDP udp;
const IPAddress STA_RECEIVER_IP(192, 168, 4, 2);
const IPAddress BROADCAST_IP(192, 168, 4, 255);
const uint16_t  UDP_PORT = 8888;
const unsigned long PACKET_INTERVAL_MS = 20; // 50 Hz

const int LED_PIN = 2;

struct PacketPayload {
  uint32_t sequence;
  uint32_t timestamp;
  char magic[8];
} payload;

uint32_t packet_seq = 0;
unsigned long last_tx_time = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS, WIFI_CHANNEL, 0, 2);

  // Force 802.11g and 802.11n OFDM (Disables 1 Mbps CCK)
  esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
  esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
  esp_wifi_set_ps(WIFI_PS_NONE);

  udp.begin(UDP_PORT);
  strncpy(payload.magic, "ESP_CSI", sizeof(payload.magic));
}

void loop() {
  unsigned long now = millis();
  if (now - last_tx_time >= PACKET_INTERVAL_MS) {
    last_tx_time = now;
    payload.sequence = packet_seq++;
    payload.timestamp = now;

    // Unicast to receiver forces 802.11n HT20 OFDM frames
    udp.beginPacket(STA_RECEIVER_IP, UDP_PORT);
    udp.write((const uint8_t*)&payload, sizeof(payload));
    udp.endPacket();

    udp.beginPacket(BROADCAST_IP, UDP_PORT);
    udp.write((const uint8_t*)&payload, sizeof(payload));
    udp.endPacket();

    if (packet_seq % 50 == 0) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
  }
}