#include "DisplayModule.h"
#include <Fonts/FreeSerif12pt7b.h>
#include <Fonts/FreeSerif18pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>

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
    M5.Lcd.setTextSize(NORMAL_FONT_SIZE);
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
    M5.Lcd.setTextSize(NORMAL_FONT_SIZE);
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
    _brewRatio = ratio;
    _brewDose = dose;
    _brewTarget = target;
}

void DisplayModule::setTargetReached(bool reached) {
    _targetReached = reached;
}

// ============================================================
// 方案 B · 暖咖氛围  —  240×135  修订版（修复重叠与溢出）
// ============================================================
// ┌────────────────────────────────────────────────────────┐
// │      POUR OVER                        [● ○ ○ ○]      │ 0–16
// │━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━│ 16
// │                    187.3                               │ 20–58  重量
// │━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━│ 60
// │  flow              time                                │ 64–90  双行
// │  4.2 g/s     01:23                                     │
// │────────────────────────────────────────────────────────│ 98
// │ ● BREWING       187/225g                               │ 102–112
// │ ████████████░░░░░                                       │ 114–118 进度条
// │────────────────────────────────────────────────────────│ 120
// │  ▁▂▃▅▆▇▆▅▃▂▁                                          │ 122–135 迷你曲线
// └────────────────────────────────────────────────────────┘

void DisplayModule::_drawMainPage(float weight, float flowRate, TimerModule* timer, FlowCalculator* flowCalc) {
    // 清除标题栏以下所有动态区域
    _clearArea(0, 18, SCREEN_WIDTH, 117);

    // === 重量（y:20-58）===
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setFreeFont(&FreeSerif18pt7b);
    char weightStr[10];
    dtostrf(weight, 5, 1, weightStr);
    M5.Lcd.drawString(weightStr, SCREEN_WIDTH / 2, 42);

    // === 粗分隔线 ===
    M5.Lcd.drawFastHLine(0, 59, SCREEN_WIDTH, COLOR_ACCENT);
    M5.Lcd.drawFastHLine(0, 60, SCREEN_WIDTH, COLOR_BG_DARK);

    // === 状态栏双行（y:64-90）===
    M5.Lcd.setFreeFont(&FreeSerif18pt7b);
    M5.Lcd.setTextColor(COLOR_ACCENT, COLOR_BG);
    char flowVal[8];
    dtostrf(flowRate, 4, 1, flowVal);
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.drawString("flow", 10, 66);
    M5.Lcd.drawString(flowVal, 10, 84);
    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    M5.Lcd.drawString("g/s", 55, 88);
    M5.Lcd.setFreeFont(&FreeSerif18pt7b);
    String timeStr = timer->getFormattedTime();
    M5.Lcd.setTextDatum(TR_DATUM);
    M5.Lcd.drawString("time", SCREEN_WIDTH - 10, 66);
    M5.Lcd.drawString(timeStr, SCREEN_WIDTH - 10, 84);

    // === 细分隔 ===
    M5.Lcd.drawFastHLine(0, 96, SCREEN_WIDTH, COLOR_DIVIDER);

    // === 状态指示 + 目标水量 + 进度条（y:99-118）===
    bool running = timer->isRunning();
    uint16_t ledColor = running ? COLOR_STATUS_ON : COLOR_STATUS_OFF;
    uint16_t statColor = running ? COLOR_STATUS_ON : COLOR_TEXT_DIM;
    M5.Lcd.fillCircle(10, 104, 3, ledColor);
    M5.Lcd.setFreeFont(&FreeSerif9pt7b);
    M5.Lcd.setTextColor(statColor, COLOR_BG);
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.drawString(running ? "BREWING" : "STANDBY", 18, 99);

    // 目标水量（右上）
    char targetStr[16];
    if (_brewTarget > 0) {
        snprintf(targetStr, sizeof(targetStr), "%.0f/%.0fg", weight, _brewTarget);
    } else {
        snprintf(targetStr, sizeof(targetStr), "%.1fg", weight);
    }
    M5.Lcd.setFreeFont(&FreeSerif9pt7b);
    M5.Lcd.setTextColor(_targetReached ? COLOR_SUCCESS : COLOR_TEXT_DIM, COLOR_BG);
    M5.Lcd.setTextDatum(TR_DATUM);
    M5.Lcd.drawString(targetStr, SCREEN_WIDTH - 10, 99);

    // 进度条（y:110-114）
    _drawProgressBar(10, 110, SCREEN_WIDTH - 20, 4, weight, _brewTarget);

    // === 迷你曲线（y:120-135）===
    int count = flowCalc->getHistoryCount();
    if (count >= 2) {
        float* weights = flowCalc->getWeightHistory();
        float minW = flowCalc->getWeightMin();
        float maxW = flowCalc->getWeightMax();
        float range = maxW - minW;
        if (range < 5) range = 5;
        minW -= range * 0.1;
        maxW += range * 0.1;
        float yScale = 14 / range;

        int prevX = -1, prevY = -1;
        for (int i = 0; i < count; i++) {
            int idx = flowCalc->getChronologicalIndex(i);
            int px = (i * SCREEN_WIDTH) / count;
            int py = 120 + 15 - (int)((weights[idx] - minW) * yScale);
            py = constrain(py, 120, 135);
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

    int curveX = 18, curveY = TITLE_HEIGHT + 5;
    int curveW = SCREEN_WIDTH - 22, curveH = SCREEN_HEIGHT - TITLE_HEIGHT - 25;
    _clearArea(curveX, curveY, curveW, curveH);
    _drawGrid(curveX, curveY, curveW, curveH);

    float minW = flowCalc->getWeightMin();
    float maxW = flowCalc->getWeightMax();
    float range = maxW - minW;
    if (range < 10) range = 10;
    minW -= range * 0.1;
    maxW += range * 0.1;

    // 绘制曲线（2px 粗线）
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
            // 加粗：再画一次
            M5.Lcd.drawLine(prevX, prevY + 1, x, y + 1, COLOR_CURVE_WEIGHT);
        }
        prevX = x; prevY = y;
    }

    _drawAxisLabels(minW, maxW, 0, curveY, curveH);
    _drawTimeAxis(flowCalc->getTimeMin(), flowCalc->getTimeMax(), curveX, SCREEN_HEIGHT - 18, curveW);
}

