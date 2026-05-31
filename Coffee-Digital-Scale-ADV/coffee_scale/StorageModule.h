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
    bool saveSettings(float autoStartThreshold, float resetThreshold,
                       float ratio, float dose);
    bool loadSettings(float& autoStartThreshold, float& resetThreshold,
                      float& ratio, float& dose);

    // 状态
    bool isSDReady();
    const char* getCurrentFileName();
    int getSessionDataCount();

private:
    bool _sdReady;
    bool _sessionActive;
    File _dataFile;
    char _currentFileName[64];
    int _dataPointCount;
    int _flushCounter;

    void _generateFileName(char* filename, size_t filenameSize);
    bool _ensureDirectory(const char* path);
};

#endif
