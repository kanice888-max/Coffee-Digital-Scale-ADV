#ifndef FLOW_CALCULATOR_H
#define FLOW_CALCULATOR_H

#include <Arduino.h>
#include "config.h"

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

    // 获取时间范围（用于曲线缩放）
    unsigned long getTimeMin();
    unsigned long getTimeMax();
    float getWeightMin();
    float getWeightMax();
    float getFlowMin();
    float getFlowMax();

    // 获取环形缓冲区按时间顺序的索引（用于曲线绘制）
    int getChronologicalIndex(int logicalIndex);

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
    float _currentFlowRate;

    // 统计值
    float _weightMin;
    float _weightMax;
    float _flowMin;
    float _flowMax;
    unsigned long _timeMin;
    unsigned long _timeMax;

    void _updateStatistics();
};

#endif
