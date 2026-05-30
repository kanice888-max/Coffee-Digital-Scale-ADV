#ifndef STORAGE_MODULE_H
#define STORAGE_MODULE_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "config.h"

class StorageModule {
public:
    StorageModule();

    // 初始化
    bool init();

    // 数据记录
    bool beginNewSession();
    bool logDataPoint(float weight, float flowRate, unsigned long elapsedMs);
    void endSession();

    // 配置管理
    bool saveCalibration(float factor);
    float loadCalibration(float defaultFactor);
    bool saveSettings(float autoStartThreshold, float resetThreshold);
    bool loadSettings(float& autoStartThreshold, float& resetThreshold);

    // 状态
    bool isSDReady();
    String getCurrentFileName();
    int getSessionDataCount();

private:
    bool _sdReady;
    bool _sessionActive;
    File _dataFile;
    String _currentFileName;
    int _dataPointCount;
    int _flushCounter;

    String _generateFileName();
    bool _ensureDirectory(const char* path);
};

#endif
