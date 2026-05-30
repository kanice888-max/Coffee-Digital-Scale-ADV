/*
 * Cardputer-ADV 手冲咖啡秤固件
 *
 * 功能：
 * - 实时重量显示（HX711 + 压力传感器）
 * - 流量计算和显示
 * - 自动计时（重量变化触发）
 * - 重量/流量历史曲线
 * - SD 卡数据记录
 *
 * 硬件：
 * - M5Stack Cardputer-adv (ESP32-S3)
 * - HX711 模块：DT/DOUT → G2, SCK/CLK → G1
 * - MicroSD 卡
 *
 * 作者：Coffee Digital Scale Team
 * 版本：1.0.0
 */

#include <M5Unified.h>
#include "config.h"
#include "WeightSensor.h"
#include "FlowCalculator.h"
#include "TimerModule.h"
#include "DisplayModule.h"
#include "StorageModule.h"

// ========== 全局模块实例 ==========
WeightSensor weightSensor(HX711_DOUT_PIN, HX711_SCK_PIN);
FlowCalculator flowCalculator;
TimerModule timerModule;
DisplayModule displayModule;
StorageModule storageModule;

// ========== 定时器 ==========
unsigned long lastSensorUpdate = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastSessionCheck = 0;

// ========== 状态缓存 ==========
float currentWeight = 0;
float currentFlowRate = 0;

// ========== 运行状态 ==========
bool isRunning = false;
bool sessionActive = false;

// ========== 错误处理 ==========
int sensorErrorCount = 0;

// ========== 初始化 ==========
void setup() {
    // 初始化 M5Stack
    auto cfg = M5.config();
    M5.begin(cfg);

    Serial.begin(115200);
    Serial.println(F("\n=== Coffee Digital Scale ==="));
    Serial.println(F("Version: 1.0.0"));

    // 初始化显示模块
    displayModule.init();
    displayModule.showMessage(F("Initializing..."), 500);

    // 初始化存储模块（非阻塞，允许无 SD 卡运行）
    bool sdReady = storageModule.init();
    if (sdReady) {
        Serial.println(F("Storage initialized"));

        // 加载用户设置
        loadSettings();

        // 加载校准参数
        loadCalibration();
    } else {
        Serial.println(F("Storage initialization failed - continuing without SD"));
    }

    // 初始化传感器模块
    displayModule.showMessage(F("Init Sensor..."), 500);
    if (!weightSensor.init()) {
        displayModule.showMessage(F("Sensor Error!"), 0);
        Serial.println(F("FATAL: HX711 initialization failed"));
        haltSystem();
    }

    // 自动去皮
    displayModule.showMessage(F("Taring..."), 500);
    weightSensor.tare();
    displayModule.showMessage(F("Ready!"), 1000);

    // 切换到主界面
    displayModule.setPage(DisplayModule::PAGE_MAIN);

    Serial.println(F("Initialization complete"));
    isRunning = true;
}

// ========== 主循环 ==========
void loop() {
    if (!isRunning) return;

    unsigned long now = millis();
    M5.update();

    // 传感器更新（10Hz）
    if (now - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL) {
        updateSensor(now);
        lastSensorUpdate = now;
    }

    // 显示更新（10Hz）
    if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
        updateDisplay();
        lastDisplayUpdate = now;
    }

    // 会话管理（每 5 秒检查一次）
    if (now - lastSessionCheck >= SESSION_CHECK_INTERVAL) {
        manageSession();
        lastSessionCheck = now;
    }
}

// ========== 传感器更新 ==========
void updateSensor(unsigned long now) {
    // 读取重量
    float weight = weightSensor.getWeight();

    // 检查传感器异常
    checkSensorHealth(weight);

    // 更新缓存
    currentWeight = weight;

    // 更新流量计算
    flowCalculator.update(weight, now);
    currentFlowRate = flowCalculator.getFlowRate();

    // 更新计时器
    timerModule.checkAutoStart(weight);

    // 记录数据（仅在计时运行时）
    if (timerModule.isRunning() && storageModule.isSDReady()) {
        if (!sessionActive) {
            storageModule.beginNewSession();
            sessionActive = true;
        }
        storageModule.logDataPoint(weight, currentFlowRate, timerModule.getElapsedMs());
    }
}

// ========== 显示更新 ==========
void updateDisplay() {
    // 使用缓存的重量和流量，避免重复读取传感器
    displayModule.update(currentWeight, currentFlowRate, &timerModule, &flowCalculator);
}

// ========== 会话管理 ==========
void manageSession() {
    // 计时停止且有数据时，结束会话
    if (!timerModule.isRunning() && sessionActive) {
        storageModule.endSession();
        sessionActive = false;
        Serial.println(F("Session ended"));
    }
}

// ========== 健康检查 ==========
void checkSensorHealth(float weight) {
    // 检测传感器读数异常（从有重量突然变为 0）
    if (weight == 0 && currentWeight > 10) {
        sensorErrorCount++;
        if (sensorErrorCount >= MAX_SENSOR_ERRORS) {
            Serial.println(F("WARNING: Sensor reading anomaly detected"));
            sensorErrorCount = 0;
        }
    } else {
        sensorErrorCount = 0;
    }
}

// ========== 设置加载 ==========
void loadSettings() {
    float autoStartThreshold = AUTO_START_THRESHOLD;
    float resetThreshold = RESET_THRESHOLD;

    if (storageModule.loadSettings(autoStartThreshold, resetThreshold)) {
        timerModule.setAutoStartThreshold(autoStartThreshold);
        timerModule.setResetThreshold(resetThreshold);
        Serial.println(F("Settings loaded"));
    }
}

// ========== 校准加载 ==========
void loadCalibration() {
    float calFactor = storageModule.loadCalibration(DEFAULT_CALIBRATION_FACTOR);
    weightSensor.setCalibrationFactor(calFactor);
    Serial.print(F("Calibration factor: "));
    Serial.println(calFactor);
}

// ========== 系统停止 ==========
void haltSystem() {
    Serial.println(F("System halted"));
    while (1) {
        delay(1000);
    }
}
