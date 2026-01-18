#include <Arduino.h>
#include "WouoUI.h"

void setup() {
    // 确保你的 GPIO 在 WouoUI.h 中配置正确
    WouoUI.begin();
}

void loop() {
    WouoUI.loop();
}