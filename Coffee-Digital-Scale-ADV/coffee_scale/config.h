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

// ========== 冲煮参数 ==========
#define DEFAULT_RATIO       15.0f   // 默认水分比 1:15
#define DEFAULT_DOSE        15.0f   // 默认粉量 15g

// ========== 历史数据 ==========
#define HISTORY_BUFFER_SIZE         240     // 曲线数据点数（屏幕宽度）

// ========== UI 配色 (RGB565) ==========
// 方案 B · 暖咖氛围（深褐 + 暖金 + 琥珀）
// RGB565 转换公式: R5=R*31/255, G6=G*63/255, B5=B*31/255, val=(R5<<11)|(G6<<5)|B5
#define COLOR_BG            0x20A2  // 深褐背景 #24140D
#define COLOR_BG_DARK       0x1861  // 更深褐 #1A0D08
#define COLOR_TEXT          0xF739  // 暖白文字 #F5E6D0
#define COLOR_TEXT_DIM      0x8BCC  // 棕灰文字 #8A7A60
#define COLOR_ACCENT        0xD52E  // 暖金色 #D4A574
#define COLOR_DIVIDER       0x3943  // 深褐分隔 #3D2A1A
#define COLOR_GRID          0x20A2  // 网格线颜色（同 BG）
#define COLOR_CURVE_WEIGHT  0xD52E  // 重量曲线暖金
#define COLOR_CURVE_FLOW    0xFC68  // 流量曲线橙琥珀 #FF8C42
#define COLOR_STATUS_ON     0xC4AD  // 运行状态暖色 #C4956A
#define COLOR_STATUS_OFF    0x3943  // 停止状态暗褐 #3A2A1A
#define COLOR_SUCCESS       0x25F0  // 目标达成青绿色 #1FC080
#define COLOR_TARGET        0x25F0  // 目标水量进度颜色（同 SUCCESS）

// ========== 演示模式 ==========
#define DEMO_MODE           1       // 1=无传感器模拟演示, 0=正式硬件模式
// 注意：正式使用 HX711 时请将 DEMO_MODE 改为 0

// ========== UI 布局 ==========
#define SCREEN_WIDTH        240
#define SCREEN_HEIGHT       135
#define TITLE_HEIGHT        18
#define WEIGHT_FONT_SIZE    4
#define NORMAL_FONT_SIZE    2
#define SMALL_FONT_SIZE     1

// ========== 更新频率 ==========
#define SENSOR_UPDATE_INTERVAL  100   // 传感器更新间隔 (ms) = 10Hz
#define DISPLAY_UPDATE_INTERVAL 100   // 屏幕更新间隔 (ms) = 10Hz
#define CURVE_UPDATE_INTERVAL   500   // 曲线更新间隔 (ms) = 2Hz
#define SESSION_CHECK_INTERVAL  5000  // 会话检查间隔 (ms)

// ========== SD 卡 ==========
#define SD_CS_PIN           12    // SD 卡 CS 引脚（G12）
#define SD_MOSI_PIN         14    // SD 卡 MOSI 引脚（G14）
#define SD_MISO_PIN         39    // SD 卡 MISO 引脚（G39）
#define SD_CLK_PIN          40    // SD 卡 CLK 引脚（G40）
#define DATA_LOG_DIR        "/coffee_data"
#define CONFIG_DIR          "/config"
#define CALIBRATION_FILE    "/config/calibration.json"
#define SETTINGS_FILE       "/config/settings.json"
#define SD_FLUSH_INTERVAL   10    // 每 N 条数据 flush 一次

// ========== 错误处理 ==========
#define MAX_SENSOR_ERRORS   10    // 最大传感器错误次数
#define SENSOR_TIMEOUT_MS   2000  // 传感器初始化超时 (ms)

// ========== 内存优化 ==========
#define MINIMAL_SERIAL      1     // 减少串口输出

#endif
