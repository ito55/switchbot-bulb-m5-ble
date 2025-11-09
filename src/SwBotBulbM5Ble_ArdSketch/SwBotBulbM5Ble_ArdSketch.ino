#include <M5Unified.h>     // M5Unified by M5Stack v0.2.10 - https://github.com/m5stack/M5Unified
#include <NimBLEDevice.h>  // NimBLE-Arduino by h2zero v2.3.6 - https://github.com/h2zero/NimBLE-Arduino
#include <UNIT_8ENCODER.h> // M5Unit-8Encoder by M5Stack v0.0.1 - https://github.com/m5stack/M5Unit-8Encoder
#include "MyEnv.h"         // Include your SwitchBot Color Bulb BLE MAC address
#include "SwitchBot_BulbBLE.h" // SwitchBot Color Bulb API over BLE

// --- Global Variables ---
NimBLEClient* pClient = nullptr;
bool connected = false;
volatile bool connection_status_changed = true; // Initialized as true to update display on startup

// Unit 8Encoder related
UNIT_8ENCODER sensor;
uint8_t r_val = 255; // Initial Red value
uint8_t g_val = 255; // Initial Green value
uint8_t b_val = 255; // Initial Blue value

// Brightness value (1-100)
// Original stepped brightness control variables removed.
uint8_t brightness_val = 100; // Initial brightness value set to 100%

// Previous states of encoders and buttons
int32_t last_encoder_vals[4] = {0}; // Stores last values for CH1-CH4 encoders
bool last_button_states[8] = {true, true, true, true, true, true, true, true}; // Stores last states for CH1-CH8 buttons
unsigned long last_command_time = 0;
const unsigned long COMMAND_INTERVAL = 100; // Interval between BLE commands (ms)

// debug
int32_t current_encoder_ch4_val = 0;

// --- Function Prototypes ---
void updateDisplay();

// --- BLE Connection Callback ---
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

// --- Display Update ---
void updateDisplay() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 10);
    M5.Lcd.setTextFont(2);

    // Display RGB values
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.printf("R: %3d\n", r_val);
    M5.Lcd.setTextColor(TFT_GREEN);
    M5.Lcd.printf("G: %3d\n", g_val);
    M5.Lcd.setTextColor(TFT_BLUE);
    M5.Lcd.printf("B: %3d\n\n", b_val);

    // Display brightness
    M5.Lcd.setTextColor(TFT_YELLOW);
    M5.Lcd.printf("Brightness: %3d %%\n\n", brightness_val);

    // --- CH4 Encoder Value Display ---
    M5.Lcd.setTextColor(TFT_ORANGE);
    M5.Lcd.printf("Enc CH4: %d\n\n", current_encoder_ch4_val);

    // Display connection status
    M5.Lcd.setTextColor(TFT_WHITE);
    if (connected) {
        M5.Lcd.println("Status: Connected");
    } else {
        M5.Lcd.println("Status: Disconnected");
    }

    // Display control instructions
    M5.Lcd.setCursor(0, 160);
    M5.Lcd.println("CH5 Push: ON");
    M5.Lcd.println("CH6 Push: OFF");
}

// --- BLE Command Transmission ---
void sendCommand(const uint8_t* data, size_t size) {
    if (!connected || pClient == nullptr) return;
    NimBLERemoteService* pService = pClient->getService(SWITCHBOT_SERVICE_UUID);
    if (pService != nullptr) {
        NimBLERemoteCharacteristic* pCharacteristic = pService->getCharacteristic(SWITCHBOT_CHARACTER_UUID);
        if (pCharacteristic != nullptr) {
            // pCharacteristic->writeValue(data, size, true);
            pCharacteristic->writeValue(data, size, false);
        }
    }
}

void sendCommand(const std::vector<uint8_t>& command_vec) {
    sendCommand(command_vec.data(), command_vec.size());
}

// --- Setup ---
void setup() {
    M5.begin();
    M5.Lcd.setTextFont(2);

    // Initialize Unit 8Encoder
    int ex_sda = M5.getPin(m5::ex_i2c_sda);
    int ex_scl = M5.getPin(m5::ex_i2c_scl);
    if (ex_sda >= 0 && ex_scl >= 0) {
        sensor.begin(&Wire, ENCODER_ADDR, ex_sda, ex_scl);
    } else {
        sensor.begin(&Wire);
    }
    delay(100);

    // Set LED colors for encoders
    sensor.setLEDColor(0, 0x110000); // CH1: Red
    sensor.setLEDColor(1, 0x001100); // CH2: Green
    sensor.setLEDColor(2, 0x000011); // CH3: Blue
    sensor.setLEDColor(3, 0x111111); // CH4: White
    for (int i = 4; i < 8; i++) {
        sensor.setLEDColor(i, 0x000000); // CH5-CH8: Off
    }

    updateDisplay();

    // Initialize BLE
    NimBLEDevice::init("");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
}

// --- Main Loop ---
void loop() {
    M5.update();

    // Update display if connection status changed
    if (connection_status_changed) {
        updateDisplay();
        connection_status_changed = false;
    }

    // BLE connection handling
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

    // --- 8Encoder Input Handling ---
    bool rgb_changed = false;
    bool brightness_changed = false;

    // Encoders CH1-CH3: RGB control
    for (int i = 0; i < 3; i++) {
        int32_t current_val = sensor.getEncoderValue(i);
        if (current_val != last_encoder_vals[i]) {
            int32_t diff = current_val - last_encoder_vals[i];
            last_encoder_vals[i] = current_val;
            switch (i) {
                case 0: r_val = constrain(r_val + diff, 0, 255);
                break;
                case 1: g_val = constrain(g_val + diff, 0, 255); break;
                case 2: b_val = constrain(b_val + diff, 0, 255);
                break;
            }
            rgb_changed = true;
        }
    }

    // Encoder CH4: Brightness (1-100), 1-step control
    int32_t current_bright_val = sensor.getEncoderValue(3);
    current_encoder_ch4_val = current_bright_val;   // debug
    if (current_bright_val != last_encoder_vals[3]) {
        int32_t diff = current_bright_val - last_encoder_vals[3];
        last_encoder_vals[3] = current_bright_val;

        // Update brightness by the difference, constrained to 1-100
        uint8_t new_brightness = constrain(brightness_val + diff, 1, 100);

        if (new_brightness != brightness_val) {
            brightness_val = new_brightness;
            brightness_changed = true;
        }
    }

    // Send BLE commands
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

    // Push buttons CH5: ON, CH6: OFF
    for (int i = 4; i < 6; i++) {
        bool current_state = sensor.getButtonStatus(i);
        if (!current_state && last_button_states[i]) { // Button pressed
            if (i == 4) {
                sendCommand(TURN_ON_COMMAND, TURN_ON_COMMAND_SIZE);
            } else {
                sendCommand(TURN_OFF_COMMAND, TURN_OFF_COMMAND_SIZE);
            }
            updateDisplay();
        }
        last_button_states[i] = current_state;
    }

    delay(20);
}