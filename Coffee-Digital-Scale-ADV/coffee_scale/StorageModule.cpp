#include "StorageModule.h"

StorageModule::StorageModule() {
    _sdReady = false;
    _sessionActive = false;
    _dataPointCount = 0;
    _flushCounter = 0;
}

bool StorageModule::init() {
    Serial.println(F("Initializing SD card..."));

    // 初始化 SPI（使用 Cardputer 的 SD 卡 SPI 引脚）
    SPI.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

    // 尝试初始化 SD 卡
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println(F("SD card initialization failed!"));
        _sdReady = false;
        return false;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println(F("No SD card attached"));
        _sdReady = false;
        return false;
    }

    Serial.print(F("SD Card Type: "));
    if (cardType == CARD_MMC) {
        Serial.println(F("MMC"));
    } else if (cardType == CARD_SD) {
        Serial.println(F("SDSC"));
    } else if (cardType == CARD_SDHC) {
        Serial.println(F("SDHC"));
    } else {
        Serial.println(F("UNKNOWN"));
    }

    Serial.printf("SD Card Size: %luMB\n", (unsigned long)(SD.cardSize() / (1024 * 1024)));

    // 确保目录存在
    _ensureDirectory(DATA_LOG_DIR);
    _ensureDirectory(CONFIG_DIR);

    _sdReady = true;
    Serial.println(F("SD card initialized"));
    return true;
}

bool StorageModule::beginNewSession() {
    if (!_sdReady) return false;

    // 结束之前的会话
    endSession();

    _currentFileName = _generateFileName();
    Serial.print(F("Creating log file: "));
    Serial.println(_currentFileName);

    _dataFile = SD.open(_currentFileName, FILE_WRITE);
    if (!_dataFile) {
        Serial.println(F("Failed to create log file"));
        return false;
    }

    // 写入 CSV 头
    _dataFile.println(F("timestamp_ms,weight_g,flow_rate_g_per_s,elapsed_s"));
    _dataFile.flush();

    _sessionActive = true;
    _dataPointCount = 0;
    _flushCounter = 0;

    Serial.println(F("New session started"));
    return true;
}

bool StorageModule::logDataPoint(float weight, float flowRate, unsigned long elapsedMs) {
    if (!_sessionActive || !_dataFile) return false;

    // 使用 snprintf 防止缓冲区溢出
    char line[64];
    int len = snprintf(line, sizeof(line), "%lu,%.1f,%.1f,%.2f",
                       millis(), weight, flowRate, elapsedMs / 1000.0f);

    if (len > 0 && len < sizeof(line)) {
        _dataFile.println(line);
        _dataPointCount++;

        // 批量 flush，平衡性能和数据安全
        _flushCounter++;
        if (_flushCounter >= SD_FLUSH_INTERVAL) {
            _dataFile.flush();
            _flushCounter = 0;
        }
        return true;
    }

    return false;
}

void StorageModule::endSession() {
    if (_sessionActive && _dataFile) {
        _dataFile.flush();
        _dataFile.close();
        _sessionActive = false;

        Serial.print(F("Session ended. Data points: "));
        Serial.println(_dataPointCount);
    }
}

bool StorageModule::saveCalibration(float factor) {
    if (!_sdReady) return false;

    // 验证校准因子
    if (factor <= 0 || factor > 10000) {
        Serial.println(F("ERROR: Invalid calibration factor"));
        return false;
    }

    // 删除旧文件
    if (SD.exists(CALIBRATION_FILE)) {
        SD.remove(CALIBRATION_FILE);
    }

    File file = SD.open(CALIBRATION_FILE, FILE_WRITE);
    if (!file) {
        Serial.println(F("Failed to save calibration"));
        return false;
    }

    // 写入 JSON 格式
    file.print(F("{\"calibration_factor\":"));
    file.print(factor, 2);
    file.println(F("}"));
    file.close();

    Serial.print(F("Calibration saved: "));
    Serial.println(factor);
    return true;
}

