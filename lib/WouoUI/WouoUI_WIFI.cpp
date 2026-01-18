#include "WouoUI_WIFI.h"
#include "WouoUI.h" // 为了访问 PIN_RELAY

WouoWIFI_Class WouoWIFI;

// HTML 页面：增加了 MQTT 配置表单
const char* index_html = 
"<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Greenhouse Config</title></head>"
"<body><h1>Greenhouse Setup</h1>"
"<form action='/save' method='POST'>"
"<h3>WiFi</h3>"
"SSID: <input type='text' name='s'><br>"
"PASS: <input type='text' name='p'><br>"
"<h3>MQTT Broker</h3>"
"Server: <input type='text' name='ms'><br>"
"Port: <input type='number' name='mp' value='1883'><br>"
"User: <input type='text' name='mu'><br>"
"Pass: <input type='text' name='mpa'><br>"
"<br><input type='submit' value='Save & Reboot'>"
"</form></body></html>";

void WouoWIFI_Class::mqttCallback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (int i = 0; i < length; i++) msg += (char)payload[i];
    
    // 简单控制逻辑：收到 "ON" 开水泵，"OFF" 关水泵
    if (msg == "ON") {
        digitalWrite(PIN_RELAY, HIGH);
    } else if (msg == "OFF") {
        digitalWrite(PIN_RELAY, LOW);
    }
}

void WouoWIFI_Class::begin() {
    pref.begin("wifi_conf", false);
    loadConfig();
    pinMode(PIN_RELAY, OUTPUT);
    digitalWrite(PIN_RELAY, LOW); // 默认关闭

    mqttClient.setClient(espClient);
    mqttClient.setCallback(mqttCallback);
    
    WiFi.mode(WIFI_OFF);
}

void WouoWIFI_Class::loadConfig() {
    ssid = pref.getString("ssid", "");
    pass = pref.getString("pass", "");
    mqtt_server = pref.getString("ms", "");
    mqtt_port = pref.getInt("mp", 1883);
    mqtt_user = pref.getString("mu", "");
    mqtt_pass = pref.getString("mpa", "");
}

void WouoWIFI_Class::loop() {
    if (server) server->handleClient();
    
    if (_mode == 1 && WiFi.status() == WL_CONNECTED) {
        if (!mqttClient.connected()) {
            reconnectMqtt();
        }
        mqttClient.loop();
    }
}

void WouoWIFI_Class::reconnectMqtt() {
    static unsigned long lastReconnectAttempt = 0;
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
        lastReconnectAttempt = now;
        if (mqtt_server.length() > 0) {
            mqttClient.setServer(mqtt_server.c_str(), mqtt_port);
            String clientId = "ESP32_Greenhouse_" + String(random(0xffff), HEX);
            if (mqttClient.connect(clientId.c_str(), mqtt_user.c_str(), mqtt_pass.c_str())) {
                mqttClient.subscribe("home/greenhouse/pump/set"); // 订阅水泵控制话题
            }
        }
    }
}

void WouoWIFI_Class::sendSensorData(float temp, float hum, int soil, int light) {
    if (mqttClient.connected()) {
        char buf[10];
        dtostrf(temp, 4, 1, buf); mqttClient.publish("home/greenhouse/temp", buf);
        dtostrf(hum, 4, 1, buf);  mqttClient.publish("home/greenhouse/hum", buf);
        itoa(soil, buf, 10);      mqttClient.publish("home/greenhouse/soil", buf);
        itoa(light, buf, 10);     mqttClient.publish("home/greenhouse/light", buf);
        mqttClient.publish("home/greenhouse/pump/state", digitalRead(PIN_RELAY) ? "ON" : "OFF");
    }
}

void WouoWIFI_Class::setMode(uint8_t mode) {
    if (_mode == mode) return;
    _mode = mode;
    stopWiFi();
    if (_mode == 1) setupSTA();
    else if (_mode == 2) setupAP();
}

uint8_t WouoWIFI_Class::getMode() { return _mode; }

void WouoWIFI_Class::stopWiFi() {
    if (server) { delete server; server = nullptr; }
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

void WouoWIFI_Class::setupSTA() {
    if (ssid.length() > 0) WiFi.begin(ssid.c_str(), pass.c_str());
}

void WouoWIFI_Class::setupAP() {
    WiFi.softAP("ESP32_Greenhouse", "12345678");
    server = new WebServer(80);
    server->on("/", [this](){ server->send(200, "text/html", index_html); });
    server->on("/save", [this](){
        if (server->hasArg("s")) pref.putString("ssid", server->arg("s"));
        if (server->hasArg("p")) pref.putString("pass", server->arg("p"));
        if (server->hasArg("ms")) pref.putString("ms", server->arg("ms"));
        if (server->hasArg("mp")) pref.putInt("mp", server->arg("mp").toInt());
        if (server->hasArg("mu")) pref.putString("mu", server->arg("mu"));
        if (server->hasArg("mpa")) pref.putString("mpa", server->arg("mpa"));
        
        server->send(200, "text/html", "<body>Saved. Rebooting...</body>");
        delay(1000);
        ESP.restart();
    });
    server->begin();
}

String WouoWIFI_Class::getSSID() {
    if (_mode == 0) return "OFF";
    if (_mode == 1) return WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "Connecting...";
    if (_mode == 2) return "ESP32_Greenhouse";
    return "";
}

String WouoWIFI_Class::getIP() {
    if (_mode == 1) return WiFi.localIP().toString();
    if (_mode == 2) return WiFi.softAPIP().toString();
    return "0.0.0.0";
}

bool WouoWIFI_Class::isMqttConnected() {
    return mqttClient.connected();
}