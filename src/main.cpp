#include <Arduino.h>
#include "WouoUI.h"

// ==================== 1. 传感器后台精密采集任务 (Core 1) ====================
void sensorTask(void *pvParameters) 
{
    // 利用内核高精度绝对延时，锁定 1000ms 的执行周期
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1000);
    static bool last_is_dry = false;

    while (true) 
    {
        // 采集物理传感器原始电平
        float t = WouoUI.dht->readTemperature();
        float h = WouoUI.dht->readHumidity();
        if (isnan(t)) t = 0; 
        if (isnan(h)) h = 0;
        
        int s = map(analogRead(PIN_SOIL), 0, 4095, 0, 100); 
        int l = map(analogRead(PIN_LIGHT), 0, 4095, 0, 100);
        bool current_pump = digitalRead(PIN_RELAY);

        // 温室水泵自动化控制 (边缘触发判定)
        bool current_is_dry = (s < AUTO_PUMP_ON_LIMIT);
        if (current_is_dry && !last_is_dry) 
        {
            digitalWrite(PIN_RELAY, HIGH);
            current_pump = true; 
        }
        last_is_dry = current_is_dry;

        // 互斥锁保护：安全地将新采集的数据抄写至前台共享数据中心
        if (g_dataMutex != NULL && xSemaphoreTake(g_dataMutex, portMAX_DELAY) == pdTRUE) 
        {
            g_sharedData.temp = t;
            g_sharedData.hum = h;
            g_sharedData.soil = s;
            g_sharedData.light = l;
            g_sharedData.pump_state = current_pump;
            xSemaphoreGive(g_dataMutex); // 迅速交还互斥锁
        }

        // 定时通过 STA 网络进行数据打包发布
        if (WouoWIFI.getMode() == 1) 
        {
            WouoWIFI.sendSensorData(t, h, s, l);
        }

        // 绝对延时，腾出核心控制权，转入阻塞等待
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