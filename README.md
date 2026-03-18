# **GreenHome ESP32-S3 Smart Greenhouse & Ecosystem**

[Chinese Version](https://github.com/ziyuan-404/Greenhome-esp32s3/blob/main/README_CH.md)

## **Project Overview**

GreenHome is an advanced smart greenhouse and environmental monitoring system based on the ESP32-S3 microcontroller. Rather than a simple sensor-reading experiment, this project is the culmination and fusion of our previous learning modules (custom UI frameworks, FreeRTOS real-time operating systems, advanced sensor control), integrating them into a highly cohesive, unified architecture.

This system is designed to replicate an industrial-grade or real-world home automation ecosystem. To ensure ultra-low latency, maximum stability, and absolute data sovereignty, we have discarded traditional solutions that rely on third-party cloud services. Instead, we built a fully localized smart ecosystem. The central server of this ecosystem is powered by a repurposed Xiaomi Mix 2S smartphone running Termux, Home Assistant (HA), and an EMQX MQTT broker.

## **Core Features**

* **Production-Grade Local Server**: Utilizes a Xiaomi Mix 2S running Termux as a powerful local central server to deploy Home Assistant and the EMQX broker. This Snapdragon-based architecture easily outperforms basic Raspberry Pi setups in performance.  
* **Force-Feedback Smart Knob Integration**: An innovative force-feedback rotary knob is integrated into the ecosystem (connected via MQTT). Users can use this knob to control the greenhouse and other smart terminals while receiving immediate physical tactile feedback (virtual detents).  
* **FreeRTOS Multi-Tasking Architecture**: The ESP32-S3 firmware deeply utilizes FreeRTOS for multi-task scheduling. This ensures that sensor data acquisition, OLED UI rendering, and MQTT network communication run concurrently without blocking the critical environmental regulation loop.  
* **Interactive OLED UI**: Powered by a heavily customized WouoUI, offering fluid animations, menus, and pop-up windows directly on the device, fully controlled via a local EC11 rotary encoder.  
* **Real-Time Monitoring & Automated Regulation**: Continuously reads data from the DHT11 (temperature/humidity), photoresistor (light level), and soil moisture sensors. The system automatically triggers the water pump relay based on thresholds dynamically saved in the Non-Volatile Storage (NVS).  
* **Bilingual Support**: The UI seamlessly switches between English and Chinese at any time.  
* **Web Configuration Portal**: Features a built-in AP (Access Point) mode and a web server, allowing users to easily configure WiFi and MQTT parameters without hardcoding credentials into the source code.

## **Project File Structure**

The project code is organized using PlatformIO, featuring a clear structure for easy secondary development:

Greenhome-esp32s3/  
├── lib/  
│   └── WouoUI/                 \# Core UI, sensor, and network control logic library  
│       ├── WouoUI.cpp          \# OLED rendering, menu logic, periodic sensor reading, waveform drawing  
│       ├── WouoUI.h            \# Pin definitions, parameter macros, and core class declarations  
│       ├── WouoUI\_Data.h       \# Menu text (bilingual), icon bitmap data, and fonts  
│       ├── WouoUI\_WIFI.cpp     \# WiFi AP portal, STA connection, and MQTT publish/subscribe logic  
│       └── WouoUI\_WIFI.h       \# WiFi and MQTT class declarations  
├── src/  
│   └── main.cpp                \# Minimal main entry point; handles initialization and UI task looping  
├── platformio.ini              \# PlatformIO configuration, build flags, and external dependency list  
└── README.md                   \# Project documentation

## **System Architecture & Hardware Requirements**

The architecture is divided into the information chain and the energy chain. The ESP32 communicates exclusively with the local Mix 2S server via MQTT. Home Assistant collects these data streams for historical logging, advanced automation decisions, and unified management of other smart peripherals, such as the force-feedback knob.

### **Main Controller & Peripherals (Node)**

* **Microcontroller**: ESP32-S3 DevKitC-1 (or compatible board)  
* **Display**: 128x128 SPI OLED Module (SH1107 driver recommended)  
* **Local Input**: EC11 Rotary Encoder (with push button)  
* **Sensors**: DHT11 Temperature & Humidity Sensor, Analog Light Sensor, Analog Soil Moisture Sensor  
* **Actuator**: 3.3V/5V Relay Module and a mini water pump

### **Server & Ecosystem Components (Hub)**

* **Central Server**: Xiaomi Mix 2S (Running Termux, Home Assistant Core, and EMQX)  
* **Smart Terminal**: Custom MQTT Force-Feedback Rotary Encoder

## **Hardware Pin Configuration (Wiring)**

All pin configurations are defined in lib/WouoUI/WouoUI.h. Please wire your components as follows:

* **OLED Display (SPI)**: SCL: 15, SDA: 7, RES: 4, DC: 6, CS: 5  
* **Local EC11 Knob**: Pin A: 18, Pin B: 8, Switch (SW): 17  
* **Sensors & Relay**: DHT11: 42, Light Sensor: 38, Soil Moisture: 45, Water Pump Relay: 21

## **Deployment & Usage Guide**

### **Part 1: ESP32-S3 Firmware Compilation & Upload**

1. Download and install Visual Studio Code, then install the **PlatformIO IDE** from the extensions marketplace.  
2. Clone or extract this project folder to your local machine, and open it in VS Code. PlatformIO will automatically read platformio.ini and download dependencies like U8g2, DHT, and PubSubClient.  
3. Connect the ESP32-S3 to your computer via a USB Type-C cable.  
4. Click the **Upload** button (the right-arrow icon) on the bottom status bar in VS Code to compile and flash the firmware.

### **Part 2: ESP32-S3 Device Network Configuration**

1. After the firmware is flashed, the OLED screen will turn on. Use the rotary encoder to navigate to Setting \-\> WiFi Setup and set it to **AP (Access Point)** mode.  
2. Use your phone or computer to search for and connect to the WiFi network named **ESP32\_Greenhouse** (Password: 12345678).  
3. Open a browser and visit http://192.168.4.1.  
4. On the configuration page, enter your home WiFi SSID, password, the Mix 2S server's IP address, MQTT port (default 1883), and authentication credentials.  
5. Click **Save & Reboot**. Then, navigate back to the settings on the OLED screen and switch the WiFi back to **STA (Station)** mode. The device will automatically join your smart home network.

## **Local Server Setup Guide: Xiaomi Mix 2S**

Running a complete HA and EMQX environment on an Android phone requires the Linux environment provided by Termux. Follow these steps:

### **1\. Basic Environment Preparation**

* **Install Termux**: It is highly recommended to download the latest version of Termux from the F-Droid app store (the Google Play version is no longer maintained).  
* **Keep-Alive Settings**: In your Android battery settings, set Termux to "Unrestricted" to allow it to run in the background, and lock the app in recent tasks. In the Termux terminal, type termux-wake-lock to acquire a wake lock and prevent the process from sleeping when the screen is off.

### **2\. Install Ubuntu Container (via proot-distro)**

Running HA directly in native Termux often leads to dependency errors. Using an Ubuntu container is the most stable approach:

pkg update && pkg upgrade \-y  
pkg install proot-distro \-y  
proot-distro install ubuntu  
proot-distro login ubuntu

### **3\. Install Home Assistant Core**

Inside the Ubuntu container, execute the following commands to set up the Python environment and install HA:

apt update && apt upgrade \-y  
apt install python3 python3-pip python3-venv libffi-dev libssl-dev build-essential tzdata \-y  
\# Create and activate a virtual environment  
python3 \-m venv ha-env  
source ha-env/bin/activate  
\# Install Home Assistant  
pip3 install wheel  
pip3 install homeassistant  
\# Start HA (The first startup will download many dependencies, please be patient for 10-20 minutes)  
hass

Once successfully started, you can access http://\<Mix\_2S\_Local\_IP\>:8123 from a browser on the same local network to perform the initial configuration.

### **4\. Install EMQX Broker**

Still inside the Ubuntu container, download the pre-compiled EMQX package for the ARM64 architecture:

apt install wget curl \-y  
\# Please check the EMQX official website for the latest open-source Linux ARM64 download link; this is an example:  
wget \[https://www.emqx.com/en/downloads/broker/v5.x.x/emqx-5.x.x-ubuntu20.04-arm64.tar.gz\](https://www.emqx.com/en/downloads/broker/v5.x.x/emqx-5.x.x-ubuntu20.04-arm64.tar.gz)  
mkdir emqx && tar \-zxvf emqx-5.x.x-ubuntu20.04-arm64.tar.gz \-C emqx  
\# Start the EMQX service  
./emqx/bin/emqx start

Once started, access the EMQX dashboard via http://\<Mix\_2S\_Local\_IP\>:18083 (Default username: admin, Password: public).

## **MQTT Topics Structure**

After configuration, the ESP32 node and the Mix 2S hub communicate using the following topics:

* **Publish (Telemetry Data, ESP32 \-\> Hub)**:  
  * home/greenhouse/temp (Temperature)  
  * home/greenhouse/hum (Air Humidity)  
  * home/greenhouse/soil (Soil Moisture)  
  * home/greenhouse/light (Light Intensity)  
  * home/greenhouse/pump/state (Current pump status: ON / OFF)  
* **Subscribe (Control Commands, HA / Force-Feedback Knob \-\> ESP32)**:  
  * home/greenhouse/pump/set (Send "ON" or "OFF" to manually override the relay)

## **Future Improvements**

Although the current system successfully implements automated regulation using ON/OFF logic, we plan to introduce a PID (Proportional-Integral-Derivative) control algorithm in the future to achieve much smoother temperature and humidity stability. Additionally, since the Mix 2S will serve as a 24/7 server, we plan to modify its hardware for direct power supply (bypassing or removing the internal battery) to eliminate safety risks such as battery swelling or fire.

## **Credits**

* **Project Developers**: Xavier d'Anselme, He Linfeng (ziyuan)  
* **WouoUI Framework**: Originally created by RQNG, ported, modified, and deeply integrated into this project by ziyuan.