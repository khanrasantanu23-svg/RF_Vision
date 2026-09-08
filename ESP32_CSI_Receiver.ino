/*
 * ===================================================================================
 * ESP32 DevKit V1 - CSI Smart Home Human Presence Light Controller
 * ===================================================================================
 * 
 * Logic:
 *   - Human enters / moves in room  -> Relay triggers ON (Bulb ON)
 *   - Room empty / quiet            -> Relay triggers OFF immediately (Bulb OFF)
 *   - Transmitter dies / drops out  -> Relay forced OFF (Safety cutoff)
 * 
 * Negated Relay Logic:
 *   - RELAY_IS_ACTIVE_LOW = false
 *   - ON  writes HIGH to GPIO 4
 *   - OFF writes LOW  to GPIO 4 (and starts LOW on boot)
 * 
 * Hardware Connections:
 *   - Relay VCC -> ESP32 VIN (5V USB)
 *   - Relay GND -> ESP32 GND
 *   - Relay IN  -> ESP32 GPIO 4
 *   - Bulb Wire -> Relay COM and NO (Normally Open)
 * ===================================================================================
 */

#include <WiFi.h>
#include "esp_wifi.h"
#include <math.h>
#include <string.h>

// ======================== CONFIGURATION ========================
const char*   AP_SSID      = "ESP32_CSI_LINK";
const char*   AP_PASS      = "csi_secure_pass123";
const int     WIFI_CHANNEL = 6;
const uint8_t TARGET_TX_MAC[6] = {0x8C, 0x94, 0xDF, 0x90, 0x2C, 0x81};

// Pin Connections
const int  RELAY_PIN           = 4;     // GPIO 4 connected to Relay IN
const int  ONBOARD_LED_PIN     = 2;     // GPIO 2 onboard blue LED

// ===================== RELAY POLARITY (NEGATED) =====================
// NEGATED LOGIC: Set to false (HIGH = Bulb ON, LOW = Bulb OFF)
// If your relay was turning ON when empty and OFF when present, this fixes it!
const bool RELAY_IS_ACTIVE_LOW = false;

// ==================== TUNED DETECTION SETTINGS ====================
// Sensitivity: 1.30 catches walking, standing, and subtle movements
const float MOTION_THRESHOLD = 1.30f;

// Immediate shutoff: 1 calm frame (100 ms) as soon as signal calms down
const int CALM_FRAMES_REQUIRED = 1;

const unsigned long STREAM_INTERVAL_MS = 100; // 10 packets per second
const unsigned long TX_TIMEOUT_MS      = 2500; // Turn off if TX stops for 2.5s

// ======================= STATE VARIABLES =======================
float prev_amp[64]       = {0};
float prev_rel_phase[64] = {0};
bool  has_valid_baseline = false;
bool  tx_is_alive        = false;

volatile unsigned long last_stream_time    = 0;
volatile unsigned long last_tx_packet_time = 0;
bool current_relay_state     = false;
int  consecutive_calm_frames = 0;
bool auto_mode               = true;

// ======================= RELAY SWITCHING =======================
void set_bulb(bool turn_on) {
  if (current_relay_state == turn_on) return;
  current_relay_state = turn_on;

  // Negated Logic: turn_on = true -> HIGH, turn_on = false -> LOW
  int pin_level;
  if (RELAY_IS_ACTIVE_LOW) {
    pin_level = turn_on ? LOW : HIGH;
  } else {
    pin_level = turn_on ? HIGH : LOW;
  }

  digitalWrite(RELAY_PIN, pin_level);
  digitalWrite(ONBOARD_LED_PIN, turn_on ? HIGH : LOW);

  if (turn_on) {
    Serial.println("\n==================================================");
    Serial.println(" [RELAY] ON  -> Human Presence Detected! (BULB ON)");
    Serial.println("==================================================");
  } else {
    Serial.println("\n==================================================");
    Serial.println(" [RELAY] OFF -> Room Empty / Signal Calm (BULB OFF)");
    Serial.println("==================================================");
  }
}

