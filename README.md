### 4.1 Board Header Pinouts

#### Left Header (SV1) Pinout (Top to Bottom)

| Pin | Function      | Description                                      |
|-----|--------------|--------------------------------------------------|
| 1   | CAN1H/2.7D   | high-level signal for the first CAN channel      |
| 2   | CAN1L/2.7D   | low-level signal for the first CAN channel       |
| 3   | CAN2H/2.7C   | high-level signal for the second CAN channel     |
| 4   | CAN2L/2.7C   | low-level signal for the second CAN channel      |
| 5   | RX Pin        | USART RX                                        |
| 6   | TX Pin        | USART TX                                        |
| 7-20| GPIO Pins     | GPIO's (3.3v tolerant; do not exceed 3.3V)      |

#### Right Header (SV2) Pinout (Bottom to Top)

| Pin   | Function   | Description                                      |
|--------|------------|--------------------------------------------------|
| 1-11   | GPIO Pins  | GPIO's (3.3v tolerant; do not exceed 3.3V)       |
| 12-13  | 3.3V       | 3.3v Voltage Output (for custom circuits)         |
| 14-15  | 5V         | 5v Voltage Output (for custom circuits)           |
| 16-18  | GND        | Ground                                           |
| 19-20  | 12 Vin     | 12v voltage Input                                |

> **12V Note:** Only connect 12v to the 12v voltage input (pins 19-20). To avoid electrical damage, do not connect 12v to any other pin on the board.

#### Top Headers Pinout (X1 and X2)

The X1 and X2 headers support CAN communication. Pin count starts at the right when viewing the board from the top.

| Pin | Function     | Description                |
|-----|--------------|----------------------------|
| 1   | +12V_AUX     | 6-12V Power Supply         |
| 2   | CAN1H/2.7A   | CAN1 High                  |
| 3   | CAN1L/2.7A   | CAN1 Low                   |
| 4   | GND          | Ground                     |

Refer to the board silkscreen and official documentation for exact header orientation and connection details.

# CAN Bus MITM Interceptor - ESP32-CAN-X2 Edition

A CAN bus man-in-the-middle (MITM) interceptor for the Autosport Labs ESP32-CAN-X2 device. This project is designed to sit inline on a single 500 kbps CAN bus, intercepting, logging, filtering, and optionally blocking or modifying messages as they pass through. It does **not** bridge two separate CAN networks, but instead is inserted between two points on the same bus, transparently relaying all traffic unless configured otherwise.

## Hardware Requirements

- **ESP32-CAN-X2** (ESP32 with built-in TWAI and MCP2515 CAN controller)
- **USB-C cable** for programming and power
- **CAN Bus connectors** (DB9 or terminal blocks, depending on your CAN network)
- **Single CAN bus** (500 kbps, automotive standard)

## Project Structure

```
esp32-can-x2/
├── platformio.ini              # PlatformIO configuration
├── src/
│   ├── canbus_mitm_esp32.ino   # Main sketch for ESP32-CAN-X2
│   └── config.h                # Configuration parameters
├── lib/                        # (Auto-downloaded by PlatformIO)
└── docs/
  └── wiring_diagram.txt      # Wiring instructions
```

## Getting Started

### 1. Install PlatformIO

- Install VS Code extension: **PlatformIO IDE**
- Wait for initialization to complete

### 2. Open Project

- Open the `esp32-can-x2` folder in VS Code
- PlatformIO will auto-detect the `platformio.ini`

### 3. Install Dependencies

- PlatformIO will automatically download:
  - ESP32 Arduino framework
  - MCP_CAN library (coryjfowler/mcp_can)

### 3.1 Custom Board Setup

This project uses a custom board definition for the ESP32-S3-WROOM-1-N8R8 module with **8MB flash**. The board definition is located in `boards/esp32s3-wroom-1-n8r8.json`.

Key `platformio.ini` settings:
```ini
[platformio]
boards_dir = boards

[env:esp32s3box]
platform = espressif32
board = esp32s3-wroom-1-n8r8
board_build.flash_size = 8MB
board_build.partitions = default_8mb.csv
framework = arduino
build_flags = 
    -DCORE_DEBUG_LEVEL=3
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1
```

> **Important:** The USB CDC flags are required for serial output on ESP32-S3.

### 4. Hardware Wiring & Pinout

#### ESP32-S3 CAN Pin Assignments

| Function         | ESP32-S3 Pin |
|------------------|-------------|
| CAN1 (TWAI) RX   | GPIO6       |
| CAN1 (TWAI) TX   | GPIO7       |
| MCP2515 CS       | GPIO10      |
| MCP2515 SCK      | GPIO12      |
| MCP2515 MISO     | GPIO13      |
| MCP2515 MOSI     | GPIO11      |
| MCP2515 INT      | GPIO3       |
| Built-in LED     | GPIO2       |

#### ESP32-CAN-X2 to CAN Bus

- Connect both CAN interfaces (TWAI and MCP2515) to the same CAN bus lines:
  - **CAN_H** → High line of CAN network
  - **CAN_L** → Low line of CAN network
  - **GND** → Ground (should be common with ESP32 GND)

**To operate as a MITM inline device:**
- Cut the CAN_H and CAN_L lines at the desired interception point.
- Connect the CAN_H and CAN_L from the "upstream" side to the ESP32-CAN-X2 input.
- Connect the ESP32-CAN-X2 output to the "downstream" side of the bus.
- The device will relay all traffic, with the ability to monitor, filter, block, or modify messages as configured.

