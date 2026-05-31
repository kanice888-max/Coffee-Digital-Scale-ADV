/*
 * Cardputer-ADV 手冲咖啡秤固件
 *
 * 功能：
 * - 实时重量显示（HX711 + 压力传感器）
 * - 流量计算和显示
 * - 自动计时（重量变化触发）
 * - 重量/流量历史曲线
 * - SD 卡数据记录
 * - 水分比/粉量设置 + 目标水量状态提示
 *
 * 硬件：
 * - M5Stack Cardputer-adv (ESP32-S3)
 * - HX711 模块：DT/DOUT → G2, SCK/CLK → G1
 * - MicroSD 卡
 */

#include <M5Cardputer.h>
#include <math.h>
#include <stdlib.h>
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
unsigned long lastBeepSegment = 0;        // 上次提醒的 30 秒段
float stableCheckWeight = 0;              // 稳定检测起始重量
unsigned long stableCheckStart = 0;        // 稳定检测开始时间
bool stableCheckActive = false;           // 是否在稳定检测中

// ========== 键盘输入状态 ==========
bool inputMode = false;
char inputBuffer[16] = "";
uint8_t inputLength = 0;
char inputTarget = 0;  // 'r' = ratio, 'd' = dose, 'c' = calibration
uint8_t settingsSelectedIndex = 0;  // 0 = ratio, 1 = dose

// ========== 函数声明 ==========
void updateSensor(unsigned long now);
void updateDisplay();
void manageSession();
void checkSensorHealth(float weight);
void loadSettings();
void loadCalibration();
void saveSettings();
void handleKeyboardInput(DisplayModule* disp);
void playTargetReachedSound();
void resetInput();
bool appendInputChar(char ch);
bool parseInputFloat(float& value);
void syncSettingsInputState();

// ========== 初始化 ==========
void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);

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
    displayModule.setCalibrationFactor(weightSensor.getCalibrationFactor());

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
    if (!weightSensor.update()) return;

    float weight = weightSensor.getWeight();
    checkSensorHealth(weight);
    currentWeight = weight;

    flowCalculator.update(weight, now);
    currentFlowRate = flowCalculator.getFlowRate();

    bool wasTimerRunning = timerModule.isRunning();
    timerModule.checkAutoStart(weight);
    bool timerRunning = timerModule.isRunning();

    // === 每 30 秒蜂鸣提醒 ===
    if (timerRunning) {
        unsigned long seg = timerModule.getElapsedMs() / 30000;
        if (seg > 0 && seg != lastBeepSegment) {
            lastBeepSegment = seg;
            M5Cardputer.Speaker.tone(1760, 150);
        }
    }

    // === 检查是否达到目标水量 ===
    if (timerRunning && !targetReached && weight >= targetWater && targetWater > 0) {
        targetReached = true;
        stableCheckWeight = weight;
        stableCheckStart = millis();
        stableCheckActive = true;
        playTargetReachedSound();
    }

    // === 目标达成后：5 秒重量无变化则停止计时 ===
    if (timerRunning && targetReached && stableCheckActive) {
        if (fabs(weight - stableCheckWeight) < 0.5f) {
            if (millis() - stableCheckStart >= 5000) {
                timerModule.stop();
                // stop() 不重置 elapsed，由 checkAutoStart 下次触发后重置
                stableCheckActive = false;
                Serial.println(F("Stable weight - timer stopped"));
            }
        } else {
            // 重量还在变化，重置稳定计时
            stableCheckWeight = weight;
            stableCheckStart = millis();
        }
    }

    // 计时器重置时重置目标状态和提醒段
    if (!timerRunning && (targetReached || lastBeepSegment > 0)) {
        targetReached = false;
        stableCheckActive = false;
        lastBeepSegment = 0;
    }

    if (wasTimerRunning && !timerRunning && sessionActive) {
        storageModule.endSession();
        sessionActive = false;
    }

    if (timerRunning && storageModule.isSDReady()) {
        if (!sessionActive) {
            sessionActive = storageModule.beginNewSession();
        }
        if (sessionActive) {
            storageModule.logDataPoint(weight, currentFlowRate, timerModule.getElapsedMs());
        }
    }
}

