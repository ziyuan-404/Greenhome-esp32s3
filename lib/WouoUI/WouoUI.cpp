#include "WouoUI.h"
#include "WouoUI_Data.h" 

// 实例化全局对象
WouoUI_Class WouoUI;

// 硬件 SPI 定义
U8G2_SH1107_PIMORONI_128X128_F_4W_HW_SPI u8g2(U8G2_R0, OLED_CS, OLED_DC, OLED_RES);

// 字体定义 (使用支持中文的字体)
// wqy12 覆盖了 GB2312 字符集，足够日常使用
#define   FONT_CN_LIST        u8g2_font_wqy12_t_gb2312
#define   FONT_CN_TITLE       u8g2_font_wqy16_t_gb2312 
#define   FONT_EN_LIST        u8g2_font_HelvetiPixel_tr 
#define   FONT_EN_TITLE       u8g2_font_helvB24_tr

// 磁贴变量
#define   TILE_B_TITLE_H      25                          
#define   TILE_S_TITLE_H      8                           
#define   TILE_ICON_H         48                          
#define   TILE_ICON_W         48                          
#define   TILE_ICON_S         57                          
#define   TILE_INDI_H         40                          
#define   TILE_INDI_W         10                          
#define   TILE_INDI_S         57                          

// 列表变量
#define   LIST_TEXT_H         8                           
#define   LIST_LINE_H         16                          
#define   LIST_TEXT_S         4                           
#define   LIST_BAR_W          5                           
#define   LIST_BOX_R          0.5f                                                

// 选择框变量
#define   CHECK_BOX_L_S       95                          
#define   CHECK_BOX_U_S       2                           
#define   CHECK_BOX_F_W       12                          
#define   CHECK_BOX_F_H       12                          
#define   CHECK_BOX_D_S       2                           

// 弹窗变量
#define   WIN_H               64                          
#define   WIN_W               102                         
#define   WIN_BAR_W           92                          
#define   WIN_BAR_H           7                           
#define   WIN_Y               - WIN_H - 2                 
#define   WIN_Y_TRG           - WIN_H - 2                 

// 关于本机页变量
#define   ABOUT_INDI_S        4                           
#define   ABOUT_INDI_W        2                           

// 按键变量
#define   BTN_PARAM_TIMES     2 

// [修改] 曲线图变量适应 SH1107 和传感器数值范围
#define   WAVE_W              128                     
#define   WAVE_L              0                           
#define   WAVE_U              0                           
#define   WAVE_BOX_H          49                          
#define   WAVE_BOX_W          DISP_W                      
#define   VOLT_LIST_U_S       94                          
#define   VOLT_TEXT_BG_U_S    53                          
#define   VOLT_TEXT_BG_H      33 

// 中断函数 (ESP32需要IRAM_ATTR)
static void IRAM_ATTR knob_inter() 
{
  WouoUI.btn.alv = digitalRead(KNOB_AIO);
  WouoUI.btn.blv = digitalRead(KNOB_BIO);
  if (!WouoUI.btn.flag && WouoUI.btn.alv == LOW) 
  {
    WouoUI.btn.CW_1 = WouoUI.btn.blv;
    WouoUI.btn.flag = true;
  }
  if (WouoUI.btn.flag && WouoUI.btn.alv) 
  {
    WouoUI.btn.CW_2 = !WouoUI.btn.blv;
    if (WouoUI.btn.CW_1 && WouoUI.btn.CW_2)
     {
      WouoUI.btn.id = WouoUI.ui.param[KNOB_DIR];
      WouoUI.btn.pressed = true;
    }
    if (WouoUI.btn.CW_1 == false && WouoUI.btn.CW_2 == false) 
    {
      WouoUI.btn.id = !WouoUI.ui.param[KNOB_DIR];
      WouoUI.btn.pressed = true;
    }
    WouoUI.btn.flag = false;
  }
}

// -------------------------------------------------------------------------
// 类实现
// -------------------------------------------------------------------------

// 新增辅助函数：根据语言设置返回字符串
const char* WouoUI_Class::getStr(const M_SELECT &item) {
    if (ui.param[UI_LANG] == 1) return item.cn; // 1 = 中文
    return item.en; // 0 = 英文
}

void WouoUI_Class::btn_scan() 
{
  btn.val = digitalRead(KNOB_SW);
  if (btn.val != btn.val_last)
  {
    btn.val_last = btn.val;
    delay(ui.param[BTN_SPT] * BTN_PARAM_TIMES);
    btn.val = digitalRead(KNOB_SW);
    if (btn.val == LOW)
    {
      // [新增] 任何按键动作都重置息屏计时
      sleep_timer = millis();

      btn.pressed = true;
      btn.count = 0;
      while (!digitalRead(KNOB_SW))
      {
        btn.count++;
        delay(1);
      }
      if (btn.count < ui.param[BTN_LPT] * BTN_PARAM_TIMES)  btn.id = BTN_ID_SP;
      else  btn.id = BTN_ID_LP;
    }
  }
}

void WouoUI_Class::btn_init() 
{
  pinMode(KNOB_AIO, INPUT);
  pinMode(KNOB_BIO, INPUT);
  pinMode(KNOB_SW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(KNOB_AIO), knob_inter, CHANGE);
}

void WouoUI_Class::eeprom_write_all_data()
{
  eeprom.address = 0;
  for (uint8_t i = 0; i < EEPROM_CHECK; ++i)    EEPROM.write(eeprom.address + i, eeprom.check_param[i]);  eeprom.address += EEPROM_CHECK;
  for (uint8_t i = 0; i < UI_PARAM; ++i)        EEPROM.write(eeprom.address + i, ui.param[i]);            eeprom.address += UI_PARAM;
  for (uint8_t i = 0; i < KNOB_PARAM; ++i)      EEPROM.write(eeprom.address + i, knob.param[i]);          eeprom.address += KNOB_PARAM;
  EEPROM.commit(); 
}

void WouoUI_Class::eeprom_read_all_data()
{
  eeprom.address = EEPROM_CHECK;   
  for (uint8_t i = 0; i < UI_PARAM; ++i)        ui.param[i]   = EEPROM.read(eeprom.address + i);          eeprom.address += UI_PARAM;
  for (uint8_t i = 0; i < KNOB_PARAM; ++i)      knob.param[i] = EEPROM.read(eeprom.address + i);          eeprom.address += KNOB_PARAM;
}

void WouoUI_Class::ui_param_init()
{
  ui.param[UI_LANG]   = 0;        //默认英文 (0:EN, 1:CN) 
  ui.param[AUTO_SLP]  = 1;        // 默认3分钟
  ui.param[WIFI_SET]  = 0;        // 默认关闭
  ui.param[DISP_BRI]  = 255;      //屏幕亮度
  ui.param[TILE_ANI]  = 30;       //磁贴动画速度
  ui.param[LIST_ANI]  = 60;       //列表动画速度
  ui.param[WIN_ANI]   = 25;       //弹窗动画速度
  ui.param[SPOT_ANI]  = 50;       //聚光动画速度
  ui.param[TAG_ANI]   = 60;       //标签动画速度
  ui.param[FADE_ANI]  = 30;       //消失动画速度
  ui.param[BTN_SPT]   = 25;       //按键短按时长
  ui.param[BTN_LPT]   = 150;      //按键长按时长
  ui.param[TILE_UFD]  = 1;        //磁贴图标从头展开开关
  ui.param[LIST_UFD]  = 1;        //菜单列表从头展开开关
  ui.param[TILE_LOOP] = 0;        //磁贴图标循环模式开关
  ui.param[LIST_LOOP] = 0;        //菜单列表循环模式开关
  ui.param[WIN_BOK]   = 0;        //弹窗背景虚化开关
  ui.param[KNOB_DIR]  = 0;        //旋钮方向切换开关   
  ui.param[DARK_MODE] = 1;        //黑暗模式开关        
}

