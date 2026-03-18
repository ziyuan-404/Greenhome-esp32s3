# **GreenHome ESP32-S3 智能温室与生态系统**

[English Version](https://github.com/ziyuan-404/Greenhome-esp32s3/blob/main/README.md)

## **项目概述**

GreenHome 是一套基于 ESP32-S3 微控制器的高级智能温室与环境监控系统。该项目并非单纯的传感器读取实验，而是我们之前多个学习模块（自定义 UI 框架、FreeRTOS 实时操作系统、传感器高级控制）的结晶与融合，将它们整合为了一个高内聚的统一架构。

本系统旨在复刻工业级或真实居家环境的智能自动化生态。为了确保极低的延迟、极高的稳定性以及绝对的数据主权，我们摒弃了依赖第三方云服务的传统方案，转而搭建了一个完全本地化的智能生态。该生态的中央服务器由一台退役的小米 Mix 2S 智能手机驱动，运行 Termux、Home Assistant (HA) 以及 EMQX MQTT 代理服务器。

## **核心特性**

* **生产级本地服务器**：使用运行 Termux 的小米 Mix 2S 作为强大的本地中控服务器，部署 Home Assistant 与 EMQX 代理。这种基于骁龙处理器的架构在性能上轻松超越基础的树莓派方案。  
* **力反馈智能旋钮联动**：生态系统内集成了一个创新的力反馈旋钮（通过 MQTT 连接）。用户可通过该旋钮控制温室及其他智能终端，并获得即时的物理触觉反馈（虚拟段落感）。  
* **FreeRTOS 多任务架构**：ESP32-S3 固件深度应用了 FreeRTOS 进行多任务调度，确保传感器数据采集、OLED UI 渲染以及 MQTT 网络通信并发运行，绝不阻塞关键的环境调节循环。  
* **交互式 OLED UI**：搭载深度定制的 WouoUI，直接在设备端提供流畅的动画、菜单和弹窗，全部通过本地的 EC11 旋转编码器进行操控。  
* **实时监控与自动化调节**：持续读取 DHT11（温湿度）、光敏电阻（光照强度）和土壤湿度传感器的数据。系统会根据非易失性存储器 (NVS) 中动态保存的阈值，自动触发水泵继电器。  
* **双语支持**：UI 界面可随时在英文和中文之间无缝切换。  
* **网页配网门户**：内置 AP（接入点）模式与 Web 服务器，允许用户轻松配置 WiFi 与 MQTT 参数，无需在源代码中硬编码账号密码。

## **项目文件结构**

项目代码基于 PlatformIO 组织，结构清晰，便于二次开发：

Greenhome-esp32s3/  
├── lib/  
│   └── WouoUI/                 \# 核心 UI、传感器及网络控制逻辑库  
│       ├── WouoUI.cpp          \# OLED 渲染、菜单逻辑、传感器定时读取与波形绘制  
│       ├── WouoUI.h            \# 引脚定义、参数宏及核心类声明  
│       ├── WouoUI\_Data.h       \# 菜单文本内容(中英双语)、图标点阵数据及字体  
│       ├── WouoUI\_WIFI.cpp     \# WiFi AP 配网页面、STA 连接及 MQTT 发布/订阅逻辑  
│       └── WouoUI\_WIFI.h       \# WiFi 与 MQTT 类的声明  
├── src/  
│   └── main.cpp                \# 极其精简的主程序入口，负责初始化并循环调用 UI 任务  
├── platformio.ini              \# PlatformIO 项目配置、编译参数及外部依赖库列表  
└── README.md                   \# 项目说明文档

## **系统架构与硬件需求**

架构分为信息链与能源链两部分。ESP32 专门通过 MQTT 与本地 Mix 2S 服务器通信。Home Assistant 负责收集这些数据流进行历史记录、高级自动化判定，并统一管理带有力反馈旋钮的其他智能外设。

### **主控端与外设 (节点端)**

* **微控制器**：ESP32-S3 DevKitC-1（或兼容开发板）  
* **显示屏**：128x128 SPI OLED 模块（推荐使用 SH1107 驱动）  
* **本地输入**：EC11 旋转编码器（带按键功能）  
* **传感器**：DHT11 温湿度传感器、模拟光敏传感器、模拟土壤湿度传感器  
* **执行器**：3.3V/5V 继电器模块与微型水泵

### **服务器与生态组件 (中枢端)**

* **中央服务器**：小米 Mix 2S（运行 Termux、Home Assistant Core 及 EMQX）  
* **智能终端**：自定义 MQTT 力反馈旋转编码器

## **硬件引脚配置 (接线说明)**

所有引脚配置均在 lib/WouoUI/WouoUI.h 中定义，请按以下说明接线：

* **OLED 显示屏 (SPI)**: SCL: 15, SDA: 7, RES: 4, DC: 6, CS: 5  
* **本地 EC11 旋钮**: Pin A: 18, Pin B: 8, Switch (SW): 17  
* **传感器与继电器**: DHT11: 42, 光照传感器: 38, 土壤湿度: 45, 水泵继电器: 21

## **部署与使用指南**

### **第一部分：ESP32-S3 固件编译与上传**

1. 下载并安装 Visual Studio Code，在扩展商店中安装 **PlatformIO IDE**。  
2. 将本项目文件夹克隆或解压到本地，使用 VS Code 打开该文件夹。PlatformIO 会自动读取 platformio.ini 并下载 U8g2、DHT、PubSubClient 等依赖库。  
3. 将 ESP32-S3 通过 USB Type-C 连接至电脑。  
4. 点击 VS Code 底部状态栏的 **Upload** (右箭头图标) 编译并烧录固件。

### **第二部分：ESP32-S3 设备网络配置**

1. 固件烧录完成后，OLED 屏幕亮起。使用旋转编码器，进入 Setting \-\> WiFi Setup，将其设置为 **AP (Access Point)** 模式。  
2. 使用手机或电脑搜索并连接名为 **ESP32\_Greenhouse** 的 WiFi 局域网（密码：12345678）。  
3. 打开浏览器，访问 http://192.168.4.1。  
4. 在配置页面中输入您家中的 WiFi SSID、密码，以及 Mix 2S 服务器的 IP 地址、MQTT 端口（默认1883）及认证信息。  
5. 点击 **Save & Reboot**。随后在 OLED 屏幕上再次进入设置，将 WiFi 切换回 **STA (Station)** 模式。设备将自动接入您的智能家居网络。

## **本地服务器搭建指南：小米 Mix 2S**

在安卓手机上运行完整的 HA 与 EMQX 需要借助 Termux 提供的 Linux 环境。具体步骤如下：

### **1\. 基础环境准备**

* **安装 Termux**：强烈建议从 F-Droid 应用商店下载最新版的 Termux（Google Play 版本已停止维护）。  
* **保活设置**：在安卓系统的电池设置中，将 Termux 设置为“无限制”，允许后台运行并锁定在最近任务中。在 Termux 终端中输入 termux-wake-lock 获取唤醒锁，防止息屏后进程休眠。

### **2\. 安装 Ubuntu 容器 (通过 proot-distro)**

直接在 Termux 本地环境中跑 HA 容易遇到依赖报错，使用 Ubuntu 容器最为稳妥：

pkg update && pkg upgrade \-y  
pkg install proot-distro \-y  
proot-distro install ubuntu  
proot-distro login ubuntu

### **3\. 安装 Home Assistant Core**

在 Ubuntu 容器内部执行以下命令部署 Python 环境并安装 HA：

apt update && apt upgrade \-y  
apt install python3 python3-pip python3-venv libffi-dev libssl-dev build-essential tzdata \-y  
\# 创建并激活虚拟环境  
python3 \-m venv ha-env  
source ha-env/bin/activate  
\# 安装 Home Assistant  
pip3 install wheel  
pip3 install homeassistant  
\# 启动 HA (初次启动需要下载大量依赖，请耐心等待 10-20 分钟)  
hass

启动成功后，即可在同一局域网的浏览器中访问 http://\<Mix\_2S\_局域网IP\>:8123 进行初始化配置。

### **4\. 安装 EMQX 代理服务器**

同样在 Ubuntu 容器内，下载 ARM64 架构的 EMQX 预编译包：

apt install wget curl \-y  
\# 请前往 EMQX 官网查看最新开源版 Linux ARM64 下载链接，此处为示例：  
wget \[https://www.emqx.com/en/downloads/broker/v5.x.x/emqx-5.x.x-ubuntu20.04-arm64.tar.gz\](https://www.emqx.com/en/downloads/broker/v5.x.x/emqx-5.x.x-ubuntu20.04-arm64.tar.gz)  
mkdir emqx && tar \-zxvf emqx-5.x.x-ubuntu20.04-arm64.tar.gz \-C emqx  
\# 启动 EMQX 服务  
./emqx/bin/emqx start

启动后，可通过 http://\<Mix\_2S\_局域网IP\>:18083 访问 EMQX 控制台（默认账号：admin，密码：public）。

## **MQTT 话题结构 (Topics)**

配置完毕后，ESP32 节点与 Mix 2S 中枢通过以下话题进行通信：

* **发布 (遥测数据，ESP32 \-\> 中枢)**:  
  * home/greenhouse/temp (温度)  
  * home/greenhouse/hum (空气湿度)  
  * home/greenhouse/soil (土壤湿度)  
  * home/greenhouse/light (光照强度)  
  * home/greenhouse/pump/state (水泵当前状态：ON / OFF)  
* **订阅 (控制指令，HA / 力反馈旋钮 \-\> ESP32)**:  
  * home/greenhouse/pump/set (发送 "ON" 或 "OFF" 以手动覆盖继电器状态)

## **未来改进计划**

尽管当前系统已经使用 ON/OFF 逻辑成功实现了自动化调节，但未来我们计划引入 PID（比例-积分-微分）控制算法，以实现更平滑的温度与湿度控制。此外，鉴于 Mix 2S 将作为 24/7 运行的服务器，我们计划对其进行硬件级直供电改造（绕过或拆除内部电池），以彻底消除电池鼓包或起火的安全隐患。

## **鸣谢**

* **项目开发者**: Xavier d'Anselme, He Linfeng (ziyuan)  
* **WouoUI 框架**: 原创作者为 RQNG，由 ziyuan 移植、修改并深度集成至本项目。