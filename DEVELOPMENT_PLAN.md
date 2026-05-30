# Cardputer-ADV 手冲咖啡秤 - 详细开发方案

## 1. 开发环境搭建

### 1.1 Arduino IDE 配置

**安装 Arduino IDE 2.x**
- 下载：https://www.arduino.cc/en/software

**添加 ESP32-S3 开发板支持**
```
文件 → 首选项 → 附加开发板管理器网址：
https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json
```

**安装开发板包**
```
工具 → 开发板 → 开发板管理器 → 搜索 "M5Stack" → 安装 "M5Stack ESP32 Boards"
```

**选择开发板**
```
工具 → 开发板 → M5Stack Arduino → M5Stack-Core-ESP32-S3
```

### 1.2 依赖库安装

通过 Arduino Library Manager 安装：
- **M5Unified** — M5Stack 硬件抽象层
- **HX711_ADC** — HX711 称重模块驱动
- **ArduinoJson** — JSON 配置文件处理（可选）

内置库（无需安装）：
- **TFT_eSPI** — 屏幕驱动（M5Stack 内置）
- **SD** — SD 卡支持（Arduino 内置）

### 1.3 项目结构

```
Coffee-Digital-Scale-ADV/
├── coffee_scale.ino          ← 主程序入口
├── WeightSensor.h            ← 称重传感器模块头文件
├── WeightSensor.cpp          ← 称重传感器模块实现
├── FlowCalculator.h          ← 流量计算模块头文件
├── FlowCalculator.cpp        ← 流量计算模块实现
├── TimerModule.h             ← 计时器模块头文件
├── TimerModule.cpp           ← 计时器模块实现
├── DisplayModule.h           ← 显示模块头文件
├── DisplayModule.cpp         ← 显示模块实现
├── StorageModule.h           ← 存储模块头文件
├── StorageModule.cpp         ← 存储模块实现
├── config.h                  ← 全局配置常量
└── README.md                 ← 项目说明
```

---

## 2. 全局配置设计 (config.h)

```cpp
#ifndef CONFIG_H
#define CONFIG_H

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

// ========== 历史数据 ==========
#define HISTORY_BUFFER_SIZE         240     // 曲线数据点数（屏幕宽度）

// ========== UI 配色 ==========
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

// ========== SD 卡 ==========
#define SD_CS_PIN           4     // SD 卡片选引脚（Cardputer 默认）
#define DATA_LOG_DIR        "/coffee_data"
#define CONFIG_DIR          "/config"
#define CALIBRATION_FILE    "/config/calibration.json"
#define SETTINGS_FILE       "/config/settings.json"

#endif
```

---

## 3. 模块详细设计

### 3.1 WeightSensor 模块

**职责**：封装 HX711 驱动，提供滤波后的稳定重量读数

**类设计**：
```cpp
class WeightSensor {
public:
    WeightSensor();
    
    // 初始化
    void init(uint8_t doutPin, uint8_t sckPin);
    
    // 去皮
    void tare(int samples = 10);
    
    // 获取滤波后的重量 (g)
    float getWeight();
    
    // 校准
    void setCalibrationFactor(float factor);
    float getCalibrationFactor();
    void calibrateWithKnownWeight(float knownWeight, int samples = 10);
    
    // 状态
    bool isReady();
    
private:
    HX711_ADC _scale;
    float _calibrationFactor;
    bool _ready;
    
    // 移动平均滤波
    float _filterBuffer[FILTER_WINDOW_SIZE];
    int _filterIndex;
    float _filterSum;
    float _applyFilter(float rawValue);
};
```

**关键算法 - 移动平均滤波**：
```cpp
float WeightSensor::_applyFilter(float rawValue) {
    _filterSum -= _filterBuffer[_filterIndex];
    _filterBuffer[_filterIndex] = rawValue;
    _filterSum += rawValue;
    _filterIndex = (_filterIndex + 1) % FILTER_WINDOW_SIZE;
    return _filterSum / FILTER_WINDOW_SIZE;
}
```

**实现要点**：
- `init()` 中调用 `_scale.begin()` 并等待就绪
- `getWeight()` 内部调用 `_scale.get_units()` 获取原始值，再应用滤波
- 校准因子持久化到 SD 卡

---