#### Wiring Diagram

```
         Upstream CAN Bus                ESP32-CAN-X2 Board                Downstream CAN Bus
   +-----------------------+         +------------------------+         +-----------------------+
   |   CAN_H -------------|---------| CAN1_RX (GPIO6)        |---------| CAN_H                |
   |   CAN_L -------------|---------| CAN1_TX (GPIO7)        |---------| CAN_L                |
   |   GND  --------------|---------| GND                    |---------| GND                  |
   +-----------------------+         +------------------------+         +-----------------------+

   [MCP2515 SPI]
   - CS:   GPIO10
   - SCK:  GPIO12
   - MISO: GPIO13
   - MOSI: GPIO11
   - INT:  GPIO3
```

### 5. CAN Bus Termination

Ensure your CAN network is terminated with 120Ω resistors at both ends. The ESP32-CAN-X2 may have a jumper for termination; enable it only if the device is at the end of the bus.

### 6. Configuration

Edit `src/config.h` to customize:

```c
// CAN Baudrate (default: 500 kbps for automotive)
#define CAN_BAUDRATE 500000

// Filtered message IDs to monitor
static const uint32_t FILTERED_IDS[] = {
  0x288,  // Rear Axle Status
  0x422,  // Terrain Response System
  // Add more IDs as needed
};
#define NUM_FILTERED_IDS (sizeof(FILTERED_IDS)/sizeof(FILTERED_IDS[0]))

// Block filtered messages (prevent forwarding)
#define BLOCK_FILTERED_MESSAGES true

// Log all messages to serial
#define LOG_ALL_MESSAGES true
```

### 7. Compile and Upload

**Using PlatformIO:**

```bash
# Build the project
platformio run

# Build and upload
platformio run --target upload

# Monitor serial output
platformio device monitor --baud 115200
```

**Or use VS Code:**
- Click the checkmark icon (✓) to build
- Click the arrow (→) to upload
- Click the plug icon to open serial monitor

### 8. Monitor Serial Output

Open Serial Monitor at **115200 baud**. You should see:

```
=== ESP32-CAN-X2 MITM Interceptor ===
TWAI (CAN) initialized at 500 kbps
MCP2515 (MCP_CAN_lib) initialized at 500 kbps
```

> **Note:** The ESP32-S3 uses USB CDC for serial output. After uploading, you may need to press the **Reset** button while the serial monitor is open to see the startup messages.

## Features

### Message Monitoring
All CAN messages are logged to serial with timestamp, ID, DLC, and data bytes.

### Message Filtering
Specific CAN message IDs can be monitored and handled separately. Configure in `config.h`.

### Selective Blocking
Enable `BLOCK_FILTERED_MESSAGES` to prevent filtered messages from being forwarded. This is useful for:
- Disabling terrain response system reactivation
- Blocking stability control notifications
- Preventing suspension alerts

### Custom Handlers
The `onFilteredMessage()` function in the main sketch allows custom handling. See `src/canbus_mitm_esp32.ino` for details.

## Serial Output Examples

### Startup
```
CAN Bus baudrate: 500000 bps
Monitored Message IDs: 0x288, 0x29B, 0x2A6, 0x3D3, 0x3EB, 0x422
Block Filtered Messages: YES
```

### Message Logging
```
[CAN] ID: 0x100 DLC: 8 Data: 01 02 03 04 05 06 07 08
[CAN] ID: 0x288 DLC: 4 Data: AA BB CC DD [FILTERED]
[BLOCKED] ID: 0x422 DLC: 8 Data: ...
```

### Statistics
```
=== CAN Bus Statistics ===
RX: 1234 TX: 0
Filtered Messages: 45
Blocked Messages: 12
Errors: 0
==========================
```

## 2008 Range Rover L322 Configuration

The default configuration is set up for monitoring suspension and traction control on a 2008 Range Rover. The MITM device operates inline on the vehicle's 500 kbps CAN bus, intercepting and optionally blocking or logging messages as specified.

| Message ID | System | Purpose |
|-----------|--------|---------|
| 0x288 | Rear Axle | Detect overheat/faults |
| 0x29B | ABS/EBD | Electronic brake distribution status |
| 0x2A6 | Suspension | Suspension sensor data |
| 0x3D3 | DSC | Stability control on/off |
| 0x3EB | ESP | Electronic stability program |
| 0x422 | Terrain Response | System reactivation message |

## Troubleshooting

### No CAN Messages Received

1. Check wiring (CAN_H, CAN_L, GND)
2. Verify CAN bus has active nodes
3. Check termination resistors (120Ω at bus ends)
4. Confirm baudrate matches network (500 kbps for Range Rover)

### Serial Monitor Shows Initialization Error

1. Ensure ESP32 Arduino framework and MCP2515 library are installed
2. Check wiring and power
3. Check USB cable and driver

### ESP32 Not Found in PlatformIO

1. Try: `platformio device list`
2. Press the reset button on ESP32-CAN-X2

## Library References

- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)
- [MCP_CAN Library](https://github.com/coryjfowler/MCP_CAN_lib) (coryjfowler/mcp_can)
- [MCP2515 Datasheet](http://ww1.microchip.com/downloads/en/DeviceDoc/MCP2515-Standalone-CAN-Controller-with-SPI-20001801J.pdf)
- [ESP32-CAN-X2 Wiki](https://wiki.autosportlabs.com/ESP32-CAN-X2)

## License

This project is provided as-is for educational and development purposes.
