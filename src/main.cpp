#include <Arduino.h>
#include "WouoUI.h"

// ==================== 1. 传感器后台精密采集任务 (Core 1) ====================
// ==================== [修复 3] main.cpp 中的后台任务 ====================
void sensorTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1000);
    static bool last_is_dry = false;

    while (true) {
        float t = WouoUI.dht->readTemperature();
        float h = WouoUI.dht->readHumidity();
        if (isnan(t)) t = 0; 
        if (isnan(h)) h = 0;
        
        int s = map(analogRead(PIN_SOIL), 4095, 0, 0, 100);
        int l = map(analogRead(PIN_LIGHT), 0, 4095, 0, 100);

        // [关键修复]：从共享变量读取当前状态，绝不依赖 digitalRead
        bool current_pump = false;
        if (g_dataMutex != NULL && xSemaphoreTake(g_dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            current_pump = g_sharedData.pump_state;
            xSemaphoreGive(g_dataMutex);
        }

        // 自动化控制逻辑 (边缘触发)
        bool current_is_dry = (s < AUTO_PUMP_ON_LIMIT);
        if (current_is_dry && !last_is_dry) {
            digitalWrite(PIN_RELAY, HIGH);
            current_pump = true; 
            Serial.println("[AUTO] 土壤过干，自动启动水泵");
        }
        last_is_dry = current_is_dry;

        // 安全写入共享数据
        if (g_dataMutex != NULL && xSemaphoreTake(g_dataMutex, portMAX_DELAY) == pdTRUE) {
            g_sharedData.temp = t;
            g_sharedData.hum = h;
            g_sharedData.soil = s;
            g_sharedData.light = l;
            g_sharedData.pump_state = current_pump;
            xSemaphoreGive(g_dataMutex);
        }

        if (WouoWIFI.getMode() == 1) {
            WouoWIFI.sendSensorData(t, h, s, l);
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// ==================== 2. WiFi 与 MQTT 后台异步通信任务 (Core 0) ====================
void networkTask(void *pvParameters) 
{
    while (true) 
    {
        WouoWIFI.loop(); // 维系软 AP 配网网页交互以及 MQTT 内部保活 ping 
        vTaskDelay(pdMS_TO_TICKS(10)); // 留给底层的 TCP/IP 协议栈充足的时间切片
    }
}

// ==================== 3. 系统总入口初始化 ====================
void setup() 
{
    // 初始化数据隔离锁
    g_dataMutex = xSemaphoreCreateMutex();
    
    // 先初始化 WiFi 类！
    WouoWIFI.begin();

    // 初始化硬件底层及前台 UI 资源
    WouoUI.begin();

    // 创建独立核心网络守护任务 (分配在 Core 0 运行)
    xTaskCreatePinnedToCore(networkTask, "NetworkTask", 4096, NULL, 1, NULL, 0);
    
    // 创建高精传感器采集任务 (分配在 Core 1 运行，优先级设为 2，确保数据准时更新)
    xTaskCreatePinnedToCore(sensorTask, "SensorTask", 4096, NULL, 2, NULL, 1);
}

void loop() 
{
    // 前台主线程完全服务于图形渲染、动画推进与按键事件消费
    WouoUI.loop();
    vTaskDelay(pdMS_TO_TICKS(2)); // 极小的释放，防止内核单线程霸占饥饿
}