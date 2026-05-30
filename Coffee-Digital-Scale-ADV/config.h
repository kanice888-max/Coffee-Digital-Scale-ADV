#ifndef CONFIG_H
#define CONFIG_H

#include <pgmspace.h>

// ========== 硬件引脚 ==========
#define HX711_DOUT_PIN    2     // G2
#define HX711_SCK_PIN     1     // G1

// ========== 称重参数 ==========
#define DEFAULT_CALIBRATION_FACTOR  420.0f  // 默认校准因子，需实际校准
#define FILTER_WINDOW_SIZE          5       // 移动平均窗口大小
#define WEIGHT_PRECISION            0.1f    // 重量显示精度 (g)

// ========== 流量参数 ==========
#define FLOW_WINDOW_MS              1000    // 流量计算窗口 (ms)
#define FLOW_PRECISION              0.1f    // 流量显示精度 (g/s)

// ========== 计时参数 ==========
#define AUTO_START_THRESHOLD        0.5f    // 自动计时触发阈值 (g)
#define RESET_THRESHOLD             0.3f    // 归零重置阈值 (g)
#define RESET_DELAY_MS              2000    // 重量归零后延迟重置 (ms)

// ========== 历史数据 ==========
#define HISTORY_BUFFER_SIZE         240     // 曲线数据点数（屏幕宽度）

// ========== UI 配色 (RGB565) ==========
#define COLOR_BG            0x1A1A  // 深灰背景
#define COLOR_TEXT          0xF5DC  // 暖白文字
#define COLOR_TEXT_DIM      0x8410  // 暗灰文字
#define COLOR_ACCENT        0xFF8C  // 琥珀色曲线
#define COLOR_SUCCESS       0x0F88  // 绿色强调
#define COLOR_WARNING       0xFBE0  // 黄色警告

// ========== UI 布局 ==========
#define SCREEN_WIDTH        240
#define SCREEN_HEIGHT       135
#define TITLE_HEIGHT        20
#define WEIGHT_FONT_SIZE    4
#define NORMAL_FONT_SIZE    2

// ========== 更新频率 ==========
#define SENSOR_UPDATE_INTERVAL  100   // 传感器更新间隔 (ms) = 10Hz
#define DISPLAY_UPDATE_INTERVAL 100   // 屏幕更新间隔 (ms) = 10Hz
#define CURVE_UPDATE_INTERVAL   500   // 曲线更新间隔 (ms) = 2Hz
#define SESSION_CHECK_INTERVAL  5000  // 会话检查间隔 (ms)

// ========== SD 卡 ==========
#define SD_CS_PIN           4     // SD 卡片选引针（Cardputer 默认）
#define DATA_LOG_DIR        "/coffee_data"
#define CONFIG_DIR          "/config"
#define CALIBRATION_FILE    "/config/calibration.json"
#define SETTINGS_FILE       "/config/settings.json"
#define SD_FLUSH_INTERVAL   10    // 每 N 条数据 flush 一次

// ========== 错误处理 ==========
#define MAX_SENSOR_ERRORS   10    // 最大传感器错误次数
#define SENSOR_TIMEOUT_MS   2000  // 传感器初始化超时 (ms)

// ========== 内存优化 ==========
#define USE_PROGMEM         1     // 使用 PROGMEM 存储常量字符串
#define MINIMAL_SERIAL      1     // 减少串口输出

// ========== PROGMEM 字符串常量 ==========
#if USE_PROGMEM
static const char PROGMEM STR_INIT[] = "Initializing...";
static const char PROGMEM STR_TARING[] = "Taring...";
static const char PROGMEM STR_READY[] = "Ready!";
static const char PROGMEM STR_SENSOR_ERROR[] = "Sensor Error!";
static const char PROGMEM STR_SD_ERROR[] = "SD Error";
static const char PROGMEM STR_CALIBRATING[] = "Calibrating...";
static const char PROGMEM STR_POUR_OVER[] = "Pour Over Scale";
static const char PROGMEM STR_WEIGHT[] = "Weight";
static const char PROGMEM STR_FLOW[] = "Flow";
static const char PROGMEM STR_TIME[] = "Time";
static const char PROGMEM STR_FLOW_UNIT[] = "g/s";
static const char PROGMEM STR_WEIGHT_UNIT[] = "g";
#endif

#endif
