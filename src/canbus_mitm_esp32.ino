/*
  canbus_mitm_esp32.ino - ESP32-CAN-X2 MITM Interceptor
  Inline CAN bus man-in-the-middle for 500 kbps bus
  Relays, logs, filters, and optionally blocks messages

  Uses MCP_CAN_lib (https://github.com/coryjfowler/MCP_CAN_lib) for MCP2515 CAN controller.
  See README for library details and API usage.
*/

// Standard includes
#include <Arduino.h>
#include "config.h"
#include <driver/twai.h> // ESP32 built-in CAN (TWAI)
#include <SPI.h>
#include <mcp_can.h>     // MCP_CAN_lib for MCP2515


// CAN1 (TWAI) Pins for ESP32-S3
#define CAN1_RX_PIN 6
#define CAN1_TX_PIN 7

// MCP2515 SPI pins (adjust as needed)
#define MCP2515_CS 5
#define MCP2515_INT 4
MCP_CAN CAN(MCP2515_CS); // MCP_CAN_lib object

// Helper: check if ID is filtered
bool isFiltered(uint32_t id) {
  for (size_t i = 0; i < NUM_FILTERED_IDS; ++i) {
    if (FILTERED_IDS[i] == id) return true;
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nESP32-CAN-X2 MITM Interceptor");

  // Init TWAI (ESP32 CAN)
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN1_TX_PIN, (gpio_num_t)CAN1_RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK && twai_start() == ESP_OK) {
    Serial.println("TWAI (CAN) initialized at 500 kbps");
  } else {
    Serial.println("TWAI init failed");
    while (1) delay(1000);
  }

    // Init MCP2515 (MCP_CAN_lib)
    SPI.begin();
    // MCP_CAN_lib expects (baudrate, CS pin)
    if (CAN.begin(MCP_STDEXT, CAN_BAUDRATE, MCP_16MHZ) == CAN_OK) {
      Serial.println("MCP2515 (MCP_CAN_lib) initialized at 500 kbps");
    } else {
      Serial.println("MCP2515 init failed");
      while (1) delay(1000);
    }
}

// Remove unused logFrame function (CAN_message_t is undefined)

void loop() {
  // Relay from TWAI to MCP2515
  twai_message_t tmsg;
  if (twai_receive(&tmsg, 0) == ESP_OK) {
    unsigned char len = tmsg.data_length_code;
    unsigned char buf[8];
    memcpy(buf, tmsg.data, len);
    unsigned long can_id = tmsg.identifier;
    if (LOG_ALL_MESSAGES) {
      Serial.printf("[CAN] ID: 0x%03lX DLC: %d Data:", can_id, len);
      for (uint8_t i = 0; i < len; ++i) Serial.printf(" %02X", buf[i]);
      if (isFiltered(can_id)) Serial.print(" [FILTERED]");
      Serial.println();
    }
    if (isFiltered(can_id)) {
      if (BLOCK_FILTERED_MESSAGES) return; // Block
      // Custom handler here
    }
    CAN.sendMsgBuf(can_id, 0, len, buf); // 0 = standard frame
  }

    // Relay from MCP2515 to TWAI
    unsigned char len = 0;
    unsigned char buf[8];
    unsigned long can_id = 0;
    if (CAN.checkReceive() == CAN_MSGAVAIL) {
      CAN.readMsgBuf(&can_id, buf, &len);
      if (LOG_ALL_MESSAGES) {
        Serial.printf("[CAN] ID: 0x%03lX DLC: %d Data:", can_id, len);
        for (uint8_t i = 0; i < len; ++i) Serial.printf(" %02X", buf[i]);
        if (isFiltered(can_id)) Serial.print(" [FILTERED]");
        Serial.println();
      }
      if (isFiltered(can_id)) {
        if (BLOCK_FILTERED_MESSAGES) return; // Block
        // Custom handler here
      }
      twai_message_t tmsg = {};
      tmsg.identifier = can_id;
      tmsg.data_length_code = len;
      memcpy(tmsg.data, buf, len);
      twai_transmit(&tmsg, 0);
    }
}
