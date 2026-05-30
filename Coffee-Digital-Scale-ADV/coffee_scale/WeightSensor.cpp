#include "WeightSensor.h"

WeightSensor::WeightSensor(uint8_t doutPin, uint8_t sckPin)
    : _scale(doutPin, sckPin) {
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

bool WeightSensor::init() {
    _scale.begin();

    // 等待 HX711 就绪（带超时）
    unsigned long startTime = millis();
    while (!_scale.update()) {
        if (millis() - startTime > SENSOR_TIMEOUT_MS) {
            Serial.println(F("ERROR: HX711 init timeout!"));
            _initialized = false;
            return false;
        }
        delay(10);
    }

    _scale.setCalFactor(_calibrationFactor);

    _initialized = true;
    _ready = true;

    Serial.println(F("HX711 initialized"));
    return true;
}

void WeightSensor::tare() {
    if (!_initialized) {
        Serial.println(F("WARNING: Cannot tare - sensor not initialized"));
        return;
    }

    _ready = false;

    // 清空滤波缓冲区
    _filterIndex = 0;
    _filterSum = 0;
    _filterFull = false;
    for (int i = 0; i < FILTER_WINDOW_SIZE; i++) {
        _filterBuffer[i] = 0;
    }

    // 执行 tare
    _scale.tare();

    _ready = true;
    Serial.println(F("Tare complete"));
}

float WeightSensor::getWeight() {
#if DEMO_MODE
    if (!_initialized) {
        // ========== 模拟注水曲线（无需 HX711 即可测试 UI） ==========
        unsigned long t = millis() / 1000;  // 运行秒数

        if (t < 5) {
            return 0.0f;                       // 0-5s: 空杯
        } else if (t < 20) {
            return (t - 5) * 250.0f / 15.0f;   // 5-20s: 0→250g 注水
        } else if (t < 25) {
            return 250.0f;                      // 20-25s: 闷蒸
        } else if (t < 35) {
            return 250.0f - (t - 25) * 30.0f;   // 25-35s: 下降 250→-50
        } else {
            return 200.0f;                      // 35s+: 稳定
        }
    }
#endif

    if (!_initialized || !_ready) return _getLastErrorWeight();

    // 更新 HX711 数据
    _scale.update();

    // 获取原始重量
    float rawWeight = _scale.getData();

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
