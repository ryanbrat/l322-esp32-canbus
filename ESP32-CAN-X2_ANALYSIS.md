# ESP32-CAN-X2 Examples Analysis
## Summary for Man-in-the-Middle CAN Bus Projects

---

## Overview
The ESP32-CAN-X2 is a dual CAN bus development board featuring:
- **CAN1**: Native ESP32 TWAI controller
- **CAN2**: External MCP2515 module via SPI
- Ideal for CAN bus bridging, monitoring, and relay applications

---

## Key Example Files & Their Roles

### 1. **CAN Forward** (Most Critical for MITM Projects)
**File**: `arduino/CAN Forward/can_demo_forward.ino`

**Purpose**: Bidirectional CAN bus bridge - the core MITM architecture

**Key Features**:
- **Message Relaying**: Reads messages from CAN1 (TWAI) and forwards to CAN2 (MCP2515)
- **Non-blocking Reception**: Uses `twai_receive(&message, 0)` for non-blocking polling
- **Message Inspection**: Prints all CAN transactions to serial for debugging/logging
- **Data Modification Hook**: Comments indicate where to add filtering/modification logic
  ```c
  // Here you can modify the data before it is sent to CAN2
  if (sendDataCAN2(message.identifier, message.extd, message.data, message.data_length_code))
  ```
- **Frame Type Support**: Handles both standard (11-bit) and extended (29-bit) CAN IDs
- **LED Indicator**: Visual feedback on successful message relay

**Hardware Configuration**:
- CAN1 (TWAI): GPIO6 (RX), GPIO7 (TX), 250 kbps
- CAN2 (MCP2515): GPIO12 (SCK), GPIO13 (MISO), GPIO11 (MOSI), GPIO10 (CS), 1 Mbps
- Serial: 115200 baud for monitoring

**Code Patterns - Critical for MITM**:
```c
// Non-blocking message read loop
while (twai_receive(&message, 0) == ESP_OK) {
    // Inspect/modify message here
    sendDataCAN2(message.identifier, message.extd, message.data, message.data_length_code);
}

// Dual CAN setup
twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(...);
twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
```

---

### 2. **Can Logger & Transmitter** (Logging & Filtering)
**Files**: `arduino/can_logger_and_transmitter/can1/can1_receiver.ino` and `can1_transmit.ino`

**Purpose**: Separate sender and receiver implementations for CAN1 with detailed logging

**Key Features**:
- **Simple Receiver**: Logs all received CAN messages with ID and data
- **Non-blocking Pattern**: `twai_receive(&message, 0)` with ESP_OK check
- **Serial Output Format**: 
  ```
  [0xID]: 0xByte1, 0xByte2, ...
  ```
- **Timing Configuration**: 500 kbps standard
- **Accept-All Filter**: No message filtering at hardware level

**Code Pattern - Receiver Loop**:
```c
void CAN1_readMsg() {
    twai_message_t message;
    while (twai_receive(&message, 0) == ESP_OK) {
        Serial.printf("[0x%X]: ", message.identifier);
        for (int i = 0; i < message.data_length_code; i++) {
            Serial.printf("0x%02X", message.data[i]);
        }
        Serial.println();
    }
}
```

**Logging Capabilities**:
- Accepts all standard frames by default
- Easy to extend with filtering logic
- Lightweight for continuous monitoring

---

### 3. **Ping Pong** (Bidirectional Communication)
**File**: `arduino/ping_pong/ping_pong.ino`

**Purpose**: Testing dual CAN interfaces with echo pattern (ping/pong between CAN1 and CAN2)

**Key Features**:
- **Alert-Based Processing**: Uses TWAI alerts instead of polling
  ```c
  uint32_t alerts_to_enable = TWAI_ALERT_TX_IDLE | TWAI_ALERT_TX_SUCCESS | 
                               TWAI_ALERT_TX_FAILED | TWAI_ALERT_RX_QUEUE_FULL | 
                               TWAI_ALERT_RX_DATA | TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_ERROR;
  twai_reconfigure_alerts(alerts_to_enable, NULL);
  ```
- **Error Handling**: Detects and reports:
  - TX transmission failures
  - RX queue overflow
  - Bus errors (bit/stuff/CRC/form/ACK)
  - Error passive state
- **Status Monitoring**: Retrieves error counts and queue status
  ```c
  twai_get_status_info(&twaistatus);
  // Access: msgs_to_tx, msgs_to_rx, tx_error_counter, rx_missed_count, etc.
  ```

**Advanced Pattern - Alert-Based vs Polling**:
- More efficient than continuous polling
- Real-time error detection
- Queue overflow detection (critical for MITM)

---

## Core Code Patterns for MITM Implementation

