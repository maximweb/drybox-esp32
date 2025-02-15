#include "config.h"

#include "mqtt.h"

MQTT::MQTT(WiFiClient& wifi_client, Dht20& dht20_sensor, Ds18b20& temp_sensor, Controller& controller)
: m_mqtt{wifi_client}
, m_wifi{wifi_client}
, m_dht20{dht20_sensor}
, m_ds18b20{temp_sensor}
, m_controller{controller}
{
}

void MQTT::begin()
{
    connect();
}

void MQTT::update()
{
    connect();
}

void MQTT::connect()
{
    const auto now{millis()};

    switch (m_state) {
        case MQTTState::MQTTWifiDisconnected:
            m_state = MQTTState::MQTTStart;
            break;

        case MQTTState::MQTTStart:
            m_mqtt.setServer(MQTT_SERVER, MQTT_PORT);
            m_mqtt.setBufferSize(5000);
            m_mqtt.connect("Drybox", MQTT_USER, MQTT_PW);
            m_time_connect = now;
            m_state = MQTTState::MQTTConnecting;
            break;

        case MQTTState::MQTTConnecting:
            if (!m_mqtt.connected()) {
                if (now - m_time_connect > 10000) {
                    m_state = MQTTState::MQTTReconnectDelay;
                }
            }
            else {
                m_state = MQTTState::MQTTConnected;
            }
            break;

        case MQTTState::MQTTConnected:
            send_autodiscover();
            m_time_connect = now;
            m_state = MQTTState::MQTTRunning;
            break;

        case MQTTState::MQTTRunning:
            if (!m_mqtt.connected()) {
                m_state = MQTTState::MQTTReconnectDelay;
                m_time_connect = 0;
            }
            else {
                // send_autodiscover();
                if (((now - m_time_connect) / 30000) % 2 == 0) {
                    send_autodiscover();
                }
                if (now - m_time_transmission > 1000) {
                    // time to send data
                    send_data();
                    m_time_transmission = now;
                }
            }
            break;

        case MQTTState::MQTTReconnectDelay:
            if (!m_mqtt.connected()) {
                if (now - m_time_connect > 10000) {
                    m_state = MQTTState::MQTTWifiDisconnected;
                }
            }
            else {
                m_state = MQTTState::MQTTConnected;
            }
            break;

        default:
            break;
    }
}

