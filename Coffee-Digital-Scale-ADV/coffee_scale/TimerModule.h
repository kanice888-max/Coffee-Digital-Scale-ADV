#ifndef TIMER_MODULE_H
#define TIMER_MODULE_H

#include <Arduino.h>
#include "config.h"

class TimerModule {
public:
    TimerModule();

    // 手动控制
    void start();
    void stop();
    void reset();

    // 自动控制（基于重量）
    void checkAutoStart(float weight);

    // 获取状态
    bool isRunning();
    unsigned long getElapsedMs();
    void formatTime(char* buffer, size_t bufferSize);  // 写入 "MM:SS.s" 格式

    // 配置
    void setAutoStartThreshold(float threshold);
    void setResetThreshold(float threshold);
    float getAutoStartThreshold();
    float getResetThreshold();

private:
    bool _running;
    bool _autoStarted;
    unsigned long _startTime;
    unsigned long _elapsed;

    // 阈值
    float _autoStartThreshold;
    float _resetThreshold;

    // 自动重置检测
    bool _weightDetected;
    unsigned long _weightLostTime;
    static const unsigned long RESET_DELAY = 2000;  // 重量归零后延迟重置 (ms)
};

#endif