// === 流量曲线页面 ===
void DisplayModule::_drawFlowCurvePage(FlowCalculator* flowCalc) {
    int count = flowCalc->getHistoryCount();
    if (count < 2) return;

    int curveX = 18, curveY = TITLE_HEIGHT + 5;
    int curveW = SCREEN_WIDTH - 22, curveH = SCREEN_HEIGHT - TITLE_HEIGHT - 25;
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
    _drawTimeAxis(flowCalc->getTimeMin(), flowCalc->getTimeMax(), curveX, SCREEN_HEIGHT - 18, curveW);
}

// === 设置页面 ===
void DisplayModule::_drawSettingsPage() {
    M5.Lcd.fillScreen(COLOR_BG);

    // 标题栏
    M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, TITLE_HEIGHT, COLOR_BG_DARK);
    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    M5.Lcd.setTextColor(COLOR_ACCENT, COLOR_BG_DARK);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.drawString("SETTINGS", SCREEN_WIDTH / 2, 3);
    M5.Lcd.drawFastHLine(0, TITLE_HEIGHT, SCREEN_WIDTH, COLOR_ACCENT);
    _drawPageIndicator();

    // 水分比设置行
    M5.Lcd.fillRoundRect(8, 22, 224, 34, 3, COLOR_BG_DARK);
    M5.Lcd.setFreeFont(&FreeSerif9pt7b);
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG_DARK);
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.drawString("ratio", 14, 25);
    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    M5.Lcd.setTextColor(COLOR_ACCENT, COLOR_BG_DARK);
    char ratioStr[10];
    snprintf(ratioStr, sizeof(ratioStr), "1:%.0f", _brewRatio);
    M5.Lcd.drawString(ratioStr, 14, 38);
    M5.Lcd.setFreeFont(&FreeSerif9pt7b);
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG_DARK);
    M5.Lcd.drawString("[r]", 100, 38);

    // 粉量设置行
    M5.Lcd.setFreeFont(&FreeSerif9pt7b);
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG_DARK);
    M5.Lcd.drawString("dose", 128, 25);
    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    M5.Lcd.setTextColor(COLOR_ACCENT, COLOR_BG_DARK);
    char doseStr[10];
    snprintf(doseStr, sizeof(doseStr), "%.0f g", _brewDose);
    M5.Lcd.drawString(doseStr, 128, 38);
    M5.Lcd.setFreeFont(&FreeSerif9pt7b);
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG_DARK);
    M5.Lcd.drawString("[d]", 210, 38);

    // 目标水量
    M5.Lcd.fillRoundRect(8, 60, 224, 24, 3, COLOR_BG_DARK);
    M5.Lcd.setFreeFont(&FreeSerif9pt7b);
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG_DARK);
    M5.Lcd.drawString("target water", 14, 63);
    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    M5.Lcd.setTextColor(COLOR_ACCENT, COLOR_BG_DARK);
    char targetStr[12];
    snprintf(targetStr, sizeof(targetStr), "%.0f g", _brewTarget);
    M5.Lcd.drawString(targetStr, 130, 63);

    // 校准因子
    M5.Lcd.fillRoundRect(8, 88, 224, 22, 3, COLOR_BG_DARK);
    M5.Lcd.setFreeFont(&FreeSerif9pt7b);
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG_DARK);
    M5.Lcd.drawString("factor", 14, 91);
    M5.Lcd.setFreeFont(&FreeSerif12pt7b);
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG_DARK);
    M5.Lcd.drawString("420.0", 74, 91);

    // 底部快捷键提示
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.setFreeFont(&FreeSerif9pt7b);
    M5.Lcd.drawString("1:return  r:ratio  d:dose  =:confirm", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 10);
}