void MQTT::send_autodiscover()
{
    // Sends parameters for home assistant autodiscovery
    // Note: If message is not send, it may be due to a mqtt buffer limit (see MQTTclient.setBufferSize in connect_MQTT for amending)

    // discovery device
    String discovery_device = "{";
    discovery_device += "\"device\": {\"name\": \"Prusa Drybox\", \"identifiers\": [\"prusa_drybox_001\"] },";
    discovery_device += "\"origin\": {\"name\": \"drybox2mqtt\"}, ";
    discovery_device += "\"components\": {";

    // relative humidity
    discovery_device += "\"drybox_rel_humidity\": {";
    discovery_device += "\"name\": \"Humidity (Relative)\", ";
    discovery_device += "\"object_id\": \"drybox_rel_humidity\", "; // entity_id
    discovery_device += "\"unique_id\": \"drybox_rel_humidity\", ";
    discovery_device += "\"expire_after\": 10 , ";
    discovery_device += "\"platform\": \"sensor\", ";
    discovery_device += "\"device_class\": \"humidity\", ";
    discovery_device += "\"state_topic\": \"" + String(MQTT_TOPIC) + String("/sensors") + "\", ";
    discovery_device += "\"unit_of_measurement\": \"%\", ";
    discovery_device += "\"value_template\": \"{{ value_json['dht20_rel_humidity'] | float() }}\"";
    discovery_device += "}, ";

    // absolute humidity
    discovery_device += "\"drybox_abs_humidity\": {";
    discovery_device += "\"name\": \"Humidity (Absolute)\", ";
    discovery_device += "\"object_id\": \"drybox_abs_humidity\", "; // entity_id
    discovery_device += "\"unique_id\": \"drybox_abs_humidity\", ";
    discovery_device += "\"expire_after\": 10 , ";
    discovery_device += "\"platform\": \"sensor\", ";
    discovery_device += "\"device_class\": \"weight\", "; // TODO: no absolute_humidity in g/m^3 in HASS
    discovery_device += "\"state_topic\": \"" + String(MQTT_TOPIC) + String("/sensors") + "\", ";
    discovery_device += "\"unit_of_measurement\": \"g\", "; // g/m^3
    discovery_device += "\"value_template\": \"{{ value_json['dht20_abs_humidity'] | float() }}\"";
    discovery_device += "}, ";

    // temperature box
    discovery_device += "\"drybox_box_temperature\": {";
    discovery_device += "\"name\": \"Temperature Box\", ";
    discovery_device += "\"object_id\": \"drybox_box_temperature\", "; // entity_id
    discovery_device += "\"unique_id\": \"drybox_box_temperature\", ";
    discovery_device += "\"expire_after\": 10 , ";
    discovery_device += "\"platform\": \"sensor\", ";
    discovery_device += "\"device_class\": \"temperature\", ";
    discovery_device += "\"state_topic\": \"" + String(MQTT_TOPIC) + String("/sensors") + "\", ";
    discovery_device += "\"unit_of_measurement\": \"°C\", ";
    discovery_device += "\"value_template\": \"{{ value_json['dht20_temperature'] | float() }}\"";
    discovery_device += "}, ";

    // temperature duct
    discovery_device += "\"drybox_duct_temperature\": {";
    discovery_device += "\"name\": \"Temperature Duct\", ";
    discovery_device += "\"object_id\": \"drybox_duct_temperature\", "; // entity_id
    discovery_device += "\"unique_id\": \"drybox_duct_temperature\", ";
    discovery_device += "\"expire_after\": 10 , ";
    discovery_device += "\"platform\": \"sensor\", ";
    discovery_device += "\"device_class\": \"temperature\", ";
    discovery_device += "\"state_topic\": \"" + String(MQTT_TOPIC) + String("/sensors") + "\", ";
    discovery_device += "\"unit_of_measurement\": \"°C\", ";
    discovery_device += "\"value_template\": \"{{ value_json['ds18b20_temperature" + String(DS18B20_DUCT_ID) + "'] | float() }}\"";
    discovery_device += "}, ";

    // temperature left
    discovery_device += "\"drybox_left_temperature\": {";
    discovery_device += "\"name\": \"Temperature Left\", ";
    discovery_device += "\"object_id\": \"drybox_left_temperature\", "; // entity_id
    discovery_device += "\"unique_id\": \"drybox_left_temperature\", ";
    discovery_device += "\"expire_after\": 10 , ";
    discovery_device += "\"platform\": \"sensor\", ";
    discovery_device += "\"device_class\": \"temperature\", ";
    discovery_device += "\"state_topic\": \"" + String(MQTT_TOPIC) + String("/sensors") + "\", ";
    discovery_device += "\"unit_of_measurement\": \"°C\", ";
    discovery_device += "\"value_template\": \"{{ value_json['ds18b20_temperature" + String(DS18B20_LEFT_ID) + "'] | float() }}\"";
    discovery_device += "}, ";

    // temperature right
    discovery_device += "\"drybox_right_temperature\": {";
    discovery_device += "\"name\": \"Temperature Right\", ";
    discovery_device += "\"object_id\": \"drybox_right_temperature\", "; // entity_id
    discovery_device += "\"unique_id\": \"drybox_right_temperature\", ";
    discovery_device += "\"expire_after\": 10 , ";
    discovery_device += "\"platform\": \"sensor\", ";
    discovery_device += "\"device_class\": \"temperature\", ";
    discovery_device += "\"state_topic\": \"" + String(MQTT_TOPIC) + String("/sensors") + "\", ";
    discovery_device += "\"unit_of_measurement\": \"°C\", ";
    discovery_device += "\"value_template\": \"{{ value_json['ds18b20_temperature" + String(DS18B20_RIGHT_ID) + "'] | float() }}\"";
    discovery_device += "}, ";

    // Controller state
    discovery_device += "\"drybox_state\": {";
    discovery_device += "\"name\": \"State\", ";
    discovery_device += "\"object_id\": \"drybox_state\", "; // entity_id
    discovery_device += "\"unique_id\": \"drybox_state\", ";
    discovery_device += "\"expire_after\": 10 , ";
    discovery_device += "\"platform\": \"sensor\", ";
    discovery_device += "\"state_topic\": \"" + String(MQTT_TOPIC) + String("/controller") + "\", ";
    // discovery_device += "\"value_template\": \"{{ value_json['controller_state'] | int() }}\"";
    discovery_device += "\"value_template\": \"{\% set value = value_json['controller_state'] | int() \%}{\% set states = [";
    discovery_device += "'Idle', ";
    discovery_device += "'Heating', ";
    discovery_device += "'Hot Duct Pause', ";
    discovery_device += "'Hot Fan', ";
    discovery_device += "'Hot No Fan', ";
    discovery_device += "'Drying', ";
    discovery_device += "'Dry Duct Pause', ";
    discovery_device += "'Dry Fan', ";
    discovery_device += "'Dry No Fan', ";
    discovery_device += "'Cooldown' ";
    discovery_device += "] \%}";
    discovery_device += "{{ states[value] }}\"";
    discovery_device += "}, ";

    // target temperature
    discovery_device += "\"drybox_target_temperature\": {";
    discovery_device += "\"name\": \"Target Temperature\", ";
    discovery_device += "\"object_id\": \"drybox_target_temperature\", "; // entity_id
    discovery_device += "\"unique_id\": \"drybox_target_temperature\", ";
    discovery_device += "\"expire_after\": 10 , ";
    discovery_device += "\"platform\": \"sensor\", ";
    discovery_device += "\"device_class\": \"temperature\", ";
    discovery_device += "\"state_topic\": \"" + String(MQTT_TOPIC) + String("/controller") + "\", ";
    discovery_device += "\"unit_of_measurement\": \"°C\", ";
    discovery_device += "\"value_template\": \"{{ value_json['target_temperature'] | float() }}\"";
    discovery_device += "}, ";

    // elapsed time since controller start
    discovery_device += "\"drybox_controller_start\": {";
    discovery_device += "\"name\": \"Time Start\", ";
    discovery_device += "\"object_id\": \"drybox_controller_start\", "; // entity_id
    discovery_device += "\"unique_id\": \"drybox_controller_start\", ";
    discovery_device += "\"expire_after\": 10 , ";
    discovery_device += "\"platform\": \"sensor\", ";
    discovery_device += "\"device_class\": \"duration\", ";
    discovery_device += "\"state_topic\": \"" + String(MQTT_TOPIC) + String("/controller") + "\", ";
    discovery_device += "\"unit_of_measurement\": \"s\", ";
    discovery_device += "\"value_template\": \"{{ value_json['time_start'] | float() }}\"";
    discovery_device += "}, ";

    // elapsed time since target value reached
    discovery_device += "\"drybox_controller_elapsed\": {";
    discovery_device += "\"name\": \"Time Since Target\", ";
    discovery_device += "\"object_id\": \"drybox_controller_elapsed\", "; // entity_id
    discovery_device += "\"unique_id\": \"drybox_controller_elapsed\", ";
    discovery_device += "\"expire_after\": 10 , ";
    discovery_device += "\"platform\": \"sensor\", ";
    discovery_device += "\"device_class\": \"duration\", ";
    discovery_device += "\"state_topic\": \"" + String(MQTT_TOPIC) + String("/controller") + "\", ";
    discovery_device += "\"unit_of_measurement\": \"s\", ";
    discovery_device += "\"value_template\": \"{{ value_json['time_elapsed'] | float() }}\"";
    discovery_device += "}, ";

    // remaining time (if duration set)
    discovery_device += "\"drybox_controller_remaining\": {";
    discovery_device += "\"name\": \"Time Remaining\", ";
    discovery_device += "\"object_id\": \"drybox_controller_remaining\", "; // entity_id
    discovery_device += "\"unique_id\": \"drybox_controller_remaining\", ";
    discovery_device += "\"expire_after\": 10 , ";
    discovery_device += "\"platform\": \"sensor\", ";
    discovery_device += "\"device_class\": \"duration\", ";
    discovery_device += "\"state_topic\": \"" + String(MQTT_TOPIC) + String("/controller") + "\", ";
    discovery_device += "\"unit_of_measurement\": \"s\", ";
    discovery_device += "\"value_template\": \"{{ value_json['time_remaining'] | float() }}\"";
    discovery_device += "}";

    // device closing statements
    discovery_device += "}"; // components
    discovery_device += "}"; // global

    m_mqtt.publish(String("homeassistant/device/drybox/config").c_str(), discovery_device.c_str());
}