void WouoUI_Class::eeprom_init()
{
  EEPROM.begin(4096); 
  eeprom.check = 0;
  eeprom.address = 0; 
  eeprom.check_param[0]='a'; eeprom.check_param[1]='b'; eeprom.check_param[2]='c';
  eeprom.check_param[3]='d'; eeprom.check_param[4]='e'; eeprom.check_param[5]='f';
  eeprom.check_param[6]='g'; eeprom.check_param[7]='h'; eeprom.check_param[8]='i';
  eeprom.check_param[9]='j'; eeprom.check_param[10]='k';

  for (uint8_t i = 0; i < EEPROM_CHECK; ++i)  if (EEPROM.read(eeprom.address + i) != eeprom.check_param[i])  eeprom.check ++;
  if (eeprom.check <= 1) eeprom_read_all_data();  
  else ui_param_init();
}

void WouoUI_Class::check_box_v_init(uint8_t *param) { check_box.v = param; }
void WouoUI_Class::check_box_m_init(uint8_t *param) { check_box.m = param; }
void WouoUI_Class::check_box_s_init(uint8_t *param, uint8_t *param_p) { check_box.s = param; check_box.s_p = param_p; }

void WouoUI_Class::check_box_m_select(uint8_t param)
{
  check_box.m[param] = !check_box.m[param];
  eeprom.change = true;
}

void WouoUI_Class::check_box_s_select(uint8_t val, uint8_t pos)
{
  *check_box.s = val;
  *check_box.s_p = pos;
  eeprom.change = true;
}

void WouoUI_Class::window_value_init(const char *title, uint8_t select, uint8_t *value, uint8_t max, uint8_t min, uint8_t step, MENU *bg, uint8_t index)
{
  strcpy(win.title, title);
  win.select = select;
  win.value = value;
  win.max = max;
  win.min = min;
  win.step = step;
  win.bg = bg;
  win.index = index;  
  ui.index = M_WINDOW;
  ui.state = S_WINDOW;
}

void WouoUI_Class::ui_init()
{
  ui.num[M_MAIN]      = sizeof( main_menu     )   / sizeof(M_SELECT);
  ui.num[M_KNOB]      = sizeof( knob_menu     )   / sizeof(M_SELECT);
  ui.num[M_KRF]       = sizeof( krf_menu      )   / sizeof(M_SELECT);
  ui.num[M_KPF]       = sizeof( kpf_menu      )   / sizeof(M_SELECT);
  ui.num[M_SENSOR]    = sizeof( sensor_menu   )   / sizeof(M_SELECT); 
  ui.num[M_SETTING]   = sizeof( setting_menu  )   / sizeof(M_SELECT);
  ui.num[M_ABOUT]     = sizeof( about_menu    )   / sizeof(M_SELECT);   

  ui.layer = 0;
  ui.index = M_SLEEP; // 或者 M_MAIN
  ui.state = S_NONE;
  ui.sleep = true;    
  ui.fade = 1;

  // 默认亮屏逻辑
  u8g2.setPowerSave(0);
  ui.sleep = false;
  ui.index = M_MAIN; 
  
  tile_param_init(); 
  tile.icon_x = tile.icon_x_trg;
  tile.icon_y = tile.icon_y_trg;
  
  sleep_timer = millis(); 

  knob.param[0] = KNOB_DISABLE;
  knob.param[1] = KNOB_DISABLE;
  knob.param[2] = 2;
  knob.param[3] = 2;
  
  // [修改] 删除 analog_pin 数组初始化，改为初始化传感器
  dht = new DHT(PIN_DHT, DHTTYPE);
  dht->begin();
  pinMode(PIN_LIGHT, INPUT);
  pinMode(PIN_SOIL, INPUT);
}

unsigned long getSleepTimeMS(uint8_t index) {
    switch(index) {
        case 0: return 0; // Never
        case 1: return 3 * 60 * 1000;
        case 2: return 5 * 60 * 1000;
        case 3: return 10 * 60 * 1000;
        default: return 0;
    }
}

void WouoUI_Class::tile_param_init()
{
  ui.init = false;
  tile.title_y_calc = TILE_INDI_S + (TILE_INDI_H - TILE_B_TITLE_H) / 2 + TILE_B_TITLE_H * 2;
  tile.title_y_trg_calc = TILE_INDI_S + (TILE_INDI_H - TILE_B_TITLE_H) / 2 + TILE_B_TITLE_H;
  
  tile.icon_x = 0;
  tile.icon_x_trg = TILE_ICON_S;
  tile.icon_y = -TILE_ICON_H;
  tile.icon_y_trg = 0;
  tile.indi_x = 0;
  tile.indi_x_trg = TILE_INDI_W;
  tile.title_y = tile.title_y_calc;
  tile.title_y_trg = tile.title_y_trg_calc;
}

void WouoUI_Class::sleep_param_init()
{
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 0, DISP_W, DISP_H);
  u8g2.setPowerSave(1);
  ui.state = S_NONE;  
  ui.sleep = true;
  if (eeprom.change)
  {
    eeprom_write_all_data();
    eeprom.change = false;
  }
}

void WouoUI_Class::knob_param_init() { check_box_v_init(knob.param); }
void WouoUI_Class::krf_param_init() { check_box_s_init(&knob.param[KNOB_ROT], &knob.param[KNOB_ROT_P]); }
void WouoUI_Class::kpf_param_init() { check_box_s_init(&knob.param[KNOB_COD], &knob.param[KNOB_COD_P]); }

void WouoUI_Class::sensor_param_init()
{
  sensor.text_bg_l = 0;
  sensor.text_bg_l_trg = DISP_W; 
  
  // 初始化历史数据
  sensor.head_index = 0;
  for(int i=0; i<128; i++) sensor.history[i] = 0;
  sensor.last_read_time = 0;
}

void WouoUI_Class::setting_param_init()
{
  check_box_v_init(ui.param);
  check_box_m_init(ui.param);
}

void WouoUI_Class::about_param_init()
{
  about.indi_x = 0;
  about.indi_x_trg = ABOUT_INDI_S;
}

void WouoUI_Class::window_param_init()
{
  win.bar = 0;
  win.l = (DISP_W - WIN_W) / 2;
  win.u = (DISP_H - WIN_H) / 2;
  win.y = WIN_Y;
  win.y_trg = win.u;
  
  // [新增] 初始化选项动画偏移
  win.option_offset = 0;
  win.option_offset_trg = 0;
  
  ui.state = S_NONE;
}

void WouoUI_Class::layer_init_in()
{
  ui.layer ++;
  ui.init = 0;
  list.line_n = DISP_H / LIST_LINE_H;
  list.y = 0;
  list.y_trg = LIST_LINE_H;
  list.box_x = 0;
  list.box_y = 0;
  list.bar_y = 0;
  ui.state = S_FADE;
  switch (ui.index)
  {
    case M_MAIN:    tile_param_init();    break;  
    case M_KNOB:    knob_param_init();    break;  
    case M_KRF:     krf_param_init();     break;    
    case M_KPF:     kpf_param_init();     break;   
    case M_SENSOR:  sensor_param_init();  break; 
    case M_SETTING: setting_param_init(); break;  
    case M_ABOUT:   about_param_init();   break;  
  }
}

