#include "DisplayModule.h"
#include <math.h>
#include <string.h>

// ============================================================
// 纸墨印刷风格 · 白底黑字
// 配色: 纸白 #F7F3EF + 黑墨 #191819
// 无背景色块 — 层级靠字号和间距区分
// 曲线用心电图纸风格黑墨细线
//
// 字体精确高度（来自 GFXfont glyph 表）：
// FreeSerif18pt7b: max_ascent=24, max_descent=9, metrics.height=33
// FreeSerif9pt7b:  max_ascent=12, max_descent=5, metrics.height=17
// Font0:           高度 ≈ 8px（5x7 像素字体）
//
// PAGE_MAIN 布局 240×135 — 基于精确字体高度确保零重叠：
// [0,  12]  标题 Font0 TC_DATUM y=2 + 分隔线 y=12
//        ↓ 6px
// [20, 44]  重量 FreeSerif18pt MC_DATUM y=29 ('0'字底 y=38)
//        ↓ 6px
// [50, 53]  进度条 h=3  fillRect(10, 40, 220, 3)
//        ↓ 6px
// [59, 76]  信息行 FreeSerif9pt TL_DATUM y=47 (含g字底 y=64)
//        ↓ 3px
// [79]      分隔线
//        ↓ 3px
//
// [82, 135] 迷你曲线 55px 黑墨细线
//
// ============================================================

DisplayModule::DisplayModule() {
    _currentPage = PAGE_MAIN;
    _lastUpdateTime = 0;
    _lastCurveUpdateTime = 0;
    _lastMiniCurveUpdateTime = 0;
    _brewRatio = 15.0f;
    _brewDose = 15.0f;
    _brewTarget = 225.0f;
    _calibrationFactor = DEFAULT_CALIBRATION_FACTOR;
    _targetReached = false;
    _lastWeightShown = 999999.0f;
    _lastFlowShown = 999999.0f;
    _lastElapsedShown = 999999;
    _lastTargetShown = false;
    _lastRunningShown = false;
    _mainPageDirty = true;
    _pageDirty = true;
    _settingsSelectedIndex = 0;
    _settingsEditing = false;
    _settingsPageDirty = true;
    _settingsInputText[0] = '\0';
    _lastMiniCurveCount = -1;
}

void DisplayModule::init() {
    M5.Lcd.setRotation(1);
    M5.Lcd.fillScreen(COLOR_BG);
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.setTextSize(1);
    _drawMainBackground();
}

