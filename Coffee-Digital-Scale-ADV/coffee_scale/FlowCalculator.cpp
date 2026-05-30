#include "FlowCalculator.h"

FlowCalculator::FlowCalculator() {
    _bufferIndex = 0;
    _bufferCount = 0;
    _lastWeight = 0;
    _lastTimestamp = 0;
    _currentFlowRate = 0;

    _flowWindowIndex = 0;
    _flowWindowSum = 0;
    _flowWindowFull = false;

    _weightMin = 0;
    _weightMax = 0;
    _flowMin = 0;
    _flowMax = 0;
    _timeMin = 0;
    _timeMax = 0;

    // 初始化缓冲区
    for (int i = 0; i < HISTORY_BUFFER_SIZE; i++) {
        _weightBuffer[i] = 0;
        _flowBuffer[i] = 0;
        _timeBuffer[i] = 0;
    }
    for (int i = 0; i < FLOW_WINDOW_SIZE; i++) {
        _flowWindow[i] = 0;
    }
}

void FlowCalculator::update(float weight, unsigned long timestamp) {
    // 计算瞬时流量
    if (_lastTimestamp > 0) {
        unsigned long dt = timestamp - _lastTimestamp;
        if (dt > 0 && dt < 10000) {  // 避免除以极小值和异常大的时间差
            float instantFlow = (weight - _lastWeight) * 1000.0f / dt;  // g/s

            // 滑动窗口平均
            _flowWindowSum -= _flowWindow[_flowWindowIndex];
            _flowWindow[_flowWindowIndex] = instantFlow;
            _flowWindowSum += instantFlow;
            _flowWindowIndex = (_flowWindowIndex + 1) % FLOW_WINDOW_SIZE;

            if (!_flowWindowFull && _flowWindowIndex == 0) {
                _flowWindowFull = true;
            }

            if (_flowWindowFull) {
                _currentFlowRate = _flowWindowSum / FLOW_WINDOW_SIZE;
            } else {
                _currentFlowRate = _flowWindowSum / (_flowWindowIndex > 0 ? _flowWindowIndex : 1);
            }
        }
    }

    _lastWeight = weight;
    _lastTimestamp = timestamp;

    // 存入环形缓冲区
    _weightBuffer[_bufferIndex] = weight;
    _flowBuffer[_bufferIndex] = _currentFlowRate;
    _timeBuffer[_bufferIndex] = timestamp;
    _bufferIndex = (_bufferIndex + 1) % HISTORY_BUFFER_SIZE;
    if (_bufferCount < HISTORY_BUFFER_SIZE) {
        _bufferCount++;
    }

    // 更新统计值（只在有数据时更新）
    if (_bufferCount > 0) {
        _updateStatistics();
    }
}

float FlowCalculator::getFlowRate() {
    return _currentFlowRate;
}

float* FlowCalculator::getWeightHistory() {
    return _weightBuffer;
}

float* FlowCalculator::getFlowHistory() {
    return _flowBuffer;
}

unsigned long* FlowCalculator::getTimeHistory() {
    return _timeBuffer;
}

int FlowCalculator::getHistoryCount() {
    return _bufferCount;
}

unsigned long FlowCalculator::getTimeMin() {
    return _timeMin;
}

unsigned long FlowCalculator::getTimeMax() {
    return _timeMax;
}

float FlowCalculator::getWeightMin() {
    return _weightMin;
}

float FlowCalculator::getWeightMax() {
    return _weightMax;
}

float FlowCalculator::getFlowMin() {
    return _flowMin;
}

float FlowCalculator::getFlowMax() {
    return _flowMax;
}

void FlowCalculator::reset() {
    _bufferIndex = 0;
    _bufferCount = 0;
    _lastWeight = 0;
    _lastTimestamp = 0;
    _currentFlowRate = 0;

    _flowWindowIndex = 0;
    _flowWindowSum = 0;
    _flowWindowFull = false;

    _weightMin = 0;
    _weightMax = 0;
    _flowMin = 0;
    _flowMax = 0;
    _timeMin = 0;
    _timeMax = 0;

    for (int i = 0; i < HISTORY_BUFFER_SIZE; i++) {
        _weightBuffer[i] = 0;
        _flowBuffer[i] = 0;
        _timeBuffer[i] = 0;
    }
    for (int i = 0; i < FLOW_WINDOW_SIZE; i++) {
        _flowWindow[i] = 0;
    }
}

void FlowCalculator::_updateStatistics() {
    if (_bufferCount == 0) return;

    // 获取刚插入的最新数据点（环形缓冲区的前一个位置）
    int latestIdx = (_bufferIndex == 0) ? HISTORY_BUFFER_SIZE - 1 : _bufferIndex - 1;
    float w = _weightBuffer[latestIdx];
    float f = _flowBuffer[latestIdx];
    unsigned long t = _timeBuffer[latestIdx];

    // 如果是第一个数据点，初始化为它
    if (_bufferCount == 1) {
        _weightMin = _weightMax = w;
        _flowMin = _flowMax = f;
        _timeMin = _timeMax = t;
        return;
    }

    // 增量更新：只与最新点比较
    if (w < _weightMin) _weightMin = w;
    if (w > _weightMax) _weightMax = w;
    if (f < _flowMin) _flowMin = f;
    if (f > _flowMax) _flowMax = f;
    if (t < _timeMin) _timeMin = t;
    if (t > _timeMax) _timeMax = t;
}
