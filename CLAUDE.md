# Cardputer-ADV 手冲咖啡秤固件

## 项目概述

基于 M5Stack Cardputer-adv（ESP32-S3）的手冲咖啡秤固件，连接 HX711 称重模块，实现实时重量显示、流量曲线和自动计时。

## 硬件平台

- **MCU**: ESP32-S3（M5Stack Cardputer-adv）
- **屏幕**: 1.14寸 IPS TFT，135×240 像素，ST7789V2 驱动
- **HX711 接线**: DT/DOUT → G2, SCK/CLK → G1, VCC → +5V, GND → GND
- **存储**: MicroSD 卡

## 技术栈

- **框架**: Arduino + M5Unified
- **称重库**: HX711_ADC（非 HX711 库）
- **屏幕驱动**: TFT_eSPI（M5Stack 内置）

## 模块架构

| 模块 | 职责 | 关键接口 |
|------|------|----------|
| WeightSensor | HX711 驱动 + 移动平均滤波 | `init()`, `tare()`, `getWeight()` |
| FlowCalculator | 流量计算 + 环形缓冲区 | `update()`, `getFlowRate()`, `getFlowHistory()` |
| TimerModule | 自动/手动计时 | `start()`, `reset()`, `getElapsed()`, `checkAutoStart()` |
| DisplayModule | 多页面 UI | `showMainPage()`, `showCurvePage()`, `handleInput()` |
| StorageModule | SD 卡数据记录 | `beginNewSession()`, `logDataPoint()`, `saveCalibration()` |

**深度模块原则**：每个模块封装复杂实现，对外暴露简单接口。禁止跨模块直接访问内部状态。

## 关键阈值

```cpp
#define AUTO_START_THRESHOLD  0.5f   // g，自动计时触发
#define RESET_THRESHOLD       0.3f   // g，归零重置
#define FILTER_WINDOW_SIZE    5      // 移动平均窗口
#define FLOW_WINDOW_MS        1000   // 流量计算窗口
#define HISTORY_BUFFER_SIZE   240    // 曲线数据点数
```

## UI 配色

```cpp
#define COLOR_BG        0x1A1A  // 深灰背景
#define COLOR_TEXT      0xF5DC  // 暖白文字
#define COLOR_ACCENT    0xFF8C  // 琥珀色曲线
#define COLOR_SUCCESS   0x0F88  // 绿色强调
```

## Issue 依赖关系

```
#1 → #2 → #3 ─┐
           ├── #4 ──→ #6 → #7 → #8
           ├── #5 ──┘   ├── #9
                        └── #10 → #11
```

## 文件结构

```
/
├── CLAUDE.md           ← 本文件（项目上下文）
├── AGENTS.md           ← AFK 代理工作规范
├── PRD.md              ← 产品需求文档
├── coffee_scale.ino    ← 主程序入口
├── WeightSensor.h/.cpp
├── FlowCalculator.h/.cpp
├── TimerModule.h/.cpp
├── DisplayModule.h/.cpp
└── StorageModule.h/.cpp
```

## 依赖库

- M5Unified
- HX711_ADC
- TFT_eSPI
- SD
- ArduinoJson（可选，用于配置文件）