### Pattern 1: CAN1 Initialization (TWAI)
```c
bool setupCAN1(void) {
    twai_general_config_t g_config = 
        TWAI_GENERAL_CONFIG_DEFAULT(TX_PIN, RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    
    twai_driver_install(&g_config, &t_config, &f_config);
    twai_start();
    return true;
}
```

### Pattern 2: CAN2 Initialization (MCP2515)
```c
bool setupCAN2() {
    CAN2_SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
    if (CAN2.begin(MCP_STDEXT, CAN_1000KBPS, MCP_16MHZ) == CAN_OK) {
        CAN2.setMode(MCP_NORMAL);
        return true;
    }
    return false;
}
```

### Pattern 3: Non-Blocking Message Relay
```c
void loop() {
    twai_message_t message;
    while (twai_receive(&message, 0) == ESP_OK) {  // Non-blocking
        // MITM: Inspect/modify message here
        // Example: if (message.identifier == 0x123) { message.data[0] ^= 0xFF; }
        
        // Relay to second bus
        CAN2.sendMsgBuf(message.identifier, message.extd, 
                        message.data_length_code, message.data);
    }
}
```

### Pattern 4: Message Filtering Hook
```c
// From CAN Forward example - ready for extension
if (message.identifier == 0xSomeID) {
    // Modify specific message
    message.data[0] = newValue;
} else if (message.identifier == 0xOtherID) {
    // Block message (skip sendDataCAN2 call)
    continue;
}
// Send modified message
sendDataCAN2(message.identifier, message.extd, message.data, message.data_length_code);
```

---

## Relevant Features for MITM Projects

| Feature | Implementation | Use Case |
|---------|-----------------|----------|
| **Message Forwarding** | `twai_receive()` → `CAN.sendMsgBuf()` | Core MITM bridge |
| **Message Inspection** | Print before relay, check ID/data | Logging & monitoring |
| **Frame Types** | Extended (`message.extd`) & Standard | Full CAN protocol support |
| **Non-blocking RX** | `twai_receive(&msg, 0)` timeout=0 | Low-latency relay |
| **Dual Speed Support** | Configurable baud rates | CAN1 250kbps, CAN2 1Mbps |
| **Error Alerts** | TWAI alerts system | Detect queue overflow, bus errors |
| **Message Modification** | Intercept before relay | Inject/modify CAN data |
| **Serial Logging** | Print all messages to UART | Persistent monitoring |

---

## Filtering & Logging Capabilities

### Current Implementation
- **Accept-All Filter**: All messages received by default
- **Serial Logging**: Optional per-message print to monitor
- **No Rate Limiting**: Real-time relay with no throttling

### Extensible Points
1. **Hardware Filtering**: Replace `TWAI_FILTER_CONFIG_ACCEPT_ALL()` with ID masks
2. **Software Filtering**: Add if-statements in message loop before relay
3. **Conditional Logging**: Print only specific IDs or data patterns
4. **Message Buffering**: Store messages with timestamps (SD card/SPIFFS support available)

---

## Library Dependencies

| Library | Purpose | Source |
|---------|---------|--------|
| `driver/twai.h` | ESP32 native CAN controller | ESP-IDF (included) |
| `mcp_can.h` | MCP2515 SPI CAN module | [coryjfowler/MCP_CAN_lib](https://github.com/coryjfowler/MCP_CAN_lib) |
| `SPI.h` | SPI communication | Arduino/ESP32 (included) |
| `mcp_canbus.h` | Alternative MCP library | Longan Labs (used in ping_pong) |

---

## Performance Characteristics

- **Message Latency**: <1ms per message (non-blocking relay)
- **Throughput**: Up to 1000 messages/sec on CAN2 (1 Mbps)
- **Memory**: Minimal (queue-based, no buffering by default)
- **CPU**: Low utilization (polling-based, can use alerts for efficiency)

---

## Recommended Starting Point

**For a basic MITM CAN bus project:**
1. Start with **CAN Forward** (`can_demo_forward.ino`)
2. Verify dual-bus relay works (ping the bus through both interfaces)
3. Add message filtering in the `CAN1_readMsg()` loop
4. Integrate **Ping Pong** alerts for robust error detection
5. Add **Can Logger** serial output for persistent monitoring

**Key modifications for your project:**
```c
// In CAN1_readMsg() loop, replace the forward-all pattern with:
if (message.identifier == TARGET_ID) {
    // Modify specific messages
    message.data[0] = modifiedValue;
}
sendDataCAN2(message.identifier, message.extd, message.data, message.data_length_code);
```

---

## Documentation Reference
Full wiki: https://wiki.autosportlabs.com/ESP32-CAN-X2