### 3.2 FlowCalculator 模块

**职责**：基于重量变化计算流量，维护历史数据

**类设计**：
```cpp
class FlowCalculator {
public:
    FlowCalculator();
    
    // 更新数据（每次重量采样调用）
    void update(float weight, unsigned long timestamp);
    
    // 获取当前流量 (g/s)
    float getFlowRate();
    
    // 获取历史数据（用于曲线绘制）
    float* getWeightHistory();
    float* getFlowHistory();
    unsigned long* getTimeHistory();
    int getHistoryCount();
    
    // 重置
    void reset();
    
private:
    // 环形缓冲区
    float _weightBuffer[HISTORY_BUFFER_SIZE];
    float _flowBuffer[HISTORY_BUFFER_SIZE];
    unsigned long _timeBuffer[HISTORY_BUFFER_SIZE];
    int _bufferIndex;
    int _bufferCount;
    
    // 流量计算
    float _lastWeight;
    unsigned long _lastTimestamp;
    float _currentFlowRate;
    
    // 滑动窗口平均
    static const int FLOW_WINDOW_SIZE = 10;  // 约 1 秒的数据
    float _flowWindow[FLOW_WINDOW_SIZE];
    int _flowWindowIndex;
    float _flowWindowSum;
};
```

**关键算法 - 流量计算**：
```cpp
void FlowCalculator::update(float weight, unsigned long timestamp) {
    if (_lastTimestamp > 0) {
        float dt = (timestamp - _lastTimestamp) / 1000.0f;  // 转换为秒
        if (dt > 0) {
            float instantFlow = (weight - _lastWeight) / dt;
            
            // 滑动窗口平均
            _flowWindowSum -= _flowWindow[_flowWindowIndex];
            _flowWindow[_flowWindowIndex] = instantFlow;
            _flowWindowSum += instantFlow;
            _flowWindowIndex = (_flowWindowIndex + 1) % FLOW_WINDOW_SIZE;
            
            _currentFlowRate = _flowWindowSum / FLOW_WINDOW_SIZE;
        }
    }
    
    _lastWeight = weight;
    _lastTimestamp = timestamp;
    
    // 存入环形缓冲区
    _weightBuffer[_bufferIndex] = weight;
    _flowBuffer[_bufferIndex] = _currentFlowRate;
    _timeBuffer[_bufferIndex] = timestamp;
    _bufferIndex = (_bufferIndex + 1) % HISTORY_BUFFER_SIZE;
    if (_bufferCount < HISTORY_BUFFER_SIZE) _bufferCount++;
}
```

**实现要点**：
- 环形缓冲区避免内存溢出
- 流量计算使用滑动窗口平均，避免瞬时波动
- `reset()` 清空所有缓冲区

---

### 3.3 TimerModule 模块

**职责**：自动/手动计时，支持重量触发

**类设计**：
```cpp
class TimerModule {
public:
    TimerModule();
    
    // 手动控制
    void start();
    void stop();
    void reset();
    
    // 自动控制（基于重量）
    void checkAutoStart(float weight);
    
    // 获取状态
    bool isRunning();
    unsigned long getElapsedMs();
    String getFormattedTime();  // 返回 "MM:SS.s" 格式
    
private:
    bool _running;
    bool _autoStarted;
    unsigned long _startTime;
    unsigned long _elapsed;
    unsigned long _pausedTime;
    
    // 自动重置检测
    bool _weightDetected;
};
```

**关键算法 - 自动计时**：
```cpp
void TimerModule::checkAutoStart(float weight) {
    // 自动启动：重量超过阈值
    if (!_running && !_autoStarted && weight > AUTO_START_THRESHOLD) {
        start();
        _autoStarted = true;
        _weightDetected = true;
    }
    
    // 自动重置：重量归零
    if (_autoStarted && weight < RESET_THRESHOLD) {
        if (_weightDetected) {
            _weightDetected = false;
            // 等待一段时间确认重量稳定后再重置
        } else {
            reset();
            _autoStarted = false;
        }
    }
    
    // 更新重量检测状态
    if (weight > RESET_THRESHOLD) {
        _weightDetected = true;
    }
}
```