void WouoUI_Class::layer_init_out()
{
  ui.select[ui.layer] = 0;
  list.box_y_trg[ui.layer] = 0;
  ui.layer --;
  ui.init = 0;
  list.y = 0;
  list.y_trg = LIST_LINE_H;
  list.bar_y = 0;
  ui.state = S_FADE;
  switch (ui.index)
  {
    case M_SLEEP: sleep_param_init(); break;    
    case M_MAIN:  tile_param_init();  break;    
  }
}

void WouoUI_Class::animation(float *a, float *a_trg, uint8_t n)
{
  if (*a != *a_trg)
  {
    if (fabs(*a - *a_trg) < 0.15f) *a = *a_trg;
    else *a += (*a_trg - *a) / (ui.param[n] / 10.0f);
  }
}

void WouoUI_Class::fade()
{
  delay(ui.param[FADE_ANI]);
  if (ui.param[DARK_MODE])
  {
    switch (ui.fade)
    {
      case 1: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] & 0xAA; break;
      case 2: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] & 0x00; break;
      case 3: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] & 0x55; break;
      case 4: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] & 0x00; break;
      default: ui.state = S_NONE; ui.fade = 0; break;
    }
  }
  else
  {
    switch (ui.fade)
    {
      case 1: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] | 0xAA; break;
      case 2: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] | 0x00; break;
      case 3: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] | 0x55; break;
      case 4: for (uint16_t i = 0; i < buf_len; ++i)  if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] | 0x00; break;
      default: ui.state = S_NONE; ui.fade = 0; break;
    }    
  }
  ui.fade++;
}

void WouoUI_Class::tile_show(struct MENU arr_1[], struct MENU arr_2[], const uint8_t icon_pic[][16*18])
{
  animation(&tile.icon_x, &tile.icon_x_trg, TILE_ANI);
  animation(&tile.icon_y, &tile.icon_y_trg, TILE_ANI);
  animation(&tile.indi_x, &tile.indi_x_trg, TILE_ANI);
  animation(&tile.title_y, &tile.title_y_trg, TILE_ANI);

  u8g2.setDrawColor(1);
  u8g2.setFontDirection(0);

  // 根据语言选择大标题字体
  if (ui.param[UI_LANG] == 1) u8g2.setFont(FONT_CN_TITLE);
  else u8g2.setFont(FONT_EN_TITLE);
  
  // 使用 getStr 获取对应语言的字符串
  u8g2.drawUTF8(((DISP_W - TILE_INDI_W) - u8g2.getUTF8Width(getStr(arr_1[ui.select[ui.layer]]))) / 2 + TILE_INDI_W, tile.title_y, getStr(arr_1[ui.select[ui.layer]]));
  // 小标题统一使用列表字体 (中文使用 wqy12)
  if (ui.param[UI_LANG] == 1) u8g2.setFont(FONT_CN_LIST);
  else u8g2.setFont(FONT_EN_LIST);
  
  u8g2.drawUTF8(((DISP_W - u8g2.getUTF8Width(getStr(arr_2[ui.select[ui.layer]]))) / 2), 0.5 * (TILE_ICON_S + TILE_INDI_H + DISP_H + LIST_TEXT_H), getStr(arr_2[ui.select[ui.layer]]));


  u8g2.drawBox(0, TILE_ICON_S, tile.indi_x, TILE_INDI_H);

  if (!ui.init)
  {
    for (uint8_t i = 0; i < ui.num[ui.index]; ++i)  
    {
      if (ui.param[TILE_UFD]) tile.temp = (DISP_W - TILE_ICON_W) / 2 + i * tile.icon_x - TILE_ICON_S * ui.select[ui.layer];
      else tile.temp = (DISP_W - TILE_ICON_W) / 2 + (i - ui.select[ui.layer]) * tile.icon_x;
      u8g2.drawXBMP(tile.temp, (int16_t)tile.icon_y, TILE_ICON_W, TILE_ICON_H, icon_pic[i]); 
    }
    if (tile.icon_x == tile.icon_x_trg) 
    {
      ui.init = true;
      tile.icon_x = tile.icon_x_trg = - ui.select[ui.layer] * TILE_ICON_S;
    }
  }
  else for (uint8_t i = 0; i < ui.num[ui.index]; ++i) u8g2.drawXBMP((DISP_W - TILE_ICON_W) / 2 + (int16_t)tile.icon_x + i * TILE_ICON_S, 0, TILE_ICON_W, TILE_ICON_H, icon_pic[i]);

  u8g2.setDrawColor(2);
  if (!ui.param[DARK_MODE]) u8g2.drawBox(0, 0, DISP_W, DISP_H);
}

void WouoUI_Class::list_draw_value(int n) 
{ 
  // [修改] 针对特殊参数显示文字，而不是数字
  uint8_t* target = &check_box.v[n - 1];

  if (target == &ui.param[UI_LANG]) {
      u8g2.print(ui.param[UI_LANG] ? "CN" : "EN");
  } 
  else if (target == &ui.param[AUTO_SLP]) {
      // 0:Off, 1:3m, 2:5m, 3:10m
      switch(ui.param[AUTO_SLP]) {
          case 0: u8g2.print("OFF"); break;
          case 1: u8g2.print("3m"); break;
          case 2: u8g2.print("5m"); break;
          case 3: u8g2.print("10m"); break;
      }
  }
  else if (target == &ui.param[WIFI_SET]) {
      switch(ui.param[WIFI_SET]) {
          case 0: u8g2.print("OFF"); break;
          case 1: u8g2.print("STA"); break;
          case 2: u8g2.print("AP"); break;
      }
  }
  else {
      // 其他普通参数显示数字
      u8g2.print(check_box.v[n - 1]); 
  }
}

void WouoUI_Class::list_draw_check_box_frame() { u8g2.drawRFrame(CHECK_BOX_L_S, list.temp + CHECK_BOX_U_S, CHECK_BOX_F_W, CHECK_BOX_F_H, 1); }
void WouoUI_Class::list_draw_check_box_dot() { u8g2.drawBox(CHECK_BOX_L_S + CHECK_BOX_D_S + 1, list.temp + CHECK_BOX_U_S + CHECK_BOX_D_S + 1, CHECK_BOX_F_W - (CHECK_BOX_D_S + 1) * 2, CHECK_BOX_F_H - (CHECK_BOX_D_S + 1) * 2); }

void WouoUI_Class::list_draw_krf(int n) 
{ 
  switch (check_box.v[n - 1])
  {
    case 0: u8g2.print("OFF"); break;
    case 1: u8g2.print("VOL"); break;
    case 2: u8g2.print("BRI"); break;
  }
}

void WouoUI_Class::list_draw_kpf(int n) 
{ 
  if (check_box.v[n - 1] == 0) u8g2.print("OFF");
  else if (check_box.v[n - 1] <= 90) u8g2.print((char)check_box.v[n - 1]);
  else u8g2.print("?");
}

void WouoUI_Class::list_draw_text_and_check_box(struct MENU arr[], int i)
{
  // 统一使用 getStr 获取当前语言文本
  u8g2.drawUTF8(LIST_TEXT_S, list.temp + LIST_TEXT_H + LIST_TEXT_S, getStr(arr[i]));
  u8g2.setCursor(CHECK_BOX_L_S, list.temp + LIST_TEXT_H + LIST_TEXT_S);
  
  // 判断逻辑基于英文原名的首字符，所以这里需要取 en 字段判断，或者保持数据文件中 en 字段为控制符开头
  switch (arr[i].en[0])
  {
    case '~': list_draw_value(i); break;
    case '+': list_draw_check_box_frame(); if (check_box.m[i - 1] == 1)  list_draw_check_box_dot(); break;
    case '=': list_draw_check_box_frame(); if (*check_box.s_p == i)      list_draw_check_box_dot(); break;
    case '#': list_draw_krf(i);   break;
    case '$': list_draw_kpf(i);   break;
  }
}

