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

    // [新增] 创建 MQTT 互斥锁
    _mqttMutex = xSemaphoreCreateMutex();

    pref.begin("wifi_conf", false);
    loadConfig();
    pinMode(PIN_RELAY, OUTPUT);
    digitalWrite(PIN_RELAY, LOW);

    mqttClient.setClient(espClient);
    mqttClient.setCallback(mqttCallback);
    
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
    // [修复] 如果系统正在切换模式或分配内存，强行截断后台任务执行
    if (_wifiBusy) return;

    if (server) server->handleClient();
    
    if (_mode == 1 && WiFi.status() == WL_CONNECTED) {
        bool conn = false;
        
        // [修复] 安全获取底层 MQTT 状态
        if (_mqttMutex != NULL && xSemaphoreTake(_mqttMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            conn = mqttClient.connected();
            xSemaphoreGive(_mqttMutex);
        }
        
        _mqtt_connected_flag = conn; // 更新缓存给 UI 用

        if (!conn) {
            reconnectMqtt();
        } else {
            // [修复] 安全执行协议栈循环
            if (_mqttMutex != NULL && xSemaphoreTake(_mqttMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                mqttClient.loop();
                xSemaphoreGive(_mqttMutex);
            }
        }
    } else {
        _mqtt_connected_flag = false;
    }
}

void WouoWIFI_Class::reconnectMqtt() {
    static unsigned long lastReconnectAttempt = 0;
    long now = millis();
    
    // 限制重连频率，每5秒最多尝试一次
    if (now - lastReconnectAttempt > 5000) {
        lastReconnectAttempt = now;
        
        // 确保已经分配到有效 IP
        if (WiFi.localIP()[0] == 0) {
            Serial.println("[MQTT] Waiting for valid IP...");
            return; 
        }

        if (mqtt_server.length() > 0) {
            Serial.print("[MQTT] Connecting to "); Serial.println(mqtt_server);
            
            // 限制底层 TCP 读写超时时间为 2 秒
            espClient.setTimeout(2); 

            if (_mqttMutex != NULL && xSemaphoreTake(_mqttMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                mqttClient.setServer(mqtt_server.c_str(), mqtt_port);
                String clientId = "ESP32_GH_" + String(random(0xffff), HEX);
                
                // ==================== [终极修复] ====================
                // 临时关闭 Core 0 的硬件看门狗 (WDT)
                // 原因：当目标 IP 不可达时，底层的 TCP 握手可能会陷入超过 5 秒的底层自旋，
                // 从而饿死 Core 0 的 IDLE 任务并触发 WDT 重启。临时关狗强行续命防重启。
                disableCore0WDT();
                
                // 执行阻塞的连接请求
                bool connected = mqttClient.connect(clientId.c_str(), mqtt_user.c_str(), mqtt_pass.c_str());
                
                // 请求结束，立刻恢复 Core 0 的硬件看门狗
                enableCore0WDT();
                // ====================================================

                if (connected) {
                    Serial.println("[MQTT] Connected");
                    mqttClient.subscribe("home/greenhouse/pump/set");
                } else {
                    Serial.print("[MQTT] Failed, rc=");
                    Serial.println(mqttClient.state()); // 打印失败状态码
                }
                xSemaphoreGive(_mqttMutex);
            }
        }
    }
}

// ==================== [修复 4] WouoUI_WIFI.cpp 中的 MQTT 推送 ====================
void WouoWIFI_Class::sendSensorData(float temp, float hum, int soil, int light) {
    // [修复] 只有确认连接且能拿到锁时才推送，防止与 loop() 冲突
    if (_mqtt_connected_flag) {
        if (_mqttMutex != NULL && xSemaphoreTake(_mqttMutex, pdMS_TO_TICKS(30)) == pdTRUE) {
            char buf[10];
            dtostrf(temp, 4, 1, buf); mqttClient.publish("home/greenhouse/temp", buf);
            dtostrf(hum, 4, 1, buf);  mqttClient.publish("home/greenhouse/hum", buf);
            itoa(soil, buf, 10);      mqttClient.publish("home/greenhouse/soil", buf);
            itoa(light, buf, 10);     mqttClient.publish("home/greenhouse/light", buf);
            
            bool pumpState = false;
            if (g_dataMutex != NULL && xSemaphoreTake(g_dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                pumpState = g_sharedData.pump_state;
                xSemaphoreGive(g_dataMutex);
            }
            mqttClient.publish("home/greenhouse/pump/state", pumpState ? "ON" : "OFF");
            
            xSemaphoreGive(_mqttMutex);
        }
    }
}

void WouoWIFI_Class::setMode(uint8_t mode) {
    if (_mode == mode) return;
    
    // [修复] 锁定网络任务，防止 Core 0 访问即将被销毁的 WebServer 指针
    _wifiBusy = true; 
    delay(20); 
    
    Serial.print("[Mode] Switching to "); Serial.println(mode);
    _mode = mode;
    stopWiFi(); 
    
    if (_mode == 1) setupSTA();
    else if (_mode == 2) setupAP();

    // [修复] 释放网络任务
    _wifiBusy = false; 
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
    return _mqtt_connected_flag;
}