#include "DisplayModule.h"

DisplayModule::DisplayModule() {
    _currentPage = PAGE_MAIN;
    _lastUpdateTime = 0;
    _lastCurveUpdateTime = 0;
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

// ============================================================
// 方案 B · 暖咖氛围  —  240×135  修订版（修复重叠与溢出）
// ============================================================
// ┌────────────────────────────────────────────────────────┐
// │      POUR OVER                        [● ○ ○ ○]      │ 0–18
// │━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━│ 18
// │                                                        │
// │                    187.3                               │ 22–62  重量 (font 6, 中心 y=42)
// │━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━│ 64
// │  flow 4.2 g/s               time 01:23                 │ 68–84  状态栏（单行紧凑）
// │────────────────────────────────────────────────────────│ 86
// │ ● BREWING                                              │ 90–98
// │────────────────────────────────────────────────────────│ 100
// │  ▁▂▃▅▆▇▆▅▃▂▁▁▂▃▅▆▇▆▅▃▂▁                              │ 102–135 迷你曲线
// └────────────────────────────────────────────────────────┘

void DisplayModule::_drawMainPage(float weight, float flowRate, TimerModule* timer, FlowCalculator* flowCalc) {
    // === 清除所有动态区域（防止文字残留）===
    _clearArea(0, 20, SCREEN_WIDTH, 115);  // 清除标题栏以下所有区域

    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setTextSize(1);
    char weightStr[10];
    dtostrf(weight, 5, 1, weightStr);
    M5.Lcd.drawString(weightStr, SCREEN_WIDTH / 2, 42, 6);

    // === 粗分隔线（y:64）===
    M5.Lcd.drawFastHLine(0, 63, SCREEN_WIDTH, COLOR_ACCENT);
    M5.Lcd.drawFastHLine(0, 64, SCREEN_WIDTH, COLOR_BG_DARK);

    // === 状态栏 — 单行紧凑（y:68-84）===
    M5.Lcd.setTextColor(COLOR_ACCENT, COLOR_BG);
    M5.Lcd.setTextSize(NORMAL_FONT_SIZE);
    M5.Lcd.setTextDatum(TL_DATUM);

    char line[30];
    snprintf(line, sizeof(line), "flow %.1f g/s", flowRate);
    M5.Lcd.drawString(line, 12, 74, 2);

    String timeStr = timer->getFormattedTime();
    M5.Lcd.setTextDatum(TR_DATUM);
    M5.Lcd.drawString(timeStr, SCREEN_WIDTH - 12, 74, 2);

    // === 分隔线 ===
    M5.Lcd.drawFastHLine(0, 88, SCREEN_WIDTH, COLOR_DIVIDER);

    // === 状态指示器（y:92-98）===
    bool running = timer->isRunning();
    uint16_t ledColor = running ? COLOR_STATUS_ON : COLOR_STATUS_OFF;
    uint16_t textColor = running ? COLOR_STATUS_ON : COLOR_TEXT_DIM;
    M5.Lcd.fillCircle(12, 95, 3, ledColor);
    M5.Lcd.setTextColor(textColor, COLOR_BG);
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.setTextSize(SMALL_FONT_SIZE);
    M5.Lcd.drawString(running ? "BREWING" : "STANDBY", 20, 91, 1);

    // === 迷你曲线（y:102-135）===
    int count = flowCalc->getHistoryCount();
    if (count >= 2) {
        float* weights = flowCalc->getWeightHistory();
        float minW = flowCalc->getWeightMin();
        float maxW = flowCalc->getWeightMax();
        float range = maxW - minW;
        if (range < 5) range = 5;
        minW -= range * 0.1;
        maxW += range * 0.1;
        float yScale = 32 / range;  // h=33px

        int prevX = -1, prevY = -1;
        for (int i = 0; i < count; i++) {
            int idx = flowCalc->getChronologicalIndex(i);
            int px = (i * SCREEN_WIDTH) / count;
            int py = 102 + 33 - (int)((weights[idx] - minW) * yScale);
            py = constrain(py, 102, 135);
            if (prevX >= 0) {
                M5.Lcd.drawLine(prevX, prevY, px, py, COLOR_ACCENT);
            }
            prevX = px; prevY = py;
        }
    }
}

// === 重量曲线页面 ===
void DisplayModule::_drawWeightCurvePage(FlowCalculator* flowCalc) {
    int count = flowCalc->getHistoryCount();
    if (count < 2) return;

    int curveX = 35, curveY = TITLE_HEIGHT + 5;
    int curveW = SCREEN_WIDTH - 40, curveH = SCREEN_HEIGHT - TITLE_HEIGHT - 25;
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

    int curveX = 35, curveY = TITLE_HEIGHT + 5;
    int curveW = SCREEN_WIDTH - 40, curveH = SCREEN_HEIGHT - TITLE_HEIGHT - 25;
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
    M5.Lcd.setTextColor(COLOR_ACCENT, COLOR_BG_DARK);
    M5.Lcd.setTextSize(NORMAL_FONT_SIZE);
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.drawString("SETTINGS", SCREEN_WIDTH / 2, 3);
    M5.Lcd.drawFastHLine(0, TITLE_HEIGHT, SCREEN_WIDTH, COLOR_ACCENT);
    _drawPageIndicator();

    // 设置项卡
    M5.Lcd.fillRoundRect(8, 24, 224, 28, 3, COLOR_BG_DARK);

    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG_DARK);
    M5.Lcd.setTextSize(SMALL_FONT_SIZE);
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.drawString("start", 14, 27);
    M5.Lcd.drawString("reset", 128, 27);

    M5.Lcd.setTextColor(COLOR_ACCENT, COLOR_BG_DARK);
    M5.Lcd.setTextSize(NORMAL_FONT_SIZE);
    M5.Lcd.drawString("0.5 g", 14, 38);
    M5.Lcd.drawString("0.3 g", 128, 38);

    M5.Lcd.fillRoundRect(8, 56, 224, 28, 3, COLOR_BG_DARK);
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG_DARK);
    M5.Lcd.setTextSize(SMALL_FONT_SIZE);
    M5.Lcd.drawString("factor", 14, 59);
    M5.Lcd.setTextColor(COLOR_ACCENT, COLOR_BG_DARK);
    M5.Lcd.setTextSize(NORMAL_FONT_SIZE);
    M5.Lcd.drawString("420.0", 14, 70);

    M5.Lcd.fillRoundRect(8, 88, 224, 22, 3, COLOR_BG_DARK);
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG_DARK);
    M5.Lcd.setTextSize(SMALL_FONT_SIZE);
    M5.Lcd.drawString("key", 14, 91);
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG_DARK);
    M5.Lcd.setTextSize(NORMAL_FONT_SIZE);
    M5.Lcd.drawString("1: page", 48, 91);
    M5.Lcd.drawString("t: tare", 126, 91);

    // 底部返回提示
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.setTextSize(SMALL_FONT_SIZE);
    M5.Lcd.drawString("[ 1 ] RETURN", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 10);
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
    M5.Lcd.setTextColor(COLOR_ACCENT, COLOR_BG_DARK);
    M5.Lcd.setTextSize(NORMAL_FONT_SIZE);
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
    M5.Lcd.drawString(label, x + 30, y);

    snprintf(label, sizeof(label), "%.0f", minVal);
    M5.Lcd.drawString(label, x + 30, y + h - 8);
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
