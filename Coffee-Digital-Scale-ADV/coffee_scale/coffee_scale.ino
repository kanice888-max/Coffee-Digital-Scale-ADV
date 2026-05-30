/*
 * Cardputer-ADV 手冲咖啡秤固件
 *
 * 功能：
 * - 实时重量显示（HX711 + 压力传感器）
 * - 流量计算和显示
 * - 自动计时（重量变化触发）
 * - 重量/流量历史曲线
 * - SD 卡数据记录
 * - 水分比/粉量设置 + 目标水量声音提示
 *
 * 硬件：
 * - M5Stack Cardputer-adv (ESP32-S3)
 * - HX711 模块：DT/DOUT → G2, SCK/CLK → G1
 * - MicroSD 卡
 */

#include <M5Cardputer.h>
#include <math.h>
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

// ========== 定时器（绝对时间对齐） ==========
unsigned long nextSensorUpdate = 0;
unsigned long nextDisplayUpdate = 0;
unsigned long nextSessionCheck = 0;

// ========== 状态缓存 ==========
float currentWeight = 0;
float currentFlowRate = 0;

// ========== 运行状态 ==========
bool isRunning = false;
bool sessionActive = false;

// ========== 错误处理 ==========
int sensorErrorCount = 0;

// ========== 冲煮配方参数 ==========
float waterRatio = DEFAULT_RATIO;       // 水分比（如 1:15）
float coffeeDose = DEFAULT_DOSE;        // 粉量 (g)
float targetWater = 0;                  // 目标注水量 (g)
bool targetReached = false;             // 是否已到达目标水量

// ========== 键盘输入状态 ==========
bool inputMode = false;
String inputBuffer = "";
char inputTarget = 0;  // 'r' = ratio, 'd' = dose

// ========== 函数声明 ==========
void updateSensor(unsigned long now);
void updateDisplay();
void manageSession();
void checkSensorHealth(float weight);
void loadSettings();
void loadCalibration();
void saveSettings();
void playTargetReachedSound();
void handleKeyboardInput(DisplayModule* disp);

// ========== 初始化 ==========
void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);
    // 初始化扬声器（M5Cardputer.Speaker 是 M5.Speaker 的别名）
    M5Cardputer.Speaker.begin();

    Serial.begin(115200);
    Serial.println(F("\n=== Coffee Digital Scale ==="));
    Serial.println(F("Version: 1.1.0"));

    // 初始化显示模块
    displayModule.init();
    displayModule.showMessage(F("Initializing..."), 500);

    // 初始化存储模块（非阻塞，允许无 SD 卡运行）
    bool sdReady = storageModule.init();
    if (sdReady) {
        Serial.println(F("Storage initialized"));
        loadSettings();
        loadCalibration();
    } else {
        Serial.println(F("Storage initialization failed - continuing without SD"));
    }

    // 计算目标水量
    targetWater = waterRatio * coffeeDose;

    // 初始化传感器模块
    displayModule.showMessage(F("Init Sensor..."), 500);
    if (!weightSensor.init()) {
        displayModule.showMessage(F("Sensor Error!"), 2000);
#if !DEMO_MODE
        Serial.println(F("FATAL: HX711 initialization failed"));
        haltSystem();
#else
        Serial.println(F("DEMO: HX711 not present, running in simulation mode"));
#endif
    }

    // 自动去皮
    displayModule.showMessage(F("Taring..."), 500);
    weightSensor.tare();
    displayModule.showMessage(F("Ready!"), 1000);

    // 切换到主界面
    displayModule.setPage(DisplayModule::PAGE_MAIN);
    displayModule.setBrewParams(waterRatio, coffeeDose, targetWater);

    // 初始化绝对定时器
    nextSensorUpdate = millis() + SENSOR_UPDATE_INTERVAL;
    nextDisplayUpdate = millis() + DISPLAY_UPDATE_INTERVAL;
    nextSessionCheck = millis() + SESSION_CHECK_INTERVAL;

    Serial.println(F("Initialization complete"));
    Serial.print(F("Target water: "));
    Serial.println(targetWater);
    isRunning = true;
}

// ========== 主循环 ==========
void loop() {
    if (!isRunning) return;

    unsigned long now = millis();
    M5Cardputer.update();

    // 处理 Cardputer 键盘输入
    handleKeyboardInput(&displayModule);

    // 传感器更新（10Hz）
    if (now >= nextSensorUpdate) {
        updateSensor(now);
        nextSensorUpdate += SENSOR_UPDATE_INTERVAL;
        if (nextSensorUpdate < now) {
            nextSensorUpdate = now + SENSOR_UPDATE_INTERVAL;
        }
    }

    // 显示更新（10Hz）
    if (now >= nextDisplayUpdate) {
        updateDisplay();
        nextDisplayUpdate += DISPLAY_UPDATE_INTERVAL;
        if (nextDisplayUpdate < now) {
            nextDisplayUpdate = now + DISPLAY_UPDATE_INTERVAL;
        }
    }

    // 会话管理
    if (now >= nextSessionCheck) {
        manageSession();
        nextSessionCheck += SESSION_CHECK_INTERVAL;
        if (nextSessionCheck < now) {
            nextSessionCheck = now + SESSION_CHECK_INTERVAL;
        }
    }
}

