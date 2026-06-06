#ifndef WOUOUI_H
#define WOUOUI_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <EEPROM.h>
#include "WouoUI_WIFI.h"
#include <DHT.h>

// ==================== [新增] FreeRTOS 核心组件头文件 ====================
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/timers.h>

// ***************** 用户引脚配置 (ESP32-S3) *****************
#define OLED_SCL   15 
#define OLED_SDA   7
#define OLED_RES   4
#define OLED_DC    6
#define OLED_CS    5

#define KNOB_AIO   18 
#define KNOB_BIO   8
#define KNOB_SW    17

// ***************** 温室传感器与执行器配置 *****************
#define PIN_DHT     42    
#define PIN_LIGHT   38    
#define PIN_SOIL    45    
#define PIN_RELAY   21    
#define DHTTYPE     DHT11

#define AUTO_PUMP_ON_LIMIT  30  
#define AUTO_PUMP_OFF_LIMIT 60  

// ==================== [新增] 全局 FreeRTOS 句柄声明 ====================
extern QueueHandle_t g_btnQueue;     // 旋钮/按键事件消息队列句柄
extern TimerHandle_t g_sleepTimer;   // 自动息屏内核软件定时器句柄

// ==================== [新增] 软件定时器辅助控制函数 ====================
void resetSleepTimer();         // 喂狗函数：重置息屏定时器时间
void updateSleepTimerPeriod();  // 动态更新函数：动态改变息屏周期

// 结构体定义
typedef struct MENU
{
  const char *en; 
  const char *cn; 
} M_SELECT;

// 定义页面
enum 
{
  M_WINDOW,
  M_SLEEP,
    M_MAIN, 
        M_KNOB,
          M_KRF,
          M_KPF,
      M_SENSOR, 
      M_SETTING,
        M_ABOUT,
};

// 状态标签
enum
{
  S_FADE,       
  S_WINDOW,     
  S_LAYER_IN,   
  S_LAYER_OUT,  
  S_NONE,       
};

#define   UI_DEPTH            20    
#define   UI_MNUMB            100   
#define   UI_PARAM            19    
#define   DISP_H              128   
#define   DISP_W              128   

// 参数索引
enum 
{
  UI_LANG,      
  AUTO_SLP,     // 息屏时间 (0:从不, 1:3min, 2:5min, 3:10min)
  WIFI_SET,     
  DISP_BRI,     
  TILE_ANI,     
  LIST_ANI,     
  WIN_ANI,      
  SPOT_ANI,     
  TAG_ANI,      
  FADE_ANI,     
  BTN_SPT,      
  BTN_LPT,      
  TILE_UFD,     
  LIST_UFD,     
  TILE_LOOP,    
  LIST_LOOP,    
  WIN_BOK,      
  KNOB_DIR,     
  DARK_MODE,    
};

#define   KNOB_PARAM          4
#define   KNOB_DISABLE        0
#define   KNOB_ROT_VOL        1
#define   KNOB_ROT_BRI        2
enum 
{
  KNOB_ROT,       
  KNOB_COD,       
  KNOB_ROT_P,     
  KNOB_COD_P,     
};

#define   EEPROM_CHECK        11

// ==================== [修改] 按键与内核事件 ID ====================
#define   BTN_ID_CC           0   // 逆时针旋转
#define   BTN_ID_CW           1   // 顺时针旋转
#define   BTN_ID_SP           2   // 短按
#define   BTN_ID_LP           3   // 长按
#define   BTN_ID_SLEEP        4   // [新增] 内核定时器触发的息屏事件

// 共享数据结构体 (用于多任务数据安全传输)
typedef struct {
    float temp;
    float hum;
    int soil;
    int light;
    bool pump_state;
} SharedData_t;

extern SharedData_t g_sharedData;
extern SemaphoreHandle_t g_dataMutex;

class WouoUI_Class {
public:
    void begin();
    void loop();

    struct UI_VARS {
      bool      init;
      uint8_t   num[UI_MNUMB];
      uint8_t   select[UI_DEPTH];
      uint8_t   layer;
      uint8_t   index;
      uint8_t   state;
      bool      sleep;
      uint8_t   fade;
      uint8_t   param[UI_PARAM];
    } ui;