void WouoUI_Class::list_show(struct MENU arr[], uint8_t ui_index)
{
  // 列表统一使用 wqy12 字体以支持中文
  u8g2.setFont(FONT_CN_LIST);
  
  // 计算背景框宽度：基于当前语言的字符串长度
  list.box_x_trg = u8g2.getUTF8Width(getStr(arr[ui.select[ui.layer]])) + LIST_TEXT_S * 2;
  list.bar_y_trg = ceil((ui.select[ui.layer]) * ((float)DISP_H / (ui.num[ui_index] - 1)));
  
  animation(&list.y, &list.y_trg, LIST_ANI);
  animation(&list.box_x, &list.box_x_trg, LIST_ANI);
  animation(&list.box_y, &list.box_y_trg[ui.layer], LIST_ANI);
  animation(&list.bar_y, &list.bar_y_trg, LIST_ANI);

  if (list.loop && list.box_y == list.box_y_trg[ui.layer]) list.loop = false;

  u8g2.setDrawColor(1);
  
  u8g2.drawHLine(DISP_W - LIST_BAR_W, 0, LIST_BAR_W);
  u8g2.drawHLine(DISP_W - LIST_BAR_W, DISP_H - 1, LIST_BAR_W);
  u8g2.drawVLine(DISP_W - ceil((float)LIST_BAR_W / 2), 0, DISP_H);
  u8g2.drawBox(DISP_W - LIST_BAR_W, 0, LIST_BAR_W, list.bar_y);

  if (!ui.init)
  {
    for (int i = 0; i < ui.num[ui_index]; ++ i)
    {
      if (ui.param[LIST_UFD]) list.temp = i * list.y - LIST_LINE_H * ui.select[ui.layer] + list.box_y_trg[ui.layer];
      else list.temp = (i - ui.select[ui.layer]) * list.y + list.box_y_trg[ui.layer];
      list_draw_text_and_check_box(arr, i);
    }
    if (list.y == list.y_trg) 
    {
      ui.init = true;
      list.y = list.y_trg = - LIST_LINE_H * ui.select[ui.layer] + list.box_y_trg[ui.layer];
    }
  }
  else for (int i = 0; i < ui.num[ui_index]; ++ i)
  {
    list.temp = LIST_LINE_H * i + list.y;
    list_draw_text_and_check_box(arr, i);
  }

  u8g2.setDrawColor(2);
  u8g2.drawRBox(0, list.box_y, list.box_x, LIST_LINE_H, LIST_BOX_R);

  if (!ui.param[DARK_MODE])
  {
    u8g2.drawBox(0, 0, DISP_W, DISP_H);
    switch(ui.index)
    {
      case M_WINDOW: 
      case M_SENSOR:
      u8g2.drawBox(0, 0, DISP_W, DISP_H);  
    }
  }
}

void WouoUI_Class::sensor_show()
{
  u8g2.setFont(FONT_CN_LIST);
  // 计算背景框宽度
  list.box_x_trg = u8g2.getUTF8Width(getStr(sensor_menu[ui.select[ui.layer]])) + LIST_TEXT_S * 2;

  animation(&list.y, &list.y_trg, LIST_ANI);
  animation(&list.box_x, &list.box_x_trg, LIST_ANI);
  animation(&list.box_y, &list.box_y_trg[ui.layer], LIST_ANI);
  animation(&sensor.text_bg_l, &sensor.text_bg_l_trg, TAG_ANI);

  if (list.loop && list.box_y == list.box_y_trg[ui.layer]) list.loop = false;

  u8g2.setDrawColor(1);  

  // 1. 绘制底部滚动列表
  u8g2.setFontDirection(1);
  if (!ui.init)
  {
    for (uint8_t i = 0; i < ui.num[ui.index]; ++ i) 
        u8g2.drawUTF8(LIST_TEXT_S + (i - ui.select[ui.layer]) * list.y + list.box_y_trg[ui.layer] - 1, VOLT_LIST_U_S , getStr(sensor_menu[i]));
    if (list.y == list.y_trg) 
    {
      ui.init = true;
      list.y = list.y_trg = - LIST_LINE_H * ui.select[ui.layer] + list.box_y_trg[ui.layer];
    }
  }
  else for (uint8_t i = 0; i < ui.num[ui.index]; ++ i) 
      u8g2.drawUTF8(LIST_TEXT_S + LIST_LINE_H * i + (int16_t)list.y - 1, VOLT_LIST_U_S , getStr(sensor_menu[i])); 
  
  // 2. 绘制上方图表外框
  u8g2.drawFrame(0, 0, WAVE_BOX_W, WAVE_BOX_H);
  
  // 3. 数据读取与自动化处理 (1秒一次)
  // 使用静态变量记录上一次的湿度状态，实现边缘触发，避免与手动控制冲突
  static bool last_is_dry = false; 

  if (millis() - sensor.last_read_time > 1000) {
      sensor.last_read_time = millis();
      
      float t = dht->readTemperature();
      float h = dht->readHumidity();
      if (isnan(t)) t = 0; 
      if (isnan(h)) h = 0;
      
      int s_raw = analogRead(PIN_SOIL);
      int l_raw = analogRead(PIN_LIGHT);
      int s = map(s_raw, 0, 4095, 0, 100); 
      int l = map(l_raw, 0, 4095, 0, 100);

      // --- 继电器自动化控制逻辑 (边缘触发) ---
      bool current_is_dry = (s < AUTO_PUMP_ON_LIMIT);
      
      // 只有当状态从“不干”变成“干”的那一瞬间，才自动开启水泵
      // 这样开启后，用户依然可以手动关闭，不会被下一秒的循环强制打开
      if (current_is_dry && !last_is_dry) {
          digitalWrite(PIN_RELAY, HIGH);
      }
      last_is_dry = current_is_dry;
      // ------------------------------------

      // MQTT 发送
      if (WouoWIFI.getMode() == 1) {
          WouoWIFI.sendSensorData(t, h, s, l);
      }

      // 决定记录哪个数据进历史曲线
      float val_to_plot = 0;
      switch(ui.select[ui.layer]) {
          case 0: val_to_plot = t; break; // Temp
          case 1: val_to_plot = h; break; // Hum
          case 2: val_to_plot = s; break; // Soil
          case 3: val_to_plot = l; break; // Light
          case 4: val_to_plot = digitalRead(PIN_RELAY) ? 100 : 0; break; // Pump
      }
      sensor.val_current = val_to_plot;

      // 写入历史缓冲区
      sensor.history[sensor.head_index] = (int)val_to_plot;
      sensor.head_index = (sensor.head_index + 1) % 128;
  }

  u8g2.setFontDirection(0);

  // 4. 根据当前选择绘制不同的内容 (传感器 vs 继电器)
  if (ui.select[ui.layer] == 4) 
  {
      // ============ 继电器页面 ============
      bool relayState = digitalRead(PIN_RELAY);
      
      // --- 绘制图表区 (ON/OFF 状态块) ---
      u8g2.setFont(u8g2_font_helvB24_tr); // 粗体大字
      int strW;
      // 字体高度约24px，框高49px。
      // 基线Y ≈ 框Y + (框高 - 字高)/2 + 字高 = 0 + (49-24)/2 + 24 ≈ 12 + 24 = 36
      int textY = 38; 

      if (relayState) {
          // ON: 白底黑字，填满
          u8g2.setDrawColor(1); // 白
          u8g2.drawBox(1, 1, WAVE_BOX_W - 2, WAVE_BOX_H - 2);
          u8g2.setDrawColor(0); // 黑字 (因为背景是白的)
          
          strW = u8g2.getStrWidth("ON");
          u8g2.drawStr((WAVE_BOX_W - strW) / 2, textY, "ON");
      } else {
          // OFF: 黑底白字
          u8g2.setDrawColor(1); // 白字
          
          strW = u8g2.getStrWidth("OFF");
          u8g2.drawStr((WAVE_BOX_W - strW) / 2, textY, "OFF");
      }
      u8g2.setDrawColor(1); // 恢复默认绘制颜色

      // --- 绘制数值区 (操作提示) ---
      u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 中文字体
      const char* tip = (ui.param[UI_LANG] == 1) ? "长按开启/关闭" : "Hold to Toggle";
      
      // 横向居中
      int tipW = u8g2.getUTF8Width(tip);
      int tipX = (DISP_W - tipW) / 2;
      // 竖向居中 (数值框区域: Y=53, H=33) -> 中心Y ≈ 53 + 33/2 + 6 ≈ 75
      u8g2.drawUTF8(tipX, 75, tip);
  } 
  else 
  {
      // ============ 传感器页面 (0-3) ============
      
      // --- 绘制波形图 ---
      int map_max = (ui.select[ui.layer] == 0) ? 50 : 100; // 温度0-50，其他0-100
      
      int prev_y = WAVE_BOX_H - map(sensor.history[(sensor.head_index) % 128], 0, map_max, 0, WAVE_BOX_H - 2) - 1;
      for (int i = 1; i < 128; i++)
      { 
        int idx = (sensor.head_index + i) % 128;
        int val = sensor.history[idx];
        if (val < 0) val = 0; if (val > map_max) val = map_max;
        
        int y = WAVE_BOX_H - map(val, 0, map_max, 0, WAVE_BOX_H - 2) - 1;
        u8g2.drawLine(i-1, prev_y, i, y);
        prev_y = y;
      }

      // --- 绘制数值 (固定位置) ---
      u8g2.setFont(u8g2_font_helvB24_tr); 
      u8g2.setCursor(20, VOLT_LIST_U_S - 12); // 数值靠左一点
      u8g2.print(sensor.val_current, 1);
      
      // --- 绘制单位 (固定在数值右侧) ---
      u8g2.setFont(u8g2_font_helvB14_tr); // 单位字体稍小
      u8g2.setCursor(95, VOLT_LIST_U_S - 12); // 固定X坐标
      switch(ui.select[ui.layer]) {
          case 0: u8g2.print("`C"); break;
          case 1: u8g2.print("%"); break;
          case 2: u8g2.print("%"); break;
          case 3: u8g2.print("%"); break;
      }
  }
  
  // 右上角 MQTT 状态点
  if(WouoWIFI.isMqttConnected()) {
      u8g2.drawDisc(124, 4, 2);
  }

  u8g2.setDrawColor(2);
  u8g2.drawRBox(list.box_y, VOLT_LIST_U_S - LIST_TEXT_S, LIST_LINE_H, list.box_x, LIST_BOX_R);
  u8g2.drawBox(DISP_W - sensor.text_bg_l, VOLT_TEXT_BG_U_S, DISP_W, VOLT_TEXT_BG_H);

  if (!ui.param[DARK_MODE]) u8g2.drawBox(0, 0, DISP_W, DISP_H);
}

