#include "DisplayModule.h"
#include <Fonts/FreeSerif18pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>

// FreeSerif 字体在 240x135 ST7789 上的实际指标（经验值）：
// FreeSerif18pt7b: ascent≈18px, descent≈5px, 总高≈23px, 基线到顶≈16px
// FreeSerif12pt7b: ascent≈12px, descent≈3px, 总高≈15px, 基线到顶≈11px
// FreeSerif9pt7b:  ascent≈9px,  descent≈2px, 总高≈11px, 基线到顶≈8px

DisplayModule::DisplayModule() {
    _currentPage = PAGE_MAIN;
    _lastUpdateTime = 0;
    _lastCurveUpdateTime = 0;
    _brewRatio = 15.0f;
    _brewDose = 15.0f;
    _brewTarget = 225.0f;
    _targetReached = false;
}

void DisplayModule::init() {
    M5.Lcd.setRotation(1);
    M5.Lcd.fillScreen(COLOR_BG);
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.setTextSize(1);
    _drawMainBackground();
}

void DisplayModule::setPage(Page page) {
    if (page != _currentPage) {
        _currentPage = page;
        M5.Lcd.fillScreen(COLOR_BG);
        switch (_currentPage) {
            case PAGE_MAIN:          _drawMainBackground(); break;
            case PAGE_WEIGHT_CURVE:  _drawCurveBackground(F("WEIGHT")); break;
            case PAGE_FLOW_CURVE:    _drawCurveBackground(F("FLOW")); break;
            case PAGE_SETTINGS:      _drawSettingsPage(); break;
        }
        _drawPageIndicator();
    }
}

DisplayModule::Page DisplayModule::getCurrentPage() { return _currentPage; }

void DisplayModule::update(float weight, float flowRate, TimerModule* timer, FlowCalculator* flowCalc) {
    unsigned long now = millis();
    handleInput();
    switch (_currentPage) {
        case PAGE_MAIN:
            if (now - _lastUpdateTime >= DISPLAY_UPDATE_INTERVAL) {
                _drawMainPage(weight, flowRate, timer, flowCalc);
                _lastUpdateTime = now;
            }
            break;
        case PAGE_WEIGHT_CURVE:
            if (now - _lastCurveUpdateTime >= CURVE_UPDATE_INTERVAL) {
                _drawWeightCurvePage(flowCalc);
                _lastCurveUpdateTime = now;
            }
            break;
        case PAGE_FLOW_CURVE:
            if (now - _lastCurveUpdateTime >= CURVE_UPDATE_INTERVAL) {
                _drawFlowCurvePage(flowCalc);
                _lastCurveUpdateTime = now;
            }
            break;
        case PAGE_SETTINGS: break;
    }
}

void DisplayModule::handleInput() {
    if (Serial.available() > 0) {
        char key = Serial.read();
        if (key == '1') {
            Page nextPage = (Page)((_currentPage + 1) % 4);
            setPage(nextPage);
        }
    }
}

