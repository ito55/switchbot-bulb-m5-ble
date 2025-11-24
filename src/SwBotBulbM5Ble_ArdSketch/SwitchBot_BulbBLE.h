#ifndef SWITCHBOT_BLE_H
#define SWITCHBOT_BLE_H

#include <NimBLEDevice.h>
#include <vector>

// SwitchBot BLE Service and Characteristic UUIDs
const NimBLEUUID SWITCHBOT_SERVICE_UUID("cba20d00-224d-11e6-9fb8-0002a5d5c51b");
const NimBLEUUID SWITCHBOT_CHARACTER_UUID("cba20002-224d-11e6-9fb8-0002a5d5c51b");

// Command arrays
inline const uint8_t TURN_ON_COMMAND[] = {0x57, 0x0F, 0x47, 0x01, 0x01};
inline const size_t TURN_ON_COMMAND_SIZE = sizeof(TURN_ON_COMMAND);

inline const uint8_t TURN_OFF_COMMAND[] = {0x57, 0x0F, 0x47, 0x01, 0x02};
inline const size_t TURN_OFF_COMMAND_SIZE = sizeof(TURN_OFF_COMMAND);

inline const uint8_t TOGGLE_COMMAND[] = {0x57, 0x0F, 0x47, 0x01, 0x03};
inline const size_t TOGGLE_COMMAND_SIZE = sizeof(TOGGLE_COMMAND);

/**
 * @brief Generates the command to set RGB values.
 * @param r Red value (0-255)
 * @param g Green value (0-255)
 * @param b Blue value (0-255)
 * @return std::vector<uint8_t> The generated command
 */
inline std::vector<uint8_t> getSetRGBCommand(uint8_t r, uint8_t g, uint8_t b) {
    return {0x57, 0x0f, 0x47, 0x01, 0x16, r, g, b};
}

/**
 * @brief Generates the command to set the brightness value.
 * @param brightness Brightness value (0-100)
 * @return std::vector<uint8_t> The generated command
 */
inline std::vector<uint8_t> getSetBrightnessCommand(uint8_t brightness) {
    return {0x57, 0x0f, 0x47, 0x01, 0x14, brightness};
}

#endif // SWITCHBOT_BLE_H