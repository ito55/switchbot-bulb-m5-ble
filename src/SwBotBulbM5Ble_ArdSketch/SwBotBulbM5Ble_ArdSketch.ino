#include <M5Unified.h>
#include <NimBLEDevice.h>
#include <UNIT_8ENCODER.h> // Unit 8Encoderライブラリ
#include "MyEnv.h"        // 機器固有の設定
#include "SwitchBot_BulbBLE.h.h" // 汎用的なBLE設定

// --- グローバル変数 ---
NimBLEClient* pClient = nullptr;
bool connected = false;
volatile bool connection_status_changed = true; // 起動時に画面を更新するためtrueで初期化

// Unit 8Encoder関連
UNIT_8ENCODER sensor;
uint8_t r_val = 255;
uint8_t g_val = 255;
uint8_t b_val = 255;

// 明るさの段階設定
const uint8_t brightness_levels[] = {1, 20, 40, 60, 80, 100};
const int num_brightness_levels = sizeof(brightness_levels) / sizeof(brightness_levels[0]);
int brightness_level_index = 5; // 初期値100%に対応するインデックス
uint8_t brightness_val = brightness_levels[brightness_level_index];

// 各エンコーダーとボタンの前回の状態を保持
int32_t last_encoder_vals[4] = {0};
bool last_button_states[8] = {true, true, true, true, true, true, true, true};
unsigned long last_command_time = 0;
const unsigned long COMMAND_INTERVAL = 100; // コマンド送信の間隔 (ms)

// --- プロトタイプ宣言 ---
void updateDisplay();

// --- 接続コールバック ---
class MyClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) {
        connected = true;
        connection_status_changed = true;
    }
    void onDisconnect(NimBLEClient* pClient) {
        connected = false;
        connection_status_changed = true;
    }
};

// --- 画面表示 ---
void updateDisplay() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 10);
    M5.Lcd.setTextFont(2);
    // RGB値の表示
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.printf("R: %3d\n", r_val);
    M5.Lcd.setTextColor(TFT_GREEN);
    M5.Lcd.printf("G: %3d\n", g_val);
    M5.Lcd.setTextColor(TFT_BLUE);
    M5.Lcd.printf("B: %3d\n\n", b_val);
    // 明るさの表示
    M5.Lcd.setTextColor(TFT_YELLOW);
    M5.Lcd.printf("Brightness: %3d %%\n\n", brightness_val);
    
    // 接続状態の表示
    M5.Lcd.setTextColor(TFT_WHITE);
    if (connected) {
        M5.Lcd.println("Status: Connected");
    } else {
        M5.Lcd.println("Status: Disconnected");
    }

    // 操作方法の表示
    M5.Lcd.setCursor(0, 160);
    M5.Lcd.println("CH5 Push: ON");
    M5.Lcd.println("CH6 Push: OFF");
}

// --- BLEコマンド送信 ---
void sendCommand(const uint8_t* data, size_t size) {
    if (!connected || pClient == nullptr) return;
    NimBLERemoteService* pService = pClient->getService(SWITCHBOT_SERVICE_UUID);
    if (pService != nullptr) {
        NimBLERemoteCharacteristic* pCharacteristic = pService->getCharacteristic(SWITCHBOT_CHARACTER_UUID);
        if (pCharacteristic != nullptr) {
            pCharacteristic->writeValue(data, size, true);
        }
    }
}

void sendCommand(const std::vector<uint8_t>& command_vec) {
    sendCommand(command_vec.data(), command_vec.size());
}