**时间格式化**：
```cpp
String TimerModule::getFormattedTime() {
    unsigned long elapsed = getElapsedMs();
    int minutes = (elapsed / 60000) % 60;
    int seconds = (elapsed / 1000) % 60;
    int tenths = (elapsed / 100) % 10;
    
    char buffer[10];
    sprintf(buffer, "%02d:%02d.%d", minutes, seconds, tenths);
    return String(buffer);
}
```

---

### 3.4 DisplayModule 模块

**职责**：管理多页面 UI 渲染

**类设计**：
```cpp
class DisplayModule {
public:
    DisplayModule();
    
    // 初始化
    void init();
    
    // 页面切换
    enum Page { PAGE_MAIN, PAGE_WEIGHT_CURVE, PAGE_FLOW_CURVE, PAGE_SETTINGS };
    void setPage(Page page);
    Page getCurrentPage();
    
    // 更新显示（根据当前页面）
    void update(float weight, float flowRate, TimerModule* timer, FlowCalculator* flowCalc);
    
    // 输入处理
    void handleInput();  // 检测按键，切换页面
    
private:
    Page _currentPage;
    unsigned long _lastUpdateTime;
    
    // 页面渲染
    void _drawMainPage(float weight, float flowRate, TimerModule* timer, FlowCalculator* flowCalc);
    void _drawWeightCurvePage(FlowCalculator* flowCalc);
    void _drawFlowCurvePage(FlowCalculator* flowCalc);
    void _drawSettingsPage();
    
    // UI 组件
    void _drawTitle(const char* title);
    void _drawCurve(float* data, int count, int x, int y, int w, int h, 
                    float minVal, float maxVal, uint16_t color);
    void _drawGrid(int x, int y, int w, int h);
    
    // 背景绘制（只在页面切换时调用）
    void _drawMainBackground();
    void _drawCurveBackground(const char* title, const char* yLabel);
};
```

**主界面布局**：
```
┌──────────────────────────────┐
│   ☕ Pour Over Scale         │ ← 标题栏 (y: 0-20)
├──────────────────────────────┤
│                              │
│         125.6 g              │ ← 重量显示 (y: 20-70)
│                              │
├──────────────────────────────┤
│  Flow: 3.2 g/s  Time: 01:23 │ ← 状态栏 (y: 70-90)
├──────────────────────────────┤
│  ▁▂▃▅▆▇▆▅▃▂▁               │ ← 曲线预览 (y: 90-135)
└──────────────────────────────┘
```

**曲线绘制算法**：
```cpp
void DisplayModule::_drawCurve(float* data, int count, int x, int y, int w, int h,
                                float minVal, float maxVal, uint16_t color) {
    if (count < 2) return;
    
    float range = maxVal - minVal;
    if (range < 0.01f) range = 1.0f;  // 防止除零
    
    for (int i = 0; i < count - 1; i++) {
        int x1 = x + (i * w) / count;
        int x2 = x + ((i + 1) * w) / count;
        int y1 = y + h - (int)((data[i] - minVal) * h / range);
        int y2 = y + h - (int)((data[i + 1] - minVal) * h / range);
        
        M5.Lcd.drawLine(x1, y1, x2, y2, color);
    }
}
```

**按键处理**：
```cpp
void DisplayModule::handleInput() {
    M5.update();
    
    // 使用 Cardputer 的按键切换页面
    if (M5.BtnA.wasPressed()) {
        Page nextPage = (Page)((_currentPage + 1) % 4);
        setPage(nextPage);
    }
}
```

**实现要点**：
- 页面切换时重绘背景，更新时只刷新数据区域
- 曲线使用环形缓冲区数据，自动缩放时间轴
- 控制刷新频率，避免闪烁

---

### 3.5 StorageModule 模块

**职责**：SD 卡数据记录和配置持久化

**类设计**：
```cpp
class StorageModule {
public:
    StorageModule();
    
    // 初始化
    bool init();
    
    // 数据记录
    bool beginNewSession();
    bool logDataPoint(float weight, float flowRate, unsigned long elapsedMs);
    void endSession();
    
    // 配置管理
    bool saveCalibration(float factor);
    float loadCalibration(float defaultFactor);
    bool saveSettings(float autoStartThreshold, float resetThreshold);
    bool loadSettings(float& autoStartThreshold, float& resetThreshold);
    
    // 状态
    bool isSDReady();
    String getCurrentFileName();
    
private:
    bool _sdReady;
    bool _sessionActive;
    File _dataFile;
    String _currentFileName;
    
    String _generateFileName();
    bool _ensureDirectory(const char* path);
};
```

