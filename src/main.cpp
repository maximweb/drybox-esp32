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
    ui.set_layout_switching(false);
    ui.set_layout(Ui::LayoutA);

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

    // DS18B20 own library
    temp_sensors.update();
    Serial.print("T ");
    for (uint8_t id = 0; id < 3; id++) {
        Serial.print(temp_sensors.temperature(id));
        if (temp_sensors.is_connected(id)) {
            Serial.print(" ");
        }
        else {
            Serial.print("* ");
        }
    }
    Serial.println();
    // delay(500);

    // encoder.update();
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

    dht20.update();
    // Serial.println(dht20.temperature());
    // Serial.println(dht20.relative_humidity());
    // Serial.println(dht20.absolute_humidity());

    ui.update();
}