// --- セットアップ ---
void setup() {
    M5.begin();
    M5.Lcd.setTextFont(2);
    // Unit 8Encoderの初期化
    if (!sensor.begin(&Wire, 0x41, 32, 33, 100000UL)) {
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setCursor(0,0);
        M5.Lcd.println("8Encoder Init Failed!");
        while(1) delay(100);
    }
    
    // ★修正点: 関数名を setLedColor から setLEDColor に変更
    sensor.setLEDColor(0, 0xFF0000); // CH1: Red
    sensor.setLEDColor(1, 0x00FF00); // CH2: Green
    sensor.setLEDColor(2, 0x0000FF); // CH3: Blue
    sensor.setLEDColor(3, 0xFFFFFF); // CH4: White
    for (int i = 4; i < 8; i++) {
      sensor.setLEDColor(i, 0x000000); // CH5-8は消灯
    }

    // ★削除点: setIndicatorColor() はライブラリに存在しないため削除

    updateDisplay();
    // BLEの初期化
    NimBLEDevice::init("");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
}


// --- メインループ ---
void loop() {
    M5.update();
    // 接続状態が変化したら画面を更新
    if (connection_status_changed) {
        updateDisplay();
        connection_status_changed = false;
    }

    // BLE接続処理
    if (!connected) {
        if (pClient != nullptr) {
            NimBLEDevice::deleteClient(pClient);
            pClient = nullptr;
        }
        
        pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(new MyClientCallbacks());

        NimBLEAddress bulb_addr(SWITCHBOT_BULB_BLE_MAC, BLE_ADDR_PUBLIC);
        pClient->connect(bulb_addr, false);
        delay(1000);
        return;
    }
    
    // --- 8Encoder入力処理 ---
    bool rgb_changed = false;
    bool brightness_changed = false;

    // エンコーダー (CH1-3: RGB)
    for (int i = 0; i < 3; i++) {
        int32_t current_val = sensor.getEncoderValue(i);
        if (current_val != last_encoder_vals[i]) {
            int32_t diff = current_val - last_encoder_vals[i];
            last_encoder_vals[i] = current_val;
            switch (i) {
                case 0: r_val = constrain(r_val + diff, 0, 255); break;
                case 1: g_val = constrain(g_val + diff, 0, 255); break;
                case 2: b_val = constrain(b_val + diff, 0, 255); break;
            }
            rgb_changed = true;
        }
    }

    // エンコーダー (CH4: Brightness) 6段階制御
    int32_t current_bright_val = sensor.getEncoderValue(3);
    if (current_bright_val != last_encoder_vals[3]) {
        int32_t diff = current_bright_val - last_encoder_vals[3];
        last_encoder_vals[3] = current_bright_val;

        if (diff > 0) {
            brightness_level_index++;
        } else if (diff < 0) {
            brightness_level_index--;
        }

        brightness_level_index = constrain(brightness_level_index, 0, num_brightness_levels - 1);
        
        uint8_t new_brightness = brightness_levels[brightness_level_index];

        if (new_brightness != brightness_val) {
            brightness_val = new_brightness;
            brightness_changed = true;
            // ★削除点: setIndicatorColor() はライブラリに存在しないため削除
        }
    }
    
    // コマンド送信
    if ((rgb_changed || brightness_changed) && (millis() - last_command_time > COMMAND_INTERVAL)) {
        if (rgb_changed) {
            sendCommand(getSetRGBCommand(r_val, g_val, b_val));
        }
        if (rgb_changed && brightness_changed) delay(50);
        if (brightness_changed) {
            sendCommand(getSetBrightnessCommand(brightness_val));
        }
        last_command_time = millis();
        updateDisplay();
    }

    // プッシュボタン (CH5: ON, CH6: OFF)
    for (int i = 4; i < 6; i++) {
        bool current_state = sensor.getButtonStatus(i);
        if (!current_state && last_button_states[i]) { // 押された瞬間
            if (i == 4) { // CH5
                sendCommand(TURN_ON_COMMAND, TURN_ON_COMMAND_SIZE);
            } else { // CH6
                sendCommand(TURN_OFF_COMMAND, TURN_OFF_COMMAND_SIZE);
            }
            updateDisplay();
        }
        last_button_states[i] = current_state;
    }

    delay(20);
}