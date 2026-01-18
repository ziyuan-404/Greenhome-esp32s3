#ifndef WOUOUI_H
#define WOUOUI_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <EEPROM.h>
#include "WouoUI_WIFI.h"
#include <DHT.h>

// ***************** 用户引脚配置 (ESP32-S3) *****************
// 软件SPI屏幕引脚
#define OLED_SCL   15 
#define OLED_SDA   7
#define OLED_RES   4
#define OLED_DC    6
#define OLED_CS    5

// EC11 旋钮引脚
#define KNOB_AIO   18 
#define KNOB_BIO   8
#define KNOB_SW    17

// ***************** 温室传感器与执行器配置 *****************
#define PIN_DHT     42    // DHT11 数据引脚
#define PIN_LIGHT   38    // 光敏电阻 AO引脚
#define PIN_SOIL    45    // 土壤湿度 AO引脚
#define PIN_RELAY   21    // 水泵继电器引脚
#define DHTTYPE     DHT11

// 自动化控制阈值
#define AUTO_PUMP_ON_LIMIT  30  // 土壤湿度低于 30% 自动开启
#define AUTO_PUMP_OFF_LIMIT 60  // 土壤湿度高于 60% 自动关闭

/************************************* 结构体定义 *************************************/
typedef struct MENU
{
  const char *en; // 英文内容
  const char *cn; // 中文内容
} M_SELECT;

/************************************* 定义页面 *************************************/
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

//状态，初始化标签
enum
{
  S_FADE,       //转场动画
  S_WINDOW,     //弹窗初始化
  S_LAYER_IN,   //层级初始化
  S_LAYER_OUT,  //层级初始化
  S_NONE,       //直接选择页面
};

//UI变量宏
#define   UI_DEPTH            20    //最深层级数
#define   UI_MNUMB            100   //菜单数量
#define   UI_PARAM            19    //参数数量

// 功能宏定义
#define   DISP_H              128   //屏幕高度
#define   DISP_W              128   //屏幕宽度

// 参数索引
enum 
{
  UI_LANG,      //语言设置 (0: EN, 1: CN) 
  AUTO_SLP,     //自动息屏时间 (0:从不, 1:3min, 2:5min, 3:10min)
  WIFI_SET,     //WiFi设置 (0:关, 1:STA, 2:AP) 
  DISP_BRI,     //屏幕亮度
  TILE_ANI,     //磁贴动画速度
  LIST_ANI,     //列表动画速度
  WIN_ANI,      //弹窗动画速度
  SPOT_ANI,     //聚光动画速度
  TAG_ANI,      //标签动画速度
  FADE_ANI,     //消失动画速度
  BTN_SPT,      //按键短按时长
  BTN_LPT,      //按键长按时长
  TILE_UFD,     //磁贴图标从头展开开关
  LIST_UFD,     //菜单列表从头展开开关
  TILE_LOOP,    //磁贴图标循环模式开关
  LIST_LOOP,    //菜单列表循环模式开关
  WIN_BOK,      //弹窗背景虚化开关
  KNOB_DIR,     //旋钮方向切换开关
  DARK_MODE,    //黑暗模式开关
  
};

// 旋钮参数
#define   KNOB_PARAM          4
#define   KNOB_DISABLE        0
#define   KNOB_ROT_VOL        1
#define   KNOB_ROT_BRI        2
enum 
{
  KNOB_ROT,       //睡眠下旋转旋钮的功能，0 禁用，1 音量，2 亮度
  KNOB_COD,       //睡眠下短按旋钮输入的字符码，0 禁用
  KNOB_ROT_P,     //旋转旋钮功能在单选框中选择的位置
  KNOB_COD_P,     //字符码在单选框中选择的位置
};

// 断电保存
#define   EEPROM_CHECK        11

// 按键ID
#define   BTN_ID_CC           0   //逆时针旋转
#define   BTN_ID_CW           1   //顺时针旋转
#define   BTN_ID_SP           2   //短按
#define   BTN_ID_LP           3   //长按

// 为了保持代码结构与原版一致，我们将变量封装在类中
class WouoUI_Class {
public:
    void begin();
    void loop();

    // 将原本的结构体定义在类内部或作为成员
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
      int     history[128]; // 记录128个点的历史数据用于绘图
      int     head_index;   // 循环缓冲区头指针
      float   val_current;  // 当前值
      float   text_bg_l; 
      float   text_bg_l_trg;
      unsigned long last_read_time; // 读取间隔控制
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
    // Buffer vars
    uint8_t   *buf_ptr;
    uint16_t  buf_len;
    
    //自动息屏计时器
    unsigned long sleep_timer = 0;


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
    
    // [修改] 传感器相关
    void sensor_proc();       // 原 volt_proc
    void sensor_param_init(); // 原 volt_param_init
    void sensor_show();       // 原 volt_show
    
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