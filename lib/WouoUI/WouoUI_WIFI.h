// --- WouoUI_WIFI.h ---

#ifndef WOUOUI_WIFI_H
#define WOUOUI_WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <PubSubClient.h>

// [新增] 引入 FreeRTOS 组件用于跨核保护
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

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

    // ================= [修复缺失] 补回这些配置变量 =================
    String ssid, pass;
    String mqtt_server, mqtt_user, mqtt_pass;
    int mqtt_port;
    // ==============================================================

    // ================= 跨核并发保护相关变量 =================
    volatile bool _wifiBusy = false;            // 标记是否正在销毁/重建网络对象
    volatile bool _mqtt_connected_flag = false; // 缓存的 MQTT 状态，供高频 UI 读取
    SemaphoreHandle_t _mqttMutex = NULL;        // MQTT 资源读写锁
    // ========================================================

    void setupAP();
    void setupSTA();
    void stopWiFi();
    void loadConfig();
    void reconnectMqtt();
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
    
    // 动态生成带扫描结果的网页
    String getWebPage(); 
};

extern WouoWIFI_Class WouoWIFI;

#endif