void WouoUI_Class::window_show()
{
  list_show(win.bg, win.index);
  if (ui.param[WIN_BOK]) for (uint16_t i = 0; i < buf_len; ++i)  buf_ptr[i] = buf_ptr[i] & (i % 2 == 0 ? 0x55 : 0xAA);

  u8g2.setFont(FONT_CN_LIST);

  bool isLangWin = (win.value == &ui.param[UI_LANG]);
  bool isSleepWin = (win.value == &ui.param[AUTO_SLP]);
  bool isWifiWin = (win.value == &ui.param[WIFI_SET]);
  bool isSpecialWin = isLangWin || isSleepWin || isWifiWin;

  if (isSpecialWin) {
      win.bar_trg = 0; 
  } else {
      win.bar_trg = (float)(*win.value - win.min) / (float)(win.max - win.min) * (WIN_BAR_W - 4);
  }

  animation(&win.bar, &win.bar_trg, WIN_ANI);
  animation(&win.y, &win.y_trg, WIN_ANI);
  
  // [新增] 选项切换动画
  animation(&win.option_offset, &win.option_offset_trg, WIN_ANI);
  // 动画复位逻辑：如果非常接近目标0，直接归零
  if (fabs(win.option_offset - 0) < 0.5) win.option_offset_trg = 0;

  u8g2.setDrawColor(0); u8g2.drawRBox(win.l, (int16_t)win.y, WIN_W, WIN_H, 2);    
  u8g2.setDrawColor(1); u8g2.drawRFrame(win.l, (int16_t)win.y, WIN_W, WIN_H, 2);  

  if (!isSpecialWin) {
      // === 普通数值弹窗 ===
      int16_t content_y_offset = 10;
      u8g2.drawRFrame(win.l + 5, (int16_t)win.y + 20 + content_y_offset, WIN_BAR_W, WIN_BAR_H, 1);       
      u8g2.drawBox(win.l + 7, (int16_t)win.y + 22 + content_y_offset, win.bar, WIN_BAR_H - 4);           
      
      u8g2.setCursor(win.l + 5, (int16_t)win.y + 14 + content_y_offset); 
      // 简单处理标题显示，如果需要完整支持请保留你之前的 if-else 翻译逻辑
      if (strcmp(win.title, "Disp Bri") == 0 && ui.param[UI_LANG] == 1) u8g2.print("屏幕亮度");
      else u8g2.print(win.title);          

      u8g2.setCursor(win.l + 78, (int16_t)win.y + 14 + content_y_offset); u8g2.print(*win.value);
  } 
  else {
      // === 特殊文本选择弹窗 (带背景动画) ===
      
      // 1. 标题
      u8g2.setCursor(win.l + 5, (int16_t)win.y + 14); 
      if (isLangWin) u8g2.print(ui.param[UI_LANG] ? "语言设置" : "Language");
      else if (isSleepWin) u8g2.print(ui.param[UI_LANG] ? "息屏时间" : "Auto Sleep");
      else if (isWifiWin) u8g2.print(ui.param[UI_LANG] ? "WiFi 设置" : "WiFi Setup");

      // 分割线
      u8g2.drawHLine(win.l + 5, (int16_t)win.y + 18, WIN_W - 10);

      // 2. 选项内容
      const char* centerStr = "";
      
      if (isLangWin) {
          centerStr = (*win.value == 0) ? "English" : "中文";
      } 
      else if (isSleepWin) {
          if (ui.param[UI_LANG]) {
               switch(*win.value) {
                   case 0: centerStr = "从不"; break;
                   case 1: centerStr = "3 分钟"; break;
                   case 2: centerStr = "5 分钟"; break;
                   case 3: centerStr = "10 分钟"; break;
               }
          } else {
               switch(*win.value) {
                   case 0: centerStr = "Never"; break;
                   case 1: centerStr = "3 Min"; break;
                   case 2: centerStr = "5 Min"; break;
                   case 3: centerStr = "10 Min"; break;
               }
          }
      }
      else if (isWifiWin) {
           switch(*win.value) {
               case 0: centerStr = "OFF"; break;
               case 1: centerStr = "STA"; break;
               case 2: centerStr = "AP"; break;
           }
      }

      // [新增] 绘制中间的圆角矩形背景 (带动画偏移 win.option_offset)
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      int strW = u8g2.getUTF8Width(centerStr);
      int centerX = win.l + WIN_W / 2;
      int bgW = strW + 12; // 背景比文字宽一点
      int bgH = 14;
      int bgX = centerX - bgW / 2 + (int)win.option_offset; // 应用动画偏移
      int bgY = (int16_t)win.y + 24;

      // 绘制反色背景框
      u8g2.setDrawColor(1);
      u8g2.drawRBox(bgX, bgY, bgW, bgH, 2); 

      // 绘制文字 (反色显示)
      u8g2.setDrawColor(0); // 背景是黑的(1)，文字画白的(0)，OLED上就是黑底白字的效果反过来
      // 注意：u8g2 drawBox 是画实心点(亮)，setDrawColor(0)是清除点(灭)。
      // 实际上要在亮色背景上显示文字，需要用 XOR 模式(2) 或者先画框再用0画字
      // 这里使用 XOR 模式比较简单，或者直接用 color 0 画字在 color 1 的框上
      u8g2.drawUTF8(bgX + 6, bgY + 11, centerStr);
      
      // 恢复正常绘制颜色
      u8g2.setDrawColor(1);

      // 绘制左右箭头 (固定位置，不随动画移动)
      u8g2.drawStr(win.l + 10, bgY + 11, "<");
      u8g2.drawStr(win.l + WIN_W - 15, bgY + 11, ">");
      
      // 3. 底部信息
      u8g2.drawHLine(win.l + 5, (int16_t)win.y + 40, WIN_W - 10);
      
      if (isWifiWin) {
          if (WouoWIFI.getMode() == *win.value) {
             // [修改] 加大 WiFi 信息字体
             u8g2.setFont(u8g2_font_profont10_mr); // 使用稍大一点的像素字体
             // 或者 u8g2_font_5x7_tr
             
             u8g2.setCursor(win.l + 5, (int16_t)win.y + 49);
             u8g2.print("W: "); u8g2.print(WouoWIFI.getSSID());
             u8g2.setCursor(win.l + 5, (int16_t)win.y + 59);
             u8g2.print("I: "); u8g2.print(WouoWIFI.getIP());
          } else {
             u8g2.setFont(u8g2_font_wqy12_t_gb2312);
             u8g2.setCursor(win.l + 5, (int16_t)win.y + 54);
             u8g2.print(ui.param[UI_LANG] ? "长按确认" : "Hold to Apply");
          }
      }
  }
  
  if (!strcmp(win.title, "Disp Bri")) u8g2.setContrast(ui.param[DISP_BRI]);

  u8g2.setDrawColor(2);
  if (!ui.param[DARK_MODE]) u8g2.drawBox(0, 0, DISP_W, DISP_H);
}

