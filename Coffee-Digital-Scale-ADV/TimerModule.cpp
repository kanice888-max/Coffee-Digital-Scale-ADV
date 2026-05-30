#include "TimerModule.h"

TimerModule::TimerModule() {
    _running = false;
    _autoStarted = false;
    _startTime = 0;
    _elapsed = 0;
    _pausedTime = 0;

    _autoStartThreshold = AUTO_START_THRESHOLD;
    _resetThreshold = RESET_THRESHOLD;

    _weightDetected = false;
    _weightLostTime = 0;
}

void TimerModule::start() {
    if (!_running) {
        _startTime = millis() - _elapsed;
        _running = true;
    }
}

void TimerModule::stop() {
    if (_running) {
        _elapsed = millis() - _startTime;
        _running = false;
    }
}

void TimerModule::reset() {
    _running = false;
    _autoStarted = false;
    _startTime = 0;
    _elapsed = 0;
    _pausedTime = 0;
    _weightDetected = false;
    _weightLostTime = 0;
}

void TimerModule::checkAutoStart(float weight) {
    unsigned long now = millis();

    // 自动启动：重量超过阈值
    if (!_running && !_autoStarted && weight > _autoStartThreshold) {
        start();
        _autoStarted = true;
        _weightDetected = true;
        _weightLostTime = 0;
        Serial.println("Timer auto-started");
    }

    // 自动重置：重量归零
    if (_autoStarted) {
        if (weight < _resetThreshold) {
            if (_weightDetected) {
                // 刚检测到重量归零，开始计时
                _weightDetected = false;
                _weightLostTime = now;
            } else if (_weightLostTime > 0 && (now - _weightLostTime) > RESET_DELAY_MS) {
                // 重量持续低于阈值超过延迟时间，重置计时器
                reset();
                Serial.println("Timer auto-reset");
            }
        } else {
            // 重量恢复，重置丢失计时
            _weightDetected = true;
            _weightLostTime = 0;
        }
    }
}

bool TimerModule::isRunning() {
    return _running;
}

unsigned long TimerModule::getElapsedMs() {
    if (_running) {
        return millis() - _startTime;
    }
    return _elapsed;
}

String TimerModule::getFormattedTime() {
    unsigned long elapsed = getElapsedMs();
    int minutes = (elapsed / 60000) % 60;
    int seconds = (elapsed / 1000) % 60;
    int tenths = (elapsed / 100) % 10;

    char buffer[10];
    sprintf(buffer, "%02d:%02d.%d", minutes, seconds, tenths);
    return String(buffer);
}

void TimerModule::setAutoStartThreshold(float threshold) {
    _autoStartThreshold = threshold;
}

void TimerModule::setResetThreshold(float threshold) {
    _resetThreshold = threshold;
}

float TimerModule::getAutoStartThreshold() {
    return _autoStartThreshold;
}

float TimerModule::getResetThreshold() {
    return _resetThreshold;
}
