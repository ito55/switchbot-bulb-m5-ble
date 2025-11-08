#ifndef SWITCHBOT_BLE_H
#define SWITCHBOT_BLE_H

#include <NimBLEDevice.h>
#include <vector>

// SwitchBot BLEサービスおよびキャラクタリスティックのUUID
const NimBLEUUID SWITCHBOT_SERVICE_UUID("cba20d00-224d-11e6-9fb8-0002a5d5c51b");
const NimBLEUUID SWITCHBOT_CHARACTER_UUID("cba20002-224d-11e6-9fb8-0002a5d5c51b");

// コマンド配列
inline const uint8_t TURN_ON_COMMAND[] = {0x57, 0x0F, 0x47, 0x01, 0x01};
inline const size_t TURN_ON_COMMAND_SIZE = sizeof(TURN_ON_COMMAND);

inline const uint8_t TURN_OFF_COMMAND[] = {0x57, 0x0F, 0x47, 0x01, 0x02};
inline const size_t TURN_OFF_COMMAND_SIZE = sizeof(TURN_OFF_COMMAND);

/**
 * @brief RGB値を設定するためのコマンドを生成します。
 * * @param r 赤色の値 (0-255)
 * @param g 緑色の値 (0-255)
 * @param b 青色の値 (0-255)
 * @return std::vector<uint8_t> 生成されたコマンド
 */
inline std::vector<uint8_t> getSetRGBCommand(uint8_t r, uint8_t g, uint8_t b) {
    return {0x57, 0x0f, 0x47, 0x01, 0x16, r, g, b};
}

/**
 * @brief 明るさの値を設定するためのコマンドを生成します。
 * * @param brightness 明るさの値 (0-100)
 * @return std::vector<uint8_t> 生成されたコマンド
 */
inline std::vector<uint8_t> getSetBrightnessCommand(uint8_t brightness) {
    return {0x57, 0x0f, 0x47, 0x01, 0x14, brightness};
}

#endif // SWITCHBOT_BLE_H