// ===================== CSI BASEBAND CALLBACK =====================
void wifi_csi_rx_cb(void *ctx, wifi_csi_info_t *info) {
  if (!info || !info->buf || info->len < 2) return;

  // 1. Strict MAC filter: Accept packets only from Target Transmitter
  for (int b = 0; b < 6; b++) {
    if (info->mac[b] != TARGET_TX_MAC[b]) return;
  }

  unsigned long now = millis();
  last_tx_packet_time = now;

  if (!tx_is_alive) {
    tx_is_alive = true;
    Serial.println("[TX] Transmitter radio link established!");
  }

  // Rate limit to 10 Hz
  if (now - last_stream_time < STREAM_INTERVAL_MS) return;
  last_stream_time = now;

  int sc_count = (info->len / 2 > 64) ? 64 : (info->len / 2);
  int8_t *raw = (int8_t*)info->buf;
  int8_t rssi = info->rx_ctrl.rssi;

  float cur_amp[64];
  float raw_phase[64];
  float rel_phase[64];
  float delta_amp_sum = 0.0f;
  float delta_phase_sum = 0.0f;

  for (int i = 0; i < sc_count; i++) {
    int8_t imag = raw[i * 2];
    int8_t real = raw[i * 2 + 1];

    cur_amp[i]   = sqrtf((float)(real * real + imag * imag));
    raw_phase[i] = atan2f((float)imag, (float)real);

    // Differential adjacent phase cancels CFO oscillator drift
    rel_phase[i] = (i > 0) ? (raw_phase[i] - raw_phase[i - 1]) : 0.0f;

    // Measure delta only across active subcarriers 6 to 58 (skip noisy guard carriers)
    if (has_valid_baseline && i >= 6 && i <= 58) {
      delta_amp_sum   += fabsf(cur_amp[i] - prev_amp[i]);
      delta_phase_sum += fabsf(rel_phase[i] - prev_rel_phase[i]);
    }

    prev_amp[i]       = cur_amp[i];
    prev_rel_phase[i] = rel_phase[i];
  }

  // First frame is only used to capture quiescent baseline
  if (!has_valid_baseline) {
    has_valid_baseline = true;
    Serial.println("[CSI] Baseline initialized. Human presence detection ACTIVE.");
    return;
  }

  float avg_delta_amp   = delta_amp_sum   / 53.0f;
  float avg_delta_phase = delta_phase_sum / 53.0f;

  // ================= PRESENCE DECISION =================
  if (auto_mode) {
    if (avg_delta_amp > MOTION_THRESHOLD) {
      // Human disturbance detected
      consecutive_calm_frames = 0;
      if (!current_relay_state) {
        set_bulb(true);
      }
    } else {
      // Signal calm (no disturbance)
      consecutive_calm_frames++;
      if (current_relay_state && (consecutive_calm_frames >= CALM_FRAMES_REQUIRED)) {
        // Sustained calm -> Confirm empty room and turn off immediately
        set_bulb(false);
        consecutive_calm_frames = 0;
      }
    }
  }

  // Stream CSV to Serial for Python Dashboard / Monitor
  Serial.printf("CSI,%lu,%d,%.2f,%.2f", now, (int)rssi, avg_delta_amp, avg_delta_phase);
  for (int i = 0; i < sc_count; i++) Serial.printf(",%.1f", cur_amp[i]);
  for (int i = 0; i < sc_count; i++) Serial.printf(",%.2f", rel_phase[i]);
  Serial.println();
}