**CSV 格式**：
```cpp
bool StorageModule::beginNewSession() {
    if (!_sdReady) return false;
    
    _currentFileName = _generateFileName();  // "/coffee_data/brew_20260530_143022.csv"
    
    _dataFile = SD.open(_currentFileName, FILE_WRITE);
    if (!_dataFile) return false;
    
    // 写入 CSV 头
    _dataFile.println("timestamp_ms,weight_g,flow_rate_g_per_s,elapsed_s");
    _sessionActive = true;
    return true;
}

bool StorageModule::logDataPoint(float weight, float flowRate, unsigned long elapsedMs) {
    if (!_sessionActive || !_dataFile) return false;
    
    char line[50];
    sprintf(line, "%lu,%.1f,%.1f,%.1f", 
            millis(), weight, flowRate, elapsedMs / 1000.0f);
    _dataFile.println(line);
    
    // 每 10 条数据 flush 一次，平衡性能和数据安全
    static int flushCounter = 0;
    if (++flushCounter >= 10) {
        _dataFile.flush();
        flushCounter = 0;
    }
    return true;
}
```

**校准参数存储**：
```cpp
bool StorageModule::saveCalibration(float factor) {
    File file = SD.open(CALIBRATION_FILE, FILE_WRITE);
    if (!file) return false;
    
    // 简单 JSON 格式
    file.print("{\"calibration_factor\":");
    file.print(factor);
    file.print(",\"updated\":\"");
    // 可以添加时间戳
    file.println("\"}");
    file.close();
    return true;
}

float StorageModule::loadCalibration(float defaultFactor) {
    File file = SD.open(CALIBRATION_FILE, FILE_READ);
    if (!file) return defaultFactor;
    
    // 简单解析（或使用 ArduinoJson）
    String content = file.readString();
    file.close();
    
    int startIndex = content.indexOf("calibration_factor\":") + 20;
    int endIndex = content.indexOf(",", startIndex);
    if (startIndex > 20 && endIndex > startIndex) {
        return content.substring(startIndex, endIndex).toFloat();
    }
    return defaultFactor;
}
```

---

## 4. 主程序设计 (coffee_scale.ino)

```cpp
#include <M5Unified.h>
#include "config.h"
#include "WeightSensor.h"
#include "FlowCalculator.h"
#include "TimerModule.h"
#include "DisplayModule.h"
#include "StorageModule.h"

// 全局模块实例
WeightSensor weightSensor;
FlowCalculator flowCalculator;
TimerModule timerModule;
DisplayModule displayModule;
StorageModule storageModule;

// 定时器
unsigned long lastSensorUpdate = 0;
unsigned long lastDisplayUpdate = 0;

void setup() {
    // 初始化 M5Stack
    M5.begin();
    Serial.begin(115200);
    
    // 初始化显示
    displayModule.init();
    displayModule.setPage(DisplayModule::PAGE_MAIN);
    
    // 初始化存储
    storageModule.init();
    
    // 加载校准参数
    float calFactor = storageModule.loadCalibration(DEFAULT_CALIBRATION_FACTOR);
    
    // 初始化传感器
    weightSensor.init(HX711_DOUT_PIN, HX711_SCK_PIN);
    weightSensor.setCalibrationFactor(calFactor);
    
    // 自动去皮
    M5.Lcd.println("Taring...");
    weightSensor.tare();
    M5.Lcd.println("Ready!");
    delay(1000);
    
    // 开始数据记录会话
    storageModule.beginNewSession();
}

void loop() {
    unsigned long now = millis();
    M5.update();
    
    // 传感器更新（10Hz）
    if (now - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL) {
        float weight = weightSensor.getWeight();
        
        // 更新流量计算
        flowCalculator.update(weight, now);
        
        // 更新计时器
        timerModule.checkAutoStart(weight);
        
        // 记录数据
        if (timerModule.isRunning()) {
            storageModule.logDataPoint(
                weight, 
                flowCalculator.getFlowRate(), 
                timerModule.getElapsedMs()
            );
        }
        
        lastSensorUpdate = now;
    }
    
    // 显示更新（10Hz）
    if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
        float weight = weightSensor.getWeight();
        float flowRate = flowCalculator.getFlowRate();
        
        displayModule.update(weight, flowRate, &timerModule, &flowCalculator);
        displayModule.handleInput();
        
        lastDisplayUpdate = now;
    }
}
```

