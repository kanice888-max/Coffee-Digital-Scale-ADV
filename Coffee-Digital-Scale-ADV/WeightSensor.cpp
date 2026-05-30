#include "WeightSensor.h"

WeightSensor::WeightSensor() {
    _calibrationFactor = DEFAULT_CALIBRATION_FACTOR;
    _ready = false;
    _initialized = false;
    _filterIndex = 0;
    _filterSum = 0;
    _filterFull = false;

    // 初始化滤波缓冲区
    for (int i = 0; i < FILTER_WINDOW_SIZE; i++) {
        _filterBuffer[i] = 0;
    }
}

void WeightSensor::init(uint8_t doutPin, uint8_t sckPin) {
    _scale.begin(doutPin, sckPin);

    // 等待 HX711 就绪
    unsigned long startTime = millis();
    while (!_scale.is_ready()) {
        if (millis() - startTime > 2000) {
            // 超时 2 秒
            Serial.println("HX711 init timeout!");
            return;
        }
        delay(10);
    }

    // 启动并稳定
    _scale.start(2000);
    _scale.setCalFactor(_calibrationFactor);

    _initialized = true;
    _ready = true;

    Serial.println("HX711 initialized");
}

void WeightSensor::tare(int samples) {
    if (!_initialized) return;

    _ready = false;

    // 清空滤波缓冲区
    _filterIndex = 0;
    _filterSum = 0;
    _filterFull = false;
    for (int i = 0; i < FILTER_WINDOW_SIZE; i++) {
        _filterBuffer[i] = 0;
    }

    // 执行 tare
    _scale.tare(samples);

    _ready = true;
    Serial.println("Tare complete");
}

float WeightSensor::getWeight() {
    if (!_initialized || !_ready) return 0;

    // 更新 HX711 数据
    _scale.update();

    // 获取原始重量
    float rawWeight = _scale.getUnits();

    // 应用滤波
    return _applyFilter(rawWeight);
}

void WeightSensor::setCalibrationFactor(float factor) {
    _calibrationFactor = factor;
    if (_initialized) {
        _scale.setCalFactor(_calibrationFactor);
    }
}

float WeightSensor::getCalibrationFactor() {
    return _calibrationFactor;
}

void WeightSensor::calibrateWithKnownWeight(float knownWeight, int samples) {
    if (!_initialized) return;

    Serial.println("Calibrating...");
    Serial.print("Place ");
    Serial.print(knownWeight);
    Serial.println("g on the scale");

    // 等待稳定
    delay(2000);

    // 获取原始值
    _scale.update();
    float rawValue = _scale.getData();

    // 计算校准因子
    if (rawValue != 0) {
        _calibrationFactor = rawValue / knownWeight;
        _scale.setCalFactor(_calibrationFactor);

        Serial.print("New calibration factor: ");
        Serial.println(_calibrationFactor);
    }
}

bool WeightSensor::isReady() {
    return _initialized && _ready;
}

float WeightSensor::_applyFilter(float rawValue) {
    // 从缓冲区减去旧值
    _filterSum -= _filterBuffer[_filterIndex];

    // 存入新值
    _filterBuffer[_filterIndex] = rawValue;
    _filterSum += rawValue;

    // 更新索引
    _filterIndex = (_filterIndex + 1) % FILTER_WINDOW_SIZE;

    // 检查缓冲区是否已满
    if (_filterIndex == 0 && !_filterFull) {
        _filterFull = true;
    }

    // 返回平均值
    if (_filterFull) {
        return _filterSum / FILTER_WINDOW_SIZE;
    } else {
        // 缓冲区未满时，使用已有数据的平均值
        return _filterSum / (_filterIndex > 0 ? _filterIndex : 1);
    }
}