// ========== 显示更新 ==========
void updateDisplay() {
    displayModule.setBrewParams(waterRatio, coffeeDose, targetWater);
    displayModule.setCalibrationFactor(weightSensor.getCalibrationFactor());
    displayModule.setTargetReached(targetReached);
    displayModule.update(currentWeight, currentFlowRate, &timerModule, &flowCalculator);
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

// ========== Cardputer 键盘输入处理 ==========
void handleKeyboardInput(DisplayModule* disp) {
    if (!M5Cardputer.Keyboard.isChange()) return;
    if (!M5Cardputer.Keyboard.isPressed()) return;

    auto& state = M5Cardputer.Keyboard.keysState();
    DisplayModule::Page page = disp->getCurrentPage();

    // === 设置页 ===
    if (page == DisplayModule::PAGE_SETTINGS) {
        if (!inputMode) {
            if (state.enter) {
                inputMode = true;
                inputTarget = (settingsSelectedIndex == 0) ? 'r' : 'd';
                resetInput();
                syncSettingsInputState();
                return;
            }

            for (auto ch : state.word) {
                if (ch == '1') {
                    disp->setPage(DisplayModule::PAGE_MAIN);
                    return;
                }
                if (ch == ';' || ch == ',') {
                    settingsSelectedIndex = (settingsSelectedIndex == 0) ? 1 : 0;
                    syncSettingsInputState();
                    return;
                }
                if (ch == '.' || ch == '/') {
                    settingsSelectedIndex = (settingsSelectedIndex + 1) % 2;
                    syncSettingsInputState();
                    return;
                }
                if (ch == 'c' || ch == 'C') {
                    inputMode = true;
                    inputTarget = 'c';
                    resetInput();
                    syncSettingsInputState();
                    Serial.println(F("Enter known weight (g): "));
                    return;
                }
            }
        } else {
            // 数字输入模式
            for (auto ch : state.word) {
                if ((ch >= '0' && ch <= '9') || ch == '.') {
                    appendInputChar(ch);
                }
            }
            if (state.enter) {
                float val = 0;
                if (parseInputFloat(val) && val > 0) {
                    if (inputTarget == 'r') {
                        waterRatio = val;
                        targetWater = waterRatio * coffeeDose;
                        targetReached = false;
                        Serial.print(F("Target water: "));
                        Serial.println(targetWater);
                        saveSettings();
                        disp->setBrewParams(waterRatio, coffeeDose, targetWater);
                        disp->setTargetReached(targetReached);
                    } else if (inputTarget == 'd') {
                        coffeeDose = val;
                        targetWater = waterRatio * coffeeDose;
                        targetReached = false;
                        Serial.print(F("Target water: "));
                        Serial.println(targetWater);
                        saveSettings();
                        disp->setBrewParams(waterRatio, coffeeDose, targetWater);
                        disp->setTargetReached(targetReached);
                    } else if (inputTarget == 'c') {
                        if (weightSensor.calibrateWithKnownWeight(val)) {
                            displayModule.setCalibrationFactor(weightSensor.getCalibrationFactor());
                            if (storageModule.isSDReady()) {
                                storageModule.saveCalibration(weightSensor.getCalibrationFactor());
                            }
                        }
                    }
                }
                inputMode = false;
                resetInput();
                syncSettingsInputState();
                return;
            }
            if (state.del) {
                if (inputLength > 0) {
                    inputLength--;
                    inputBuffer[inputLength] = '\0';
                } else {
                    inputMode = false;
                    resetInput();
                }
                syncSettingsInputState();
                return;
            }
            syncSettingsInputState();
        }
        return;
    }

    // === 主界面 ===
    if (page == DisplayModule::PAGE_MAIN) {
        for (auto ch : state.word) {
            if (ch == '1') {
                syncSettingsInputState();
                DisplayModule::Page nextPage = static_cast<DisplayModule::Page>((page + 1) % 4);
                disp->setPage(nextPage);
                return;
            }
            if (ch == '2') {
                weightSensor.tare();
                flowCalculator.reset();
                currentWeight = 0;
                currentFlowRate = 0;
                targetReached = false;
                sensorErrorCount = 0;
                displayModule.markMainPageDirty();
                Serial.println(F("Manual tare"));
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
    displayModule.setCalibrationFactor(weightSensor.getCalibrationFactor());
    Serial.print(F("Calibration factor: "));
    Serial.println(calFactor);
}

void resetInput() {
    inputLength = 0;
    inputBuffer[0] = '\0';
}

bool appendInputChar(char ch) {
    if (inputLength >= sizeof(inputBuffer) - 1) return false;

    if (ch == '.') {
        for (uint8_t i = 0; i < inputLength; i++) {
            if (inputBuffer[i] == '.') return false;
        }
    }

    inputBuffer[inputLength++] = ch;
    inputBuffer[inputLength] = '\0';
    return true;
}

bool parseInputFloat(float& value) {
    if (inputLength == 0) return false;

    char* endPtr = nullptr;
    value = strtof(inputBuffer, &endPtr);
    return endPtr != inputBuffer && *endPtr == '\0';
}

void syncSettingsInputState() {
    bool showEditor = inputMode && inputTarget != 'c';
    displayModule.setSettingsInputState(settingsSelectedIndex, showEditor, showEditor ? inputBuffer : "");
}

// ========== 目标水量到达声音提示 ==========
void playTargetReachedSound() {
    M5Cardputer.Speaker.tone(880, 200);
    delay(250);
    M5Cardputer.Speaker.tone(1320, 200);
    delay(250);
    M5Cardputer.Speaker.tone(1760, 400);
    Serial.println(F("Target water reached!"));
}

// ========== 系统停止 ==========
void haltSystem() {
    Serial.println(F("System halted"));
    while (1) {
        M5Cardputer.update();
        yield();
    }
}