---

## 5. 开发顺序和依赖关系

### Phase 1: 基础框架（Issue #1）

**目标**：验证硬件连接，建立项目骨架

**任务**：
1. 创建 Arduino 项目结构
2. 配置 M5Unified 和 HX711_ADC
3. 实现基本的 HX711 读取
4. 在屏幕显示原始读数
5. 实现手动 tare 功能

**验收标准**：
- [ ] 编译成功
- [ ] 屏幕正常初始化
- [ ] HX711 读数稳定
- [ ] 按键 tare 后读数归零

---

### Phase 2: 核心模块（Issues #2, #3, #4, #5）

**目标**：实现所有数据处理模块

**任务**：
1. 实现 WeightSensor 模块（#2）
   - 移动平均滤波
   - 校准接口
2. 实现自动去皮（#3）
3. 实现 TimerModule（#4）
   - 自动启动逻辑
   - 时间格式化
4. 实现 FlowCalculator（#5）
   - 环形缓冲区
   - 滑动窗口平均

**验收标准**：
- [ ] 重量读数稳定，无跳动
- [ ] 启动后自动 tare
- [ ] 重量变化时自动开始计时
- [ ] 重量归零时自动重置计时
- [ ] 流量计算平滑

---

### Phase 3: UI 开发（Issue #6）

**目标**：实现完整的主界面

**任务**：
1. 设计主界面布局
2. 实现 DisplayModule 主页面
3. 整合所有数据到一个界面
4. 添加简化曲线预览
5. 优化刷新频率

**验收标准**：
- [ ] 主界面显示重量、流量、时间
- [ ] 深色主题配色
- [ ] 重量大字体居中
- [ ] 曲线预览实时更新
- [ ] 无明显闪烁

---

### Phase 4: 曲线页面（Issues #7, #8）

**目标**：实现历史曲线显示

**任务**：
1. 实现曲线绘制函数（#7）
2. 实现页面切换逻辑（#7）
3. 实现重量曲线页面（#7）
4. 实现流量曲线页面（#8）
5. 实现时间轴自动缩放

**验收标准**：
- [ ] 重量曲线实时绘制
- [ ] 流量曲线实时绘制
- [ ] 时间轴自动缩放
- [ ] 按键切换页面流畅

---

### Phase 5: 数据持久化（Issue #9）

**目标**：实现 SD 卡数据记录

**任务**：
1. 实现 StorageModule
2. 实现 CSV 数据记录
3. 实现文件管理
4. 测试数据导出

**验收标准**：
- [ ] SD 卡正常初始化
- [ ] 每次冲煮创建独立 CSV 文件
- [ ] CSV 格式正确，可被 Excel 打开
- [ ] 文件名包含时间戳

---

### Phase 6: 设置界面（Issues #10, #11）

**目标**：实现校准和设置功能

**任务**：
1. 实现校准界面（#10）
2. 实现校准流程
3. 实现阈值设置界面（#11）
4. 实现配置持久化

**验收标准**：
- [ ] 校准界面可进入
- [ ] 校准因子计算正确
- [ ] 校准后重量准确
- [ ] 阈值可配置
- [ ] 设置保存到 SD 卡

---

## 6. 测试策略

### 6.1 硬件测试（手动）

每个 Phase 完成后进行硬件验证：

**Phase 1 测试**：
- 串口输出原始 ADC 值
- 按键 tare 功能
- 屏幕显示基本文字

**Phase 2 测试**：
- 放置已知重量，验证读数准确
- 重量变化时计时自动启动
- 重量移除后计时自动重置
- 流量显示是否平滑

**Phase 3-6 测试**：
- 主界面数据完整性
- 曲线绘制正确性
- 页面切换流畅性
- SD 卡数据记录

### 6.2 单元测试（可选）

对于纯逻辑模块，可以编写单元测试：

