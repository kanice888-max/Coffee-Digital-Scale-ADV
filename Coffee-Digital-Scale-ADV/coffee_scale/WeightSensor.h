#ifndef WEIGHT_SENSOR_H
#define WEIGHT_SENSOR_H

#include <HX711_ADC.h>
#include "config.h"

class WeightSensor {
public:
    WeightSensor(uint8_t doutPin, uint8_t sckPin);

    // 初始化传感器（返回是否成功）
    bool init();

    // 去皮
    void tare();

    // 获取滤波后的重量 (g)
    float getWeight();

    // 校准
    void setCalibrationFactor(float factor);
    float getCalibrationFactor();

    // 状态
    bool isReady();

private:
    HX711_ADC _scale;
    float _calibrationFactor;
    bool _ready;
    bool _initialized;

    // 移动平均滤波
    float _filterBuffer[FILTER_WINDOW_SIZE];
    int _filterIndex;
    float _filterSum;
    bool _filterFull;
    float _applyFilter(float rawValue);

    // 错误处理
    float _getLastErrorWeight();
};

#endif
