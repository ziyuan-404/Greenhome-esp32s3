#ifndef WOUOUI_WIFI_H
#define WOUOUI_WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <PubSubClient.h>

class WouoWIFI_Class {
public:
    void begin();
    void loop();
    
    // 0: OFF, 1: STA, 2: AP
    void setMode(uint8_t mode);
    uint8_t getMode();
    
    String getSSID();
    String getIP();
    
    // MQTT 相关
    void mqttLoop();
    void sendSensorData(float temp, float hum, int soil, int light);
    bool isMqttConnected();

private:
    uint8_t _mode = 0;
    WebServer* server = nullptr;
    Preferences pref;
    WiFiClient espClient;
    PubSubClient mqttClient;

    // 配置变量
    String ssid, pass;
    String mqtt_server, mqtt_user, mqtt_pass;
    int mqtt_port;

    void setupAP();
    void setupSTA();
    void stopWiFi();
    void loadConfig();
    void reconnectMqtt();
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
};

extern WouoWIFI_Class WouoWIFI;

#endif