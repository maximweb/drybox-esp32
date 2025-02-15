#include "config.h"

#include <Arduino.h>

#include <WiFi.h>
// #include <PubSubClient.h>
#include <Wire.h>

#include "ds18b20.h"

#include "button.h"
#include "display.h"
#include "humidity_sensor.h"

#include "controller.h"
#include "dht20sensor.h"
#include "ky040.h"
#include "mqtt.h"
#include "ssd1306.h"
#include "ui.h"

// Display
Ssd1306 display{&Wire};

// DHT20 humidity sensor
Dht20 dht20{&Wire};

// DS18B20 temperature sensors
Ds18b20 temp_sensors(ONEWIRE_PIN, DS18B20_RESOLUTION, DS18B20_TIMEOUT_CONNECTED, DS18B20_INTERVAL_RECONNECT);

// Rotary encoder
Ky040 encoder(KY040_ENCODER_A_PIN, KY040_ENCODER_B_PIN);
Button encoder_button(KY040_BUTTON_PIN);

// Controller
Controller controller(HEATER_PIN, FAN_PIN);

// MQTT
WiFiClient wifiClient;
MQTT mqttClient(wifiClient, dht20, temp_sensors, controller);

// User interface
Ui ui{display, dht20, temp_sensors, encoder, encoder_button, controller};

void IRAM_ATTR encoder_button_interrupt()
{
    encoder_button.update();
}

void setup()
{
    // Start controller FIRST, as it sets heater and fan off
    controller.begin();

    // LED
    digitalWrite(LED_PIN, LOW);
    pinMode(LED_PIN, OUTPUT);

    // serial connection
    Serial.begin(115200);
    // delay(5000);

    // DHT20 humidity sensor
    dht20.begin();

    // DS18B20 temperature sensors
    pinMode(ONEWIRE_PIN, OUTPUT); // unclear why using m_pin in Ds18b20::begin() does NOT work?
    temp_sensors.begin();

    // Encoder button
    encoder_button.begin();
    attachInterrupt(KY040_BUTTON_PIN, encoder_button_interrupt, CHANGE);

    // Rotary encoder (interrupts in library)
    encoder.begin();

    // Wifi
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PW);

    // MQTT
    mqttClient.begin();

    // display
    display.begin();
    ui.set_layout(Ui::LayoutBoxTemperature);
    ui.set_layout_switching(true);
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
    float dht20_temperature{0};
    if (dht20.is_connected()) {
        dht20_connected = true;
        const auto current_humidity{dht20.relative_humidity()};
        dht20_temperature = dht20.temperature();
        // Don't pass temperature to controller here, as we average all sensors.
    }

    // Read DS18B20 sensors
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
    }

    // Average available temperatures and pass to controller
    uint8_t n_sensors{0};
    float avg_temperature{0};
    if (dht20_connected) {
        avg_temperature += dht20_temperature;
        n_sensors += 1;
    }
    if (ds18b20_connected[DS18B20_LEFT_ID]) {
        avg_temperature += ds18b20_temperature[DS18B20_LEFT_ID];
        n_sensors += 1;
    }
    if (ds18b20_connected[DS18B20_RIGHT_ID]) {
        avg_temperature += ds18b20_temperature[DS18B20_RIGHT_ID];
        n_sensors += 1;
    }
    if (n_sensors > 0) {
        avg_temperature /= n_sensors;
        controller.set_current_box_temperature(avg_temperature);
    }
    else {
        // No sensor connected, disable controller/heating for safety
        controller.set_target_temperature(INVALID_FLOAT);
        controller.set_duration(0);
    }

    // Encoder button and rotary encoder
    encoder.update();
    encoder_button.update();

    // Toggle LED (only allow short press when UI not in menu)
    if (encoder_button.peekState() == Button::State::Click && ui.current_layout() < Ui::Layout::MenuStart) {
        encoder_button.reset();
        digitalWrite(LED_PIN, digitalRead(LED_PIN) ? LOW : HIGH);
    }

    // Update controller
    controller.update();

    // MQTT update
    mqttClient.update();

    // Update user interface
    ui.update();
}