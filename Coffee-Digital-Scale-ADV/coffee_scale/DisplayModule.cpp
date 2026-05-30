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
// 方案 B · 暖咖氛围  —  240×135
// 配色：深褐 #24140D | 暖金 #D4A574 | 暖白 #F5E6D0
// ============================================================
// ┌────────────────────────────────────────────────────────┐
// │      POUR OVER                           [● ○ ○ ○]  │ 0–18
// │━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━│ 18  (粗)
// │                                                        │
// │                    187.3                               │ 19–68  重量
// │                    grams                               │ 68–78
// │━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━│ 78  (粗)
// │  flow            time                                  │ 80–95
// │  4.2 g/s    01:23                                     │ 95–114
// │────────────────────────────────────────────────────────│ 114
// │ ● BREWING         ▁▂▃▅▆▇▆▅▃▂▁                       │ 115–135
// └────────────────────────────────────────────────────────┘

void DisplayModule::_drawMainPage(float weight, float flowRate, TimerModule* timer, FlowCalculator* flowCalc) {
    // === 标题栏（仅首次绘制，背景不变）===

    // === 重量区域（y:19-78）===
    // 清除重量区域
    _clearArea(0, 20, SCREEN_WIDTH, 58);

    // 重量数值（font 6 ≈ 48px 大，如果装不下回退 font 4）
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.setTextDatum(MC_DATUM);

    // 尝试 font 6 — 如果数字太长（>999g）可能超出屏幕宽度
    M5.Lcd.setTextSize(1);
    // Font 6 是 48px 高的内置大字体
    char weightStr[10];
    dtostrf(weight, 5, 1, weightStr);

    // 计算显示位置：重量数字居中偏上，单位偏下
    M5.Lcd.drawString(weightStr, SCREEN_WIDTH / 2, 48, 6);

    // "grams" 标签小字
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    M5.Lcd.setTextSize(NORMAL_FONT_SIZE);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.drawString("grams", SCREEN_WIDTH / 2, 68, 2);

    // === 粗分隔线（y:78）===
    M5.Lcd.drawFastHLine(0, 77, SCREEN_WIDTH, COLOR_ACCENT);
    M5.Lcd.drawFastHLine(0, 78, SCREEN_WIDTH, COLOR_BG_DARK);

    // === 状态栏（y:79-114）===
    _drawStatusBar(flowRate, timer);

    // === 状态指示器（y:115-122）===
    _drawStatusIndicators(timer);

    // === 迷你曲线（y:122-135）===
    _drawMiniCurve(flowCalc, 0, 122, SCREEN_WIDTH, 13);
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
// 状态栏
// ============================================================
void DisplayModule::_drawStatusBar(float flowRate, TimerModule* timer) {
    // 清除状态栏区域
    _clearArea(0, 80, SCREEN_WIDTH, 34);

    // 流量
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    M5.Lcd.setTextSize(SMALL_FONT_SIZE);
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.drawString("flow", 12, 82);

    M5.Lcd.setTextColor(COLOR_ACCENT, COLOR_BG);
    M5.Lcd.setTextSize(NORMAL_FONT_SIZE);
    char flowStr[16];
    snprintf(flowStr, sizeof(flowStr), "%.1f g/s", flowRate);
    M5.Lcd.drawString(flowStr, 12, 93);

    // 计时
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    M5.Lcd.setTextSize(SMALL_FONT_SIZE);
    M5.Lcd.setTextDatum(TR_DATUM);
    M5.Lcd.drawString("time", SCREEN_WIDTH - 12, 82);

    M5.Lcd.setTextColor(COLOR_ACCENT, COLOR_BG);
    M5.Lcd.setTextSize(NORMAL_FONT_SIZE);
    String timeStr = timer->getFormattedTime();
    M5.Lcd.drawString(timeStr, SCREEN_WIDTH - 12, 93);
}

// ============================================================
// 状态指示器
// ============================================================
void DisplayModule::_drawStatusIndicators(TimerModule* timer) {
    int indicatorY = 122;
    int indicatorX = 12;

    // 分隔线
    M5.Lcd.drawFastHLine(0, 118, SCREEN_WIDTH, COLOR_DIVIDER);

    // 状态 LED
    bool running = timer->isRunning();
    uint16_t ledColor = running ? COLOR_STATUS_ON : COLOR_STATUS_OFF;
    uint16_t textColor = running ? COLOR_STATUS_ON : COLOR_TEXT_DIM;

    M5.Lcd.fillCircle(indicatorX, indicatorY + 2, 4, ledColor);
    M5.Lcd.setTextColor(textColor, COLOR_BG);
    M5.Lcd.setTextSize(SMALL_FONT_SIZE);
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.drawString(running ? "BREWING" : "STANDBY", indicatorX + 10, indicatorY);
}

// ============================================================
// 迷你曲线
// ============================================================
void DisplayModule::_drawMiniCurve(FlowCalculator* flowCalc, int x, int y, int w, int h) {
    int count = flowCalc->getHistoryCount();
    if (count < 2) return;

    float* weights = flowCalc->getWeightHistory();
    float minW = flowCalc->getWeightMin();
    float maxW = flowCalc->getWeightMax();
    float range = maxW - minW;
    if (range < 5) range = 5;
    minW -= range * 0.1;
    maxW += range * 0.1;

    float yScale = h / range;
    int prevX = -1, prevY = -1;

    // 细底纹线
    M5.Lcd.drawFastHLine(0, y + h, SCREEN_WIDTH, COLOR_ACCENT);

    for (int i = 0; i < count; i++) {
        int idx = flowCalc->getChronologicalIndex(i);
        int px = x + (i * w) / count;
        int py = y + h - (int)((weights[idx] - minW) * yScale);
        py = constrain(py, y, y + h);
        if (prevX >= 0) {
            M5.Lcd.drawLine(prevX, prevY, px, py, COLOR_ACCENT);
        }
        prevX = px; prevY = py;
    }
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
    int indicatorY = 118;
    int indicatorX = SCREEN_WIDTH - 35;
    for (int i = 0; i < 4; i++) {
        uint16_t color = (i == _currentPage) ? COLOR_ACCENT : COLOR_TEXT_DIM;
        M5.Lcd.fillCircle(indicatorX + i * 8, indicatorY, 3, color);
    }
}

void DisplayModule::_clearArea(int x, int y, int w, int h) {
    M5.Lcd.fillRect(x, y, w, h, COLOR_BG);
}