// ========== 传感器更新 ==========
void updateSensor(unsigned long now) {
    float weight = weightSensor.getWeight();
    checkSensorHealth(weight);
    currentWeight = weight;

    flowCalculator.update(weight, now);
    currentFlowRate = flowCalculator.getFlowRate();

    timerModule.checkAutoStart(weight);

    // 检查是否达到目标水量
    if (timerModule.isRunning() && !targetReached && weight >= targetWater && targetWater > 0) {
        targetReached = true;
        playTargetReachedSound();
    }

    // 计时器重置时重置目标状态
    if (!timerModule.isRunning() && targetReached) {
        targetReached = false;
    }

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
    displayModule.update(currentWeight, currentFlowRate, &timerModule, &flowCalculator);
    displayModule.setBrewParams(waterRatio, coffeeDose, targetWater);
    displayModule.setTargetReached(targetReached);
}

// ========== 会话管理 ==========
void manageSession() {
    if (!timerModule.isRunning() && sessionActive) {
        storageModule.endSession();
        sessionActive = false;
        Serial.println(F("Session ended"));
    }
}

// ========== 健康检查 ==========
void checkSensorHealth(float weight) {
    // 检测异常：读数超出合理范围
    if (weight < -500 || weight > 50000) {
        sensorErrorCount++;
    } else if (timerModule.isRunning() && fabs(weight) < 0.01f && currentWeight > 5) {
        // 计时运行中重量突然归零，可能传感器异常
        sensorErrorCount++;
    } else {
        sensorErrorCount = 0;
    }

    if (sensorErrorCount >= MAX_SENSOR_ERRORS) {
        Serial.println(F("WARNING: Sensor reading anomaly detected"));
        sensorErrorCount = 0;
    }
}

// ========== 目标水量声音提示 ==========
void playTargetReachedSound() {
    M5Cardputer.Speaker.tone(880, 200);
    delay(250);
    M5Cardputer.Speaker.tone(1320, 200);
    delay(250);
    M5Cardputer.Speaker.tone(1760, 400);
    Serial.println(F("Target water reached!"));
}

// ========== Cardputer 键盘输入处理 ==========
void handleKeyboardInput(DisplayModule* disp) {
    if (!M5Cardputer.Keyboard.isChange()) return;
    if (!M5Cardputer.Keyboard.isPressed()) return;

    auto& state = M5Cardputer.Keyboard.keysState();
    DisplayModule::Page page = disp->getCurrentPage();

    // === 设置页 ===
    if (page == DisplayModule::PAGE_SETTINGS) {
        if (!inputMode) {
            // 选择功能模式
            for (auto ch : state.word) {
                if (ch == '1') {
                    disp->setPage(DisplayModule::PAGE_MAIN);
                    return;
                }
                if (ch == 'r' || ch == 'R') {
                    inputMode = true;
                    inputTarget = 'r';
                    inputBuffer = "";
                    Serial.println(F("Enter ratio: "));
                    return;
                }
                if (ch == 'd' || ch == 'D') {
                    inputMode = true;
                    inputTarget = 'd';
                    inputBuffer = "";
                    Serial.println(F("Enter dose (g): "));
                    return;
                }
            }
        } else {
            // 数字输入模式
            for (auto ch : state.word) {
                if (ch >= '0' && ch <= '9') {
                    inputBuffer += ch;
                }
            }
            if (state.enter) {
                float val = inputBuffer.toFloat();
                if (val > 0) {
                    if (inputTarget == 'r') {
                        waterRatio = val;
                    } else if (inputTarget == 'd') {
                        coffeeDose = val;
                    }
                    targetWater = waterRatio * coffeeDose;
                    targetReached = false;
                    Serial.print(F("Target water: "));
                    Serial.println(targetWater);
                    saveSettings();
                }
                inputMode = false;
                inputBuffer = "";
                return;
            }
            if (state.del && inputBuffer.length() > 0) {
                inputBuffer.remove(inputBuffer.length() - 1);
                return;
            }
        }
        return;
    }

    // === 主界面 ===
    if (page == DisplayModule::PAGE_MAIN) {
        for (auto ch : state.word) {
            if (ch == '1') {
                disp->setPage(DisplayModule::PAGE_SETTINGS);
                return;
            }
        }
        return;
    }

    // === 其他页面（曲线页）：按 '1' 翻页 ===
    for (auto ch : state.word) {
        if (ch == '1') {
            DisplayModule::Page nextPage = static_cast<DisplayModule::Page>((page + 1) % 4);
            disp->setPage(nextPage);
            return;
        }
    }
}

// ========== 设置加载 ==========
void loadSettings() {
    float autoStartThreshold = AUTO_START_THRESHOLD;
    float resetThreshold = RESET_THRESHOLD;
    float ratio = DEFAULT_RATIO;
    float dose = DEFAULT_DOSE;

    if (storageModule.loadSettings(autoStartThreshold, resetThreshold, ratio, dose)) {
        timerModule.setAutoStartThreshold(autoStartThreshold);
        timerModule.setResetThreshold(resetThreshold);
        waterRatio = ratio;
        coffeeDose = dose;
        Serial.print(F("Settings loaded: ratio="));
        Serial.print(waterRatio);
        Serial.print(F(" dose="));
        Serial.println(coffeeDose);
    }
}

// ========== 设置保存 ==========
void saveSettings() {
    if (storageModule.isSDReady()) {
        storageModule.saveSettings(
            timerModule.getAutoStartThreshold(),
            timerModule.getResetThreshold(),
            waterRatio,
            coffeeDose
        );
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