void DisplayModule::showMessage(const String& message, int durationMs) {
    Page savedPage = _currentPage;
    M5.Lcd.fillScreen(COLOR_BG);
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.drawString(message, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    if (durationMs > 0) delay(durationMs);
    M5.Lcd.fillScreen(COLOR_BG);
    switch (savedPage) {
        case PAGE_MAIN:          _drawMainBackground(); break;
        case PAGE_WEIGHT_CURVE:  _drawCurveBackground(F("WEIGHT")); break;
        case PAGE_FLOW_CURVE:    _drawCurveBackground(F("FLOW")); break;
        case PAGE_SETTINGS:      _drawSettingsPage(); break;
    }
    _drawPageIndicator();
}

void DisplayModule::setBrewParams(float ratio, float dose, float target) {
    _brewRatio = ratio; _brewDose = dose; _brewTarget = target;
}
void DisplayModule::setTargetReached(bool reached) { _targetReached = reached; }

// ============================================================
// 主界面布局 240×135 (FreeSerif 字体指标实测版)
// 各层精确 Y 坐标（避免重叠）：
// [0-16]   标题栏
// [18-50]  重量数字 (FreeSerif18pt, 基线y:42, 文字顶部≈26)
// [56-59]  进度条 (4px 高)
// [64-70]  信息行 (FreeSerif12pt, 基线y:68, 文字顶部≈57)
// [74-82]  BREWING (FreeSerif9pt, 基线y:80, 文字顶部≈72)
// [84-135] 迷你曲线 (51px)
// ============================================================
void DisplayModule::_drawMainPage(float weight, float flowRate, TimerModule* timer, FlowCalculator* flowCalc) {
    _clearArea(0, 18, SCREEN_WIDTH, 117);

    // === 重量 FreeSerif18pt (基线 y:42, 顶≈26) ===
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setFreeFont(&FreeSerif18pt7b);
    char wStr[10];
    dtostrf(weight, 5, 1, wStr);
    M5.Lcd.drawString(wStr, SCREEN_WIDTH / 2, 42);

    // === 进度条 y:56-59 ===
    _drawProgressBar(10, 56, SCREEN_WIDTH - 20, 4, weight, _brewTarget);

    // === 信息行 FreeSerif12pt (基线 y:68, 顶≈57, 底≈71) ===
    char targetStr[16];
    if (_brewTarget > 0)
        snprintf(targetStr, sizeof(targetStr), "%.0f/%.0fg", weight, _brewTarget);
    else
        snprintf(targetStr, sizeof(targetStr), "%.1fg", weight);

    char buf[20];
    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    M5.Lcd.setTextSize(1);

    // flow 值（左）
    snprintf(buf, sizeof(buf), "flow %.1f", flowRate);
    M5.Lcd.setTextColor(COLOR_ACCENT, COLOR_BG);
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.drawString(buf, 8, 68);

    // g/s 小标签（flow 值右侧）
    M5.Lcd.setFreeFont(&FreeSerif9pt7b);
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    snprintf(buf, sizeof(buf), "%.1f", flowRate);
    int flowChars = strlen(buf) + 3;
    M5.Lcd.drawString("g/s", 8 + flowChars * 7, 71);

    // 注水量（居中）
    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    M5.Lcd.setTextColor(_targetReached ? COLOR_SUCCESS : COLOR_TEXT_DIM, COLOR_BG);
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.drawString(targetStr, SCREEN_WIDTH / 2, 68);

    // 时间（右）
    M5.Lcd.setTextColor(COLOR_ACCENT, COLOR_BG);
    M5.Lcd.setTextDatum(TR_DATUM);
    M5.Lcd.drawString(timer->getFormattedTime(), SCREEN_WIDTH - 8, 68);

    // === BREWING FreeSerif9pt (基线 y:80, 顶≈72, 底≈82) ===
    bool running = timer->isRunning();
    uint16_t ledColor = running ? COLOR_STATUS_ON : COLOR_STATUS_OFF;
    uint16_t statColor = running ? COLOR_STATUS_ON : COLOR_TEXT_DIM;
    M5.Lcd.fillCircle(8, 82, 3, ledColor);
    M5.Lcd.setFreeFont(&FreeSerif9pt7b);
    M5.Lcd.setTextColor(statColor, COLOR_BG);
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.drawString(running ? "BREWING" : "STANDBY", 16, 79);

    // === 迷你曲线 y:84-135 (51px) ===
    int count = flowCalc->getHistoryCount();
    if (count >= 2) {
        float* weights = flowCalc->getWeightHistory();
        float minW = flowCalc->getWeightMin();
        float maxW = flowCalc->getWeightMax();
        float range = maxW - minW;
        if (range < 5) range = 5;
        minW -= range * 0.1;
        maxW += range * 0.1;
        float yScale = 50 / range;

        int prevX = -1, prevY = -1;
        for (int i = 0; i < count; i++) {
            int idx = flowCalc->getChronologicalIndex(i);
            int px = (i * SCREEN_WIDTH) / count;
            int py = 84 + 51 - (int)((weights[idx] - minW) * yScale);
            py = constrain(py, 84, 135);
            if (prevX >= 0) {
                M5.Lcd.drawLine(prevX, prevY, px, py, COLOR_ACCENT);
                M5.Lcd.drawLine(prevX, prevY + 1, px, py + 1, COLOR_ACCENT);
            }
            prevX = px; prevY = py;
        }
    }
}

// === 重量曲线页面 ===
void DisplayModule::_drawWeightCurvePage(FlowCalculator* flowCalc) {
    int count = flowCalc->getHistoryCount();
    if (count < 2) return;

    int curveX = 20, curveY = TITLE_HEIGHT + 5;
    int curveW = SCREEN_WIDTH - 25, curveH = 108;
    _clearArea(curveX, curveY, curveW, curveH);
    _drawGrid(curveX, curveY, curveW, curveH);

    float minW = flowCalc->getWeightMin();
    float maxW = flowCalc->getWeightMax();
    float range = maxW - minW;
    if (range < 10) range = 10;
    minW -= range * 0.1;
    maxW += range * 0.1;

    float* weights = flowCalc->getWeightHistory();
    if (count < 2) return;
    float yScale = curveH / range;
    int prevX = -1, prevY = -1;

    for (int i = 0; i < count; i++) {
        int idx = flowCalc->getChronologicalIndex(i);
        int x = curveX + (i * curveW) / count;
        int y = curveY + curveH - (int)((weights[idx] - minW) * yScale);
        y = constrain(y, curveY, curveY + curveH);
        if (prevX >= 0) {
            M5.Lcd.drawLine(prevX, prevY, x, y, COLOR_CURVE_WEIGHT);
            M5.Lcd.drawLine(prevX, prevY + 1, x, y + 1, COLOR_CURVE_WEIGHT);
        }
        prevX = x; prevY = y;
    }
    _drawAxisLabels(minW, maxW, 0, curveY, curveH);
    _drawTimeAxis(flowCalc->getTimeMin(), flowCalc->getTimeMax(), curveX, SCREEN_HEIGHT - 14, curveW);
}

// === 流量曲线页面 ===
void DisplayModule::_drawFlowCurvePage(FlowCalculator* flowCalc) {
    int count = flowCalc->getHistoryCount();
    if (count < 2) return;

    int curveX = 20, curveY = TITLE_HEIGHT + 5;
    int curveW = SCREEN_WIDTH - 25, curveH = 108;
    _clearArea(curveX, curveY, curveW, curveH);
    _drawGrid(curveX, curveY, curveW, curveH);

    float minF = flowCalc->getFlowMin();
    float maxF = flowCalc->getFlowMax();
    float range = maxF - minF;
    if (range < 5) range = 5;
    minF -= range * 0.1;
    maxF += range * 0.1;
    if (minF > 0) minF = 0;

    float* flows = flowCalc->getFlowHistory();
    if (count < 2) return;
    float yScale = curveH / range;
    int prevX = -1, prevY = -1;

    for (int i = 0; i < count; i++) {
        int idx = flowCalc->getChronologicalIndex(i);
        int x = curveX + (i * curveW) / count;
        int y = curveY + curveH - (int)((flows[idx] - minF) * yScale);
        y = constrain(y, curveY, curveY + curveH);
        if (prevX >= 0) {
            M5.Lcd.drawLine(prevX, prevY, x, y, COLOR_CURVE_FLOW);
            M5.Lcd.drawLine(prevX, prevY + 1, x, y + 1, COLOR_CURVE_FLOW);
        }
        prevX = x; prevY = y;
    }
    _drawAxisLabels(minF, maxF, 0, curveY, curveH);
    _drawTimeAxis(flowCalc->getTimeMin(), flowCalc->getTimeMax(), curveX, SCREEN_HEIGHT - 14, curveW);
}

// === 设置页面 ===
void DisplayModule::_drawSettingsPage() {
    M5.Lcd.fillScreen(COLOR_BG);
    M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, TITLE_HEIGHT, COLOR_BG_DARK);
    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    M5.Lcd.setTextColor(COLOR_ACCENT, COLOR_BG_DARK);
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.drawString("SETTINGS", SCREEN_WIDTH / 2, 3);
    M5.Lcd.drawFastHLine(0, TITLE_HEIGHT, SCREEN_WIDTH, COLOR_ACCENT);
    _drawPageIndicator();

    // Ratio
    M5.Lcd.fillRoundRect(8, 22, 108, 32, 3, COLOR_BG_DARK);
    M5.Lcd.setFreeFont(&FreeSerif9pt7b);
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG_DARK);
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.drawString("RATIO", 14, 25);
    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    M5.Lcd.setTextColor(COLOR_ACCENT, COLOR_BG_DARK);
    char rBuf[10];
    snprintf(rBuf, sizeof(rBuf), "1:%.0f", _brewRatio);
    M5.Lcd.drawString(rBuf, 14, 38);

    // Dose
    M5.Lcd.fillRoundRect(124, 22, 108, 32, 3, COLOR_BG_DARK);
    M5.Lcd.setFreeFont(&FreeSerif9pt7b);
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG_DARK);
    M5.Lcd.drawString("DOSE", 130, 25);
    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    M5.Lcd.setTextColor(COLOR_ACCENT, COLOR_BG_DARK);
    snprintf(rBuf, sizeof(rBuf), "%.0f g", _brewDose);
    M5.Lcd.drawString(rBuf, 130, 38);

    // Target
    M5.Lcd.fillRoundRect(8, 58, 224, 26, 3, COLOR_BG_DARK);
    M5.Lcd.setFreeFont(&FreeSerif9pt7b);
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG_DARK);
    M5.Lcd.drawString("TARGET WATER", 14, 62);
    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    M5.Lcd.setTextColor(COLOR_ACCENT, COLOR_BG_DARK);
    snprintf(rBuf, sizeof(rBuf), "%.0f g", _brewTarget);
    M5.Lcd.drawString(rBuf, 180, 62);

    // Calibration
    M5.Lcd.fillRoundRect(8, 88, 224, 22, 3, COLOR_BG_DARK);
    M5.Lcd.setFreeFont(&FreeSerif9pt7b);
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG_DARK);
    M5.Lcd.drawString("CAL", 14, 92);
    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    M5.Lcd.setTextColor(COLOR_ACCENT, COLOR_BG_DARK);
    M5.Lcd.drawString("420.0", 60, 92);

    // 快捷键
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.setFreeFont(&FreeSerif9pt7b);
    M5.Lcd.drawString("1:page  r:ratio  d:dose", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 10);
}