void WouoUI_Class::tile_rotate_switch()
{
  switch (btn.id)
  { 
    case BTN_ID_CC:
      if (ui.init)
      {
        if (ui.select[ui.layer] > 0)
        {
          ui.select[ui.layer] -= 1;
          tile.icon_x_trg += TILE_ICON_S;
          tile.select_flag = false;
        }
        else 
        {
          if (ui.param[TILE_LOOP])
          {
            ui.select[ui.layer] = ui.num[ui.index] - 1;
            tile.icon_x_trg = - TILE_ICON_S * (ui.num[ui.index] - 1);
            break;
          }
          else tile.select_flag = true;
        }
      }
      break;

    case BTN_ID_CW:
      if (ui.init)
      {
        if (ui.select[ui.layer] < (ui.num[ui.index] - 1)) 
        {
          ui.select[ui.layer] += 1;
          tile.icon_x_trg -= TILE_ICON_S;
          tile.select_flag = false;
        }
        else 
        {
          if (ui.param[TILE_LOOP])
          {
            ui.select[ui.layer] = 0;
            tile.icon_x_trg = 0;
            break;
          }
          else tile.select_flag = true;
        }
      }
      break;
  }
}

void WouoUI_Class::list_rotate_switch()
{
  if (!list.loop)
  {
    switch (btn.id)
    {
      case BTN_ID_CC:
        if (ui.select[ui.layer] == 0)
        {
          if (ui.param[LIST_LOOP] && ui.init)
          {
            list.loop = true;
            ui.select[ui.layer] = ui.num[ui.index] - 1;
            if (ui.num[ui.index] > list.line_n) 
            {
              list.box_y_trg[ui.layer] = DISP_H - LIST_LINE_H;
              list.y_trg = DISP_H - ui.num[ui.index] * LIST_LINE_H;
            }
            else list.box_y_trg[ui.layer] = (ui.num[ui.index] - 1) * LIST_LINE_H;
            break;
          }
          else break;
        }
        if (ui.init)
        {
          ui.select[ui.layer] -= 1;
          if (ui.select[ui.layer] < - (list.y_trg / LIST_LINE_H)) 
          {
            if (!(DISP_H % LIST_LINE_H)) list.y_trg += LIST_LINE_H;
            else
            {
              if (list.box_y_trg[ui.layer] == DISP_H - LIST_LINE_H * list.line_n)
              {
                list.y_trg += (list.line_n + 1) * LIST_LINE_H - DISP_H;
                list.box_y_trg[ui.layer] = 0;
              }
              else if (list.box_y_trg[ui.layer] == LIST_LINE_H)
              {
                list.box_y_trg[ui.layer] = 0;
              }
              else list.y_trg += LIST_LINE_H;
            }
          }
          else list.box_y_trg[ui.layer] -= LIST_LINE_H;
          break;
        }

      case BTN_ID_CW:
        if (ui.select[ui.layer] == (ui.num[ui.index] - 1))
        {
          if (ui.param[LIST_LOOP] && ui.init)
          {
            list.loop = true;
            ui.select[ui.layer] = 0;
            list.y_trg = 0;
            list.box_y_trg[ui.layer] = 0;
            break;
          }
          else break;
        }
        if (ui.init)
        {
          ui.select[ui.layer] += 1;
          if ((ui.select[ui.layer] + 1) > (list.line_n - list.y_trg / LIST_LINE_H))
          {
            if (!(DISP_H % LIST_LINE_H)) list.y_trg -= LIST_LINE_H;
            else
            {
              if (list.box_y_trg[ui.layer] == LIST_LINE_H * (list.line_n - 1))
              {
                list.y_trg -= (list.line_n + 1) * LIST_LINE_H - DISP_H;
                list.box_y_trg[ui.layer] = DISP_H - LIST_LINE_H;
              }
              else if (list.box_y_trg[ui.layer] == DISP_H - LIST_LINE_H * 2)
              {
                list.box_y_trg[ui.layer] = DISP_H - LIST_LINE_H;
              }
              else list.y_trg -= LIST_LINE_H;
            }
          }
          else list.box_y_trg[ui.layer] += LIST_LINE_H;
          break;
        }
        break;
    }
  }
}