float StorageModule::loadCalibration(float defaultFactor) {
    if (!_sdReady) return defaultFactor;

    File file = SD.open(CALIBRATION_FILE, FILE_READ);
    if (!file) {
        Serial.println(F("No calibration file found, using default"));
        return defaultFactor;
    }

    String content = file.readString();
    file.close();

    // 简单解析 JSON
    int startIndex = content.indexOf(F("calibration_factor\":")) + 20;
    int endIndex = content.indexOf(F("}"), startIndex);

    if (startIndex > 20 && endIndex > startIndex) {
        float factor = content.substring(startIndex, endIndex).toFloat();
        if (factor > 0 && factor < 10000) {
            Serial.print(F("Calibration loaded: "));
            Serial.println(factor);
            return factor;
        }
    }

    Serial.println(F("Failed to parse calibration, using default"));
    return defaultFactor;
}

bool StorageModule::saveSettings(float autoStartThreshold, float resetThreshold,
                                 float ratio, float dose) {
    if (!_sdReady) return false;

    // 验证阈值范围
    if (autoStartThreshold < 0 || autoStartThreshold > 100 ||
        resetThreshold < 0 || resetThreshold > 100) {
        Serial.println(F("ERROR: Invalid threshold values"));
        return false;
    }

    // 删除旧文件
    if (SD.exists(SETTINGS_FILE)) {
        SD.remove(SETTINGS_FILE);
    }

    File file = SD.open(SETTINGS_FILE, FILE_WRITE);
    if (!file) {
        Serial.println(F("Failed to save settings"));
        return false;
    }

    // 写入 JSON 格式
    file.print(F("{\"auto_start_threshold\":"));
    file.print(autoStartThreshold, 2);
    file.print(F(",\"reset_threshold\":"));
    file.print(resetThreshold, 2);
    file.print(F(",\"ratio\":"));
    file.print(ratio, 2);
    file.print(F(",\"dose\":"));
    file.print(dose, 2);
    file.println(F("}"));
    file.close();

    Serial.println(F("Settings saved"));
    return true;
}

bool StorageModule::loadSettings(float& autoStartThreshold, float& resetThreshold,
                                 float& ratio, float& dose) {
    if (!_sdReady) return false;

    File file = SD.open(SETTINGS_FILE, FILE_READ);
    if (!file) {
        Serial.println(F("No settings file found, using defaults"));
        return false;
    }

    String content = file.readString();
    file.close();

    // 简单解析 JSON
    int startIdx = content.indexOf(F("auto_start_threshold\":")) + 22;
    int endIdx = content.indexOf(F(","), startIdx);

    if (startIdx > 22 && endIdx > startIdx) {
        float value = content.substring(startIdx, endIdx).toFloat();
        if (value >= 0 && value <= 100) {
            autoStartThreshold = value;
        }
    }

    startIdx = content.indexOf(F("reset_threshold\":")) + 17;
    endIdx = content.indexOf(F("}"), startIdx);

    if (startIdx > 17 && endIdx > startIdx) {
        float value = content.substring(startIdx, endIdx).toFloat();
        if (value >= 0 && value <= 100) {
            resetThreshold = value;
        }
    }

    // 只有成功解析到至少一个值才返回 true
    if (autoStartThreshold != 0 || resetThreshold != 0) {
        Serial.print(F("Settings loaded: auto_start="));
        Serial.print(autoStartThreshold);
        Serial.print(F(", reset="));
        Serial.println(resetThreshold);
        return true;
    }

    return false;
}

bool StorageModule::isSDReady() {
    return _sdReady;
}

String StorageModule::getCurrentFileName() {
    return _currentFileName;
}

int StorageModule::getSessionDataCount() {
    return _dataPointCount;
}

String StorageModule::_generateFileName() {
    // 使用 millis() 作为文件名的一部分
    char filename[40];
    snprintf(filename, sizeof(filename), "%s/brew_%lu.csv", DATA_LOG_DIR, millis());
    return String(filename);
}

bool StorageModule::_ensureDirectory(const char* path) {
    if (!SD.exists(path)) {
        return SD.mkdir(path);
    }
    return true;
}

