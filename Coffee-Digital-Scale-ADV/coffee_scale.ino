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
 */

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

// 状态
bool isRunning = false;

void setup() {
    // 初始化 M5Stack
    auto cfg = M5.config();
    M5.begin(cfg);

    Serial.begin(115200);
    Serial.println("\n=== Coffee Digital Scale ===");

    // 初始化显示
    displayModule.init();
    displayModule.showMessage("Initializing...", 500);

    // 初始化存储
    if (storageModule.init()) {
        Serial.println("Storage initialized");

        // 加载设置
        float autoStartThreshold = AUTO_START_THRESHOLD;
        float resetThreshold = RESET_THRESHOLD;
        storageModule.loadSettings(autoStartThreshold, resetThreshold);
        timerModule.setAutoStartThreshold(autoStartThreshold);
        timerModule.setResetThreshold(resetThreshold);

        // 加载校准参数
        float calFactor = storageModule.loadCalibration(DEFAULT_CALIBRATION_FACTOR);
        weightSensor.setCalibrationFactor(calFactor);
    } else {
        Serial.println("Storage initialization failed");
    }

    // 初始化传感器
    displayModule.showMessage("Init Sensor...", 500);
    weightSensor.init(HX711_DOUT_PIN, HX711_SCK_PIN);

    if (!weightSensor.isReady()) {
        displayModule.showMessage("Sensor Error!", 0);
        while (1) {
            delay(1000);  // 停止运行
        }
    }

    // 自动去皮
    displayModule.showMessage("Taring...", 500);
    weightSensor.tare();
    displayModule.showMessage("Ready!", 1000);

    // 开始数据记录会话
    if (storageModule.isSDReady()) {
        storageModule.beginNewSession();
    }

    // 切换到主界面
    displayModule.setPage(DisplayModule::PAGE_MAIN);

    Serial.println("Initialization complete");
    isRunning = true;
}

void loop() {
    if (!isRunning) return;

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
        if (timerModule.isRunning() && storageModule.isSDReady()) {
            storageModule.logDataPoint(
                weight,
                flowCalculator.getFlowRate(),
                timerModule.getElapsedMs()
            );
        }

        lastSensorUpdate = now;
    }

    // 显示更新
    if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
        float weight = weightSensor.getWeight();
        float flowRate = flowCalculator.getFlowRate();

        displayModule.update(weight, flowRate, &timerModule, &flowCalculator);

        lastDisplayUpdate = now;
    }

    // 检查是否需要结束会话
    if (!timerModule.isRunning() && storageModule.getSessionDataCount() > 0) {
        storageModule.endSession();

        // 开始新会话
        if (storageModule.isSDReady()) {
            storageModule.beginNewSession();
        }
    }
}