void WouoUI_Class::window_proc()
{
  window_show();
  if (win.y == WIN_Y_TRG) ui.index = win.index;
  if (btn.pressed && win.y == win.y_trg && win.y != WIN_Y_TRG)
  {
    btn.pressed = false;
    sleep_timer = millis();

    bool isWifiWin = (win.value == &ui.param[WIFI_SET]);
    bool isSpecial = (win.value == &ui.param[AUTO_SLP]) || (win.value == &ui.param[UI_LANG]) || isWifiWin;

    if (isSpecial) {
        switch (btn.id)
        {
            case BTN_ID_CW: 
               // 向右旋转，内容向左飞入效果 (偏移量设为正数，然后回归0)
               win.option_offset = 15; 
               win.option_offset_trg = 0;
               
               if (*win.value < win.max) *win.value += win.step; 
               else *win.value = win.min; 
               if (!isWifiWin) eeprom.change = true; 
               break;

            case BTN_ID_CC: 
               // 向左旋转，内容向右飞入效果
               win.option_offset = -15; 
               win.option_offset_trg = 0;

               if (*win.value > win.min) *win.value -= win.step; 
               else *win.value = win.max; 
               if (!isWifiWin) eeprom.change = true; 
               break;

            case BTN_ID_LP: 
               if (isWifiWin) {
                   WouoWIFI.setMode(*win.value);
                   eeprom.change = true;
               } else {
                   win.y_trg = WIN_Y_TRG; 
               }
               break;
               
            case BTN_ID_SP: 
               win.y_trg = WIN_Y_TRG; 
               break;
        }
    } 
    else {
        // 普通数值窗口
        switch (btn.id)
        {
          case BTN_ID_CW: if (*win.value < win.max)  *win.value += win.step;  eeprom.change = true;  break;
          case BTN_ID_CC: if (*win.value > win.min)  *win.value -= win.step;  eeprom.change = true;  break;  
          case BTN_ID_SP: case BTN_ID_LP: win.y_trg = WIN_Y_TRG; break;
        }
    }
  }
}

void WouoUI_Class::sleep_proc()
{
  while (ui.sleep)
  {
    btn_scan();
    if (btn.pressed) { 
        btn.pressed = false; 
        switch (btn.id) {    
            case BTN_ID_CW: break; // 旋钮不唤醒
            case BTN_ID_CC: break; // 旋钮不唤醒
            
            // [修改 2] 增加短按唤醒逻辑
            case BTN_ID_SP: 
            case BTN_ID_LP: 
                ui.index = M_MAIN;  
                ui.state = S_LAYER_IN; 
                u8g2.setPowerSave(0); 
                ui.sleep = false; 
                sleep_timer = millis(); // 唤醒时重置计时
                break;
        }
    }
    // 睡眠时保持 WiFi 后台运行
    WouoWIFI.loop(); 
  }
}

void WouoUI_Class::main_proc()
{
  tile_show(main_menu, main_menu_exp, main_icon_pic);
  if (btn.pressed) { 
    btn.pressed = false; 
    switch (btn.id) { 
      case BTN_ID_CW: 
      case BTN_ID_CC: 
        tile_rotate_switch(); 
        break; 
      case BTN_ID_SP: 
        switch (ui.select[ui.layer]) {
          case 0: ui.index = M_SLEEP;   ui.state = S_LAYER_OUT; break;
          // [修改] 索引 1 改为 M_SENSOR
          case 1: ui.index = M_SENSOR;  ui.state = S_LAYER_IN;  break;
          case 2: ui.index = M_KNOB;    ui.state = S_LAYER_IN;  break;
          case 3: ui.index = M_SETTING; ui.state = S_LAYER_IN;  break;
        }
    }
    if (!tile.select_flag && ui.init) { tile.indi_x = 0; tile.title_y = tile.title_y_calc; }
  }
}

void WouoUI_Class::knob_proc()
{
  list_show(knob_menu, M_KNOB);
  if (btn.pressed) { 
    btn.pressed = false; 
    switch (btn.id) { 
      case BTN_ID_CW: 
      case BTN_ID_CC: 
        list_rotate_switch(); 
        break; 
      case BTN_ID_LP: 
        ui.select[ui.layer] = 0; 
        // fallthrough
      case BTN_ID_SP: 
        switch (ui.select[ui.layer]) {
          case 0: ui.index = M_MAIN;  ui.state = S_LAYER_OUT; break; // [修改] 原 M_EDITOR 改为 M_MAIN
          case 1: ui.index = M_KRF;     ui.state = S_LAYER_IN;  check_box_s_init(&knob.param[KNOB_ROT], &knob.param[KNOB_ROT_P]); break;
          case 2: ui.index = M_KPF;     ui.state = S_LAYER_IN;  check_box_s_init(&knob.param[KNOB_COD], &knob.param[KNOB_COD_P]); break;
        }
    }
  }
}

void WouoUI_Class::krf_proc()
{
  list_show(krf_menu, M_KRF);
  if (btn.pressed) { btn.pressed = false; switch (btn.id) { case BTN_ID_CW: case BTN_ID_CC: list_rotate_switch(); break; case BTN_ID_LP: ui.select[ui.layer] = 0; case BTN_ID_SP: switch (ui.select[ui.layer]) {
        case 0: ui.index = M_KNOB;  ui.state = S_LAYER_OUT; break;
        case 1: break;
        case 2: check_box_s_select(KNOB_DISABLE, ui.select[ui.layer]); break;
        case 3: break;
        case 4: check_box_s_select(KNOB_ROT_VOL, ui.select[ui.layer]); break;
        case 5: check_box_s_select(KNOB_ROT_BRI, ui.select[ui.layer]); break;
        case 6: break;
      }
    }
  }
}

void WouoUI_Class::kpf_proc()
{
  list_show(kpf_menu, M_KPF);
  if (btn.pressed) { btn.pressed = false; switch (btn.id) { case BTN_ID_CW: case BTN_ID_CC: list_rotate_switch(); break;  case BTN_ID_LP: ui.select[ui.layer] = 0; case BTN_ID_SP: switch (ui.select[ui.layer]) {
        case 0:   ui.index = M_KNOB;  ui.state = S_LAYER_OUT; break;
        default:  check_box_s_select('A' + ui.select[ui.layer], ui.select[ui.layer]); break; 
      }
    }
  }
}

void WouoUI_Class::sensor_proc()
{
  sensor_show(); 
  if (btn.pressed) { 
    btn.pressed = false; 
    switch (btn.id) { 
      case BTN_ID_CW: 
      case BTN_ID_CC: 
        list_rotate_switch(); 
        // 切换传感器时清空波形
        for(int i=0; i<128; i++) sensor.history[i] = 0;
        break;
      
      case BTN_ID_LP: // 长按
          // 如果当前选择的是第5项 (Index 4) -> 水泵
          if (ui.select[ui.layer] == 4) {
              // 切换继电器状态
              digitalWrite(PIN_RELAY, !digitalRead(PIN_RELAY));
              // 强制刷新一次状态到历史记录，避免UI延迟
              sensor.val_current = digitalRead(PIN_RELAY) ? 100 : 0;
              // 增加蜂鸣器或串口反馈（可选）
              // Serial.println("Relay Toggled");
              break; // 这里的 break 阻止了 switch 继续向下执行 fallthrough
          }
          // 如果不是水泵页面，长按则执行退出操作 (fallthrough)
          // 注意：这里没有 break，会自然滑落到 BTN_ID_SP，从而执行退出。
          // 这种写法在C语言中是允许的，但容易让人误解。
          // 为了稳妥，这里显式写一下：
          ui.index = M_MAIN;  
          ui.state = S_LAYER_OUT; 
          break;

      case BTN_ID_SP: // 短按
        ui.index = M_MAIN;  
        ui.state = S_LAYER_OUT; 
        break;
    }
  }
}