**FlowCalculator 测试**：
```cpp
// 测试流量计算
void test_flow_calculation() {
    FlowCalculator calc;
    calc.update(0.0, 0);
    calc.update(10.0, 1000);  // 10g 在 1 秒内
    assert(abs(calc.getFlowRate() - 10.0f) < 0.1f);
}
```

**TimerModule 测试**：
```cpp
// 测试自动启动
void test_auto_start() {
    TimerModule timer;
    timer.checkAutoStart(0.0);
    assert(!timer.isRunning());
    timer.checkAutoStart(1.0);  // 超过阈值
    assert(timer.isRunning());
}
```

### 6.3 集成测试

**完整冲煮流程测试**：
1. 启动设备，验证自动 tare
2. 放置咖啡杯，验证重量显示
3. 开始注水，验证：
   - 计时自动启动
   - 流量实时更新
   - 曲线开始绘制
4. 停止注水，验证：
   - 流量归零
   - 计时继续
5. 移除咖啡杯，验证：
   - 计时自动重置
   - 数据保存到 SD 卡
6. 检查 CSV 文件内容

---

## 7. 风险和应对

### 7.1 HX711 噪声干扰

**风险**：ESP32 的 WiFi/BLE 可能引入噪声

**应对**：
- 在采样期间禁用无线功能
- 增加滤波窗口大小
- 使用屏蔽线连接 HX711

### 7.2 屏幕闪烁

**风险**：频繁刷新导致视觉闪烁

**应对**：
- 使用局部刷新，只更新变化区域
- 控制刷新频率（10Hz 足够）
- 使用双缓冲（如果支持）

### 7.3 内存不足

**风险**：历史数据缓冲区占用过多内存

**应对**：
- 使用固定大小的环形缓冲区
- HISTORY_BUFFER_SIZE = 240（屏幕宽度）
- 每个数据点约 12 字节，总计约 2.8KB

### 7.4 SD 卡写入性能

**风险**：频繁写入导致性能问题

**应对**：
- 批量写入，每 10 条数据 flush 一次
- 使用高速 SD 卡（Class 10 以上）
- 在非关键路径写入（不影响主循环）

### 7.5 电源稳定性

**风险**：HX711 和屏幕同时工作可能导致电源波动

**应对**：
- 使用独立的 5V 供电给 HX711
- 添加滤波电容
- 避免同时开启背光和采样

---

## 8. 性能优化

### 8.1 主循环优化

```cpp
void loop() {
    unsigned long now = millis();
    
    // 使用状态机避免阻塞
    static enum { STATE_READ, STATE_UPDATE, STATE_DISPLAY } state = STATE_READ;
    
    switch (state) {
        case STATE_READ:
            if (now - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL) {
                readSensor();
                lastSensorUpdate = now;
                state = STATE_UPDATE;
            }
            break;
            
        case STATE_UPDATE:
            updateCalculations();
            state = STATE_DISPLAY;
            break;
            
        case STATE_DISPLAY:
            if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
                updateDisplay();
                lastDisplayUpdate = now;
                state = STATE_READ;
            }
            break;
    }
}
```

### 8.2 显示优化

- 使用 `fillRect()` 清除旧数据，而不是全屏重绘
- 曲线使用 `drawLine()` 逐段绘制
- 文字使用固定宽度字体，避免重绘

### 8.3 内存优化

- 使用 `PROGMEM` 存储常量字符串
- 避免动态内存分配
- 使用 `sizeof()` 检查结构体大小

---

## 9. 扩展功能（未来考虑）

### 9.1 蓝牙数据传输

- 使用 ESP32 BLE 将数据发送到手机 App
- 实现实时监控和数据导出

### 9.2 WiFi OTA 更新

- 实现空中固件更新
- 无需 USB 连接即可升级

### 9.3 配方管理

- 存储冲煮配方（水量、时间、流速）
- 支持回放和对比

### 9.4 声音提示

- 蜂鸣器提示冲煮阶段
- 语音播报关键数据

---

## 10. 参考资源

- **M5Stack 文档**：https://docs.m5stack.com
- **M5Unified 库**：https://github.com/m5stack/M5Unified
- **HX711_ADC 库**：https://github.com/olkal/HX711_ADC
- **ESP32-S3 数据手册**：https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
- **Arduino 参考**：https://www.arduino.cc/reference/en
