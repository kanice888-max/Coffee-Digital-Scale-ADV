#include "StorageModule.h"

StorageModule::StorageModule() {
    _sdReady = false;
    _sessionActive = false;
    _dataPointCount = 0;
    _flushCounter = 0;
}

bool StorageModule::init() {
    Serial.println("Initializing SD card...");

    // 初始化 SPI
    SPI.begin(SCK, MISO, MOSI, SD_CS_PIN);

    // 尝试初始化 SD 卡
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("SD card initialization failed!");
        _sdReady = false;
        return false;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("No SD card attached");
        _sdReady = false;
        return false;
    }

    Serial.print("SD Card Type: ");
    if (cardType == CARD_MMC) {
        Serial.println("MMC");
    } else if (cardType == CARD_SD) {
        Serial.println("SDSC");
    } else if (cardType == CARD_SDHC) {
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    Serial.printf("SD Card Size: %lluMB\n", SD.cardSize() / (1024 * 1024));

    // 确保目录存在
    _ensureDirectory(DATA_LOG_DIR);
    _ensureDirectory(CONFIG_DIR);

    _sdReady = true;
    Serial.println("SD card initialized");
    return true;
}

bool StorageModule::beginNewSession() {
    if (!_sdReady) return false;

    // 结束之前的会话
    endSession();

    _currentFileName = _generateFileName();
    Serial.print("Creating log file: ");
    Serial.println(_currentFileName);

    _dataFile = SD.open(_currentFileName, FILE_WRITE);
    if (!_dataFile) {
        Serial.println("Failed to create log file");
        return false;
    }

    // 写入 CSV 头
    _dataFile.println("timestamp_ms,weight_g,flow_rate_g_per_s,elapsed_s");
    _dataFile.flush();

    _sessionActive = true;
    _dataPointCount = 0;
    _flushCounter = 0;

    Serial.println("New session started");
    return true;
}

bool StorageModule::logDataPoint(float weight, float flowRate, unsigned long elapsedMs) {
    if (!_sessionActive || !_dataFile) return false;

    char line[60];
    sprintf(line, "%lu,%.1f,%.1f,%.2f",
            millis(), weight, flowRate, elapsedMs / 1000.0f);

    _dataFile.println(line);
    _dataPointCount++;

    // 批量 flush，平衡性能和数据安全
    _flushCounter++;
    if (_flushCounter >= 10) {
        _dataFile.flush();
        _flushCounter = 0;
    }

    return true;
}

void StorageModule::endSession() {
    if (_sessionActive && _dataFile) {
        _dataFile.flush();
        _dataFile.close();
        _sessionActive = false;

        Serial.print("Session ended. Data points: ");
        Serial.println(_dataPointCount);
    }
}

bool StorageModule::saveCalibration(float factor) {
    if (!_sdReady) return false;

    // 删除旧文件
    if (SD.exists(CALIBRATION_FILE)) {
        SD.remove(CALIBRATION_FILE);
    }

    File file = SD.open(CALIBRATION_FILE, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to save calibration");
        return false;
    }

    // 写入 JSON 格式
    file.print("{\"calibration_factor\":");
    file.print(factor, 2);
    file.println("}");
    file.close();

    Serial.print("Calibration saved: ");
    Serial.println(factor);
    return true;
}

float StorageModule::loadCalibration(float defaultFactor) {
    if (!_sdReady) return defaultFactor;

    File file = SD.open(CALIBRATION_FILE, FILE_READ);
    if (!file) {
        Serial.println("No calibration file found, using default");
        return defaultFactor;
    }

    String content = file.readString();
    file.close();

    // 简单解析 JSON
    int startIndex = content.indexOf("calibration_factor\":") + 20;
    int endIndex = content.indexOf("}", startIndex);

    if (startIndex > 20 && endIndex > startIndex) {
        float factor = content.substring(startIndex, endIndex).toFloat();
        if (factor > 0) {
            Serial.print("Calibration loaded: ");
            Serial.println(factor);
            return factor;
        }
    }

    Serial.println("Failed to parse calibration, using default");
    return defaultFactor;
}

bool StorageModule::saveSettings(float autoStartThreshold, float resetThreshold) {
    if (!_sdReady) return false;

    // 删除旧文件
    if (SD.exists(SETTINGS_FILE)) {
        SD.remove(SETTINGS_FILE);
    }

    File file = SD.open(SETTINGS_FILE, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to save settings");
        return false;
    }

    // 写入 JSON 格式
    file.print("{\"auto_start_threshold\":");
    file.print(autoStartThreshold, 2);
    file.print(",\"reset_threshold\":");
    file.print(resetThreshold, 2);
    file.println("}");
    file.close();

    Serial.println("Settings saved");
    return true;
}

bool StorageModule::loadSettings(float& autoStartThreshold, float& resetThreshold) {
    if (!_sdReady) return false;

    File file = SD.open(SETTINGS_FILE, FILE_READ);
    if (!file) {
        Serial.println("No settings file found, using defaults");
        return false;
    }

    String content = file.readString();
    file.close();

    // 简单解析 JSON
    int startIdx = content.indexOf("auto_start_threshold\":") + 22;
    int endIdx = content.indexOf(",", startIdx);

    if (startIdx > 22 && endIdx > startIdx) {
        autoStartThreshold = content.substring(startIdx, endIdx).toFloat();
    }

    startIdx = content.indexOf("reset_threshold\":") + 17;
    endIdx = content.indexOf("}", startIdx);

    if (startIdx > 17 && endIdx > startIdx) {
        resetThreshold = content.substring(startIdx, endIdx).toFloat();
    }

    Serial.print("Settings loaded: auto_start=");
    Serial.print(autoStartThreshold);
    Serial.print(", reset=");
    Serial.println(resetThreshold);

    return true;
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
    sprintf(filename, "%s/brew_%lu.csv", DATA_LOG_DIR, millis());
    return String(filename);
}

bool StorageModule::_ensureDirectory(const char* path) {
    if (!SD.exists(path)) {
        return SD.mkdir(path);
    }
    return true;
}

String StorageModule::_getTimestamp() {
    // 简单的时间戳（使用 millis）
    char timestamp[20];
    sprintf(timestamp, "%lu", millis());
    return String(timestamp);
}