void WouoUI_Class::setting_proc()
{
  list_show(setting_menu, M_SETTING);
  if (btn.pressed) { btn.pressed = false; switch (btn.id) { case BTN_ID_CW: case BTN_ID_CC: list_rotate_switch(); break; case BTN_ID_LP: ui.select[ui.layer] = 0; case BTN_ID_SP: switch (ui.select[ui.layer]) {
        case 0:   ui.index = M_MAIN;  ui.state = S_LAYER_OUT; break;
        
        case 1:   window_value_init("Language", UI_LANG,  &ui.param[UI_LANG],   1,    0,  1, setting_menu, M_SETTING);  break;
        case 2:   window_value_init("Auto Sleep", AUTO_SLP, &ui.param[AUTO_SLP], 3,   0,  1, setting_menu, M_SETTING);  break; // 0-3
        case 3:   window_value_init("WiFi Setup", WIFI_SET, &ui.param[WIFI_SET], 2,   0,  1, setting_menu, M_SETTING);  break; // 0-2
        
        case 4:   window_value_init("Disp Bri", DISP_BRI, &ui.param[DISP_BRI],  255,  0,  5, setting_menu, M_SETTING);  break;
        case 5:   window_value_init("Tile Ani", TILE_ANI, &ui.param[TILE_ANI],  100, 10,  1, setting_menu, M_SETTING);  break;
        case 6:   window_value_init("List Ani", LIST_ANI, &ui.param[LIST_ANI],  100, 10,  1, setting_menu, M_SETTING);  break;
        case 7:   window_value_init("Win Ani",  WIN_ANI,  &ui.param[WIN_ANI],   100, 10,  1, setting_menu, M_SETTING);  break;
        case 8:   window_value_init("Spot Ani", SPOT_ANI, &ui.param[SPOT_ANI],  100, 10,  1, setting_menu, M_SETTING);  break;
        case 9:   window_value_init("Tag Ani",  TAG_ANI,  &ui.param[TAG_ANI],   100, 10,  1, setting_menu, M_SETTING);  break;
        case 10:  window_value_init("Fade Ani", FADE_ANI, &ui.param[FADE_ANI],  255,  0,  1, setting_menu, M_SETTING);  break;
        case 11:  window_value_init("Btn SPT",  BTN_SPT,  &ui.param[BTN_SPT],   255,  0,  1, setting_menu, M_SETTING);  break;
        case 12:  window_value_init("Btn LPT",  BTN_LPT,  &ui.param[BTN_LPT],   255,  0,  1, setting_menu, M_SETTING);  break;
        
        case 13:  check_box_m_select( TILE_UFD  );  break;
        case 14:  check_box_m_select( LIST_UFD  );  break;
        case 15:  check_box_m_select( TILE_LOOP );  break;
        case 16:  check_box_m_select( LIST_LOOP );  break;
        case 17:  check_box_m_select( WIN_BOK   );  break;
        case 18:  check_box_m_select( KNOB_DIR  );  break;
        case 19:  check_box_m_select( DARK_MODE );  break;
        case 20:  ui.index = M_ABOUT; ui.state = S_LAYER_IN; break;
      }
    }
  }
}

void WouoUI_Class::about_proc()
{
  about_show();
  if (btn.pressed) { btn.pressed = false; switch (btn.id) {
      case BTN_ID_SP: case BTN_ID_LP: ui.index = M_SETTING;  ui.state = S_LAYER_OUT;  break;
    }
  }
}

void WouoUI_Class::ui_proc()
{
  if (!ui.sleep && ui.param[AUTO_SLP] > 0) {
      if (millis() - sleep_timer > getSleepTimeMS(ui.param[AUTO_SLP])) {
          ui.index = M_SLEEP;
          ui.state = S_LAYER_OUT; 
      }
  }

  u8g2.sendBuffer();
  switch (ui.state)
  {
    case S_FADE:          fade();                   break;  
    case S_WINDOW:        window_param_init();      break;  
    case S_LAYER_IN:      layer_init_in();          break;  
    case S_LAYER_OUT:     layer_init_out();         break;  
  
    case S_NONE: u8g2.clearBuffer(); switch (ui.index)      
    {
      case M_WINDOW:      window_proc();            break;
      case M_SLEEP:       sleep_proc();             break;
      case M_MAIN:        main_proc();              break;
      // case M_EDITOR:   editor_proc();            break; // [已删除]
      case M_KNOB:        knob_proc();              break;
      case M_KRF:         krf_proc();               break;
      case M_KPF:         kpf_proc();               break;
      case M_SENSOR:      sensor_proc();            break; // [修改] 对应 M_SENSOR
      case M_SETTING:     setting_proc();           break;
      case M_ABOUT:       about_proc();             break;
    }
  }  
}

void WouoUI_Class::about_show()
{
  // 必须使用支持中文的字体
  u8g2.setFont(FONT_CN_LIST);
  
  // 使用 getStr 获取多语言文本计算宽度
  list.box_x_trg = u8g2.getUTF8Width(getStr(about_menu[0])) + LIST_TEXT_S * 2;

  animation(&list.box_x, &list.box_x_trg, TAG_ANI);
  animation(&about.indi_x, &about.indi_x_trg, TAG_ANI);

  u8g2.setDrawColor(1);

  // 绘制标题和版本号
  u8g2.drawUTF8(ABOUT_INDI_S + LIST_TEXT_S, ABOUT_INDI_S + LIST_TEXT_S + LIST_TEXT_H, getStr(about_menu[0]));
  u8g2.drawUTF8(ABOUT_INDI_S + list.box_x_trg + ABOUT_INDI_S, ABOUT_INDI_S + LIST_TEXT_S + LIST_TEXT_H, getStr(about_menu[1]));
  
  for (int i = 2 ; i < ui.num[M_ABOUT] ; i++) 
  {
      u8g2.drawUTF8(about.indi_x_trg + ABOUT_INDI_W + ABOUT_INDI_S * 2, ABOUT_INDI_S + LIST_LINE_H + LIST_TEXT_S / 2 + (i - 1) * LIST_LINE_H, getStr(about_menu[i]));
  }
  
  // 绘制左侧指示条
  u8g2.drawBox(about.indi_x, ABOUT_INDI_S + LIST_LINE_H + ABOUT_INDI_S * 2, ABOUT_INDI_W, (ui.num[M_ABOUT] - 2) * LIST_LINE_H - LIST_TEXT_S);

  // 绘制反色背景块
  u8g2.setDrawColor(2);
  u8g2.drawRBox(ABOUT_INDI_S, ABOUT_INDI_S, list.box_x, LIST_LINE_H, LIST_BOX_R);

  if (!ui.param[DARK_MODE]) u8g2.drawBox(0, 0, DISP_W, DISP_H);
}

void WouoUI_Class::oled_init()
{
  SPI.begin(OLED_SCL, -1, OLED_SDA, OLED_CS);
  u8g2.setBusClock(10000000); 
  u8g2.begin();
  u8g2.enableUTF8Print(); // 启用 UTF8 以支持中文打印
  u8g2.setContrast(ui.param[DISP_BRI]);
  buf_ptr = u8g2.getBufferPtr();
  buf_len = 8 * u8g2.getBufferTileHeight() * u8g2.getBufferTileWidth();
}

void WouoUI_Class::begin() 
{
  analogReadResolution(12); 
  eeprom_init();
  ui_init();
  oled_init();
  btn_init();
}

void WouoUI_Class::loop() 
{
  // 旋钮扫描 (会更新 sleep_timer)
  btn_scan();
  // WiFi 任务处理
  WouoWIFI.loop(); 
  // UI 刷新
  ui_proc();
}