void MQTT::send_data()
{
    // Data structure
    // {
    //     "dht20_temperature": <value precision 2>,
    //     "dht20_humidity": <value precision 1>,
    //     ...
    // }

    char buffer[100];

    // Payload opener
    String payload = "{";

    // DS18B20 data
    for (uint8_t ID = 0; ID < 3; ID++) {
        if (m_ds18b20.is_connected(ID)) {
            const auto temperature{m_ds18b20.temperature(ID)};
            sprintf(buffer, "\"ds18b20_temperature%u\": %.2f ,", ID, temperature);
            payload += String(buffer);
        }
    }

    // DHT20 data
    if (m_dht20.is_connected()) {
        const auto dht20_T{m_dht20.temperature()};
        sprintf(buffer, "\"dht20_temperature\": %.2f ,", dht20_T);
        payload += String(buffer);

        const auto dht20_rel_humidity{m_dht20.relative_humidity()};
        sprintf(buffer, "\"dht20_rel_humidity\": %.1f ,", dht20_rel_humidity);
        payload += String(buffer);

        const auto dht20_abs_humidity{m_dht20.absolute_humidity()};
        sprintf(buffer, "\"dht20_abs_humidity\": %.1f ", dht20_abs_humidity);
        payload += String(buffer);
    }

    payload += "}";
    m_mqtt.publish((String(MQTT_TOPIC) + String("/sensors")).c_str(), payload.c_str());

    // Controller data
    payload = "{";

    sprintf(buffer, "\"controller_state\": %u ,", (uint8_t) m_controller.get_state());
    payload += String(buffer);

    const auto target_temp = m_controller.get_target_temperature();
    if (target_temp != INVALID_FLOAT) {
        sprintf(buffer, "\"target_temperature\": %.2f ,", target_temp);
        payload += String(buffer);
    }
    else {
        payload += String("\"target_temperature\": null ,");
    }

    const auto time_remaining = m_controller.get_time_remaining() / 1000;
    sprintf(buffer, "\"time_remaining\": %u ,", time_remaining);
    payload += String(buffer);

    const auto time_elapsed = m_controller.get_time_elapsed() / 1000;
    sprintf(buffer, "\"time_elapsed\": %u ,", time_elapsed);
    payload += String(buffer);

    const auto time_start = m_controller.get_time_start() / 1000;
    sprintf(buffer, "\"time_start\": %u ", time_start);
    payload += String(buffer);

    payload += "}";
    m_mqtt.publish((String(MQTT_TOPIC) + String("/controller")).c_str(), payload.c_str());
}

uint32_t MQTT::last_transmission()
{
    return m_time_transmission;
}