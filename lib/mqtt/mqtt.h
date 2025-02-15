#pragma once

#include <Arduino.h>

#include <PubSubClient.h>
#include <WiFi.h>

#include "controller.h"
#include "dht20sensor.h"
#include "ds18b20.h"

#define INVALID_FLOAT -127.0f

class MQTT {
public:
    enum MQTTState : uint8_t {
        MQTTWifiDisconnected = 0,
        MQTTStart = 1,
        MQTTConnecting = 2,
        MQTTConnected = 3,
        MQTTRunning = 4,
        MQTTReconnectDelay = 5,
    };

    MQTT(WiFiClient& wifi_client, Dht20& dht20_sensor, Ds18b20& temp_sensor, Controller& controller);

    void begin();

    void update();

    uint32_t last_transmission();

private:
    PubSubClient m_mqtt;
    WiFiClient& m_wifi;
    Dht20& m_dht20;
    Ds18b20& m_ds18b20;
    Controller& m_controller;

    MQTTState m_state{MQTTStart};

    void connect();
    void send_autodiscover();
    void send_data();

    uint32_t m_time_connect{0};
    uint32_t m_time_transmission{0};
};