// ============================= SETUP =============================
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(ONBOARD_LED_PIN, OUTPUT);

  // Strictly enforce bulb is OFF on boot (negated logic: LOW)
  if (RELAY_IS_ACTIVE_LOW) {
    digitalWrite(RELAY_PIN, HIGH);
  } else {
    digitalWrite(RELAY_PIN, LOW);
  }
  digitalWrite(ONBOARD_LED_PIN, LOW);
  current_relay_state = false;

  Serial.println("\n==================================================================");
  Serial.println("   ESP32 CSI Smart Home Light Automation (Receiver)               ");
  Serial.println("==================================================================");
  Serial.printf("  Relay Pin         : GPIO %d (Active-%s)\n", RELAY_PIN, RELAY_IS_ACTIVE_LOW ? "LOW" : "HIGH");
  Serial.printf("  Motion Threshold  : %.2f\n", MOTION_THRESHOLD);
  Serial.printf("  Calm Hold Time    : %.1f seconds (%d frame)\n", CALM_FRAMES_REQUIRED / 10.0f, CALM_FRAMES_REQUIRED);
  Serial.printf("  Target TX MAC     : %02X:%02X:%02X:%02X:%02X:%02X\n",
                TARGET_TX_MAC[0], TARGET_TX_MAC[1], TARGET_TX_MAC[2],
                TARGET_TX_MAC[3], TARGET_TX_MAC[4], TARGET_TX_MAC[5]);
  Serial.println("  Type 'TEST' in Serial Monitor to test Relay & Bulb wiring!");
  Serial.println("==================================================================\n");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);

  Serial.printf("Connecting to Transmitter SoftAP '%s' on Channel %d...\n", AP_SSID, WIFI_CHANNEL);
  WiFi.begin(AP_SSID, AP_PASS, WIFI_CHANNEL);

  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

  esp_wifi_set_promiscuous(true);
  wifi_promiscuous_filter_t filter;
  filter.filter_mask = WIFI_PROMIS_FILTER_MASK_ALL;
  esp_wifi_set_promiscuous_filter(&filter);

  wifi_csi_config_t csi_cfg;
  memset(&csi_cfg, 0, sizeof(csi_cfg));
  csi_cfg.lltf_en           = 1;
  csi_cfg.htltf_en          = 1;
  csi_cfg.stbc_htltf2_en    = 1;
  csi_cfg.ltf_merge_en      = 1;
  csi_cfg.channel_filter_en = 0;
  csi_cfg.manu_scale        = 0;

  ESP_ERROR_CHECK(esp_wifi_set_csi_config(&csi_cfg));
  ESP_ERROR_CHECK(esp_wifi_set_csi_rx_cb(wifi_csi_rx_cb, NULL));
  ESP_ERROR_CHECK(esp_wifi_set_csi(true));

  last_tx_packet_time = millis();
  Serial.println("[READY] Listening for transmitter packets...\n");
}

// ============================== LOOP ==============================
void loop() {
  unsigned long now = millis();

  // 1. SAFETY WATCHDOG: If transmitter stops, force bulb OFF
  if (tx_is_alive && (now - last_tx_packet_time > TX_TIMEOUT_MS)) {
    tx_is_alive = false;
    has_valid_baseline = false;
    consecutive_calm_frames = 0;
    if (current_relay_state) {
      Serial.println("\n[SAFETY] Transmitter link lost! Forcing bulb OFF.");
      set_bulb(false);
    }
  }

  // 2. MANUAL TEST COMMANDS FROM SERIAL MONITOR
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    if (cmd == "TEST" || cmd == "TEST_BULB") {
      Serial.println("\n[TEST] Testing Relay & Bulb for 4 seconds...");
      set_bulb(true);
      delay(4000);
      set_bulb(false);
      Serial.println("[TEST] Test finished. Resuming auto presence detection.\n");
      auto_mode = true;
    } else if (cmd == "ON" || cmd == "RELAY_ON") {
      auto_mode = false;
      set_bulb(true);
      Serial.println("[MANUAL] Bulb forced ON. (Send 'AUTO' to resume automatic sensing)");
    } else if (cmd == "OFF" || cmd == "RELAY_OFF") {
      auto_mode = false;
      set_bulb(false);
      Serial.println("[MANUAL] Bulb forced OFF. (Send 'AUTO' to resume automatic sensing)");
    } else if (cmd == "AUTO") {
      auto_mode = true;
      consecutive_calm_frames = 0;
      Serial.println("[AUTO] Automatic Human Presence Detection Resumed!");
    }
  }

  // 3. Keep-alive Wi-Fi reconnect
  static unsigned long last_wifi_poll = 0;
  if (now - last_wifi_poll > 5000) {
    last_wifi_poll = now;
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(AP_SSID, AP_PASS, WIFI_CHANNEL);
      esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
    }
  }

  delay(20);
}