// ============================================================
// 背景绘制
// ============================================================
void DisplayModule::_drawMainBackground() {
    M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, TITLE_HEIGHT, COLOR_BG_DARK);
    _drawTitle(F("POUR OVER"));
}
void DisplayModule::_drawCurveBackground(const __FlashStringHelper* title) {
    M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, TITLE_HEIGHT, COLOR_BG_DARK);
    _drawTitle(title);
    M5.Lcd.drawFastHLine(0, TITLE_HEIGHT, SCREEN_WIDTH, COLOR_ACCENT);
}
void DisplayModule::_drawTitle(const __FlashStringHelper* title) {
    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    M5.Lcd.setTextColor(COLOR_ACCENT, COLOR_BG_DARK);
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.drawString(title, SCREEN_WIDTH / 2, 3);
    _drawPageIndicator();
}

// ============================================================
// 曲线绘制
// ============================================================
void DisplayModule::_drawGrid(int x, int y, int w, int h) {
    M5.Lcd.drawRect(x, y, w, h, COLOR_DIVIDER);
    for (int i = 1; i < 4; i++) {
        int lineY = y + (i * h) / 4;
        for (int px = x; px < x + w; px += 5)
            M5.Lcd.drawPixel(px, lineY, COLOR_GRID);
    }
}
void DisplayModule::_drawAxisLabels(float minVal, float maxVal, int x, int y, int h) {
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    M5.Lcd.setTextSize(SMALL_FONT_SIZE);
    M5.Lcd.setTextDatum(TR_DATUM);
    char label[8];
    snprintf(label, sizeof(label), "%.0f", maxVal);
    M5.Lcd.drawString(label, x + 15, y);
    snprintf(label, sizeof(label), "%.0f", minVal);
    M5.Lcd.drawString(label, x + 15, y + h - 8);
}
void DisplayModule::_drawTimeAxis(unsigned long tMin, unsigned long tMax, int x, int y, int w) {
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    M5.Lcd.setTextSize(SMALL_FONT_SIZE);
    M5.Lcd.setTextDatum(TC_DATUM);
    char label[8];
    snprintf(label, sizeof(label), "%.0f", tMin / 1000.0f);
    M5.Lcd.drawString(label, x, y);
    snprintf(label, sizeof(label), "%.0f", tMax / 1000.0f);
    M5.Lcd.drawString(label, x + w, y);
}
void DisplayModule::_drawProgressBar(int x, int y, int w, int h, float current, float target) {
    M5.Lcd.drawRect(x, y, w, h, COLOR_DIVIDER);
    if (target > 0) {
        float pct = current / target;
        if (pct > 1.0f) pct = 1.0f;
        if (pct > 0) {
            uint16_t fillColor = _targetReached ? COLOR_SUCCESS : COLOR_STATUS_ON;
            M5.Lcd.fillRect(x + 1, y + 1, (int)((w - 2) * pct), h - 2, fillColor);
        }
    }
}
void DisplayModule::_drawPageIndicator() {
    int indicatorX = SCREEN_WIDTH - 35;
    for (int i = 0; i < 4; i++) {
        uint16_t color = (i == _currentPage) ? COLOR_ACCENT : COLOR_TEXT_DIM;
        M5.Lcd.fillCircle(indicatorX + i * 8, 10, 3, color);
    }
}
void DisplayModule::_clearArea(int x, int y, int w, int h) {
    M5.Lcd.fillRect(x, y, w, h, COLOR_BG);
}
