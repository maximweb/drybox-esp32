#include "config.h"

#include <Arduino.h>
// #include <PubSubClient.h>
#include <WiFi.h>
#include <Wire.h>

#include "ds18b20.h"

#include "button.h"
#include "display.h"
#include "humidity_sensor.h"

#include "controller.h"
#include "dht20sensor.h"
#include "ky040.h"
#include "ssd1306.h"
#include "ui.h"

// Display
Ssd1306 display{&Wire};

// DHT20 humidity sensor
Dht20 dht20{&Wire};

// DS18B20 temperature sensors
Ds18b20 temp_sensors(ONEWIRE_PIN, DS18B20_RESOLUTION, DS18B20_TIMEOUT_CONNECTED, DS18B20_INTERVAL_RECONNECT);

// User interface
Ui ui{display};

WiFiClient wifiClient;
// PubSubClient mqttClient(wifiClient);

// Rotary encoder
Ky040 encoder(KY040_ENCODER_A_PIN, KY040_ENCODER_B_PIN);
Button encoder_button(KY040_BUTTON_PIN);

// Controller
Controller controller(D8, D9); // TODO: which pins? move to config.h

void IRAM_ATTR encoder_button_interrupt()
{
    encoder_button.update();
}

void setup()
{
    // Start controller FIRST, as it sets heater and fan off
    controller.begin();

    // serial connection
    Serial.begin(115200);
    // delay(5000);

    // display
    display.begin();
    ui.set_layout(Ui::LayoutA);
    ui.set_layout_switching(true);

    // DHT20 humidity sensor
    dht20.begin();

    // DS18B20 temperature sensors
    pinMode(ONEWIRE_PIN, OUTPUT); // unclear why using m_pin in Ds18b20::begin() does NOT work?
    temp_sensors.begin();

    // Rotary Encoder
    attachInterrupt(KY040_BUTTON_PIN, encoder_button_interrupt, CHANGE);
    // ESP32Encoder::useInternalWeakPullResistors = puType::up;
    // encoder.attachFullQuad(digitalPinToGPIONumber(KY040_ENCODER_A_PIN), digitalPinToGPIONumber(KY040_ENCODER_B_PIN));
    // attachInterrupt(KY040_ENCODER_A_PIN, encoder_interrupt, CHANGE);
    // attachInterrupt(KY040_ENCODER_B_PIN, encoder_interrupt, CHANGE);

    // Wifi
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PW);
}

void loop()
{
    // Update wifi state
    if (WiFi.isConnected()) {
        int8_t wifi_rssi = WiFi.RSSI();
        switch (wifi_rssi) {
            case -54 ... 0:
                ui.set_wifi_state(Ui::WifiState::WIFIConnectedGood);
                break;
            case -69 ... - 55:
                ui.set_wifi_state(Ui::WifiState::WIFIConnectedOK);
                break;
            case -79 ... - 70:
                ui.set_wifi_state(Ui::WifiState::WIFIConnectedWeak);
                break;
            default:
                ui.set_wifi_state(Ui::WifiState::WIFIConnectedVeryWeak);
                break;
        }
    }
    else {
        ui.set_wifi_state(Ui::WifiState::WIFIConnecting);
    }

    // Read DHT20 sensor
    dht20.update();
    bool dht20_connected{false};
    if (dht20.is_connected()) {
        // TODO: how to handle disconnected sensor?
        dht20_connected = true;

        const auto current_humidity{dht20.relative_humidity()};
        const auto current_temperature{dht20.temperature()};

        controller.set_current_humidity(current_humidity);
        ui.set_current_humidity(current_humidity);
        controller.set_current_box_temperature(current_temperature); // TODO: use avg. value?
        ui.set_current_box_temperature(current_temperature);         // TODO: use avg. value?
        // TODO absolute humidity
    }

    // Read DS18B20 sensors
    // TODO: how to handle disconnected sensor?
    temp_sensors.update();
    bool ds18b20_connected[3]{false, false, false};
    float ds18b20_temperature[3]{0.0f, 0.0f, 0.0f};
    for (uint8_t id = 0; id < 3; id++) {
        if (temp_sensors.is_connected(id)) {
            ds18b20_connected[id] = true;
            ds18b20_temperature[id] = temp_sensors.temperature(id);
        }
    }
    if (ds18b20_connected[DS18B20_DUCT_ID]) {
        controller.set_current_duct_temperature(ds18b20_temperature[DS18B20_DUCT_ID]);
        ui.set_current_duct_temperature(ds18b20_temperature[DS18B20_DUCT_ID]);
    }

    // Read button encoder
    encoder.update();
    // Encoder::Direction enc_direction{encoder.direction()};
    // if (enc_direction != Encoder::Direction::None) {
    //     Serial.println((int8_t) enc_direction);
    // }
    // delay(10);

    // encoder_button.update();
    // Button::State encoder_button_state{encoder_button.getState()};
    // if (encoder_button_state != Button::State::Idle) {
    //     switch (encoder_button_state) {
    //         case Button::State::Click:
    //             Serial.println("Button Click");
    //             break;
    //         case Button::State::DoubleClick:
    //             Serial.println("Button DoubleClick");
    //             break;
    //         case Button::State::LongPress:
    //             Serial.println("Button LongPress");
    //             break;
    //     }
    // }
    // Serial.print("encoder: ");
    // Serial.println((int8_t) encoder.direction());

    // Update controller
    controller.update();

    // Update user interface
    ui.update();
}