// config.h - ESP32-CAN-X2 MITM configuration

#pragma once

// CAN bus baudrate (500 kbps typical for automotive)
#define CAN_BAUDRATE 500000

// List of filtered CAN IDs (add or remove as needed)
static const uint32_t FILTERED_IDS[] = {
    0x288, // Rear Axle Status
    0x422, // Terrain Response System
    // Add more IDs as needed
};
#define NUM_FILTERED_IDS (sizeof(FILTERED_IDS)/sizeof(FILTERED_IDS[0]))

// Block filtered messages (true = block, false = just log)
#define BLOCK_FILTERED_MESSAGES true

// Log all messages to serial
#define LOG_ALL_MESSAGES false
