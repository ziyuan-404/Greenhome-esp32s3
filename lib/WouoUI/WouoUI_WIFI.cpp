#include "WouoUI_WIFI.h"
#include "WouoUI.h" 

WouoWIFI_Class WouoWIFI;

// 在 WouoUI_WIFI.cpp 顶部声明引入外部互斥锁和共享数据
extern SharedData_t g_sharedData;
extern SemaphoreHandle_t g_dataMutex;

//生成动态网页内容，包含 WiFi 扫描结果和当前配置
void WouoWIFI_Class::mqttCallback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (int i = 0; i < length; i++) msg += (char)payload[i];
    
    bool newState = false;
    if (msg == "ON") {
        digitalWrite(PIN_RELAY, HIGH);
        newState = true;
    } else if (msg == "OFF") {
        digitalWrite(PIN_RELAY, LOW);
        newState = false;
    }

    // 同步更新给共享结构体
    if (g_dataMutex != NULL && xSemaphoreTake(g_dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        g_sharedData.pump_state = newState;
        xSemaphoreGive(g_dataMutex);
    }
}

void WouoWIFI_Class::begin() {
    Serial.begin(115200);
    Serial.println("\n[System] WouoUI WiFi Starting...");

    pref.begin("wifi_conf", false);
    loadConfig();
    pinMode(PIN_RELAY, OUTPUT);
    digitalWrite(PIN_RELAY, LOW);

    mqttClient.setClient(espClient);
    mqttClient.setCallback(mqttCallback);
    
    // 初始状态完全关闭 WiFi
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

void WouoWIFI_Class::loadConfig() {
    ssid = pref.getString("ssid", "");
    pass = pref.getString("pass", "");
    mqtt_server = pref.getString("ms", "");
    mqtt_port = pref.getInt("mp", 1883);
    mqtt_user = pref.getString("mu", "");
    mqtt_pass = pref.getString("mpa", "");
    
    ssid.trim();
    pass.trim();
}

void WouoWIFI_Class::loop() {
    // [关键] 无论什么模式，只要 server 对象存在，就处理网页请求
    if (server) server->handleClient();
    
    // STA 模式下的 MQTT 处理
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
            Serial.print("[MQTT] Connecting to "); Serial.println(mqtt_server);
            mqttClient.setServer(mqtt_server.c_str(), mqtt_port);
            String clientId = "ESP32_GH_" + String(random(0xffff), HEX);
            if (mqttClient.connect(clientId.c_str(), mqtt_user.c_str(), mqtt_pass.c_str())) {
                Serial.println("[MQTT] Connected");
                mqttClient.subscribe("home/greenhouse/pump/set");
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
    
    Serial.print("[Mode] Switching to "); Serial.println(mode);
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
    delay(100); 
}

// [辅助函数] 启动 Web 服务器并定义路由
// 这是一个内部 lambda 风格的封装，避免在 .h 中声明导致你需要改多个文件
void WouoWIFI_Class::setupSTA() {
    if (ssid.length() > 0) {
        Serial.print("[WiFi] Connecting to: "); Serial.println(ssid);
        
        // STA 模式也开启 AP_STA 混合模式，为了让 scanNetworks() 能正常工作不报错
        // 或者直接用 STA 模式，但某些版本 SDK 在 STA 连接后扫描会卡顿
        WiFi.mode(WIFI_STA); 
        WiFi.begin(ssid.c_str(), pass.c_str());
        WiFi.setAutoReconnect(true);

        // [关键修复] 在 STA 模式下也启动 Web 服务器
        if (server) delete server;
        server = new WebServer(80);
        
        server->on("/", [this](){
            server->send(200, "text/html", getWebPage());
        });
        
        server->on("/save", [this](){
            // 保存逻辑复用
            String s = server->arg("s"); s.trim();
            String p = server->arg("p"); p.trim();
            if (server->hasArg("s")) pref.putString("ssid", s);
            if (server->hasArg("p")) pref.putString("pass", p);
            if (server->hasArg("ms")) pref.putString("ms", server->arg("ms"));
            if (server->hasArg("mp")) pref.putInt("mp", server->arg("mp").toInt());
            if (server->hasArg("mu")) pref.putString("mu", server->arg("mu"));
            if (server->hasArg("mpa")) pref.putString("mpa", server->arg("mpa"));
            server->send(200, "text/html", "<html><body><h1>Saved!</h1><p>Rebooting...</p></body></html>");
            delay(1000);
            ESP.restart();
        });

        server->begin();
        Serial.println("[Web] Server started in STA mode");

    } else {
        Serial.println("[WiFi] Error: SSID is empty");
    }
}

void WouoWIFI_Class::setupAP() {
    Serial.println("[WiFi] Starting AP Mode");
    WiFi.mode(WIFI_AP_STA); // AP+STA 允许扫描
    WiFi.softAP("ESP32_Greenhouse", "12345678");
    Serial.print("[WiFi] AP IP: "); Serial.println(WiFi.softAPIP());
    
    if (server) delete server;
    server = new WebServer(80);
    
    server->on("/", [this](){
        server->send(200, "text/html", getWebPage());
    });
    
    server->on("/save", [this](){
        String s = server->arg("s"); s.trim();
        String p = server->arg("p"); p.trim();
        if (server->hasArg("s")) pref.putString("ssid", s);
        if (server->hasArg("p")) pref.putString("pass", p);
        if (server->hasArg("ms")) pref.putString("ms", server->arg("ms"));
        if (server->hasArg("mp")) pref.putInt("mp", server->arg("mp").toInt());
        if (server->hasArg("mu")) pref.putString("mu", server->arg("mu"));
        if (server->hasArg("mpa")) pref.putString("mpa", server->arg("mpa"));
        server->send(200, "text/html", "<html><body><h1>Saved!</h1><p>Rebooting...</p></body></html>");
        delay(1000);
        ESP.restart();
    });

    server->begin();
}

String WouoWIFI_Class::getWebPage() {
    // 获取当前 IP
    String currentIP = (_mode == 1) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();

    // 扫描网络
    int n = WiFi.scanNetworks();
    
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<style>body{font-family:Arial;padding:20px;max-width:400px;margin:auto} input,select{width:100%;padding:8px;margin:5px 0} input[type=submit]{background:#4CAF50;color:white;border:none;cursor:pointer}</style>";
    html += "<title>Greenhouse Config</title></head><body><h2>Greenhouse Setup</h2>";
    html += "<p><strong>Current IP:</strong> " + currentIP + "</p>";
    
    html += "<form action='/save' method='POST'>";
    html += "<h3>WiFi Settings</h3>";
    
    if (n == 0) {
        html += "<p>No networks found (or scan failed).</p>";
    } else {
        html += "<label>Scan Result:</label>";
        html += "<select id='ssidList' onchange='document.getElementById(\"ssidInput\").value=this.value'>";
        html += "<option value=''>-- Select Network --</option>";
        for (int i = 0; i < n; ++i) {
            html += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + "dBm)</option>";
        }
        html += "</select>";
    }

    html += "<label>SSID:</label><input type='text' id='ssidInput' name='s' value='" + ssid + "' placeholder='Enter SSID'>";
    html += "<label>Password:</label><input type='text' name='p' value='" + pass + "' placeholder='Enter WiFi Password'>";
    
    html += "<h3>MQTT Broker</h3>";
    html += "<label>Server:</label><input type='text' name='ms' value='" + mqtt_server + "'>";
    html += "<label>Port:</label><input type='number' name='mp' value='" + String(mqtt_port) + "'>";
    html += "<label>User:</label><input type='text' name='mu' value='" + mqtt_user + "'>";
    html += "<label>Pass:</label><input type='text' name='mpa' value='" + mqtt_pass + "'>";
    
    html += "<br><br><input type='submit' value='Save & Reboot'>";
    html += "</form></body></html>";
    
    return html;
}

String WouoWIFI_Class::getSSID() {
    if (_mode == 0) return "OFF";
    if (_mode == 1) {
        switch (WiFi.status()) {
            case WL_CONNECTED:      return WiFi.SSID();
            case WL_NO_SSID_AVAIL:  return "No SSID";   
            case WL_CONNECT_FAILED: return "Wrong PWD"; 
            case WL_IDLE_STATUS:    return "Idle";
            case WL_DISCONNECTED:   return "Wait...";   
            default:                return "Conn...";   
        }
    }
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