// ============================================================
// 背景绘制
// ============================================================
void DisplayModule::_drawMainBackground() {
    // 标题栏深色底
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
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.drawString(title, SCREEN_WIDTH / 2, 3);
    _drawPageIndicator();
}


// ============================================================
// 曲线绘制
// ============================================================
void DisplayModule::_drawGrid(int x, int y, int w, int h) {
    // 边框（粗线）
    M5.Lcd.drawRect(x, y, w, h, COLOR_DIVIDER);
    // 水平网格线（虚线）
    for (int i = 1; i < 4; i++) {
        int lineY = y + (i * h) / 4;
        for (int px = x; px < x + w; px += 5) {
            M5.Lcd.drawPixel(px, lineY, COLOR_GRID);
        }
    }
}

void DisplayModule::_drawAxisLabels(float minVal, float maxVal, int x, int y, int h) {
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    M5.Lcd.setTextSize(SMALL_FONT_SIZE);
    M5.Lcd.setTextDatum(TR_DATUM);

    char label[8];
    snprintf(label, sizeof(label), "%.0f", maxVal);
    M5.Lcd.drawString(label, x + 14, y);

    snprintf(label, sizeof(label), "%.0f", minVal);
    M5.Lcd.drawString(label, x + 14, y + h - 8);
}

void DisplayModule::_drawTimeAxis(unsigned long timeMin, unsigned long timeMax, int x, int y, int w) {
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    M5.Lcd.setTextSize(SMALL_FONT_SIZE);
    M5.Lcd.setTextDatum(TC_DATUM);

    float startSec = timeMin / 1000.0f;
    float endSec = timeMax / 1000.0f;

    char label[8];
    snprintf(label, sizeof(label), "%.0f", startSec);
    M5.Lcd.drawString(label, x, y);

    snprintf(label, sizeof(label), "%.0f", endSec);
    M5.Lcd.drawString(label, x + w, y);
}

void DisplayModule::_drawProgressBar(int x, int y, int w, int h, float current, float target) {
    // 背景
    M5.Lcd.drawRect(x, y, w, h, COLOR_DIVIDER);
    // 填充
    if (target > 0) {
        float pct = current / target;
        if (pct > 1.0f) pct = 1.0f;
        if (pct > 0) {
            uint16_t fillColor = _targetReached ? COLOR_SUCCESS : COLOR_STATUS_ON;  // 暖金匹配主题
            M5.Lcd.fillRect(x + 1, y + 1, (int)((w - 2) * pct), h - 2, fillColor);
        }
    }
}

void DisplayModule::_drawPageIndicator() {
    // 页面指示器在标题栏右侧，不和曲线/状态区域抢位置
    int indicatorY = 9;
    int indicatorX = SCREEN_WIDTH - 35;
    for (int i = 0; i < 4; i++) {
        uint16_t color = (i == _currentPage) ? COLOR_ACCENT : COLOR_TEXT_DIM;
        M5.Lcd.fillCircle(indicatorX + i * 8, indicatorY, 3, color);
    }
}

void DisplayModule::_clearArea(int x, int y, int w, int h) {
    M5.Lcd.fillRect(x, y, w, h, COLOR_BG);
}
