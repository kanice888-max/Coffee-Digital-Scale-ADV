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

bool WeightSensor::init(uint8_t doutPin, uint8_t sckPin) {
    // 验证引脚有效性
    if (doutPin == sckPin) {
        Serial.println(F("ERROR: DOUT and SCK pins cannot be the same"));
        return false;
    }

    _scale.begin(doutPin, sckPin);

    // 等待 HX711 就绪（带超时）
    unsigned long startTime = millis();
    while (!_scale.is_ready()) {
        if (millis() - startTime > SENSOR_TIMEOUT_MS) {
            Serial.println(F("ERROR: HX711 init timeout!"));
            _initialized = false;
            return false;
        }
        yield();  // 允许系统处理其他任务
    }

    // 启动并稳定
    if (!_scale.start(2000)) {
        Serial.println(F("ERROR: HX711 start failed"));
        _initialized = false;
        return false;
    }

    _scale.setCalFactor(_calibrationFactor);

    _initialized = true;
    _ready = true;

    Serial.println(F("HX711 initialized"));
    return true;
}

void WeightSensor::tare(int samples) {
    if (!_initialized) {
        Serial.println(F("WARNING: Cannot tare - sensor not initialized"));
        return;
    }

    // 验证采样数
    if (samples < 1) samples = 1;
    if (samples > 100) samples = 100;

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
    Serial.println(F("Tare complete"));
}

float WeightSensor::getWeight() {
    if (!_initialized || !_ready) return 0;

    // 更新 HX711 数据
    if (!_scale.update()) {
        return _getLastErrorWeight();
    }

    // 获取原始重量
    float rawWeight = _scale.getUnits();

    // 应用滤波
    return _applyFilter(rawWeight);
}

void WeightSensor::setCalibrationFactor(float factor) {
    // 验证校准因子
    if (factor <= 0 || factor > 10000) {
        Serial.println(F("WARNING: Invalid calibration factor"));
        return;
    }

    _calibrationFactor = factor;
    if (_initialized) {
        _scale.setCalFactor(_calibrationFactor);
    }
}

float WeightSensor::getCalibrationFactor() {
    return _calibrationFactor;
}

bool WeightSensor::calibrateWithKnownWeight(float knownWeight, int samples) {
    if (!_initialized) {
        Serial.println(F("ERROR: Sensor not initialized"));
        return false;
    }

    // 验证已知重量
    if (knownWeight <= 0 || knownWeight > 5000) {
        Serial.println(F("ERROR: Invalid known weight"));
        return false;
    }

    // 验证采样数
    if (samples < 1) samples = 1;
    if (samples > 100) samples = 100;

    Serial.println(F("Calibrating..."));
    Serial.print(F("Place "));
    Serial.print(knownWeight);
    Serial.println(F("g on the scale"));

    // 等待稳定
    delay(2000);

    // 获取原始值
    _scale.update();
    float rawValue = _scale.getData();

    // 计算校准因子
    if (rawValue == 0) {
        Serial.println(F("ERROR: Raw value is zero"));
        return false;
    }

    _calibrationFactor = rawValue / knownWeight;

    // 验证校准因子合理性
    if (_calibrationFactor < 1 || _calibrationFactor > 10000) {
        Serial.println(F("ERROR: Calculated calibration factor out of range"));
        return false;
    }

    _scale.setCalFactor(_calibrationFactor);

    Serial.print(F("New calibration factor: "));
    Serial.println(_calibrationFactor);
    return true;
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

float WeightSensor::_getLastErrorWeight() {
    // 返回最后一次有效的重量值
    // 如果缓冲区有数据，返回平均值
    if (_filterFull || _filterIndex > 0) {
        int count = _filterFull ? FILTER_WINDOW_SIZE : _filterIndex;
        return _filterSum / count;
    }
    return 0;
}