void DisplayModule::setPage(Page page) {
    if (page != _currentPage || _pageDirty) {
        _currentPage = page;
        _pageDirty = false;
        _mainPageDirty = true;
        _settingsPageDirty = true;
        _lastMiniCurveUpdateTime = 0;
        _lastMiniCurveCount = -1;
        M5.Lcd.fillScreen(COLOR_BG);
        switch (_currentPage) {
            case PAGE_MAIN:          _drawMainBackground(); break;
            case PAGE_WEIGHT_CURVE:  _drawCurveBackground(F("WEIGHT")); break;
            case PAGE_FLOW_CURVE:    _drawCurveBackground(F("FLOW")); break;
            case PAGE_SETTINGS:      _drawSettingsPage(); break;
        }
        _drawBottomHints();
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

void DisplayModule::showMessage(const __FlashStringHelper* message, int durationMs) {
    (void)durationMs;
    Page savedPage = _currentPage;
    M5.Lcd.fillScreen(COLOR_BG);
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.setFont(&fonts::FreeSerif12pt7b);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.drawString(message, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    _currentPage = savedPage;
    _mainPageDirty = true;
    _pageDirty = true;
    _settingsPageDirty = true;
}

void DisplayModule::setBrewParams(float ratio, float dose, float target) {
    if (_brewRatio != ratio || _brewDose != dose || _brewTarget != target) {
        _mainPageDirty = true;
        _brewRatio = ratio; _brewDose = dose; _brewTarget = target;
        if (_currentPage == PAGE_SETTINGS) {
            _settingsPageDirty = true;
            _drawSettingsPage();
        }
        return;
    }
    _brewRatio = ratio; _brewDose = dose; _brewTarget = target;
}
void DisplayModule::setCalibrationFactor(float factor) {
    if (_calibrationFactor != factor) {
        _mainPageDirty = true;
        _calibrationFactor = factor;
        if (_currentPage == PAGE_SETTINGS) {
            _settingsPageDirty = true;
            _drawSettingsPage();
        }
        return;
    }
    _calibrationFactor = factor;
}
void DisplayModule::setTargetReached(bool reached) {
    if (_targetReached != reached) {
        _mainPageDirty = true;
    }
    _targetReached = reached;
}

void DisplayModule::setSettingsInputState(uint8_t selectedIndex, bool editing, const char* inputText) {
    if (selectedIndex > 1) selectedIndex = 1;
    if (inputText == nullptr) inputText = "";

    bool changed = _settingsSelectedIndex != selectedIndex || _settingsEditing != editing;
    for (int i = 0; i < (int)sizeof(_settingsInputText); i++) {
        if (_settingsInputText[i] != inputText[i]) {
            changed = true;
            break;
        }
        if (inputText[i] == '\0') break;
    }

    if (changed) {
        _settingsSelectedIndex = selectedIndex;
        _settingsEditing = editing;
        snprintf(_settingsInputText, sizeof(_settingsInputText), "%s", inputText);
        _settingsPageDirty = true;
        if (_currentPage == PAGE_SETTINGS) {
            _drawSettingsPage();
        }
    }
}

void DisplayModule::markMainPageDirty() {
    _mainPageDirty = true;
    _lastMiniCurveUpdateTime = 0;
    _lastMiniCurveCount = -1;
}

// ============================================================
// PAGE_MAIN 布局 240×135 — 基于精确字体高度确保零重叠：
// [0,  12]  标题 Font0 TC_DATUM y=2 + 分隔线 y=12
//        ↓ 9px
// [23, 47]  重量 FreeSerif18pt MC_DATUM y=29 ('0'字底 y=38)
//        ↓ 3px
// [50, 53]  进度条 h=3  fillRect(10, 50, 220, 3)
//        ↓ 6px
// [59, 76]  信息行 FreeSerif9pt TL_DATUM y=59 (含g字底 y=76)
//        ↓ 3px
// [79]      分隔线
//        ↓ 3px
//
// [82, 135] 迷你曲线 53px 黑墨细线
// ============================================================
void DisplayModule::_drawMainPage(float weight, float flowRate, TimerModule* timer, FlowCalculator* flowCalc) {
    unsigned long now = millis();
    unsigned long elapsedTenths = timer->getElapsedMs() / 100;
    bool running = timer->isRunning();
    int count = flowCalc->getHistoryCount();

    bool fullDirty = _mainPageDirty;
    bool weightDirty = fullDirty || fabs(weight - _lastWeightShown) >= WEIGHT_PRECISION;
    bool flowDirty = fullDirty || fabs(flowRate - _lastFlowShown) >= FLOW_PRECISION || running != _lastRunningShown;
    bool timeDirty = fullDirty || elapsedTenths != _lastElapsedShown || running != _lastRunningShown;
    bool targetDirty = fullDirty || _targetReached != _lastTargetShown;
    bool curveDirty = fullDirty ||
        (count >= 2 && (count != _lastMiniCurveCount || running) &&
         now - _lastMiniCurveUpdateTime >= CURVE_UPDATE_INTERVAL);

    if (!weightDirty && !flowDirty && !timeDirty && !targetDirty && !curveDirty) return;

    if (fullDirty) {
        M5.Lcd.drawFastHLine(0, 79, SCREEN_WIDTH, COLOR_DIVIDER);
    }

    if (weightDirty) {
        _clearArea(0, 13, SCREEN_WIDTH, 34);

        // === 重量 FreeSerif18pt MC_DATUM y=29 ('0': y=14→38) ===
        M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
        M5.Lcd.setTextDatum(MC_DATUM);
        M5.Lcd.setFont(&fonts::FreeSerif18pt7b);
        char wStr[10];
        dtostrf(weight, 5, 1, wStr);
        M5.Lcd.drawString(wStr, SCREEN_WIDTH / 2 - 8, 29);

        // "g" 单位
        M5.Lcd.setFont(&fonts::Font0);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
        M5.Lcd.setTextDatum(TL_DATUM);
        M5.Lcd.drawString("g", 196, 22);
    }

    if (weightDirty || targetDirty) {
        // === 进度条 y:50-53 ===
        _clearArea(0, 48, SCREEN_WIDTH, 5);
        if (_brewTarget > 0 && weight > 0) {
            float pct = weight / _brewTarget;
            if (pct > 1.0f) pct = 1.0f;
            M5.Lcd.fillRect(10, 50, 220, 3, COLOR_DIVIDER);
            uint16_t barColor = _targetReached ? COLOR_SUCCESS : COLOR_TEXT;
            M5.Lcd.fillRect(10, 50, (int)(220 * pct), 3, barColor);
        }
    }

    // === 信息行 FreeSerif9pt TL_DATUM y=59 (含g字底 y=76) ===
    uint16_t rowColor = running ? COLOR_TEXT : COLOR_TEXT_DIM;

    if (flowDirty) {
        _clearArea(0, 54, 128, 23);
        char buf[20];
        // flow（左）
        snprintf(buf, sizeof(buf), "flow %.1f", flowRate);
        M5.Lcd.setFont(&fonts::FreeSerif9pt7b);
        M5.Lcd.setTextColor(rowColor, COLOR_BG);
        M5.Lcd.setTextDatum(TL_DATUM);
        M5.Lcd.drawString(buf, 6, 59);

        // g/s（用 textWidth 精确测算位置，避免重叠）
        M5.Lcd.setTextColor(rowColor, COLOR_BG);
        int flowW = M5.Lcd.textWidth(buf);
        M5.Lcd.drawString("g/s", 6 + flowW + 3, 62);
    }

    if (timeDirty) {
        _clearArea(128, 54, SCREEN_WIDTH - 128, 23);
        M5.Lcd.setFont(&fonts::FreeSerif9pt7b);
        M5.Lcd.setTextColor(running ? COLOR_TEXT : COLOR_TEXT_DIM, COLOR_BG);
        M5.Lcd.setTextDatum(TR_DATUM);
        char timeBuffer[10];
        timer->formatTime(timeBuffer, sizeof(timeBuffer));
        M5.Lcd.drawString(timeBuffer, SCREEN_WIDTH - 6, 59);
    }

    if (curveDirty) {
        // === 迷你曲线 y:82-135 (53px) ===
        _clearArea(0, 79, SCREEN_WIDTH, 56);
        if (count >= 2) {
            float* flows = flowCalc->getFlowHistory();
            float maxFlow = flowCalc->getFlowMax();
            if (maxFlow < 2.0f) maxFlow = 2.0f;
            float yScale = 53 / maxFlow;
            int prevX = -1, prevY = -1;
            for (int i = 0; i < count; i++) {
                int idx = flowCalc->getChronologicalIndex(i);
                int px = (i * SCREEN_WIDTH) / count;
                float flow = flows[idx];
                if (flow < 0) flow = 0;
                if (flow > maxFlow) flow = maxFlow;
                int py = 82 + 53 - (int)(flow * yScale);
                py = constrain(py, 82, 135);
                if (prevX >= 0) {
                    M5.Lcd.drawLine(prevX, prevY, px, py, COLOR_TEXT);
                }
                prevX = px; prevY = py;
            }
        }
        _lastMiniCurveUpdateTime = now;
        _lastMiniCurveCount = count;
    }

    if (weightDirty) _lastWeightShown = weight;
    if (flowDirty) _lastFlowShown = flowRate;
    if (timeDirty) _lastElapsedShown = elapsedTenths;
    if (targetDirty || weightDirty) _lastTargetShown = _targetReached;
    if (flowDirty || timeDirty) _lastRunningShown = running;
    _mainPageDirty = false;
}

// === 重量曲线（纸墨风格）===
void DisplayModule::_drawWeightCurvePage(FlowCalculator* flowCalc) {
    int count = flowCalc->getHistoryCount();
    if (count < 2) return;

    int curveX = 25, curveY = 22, curveW = 195, curveH = 90;
    _clearArea(curveX - 2, curveY - 2, curveW + 4, curveH + 4);
    _drawGrid(curveX, curveY, curveW, curveH);

    float minW = flowCalc->getWeightMin();
    float maxW = flowCalc->getWeightMax();
    float range = maxW - minW;
    if (range < 10) range = 10;
    minW -= range * 0.1;
    maxW += range * 0.1;

    float* weights = flowCalc->getWeightHistory();
    float yScale = curveH / range;
    int prevX = -1, prevY = -1;

    for (int i = 0; i < count; i++) {
        int idx = flowCalc->getChronologicalIndex(i);
        int x = curveX + (i * curveW) / count;
        int y = curveY + curveH - (int)((weights[idx] - minW) * yScale);
        y = constrain(y, curveY, curveY + curveH);
        if (prevX >= 0) {
            M5.Lcd.drawLine(prevX, prevY, x, y, COLOR_TEXT);
        }
        prevX = x; prevY = y;
    }
    _drawAxisLabels(minW, maxW, curveX, curveY, curveH);
    _drawTimeAxis(flowCalc->getTimeMin(), flowCalc->getTimeMax(), curveX, curveY + curveH + 4, curveW);
}

// === 流量曲线 ===
void DisplayModule::_drawFlowCurvePage(FlowCalculator* flowCalc) {
    int count = flowCalc->getHistoryCount();
    if (count < 2) return;

    int curveX = 25, curveY = 22, curveW = 195, curveH = 90;
    _clearArea(curveX - 2, curveY - 2, curveW + 4, curveH + 4);
    _drawGrid(curveX, curveY, curveW, curveH);

    float minF = flowCalc->getFlowMin();
    float maxF = flowCalc->getFlowMax();
    float range = maxF - minF;
    if (range < 5) range = 5;
    minF -= range * 0.1;
    maxF += range * 0.1;
    if (minF > 0) minF = 0;

    float* flows = flowCalc->getFlowHistory();
    float yScale = curveH / range;
    int prevX = -1, prevY = -1;

    for (int i = 0; i < count; i++) {
        int idx = flowCalc->getChronologicalIndex(i);
        int x = curveX + (i * curveW) / count;
        int y = curveY + curveH - (int)((flows[idx] - minF) * yScale);
        y = constrain(y, curveY, curveY + curveH);
        if (prevX >= 0) {
            M5.Lcd.drawLine(prevX, prevY, x, y, COLOR_TEXT);
        }
        prevX = x; prevY = y;
    }
    _drawAxisLabels(minF, maxF, curveX, curveY, curveH);
    _drawTimeAxis(flowCalc->getTimeMin(), flowCalc->getTimeMax(), curveX, curveY + curveH + 4, curveW);
}

// === 设置页（纸墨印刷列表）===
void DisplayModule::_drawSettingsPage() {
    if (!_settingsPageDirty) return;

    _clearArea(0, 13, SCREEN_WIDTH, SCREEN_HEIGHT - 13);

    // 标题（5px Font0）
    M5.Lcd.setFont(&fonts::Font0);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.drawString("SETTINGS", SCREEN_WIDTH / 2, 2);
    _drawPageIndicator();
    M5.Lcd.drawFastHLine(0, 12, SCREEN_WIDTH, COLOR_DIVIDER);

    int y = 18;

    struct { const char* label; float val; bool editable; } items[] = {
        {"RATIO", _brewRatio, true},
        {"DOSE", _brewDose, true},
        {"TARGET", _brewTarget, false},
    };

    for (int i = 0; i < 3; i++) {
        bool selected = items[i].editable && _settingsSelectedIndex == i;
        if (selected) {
            M5.Lcd.fillRect(6, y + 8, 6, 2, COLOR_TEXT);
        }

        // 标签（左）
        M5.Lcd.setFont(&fonts::FreeSerif9pt7b);
        M5.Lcd.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, COLOR_BG);
        M5.Lcd.setTextDatum(TL_DATUM);
        M5.Lcd.drawString(items[i].label, 16, y);

        // 值（右）
        M5.Lcd.setFont(&fonts::FreeSerif12pt7b);
        M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
        M5.Lcd.setTextDatum(TR_DATUM);
        char buf[18];
        if (_settingsEditing && selected) {
            if (i == 0) {
                snprintf(buf, sizeof(buf), "1:%s", _settingsInputText);
            } else {
                snprintf(buf, sizeof(buf), "%s g", _settingsInputText);
            }
        } else if (i == 0) {
            snprintf(buf, sizeof(buf), "1:%.1f", items[i].val);
        } else if (i == 1) {
            snprintf(buf, sizeof(buf), "%.1f g", items[i].val);
        } else {
            snprintf(buf, sizeof(buf), "%.0f g", items[i].val);
        }
        M5.Lcd.drawString(buf, 224, y);

        // 分隔线
        M5.Lcd.drawFastHLine(16, y + 20, 208, COLOR_DIVIDER);
        y += 24;
    }

    // CAL
    M5.Lcd.setFont(&fonts::FreeSerif9pt7b);
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.drawString("CAL", 16, y);
    M5.Lcd.setFont(&fonts::FreeSerif12pt7b);
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    char calBuf[16];
    snprintf(calBuf, sizeof(calBuf), "%.2f", _calibrationFactor);
    M5.Lcd.drawString(calBuf, 66, y);

    _settingsPageDirty = false;
}

// ============================================================
// 背景绘制
// ============================================================
void DisplayModule::_drawMainBackground() {
    M5.Lcd.setFont(&fonts::Font0);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.drawString("POUR OVER", SCREEN_WIDTH / 2, 2);
    M5.Lcd.drawFastHLine(0, 12, SCREEN_WIDTH, COLOR_DIVIDER);
    _drawPageIndicator();
}

void DisplayModule::_drawCurveBackground(const __FlashStringHelper* title) {
    M5.Lcd.setFont(&fonts::Font0);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.drawString(title, SCREEN_WIDTH / 2, 2);
    M5.Lcd.drawFastHLine(0, 12, SCREEN_WIDTH, COLOR_DIVIDER);
    _drawPageIndicator();

    // 曲线框
    M5.Lcd.drawRect(25, 18, 195, 92, COLOR_DIVIDER);
}

void DisplayModule::_drawBottomHints() {
    // 底部按键提示已移除（用户要求取消）
}

// ============================================================
// 曲线绘制
// ============================================================
void DisplayModule::_drawGrid(int x, int y, int w, int h) {
    // 纸墨风格：极淡网格
    for (int i = 1; i < 4; i++) {
        int lineY = y + (i * h) / 4;
        M5.Lcd.drawFastHLine(x, lineY, w, COLOR_GRID);
    }
}

void DisplayModule::_drawAxisLabels(float minVal, float maxVal, int x, int y, int h) {
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    M5.Lcd.setTextSize(SMALL_FONT_SIZE);
    M5.Lcd.setTextDatum(TR_DATUM);
    char label[8];
    snprintf(label, sizeof(label), "%.0f", maxVal);
    M5.Lcd.drawString(label, x - 2, y + 2);
    snprintf(label, sizeof(label), "%.0f", minVal);
    M5.Lcd.drawString(label, x - 2, y + h - 8);
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

void DisplayModule::_drawPageIndicator() {
    int x = SCREEN_WIDTH - 35;
    for (int i = 0; i < 4; i++) {
        uint16_t c = (i == _currentPage) ? COLOR_TEXT : COLOR_TEXT_DIM;
        M5.Lcd.fillCircle(x + i * 8, 10, 2, c);
    }
}

void DisplayModule::_clearArea(int x, int y, int w, int h) {
    M5.Lcd.fillRect(x, y, w, h, COLOR_BG);
}