    struct TILE_VARS {
      float   title_y_calc;
      float   title_y_trg_calc;
      int16_t temp;
      bool    select_flag;
      float   icon_x;
      float   icon_x_trg;
      float   icon_y;
      float   icon_y_trg;
      float   indi_x; 
      float   indi_x_trg;
      float   title_y;
      float   title_y_trg;
    } tile;

    struct LIST_VARS {
      uint8_t line_n;
      int16_t temp;
      bool    loop;
      float   y;
      float   y_trg;
      float   box_x;
      float   box_x_trg;
      float   box_y;
      float   box_y_trg[UI_DEPTH];
      float   bar_y;
      float   bar_y_trg;
    } list;

    struct SENSOR_VARS {
      int     history[128]; 
      int     head_index;   
      float   val_current;  
      float   text_bg_l; 
      float   text_bg_l_trg;
      unsigned long last_read_time; 
    } sensor;

    struct CHECK_BOX_VARS {
      uint8_t *v;
      uint8_t *m;
      uint8_t *s;
      uint8_t *s_p;
    } check_box;

    struct WIN_VARS {
      uint8_t   *value;
      uint8_t   max;
      uint8_t   min;
      uint8_t   step;
      MENU      *bg;
      uint8_t   index;
      char      title[20];
      uint8_t   select;
      uint8_t   l;
      uint8_t   u;
      float     bar;
      float     bar_trg;
      float     y;
      float     y_trg;
      float     option_offset; 
      float     option_offset_trg;
    } win;

    struct ABOUT_VARS {
      float   indi_x; 
      float   indi_x_trg;
    } about;

    struct KNOB_VARS {
      uint8_t param[KNOB_PARAM];
    } knob;

    struct EEPROM_VARS {
      bool    init;
      bool    change;
      int     address;
      uint8_t check;
      uint8_t check_param[EEPROM_CHECK]; 
    } eeprom;

    struct BTN_VARS {
      uint8_t   id;
      volatile bool      flag;
      volatile bool      pressed;
      volatile bool      CW_1;
      volatile bool      CW_2;
      bool      val;
      bool      val_last;  
      volatile bool      alv;  
      volatile bool      blv;
      long      count;
    } volatile btn;

    DHT* dht;
    uint8_t   *buf_ptr;
    uint16_t  buf_len;

private:
    void oled_init();
    void btn_init();
    void btn_scan();
    void ui_init();
    void ui_param_init();
    void ui_proc();
    
    void eeprom_init();
    void eeprom_read_all_data();
    void eeprom_write_all_data();

    void sleep_param_init();
    void sleep_proc();

    void main_proc();
    void tile_param_init();
    void tile_show(struct MENU arr_1[], struct MENU arr_2[], const uint8_t icon_pic[][16*18]);
    void tile_rotate_switch();

    void list_show(struct MENU arr[], uint8_t ui_index);
    void list_rotate_switch();
    void list_draw_text_and_check_box(struct MENU arr[], int i);
    void list_draw_value(int n);
    void list_draw_check_box_frame();
    void list_draw_check_box_dot();
    void list_draw_krf(int n);
    void list_draw_kpf(int n);

    void knob_proc();
    void knob_param_init();
    void krf_proc();
    void krf_param_init();
    void kpf_proc();
    void kpf_param_init();
    
    void sensor_proc();       
    void sensor_param_init(); 
    void sensor_show();       
    
    void setting_proc();
    void setting_param_init();
    
    void about_proc();
    void about_param_init();
    void about_show();

    void window_param_init();
    void window_value_init(const char *title, uint8_t select, uint8_t *value, uint8_t max, uint8_t min, uint8_t step, MENU *bg, uint8_t index);
    void window_proc();
    void window_show();

    void layer_init_in();
    void layer_init_out();

    void check_box_v_init(uint8_t *param);
    void check_box_m_init(uint8_t *param);
    void check_box_s_init(uint8_t *param, uint8_t *param_p);
    void check_box_m_select(uint8_t param);
    void check_box_s_select(uint8_t val, uint8_t pos);

    void animation(float *a, float *a_trg, uint8_t n);
    void fade();
    const char* getStr(const M_SELECT &item);
};

extern WouoUI_Class WouoUI;

#endif