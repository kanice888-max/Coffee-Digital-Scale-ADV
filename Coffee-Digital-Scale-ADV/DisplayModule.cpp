#include "DisplayModule.h"

DisplayModule::DisplayModule() {
    _currentPage = PAGE_MAIN;
    _lastPage = PAGE_MAIN;
    _lastUpdateTime = 0;
    _lastCurveUpdateTime = 0;
}

void DisplayModule::init() {
    M5.Lcd.setRotation(1);  // 横屏模式：240x135
    M5.Lcd.fillScreen(COLOR_BG);
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.setTextSize(NORMAL_FONT_SIZE);

    _drawMainBackground();
}

void DisplayModule::setPage(Page page) {
    if (page != _currentPage) {
        _lastPage = _currentPage;
        _currentPage = page;

        // 切换页面时重绘背景
        M5.Lcd.fillScreen(COLOR_BG);

        switch (_currentPage) {
            case PAGE_MAIN:
                _drawMainBackground();
                break;
            case PAGE_WEIGHT_CURVE:
                _drawCurveBackground(F("Weight Curve"), F("g"));
                break;
            case PAGE_FLOW_CURVE:
                _drawCurveBackground(F("Flow Curve"), F("g/s"));
                break;
            case PAGE_SETTINGS:
                _drawSettingsPage();
                break;
        }

        _drawPageIndicator();
    }
}

DisplayModule::Page DisplayModule::getCurrentPage() {
    return _currentPage;
}

void DisplayModule::update(float weight, float flowRate, TimerModule* timer, FlowCalculator* flowCalc) {
    unsigned long now = millis();

    // 检查输入
    handleInput();

    // 根据当前页面更新显示
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

        case PAGE_SETTINGS:
            // 设置页面静态显示，不需要更新
            break;
    }
}

void DisplayModule::handleInput() {
    M5.update();

    // 使用键盘输入（Cardputer 没有传统的 BtnA/BtnB/BtnC）
    if (M5.Keyboard.isChange()) {
        if (M5.Keyboard.isPressed()) {
            // 按 '1' 键切换页面
            if (M5.Keyboard.isKeyPressed('1')) {
                Page nextPage = (Page)((_currentPage + 1) % 4);
                setPage(nextPage);
            }
            // 按 '2' 键手动去皮（需要在主程序中处理）
            // 按 '3' 键开始/停止计时（需要在主程序中处理）
        }
    }
}

void DisplayModule::showMessage(const String& message, int durationMs) {
    // 保存当前状态
    Page savedPage = _currentPage;

    // 显示消息
    M5.Lcd.fillScreen(COLOR_BG);
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.setTextSize(NORMAL_FONT_SIZE);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.drawString(message, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

    if (durationMs > 0) {
        delay(durationMs);
    }

    // 恢复
    M5.Lcd.fillScreen(COLOR_BG);
    switch (savedPage) {
        case PAGE_MAIN:
            _drawMainBackground();
            break;
        case PAGE_WEIGHT_CURVE:
            _drawCurveBackground(F("Weight Curve"), F("g"));
            break;
        case PAGE_FLOW_CURVE:
            _drawCurveBackground(F("Flow Curve"), F("g/s"));
            break;
        case PAGE_SETTINGS:
            _drawSettingsPage();
            break;
    }
    _drawPageIndicator();
}

// ============================================================
// 主界面布局（240x135 横屏）
// ============================================================
// ┌────────────────────────────────────────────────────────┐
// │  ☕ Pour Over Scale                      [● ○ ○ ○]   │ 0-18: 标题栏
// ├────────────────────────────────────────────────────────┤
// │                                                        │
// │                    125.6 g                             │ 18-75: 重量显示（大字体）
// │                                                        │
// ├────────────────────────────────────────────────────────┤
// │  Flow: 3.2 g/s                       Time: 01:23.4   │ 75-95: 状态栏
// ├────────────────────────────────────────────────────────┤
// │  ▁▂▃▅▆▇▆▅▃▂▁▁▂▃▅▆▇▆▅▃▂▁                              │ 95-135: 迷你曲线
// └────────────────────────────────────────────────────────┘

void DisplayModule::_drawMainPage(float weight, float flowRate, TimerModule* timer, FlowCalculator* flowCalc) {
    // === 重量显示区域 ===
    _clearArea(0, TITLE_HEIGHT + 2, SCREEN_WIDTH, 53);

    // 重量数值（大字体，居中）
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.setTextSize(WEIGHT_FONT_SIZE);
    M5.Lcd.setTextDatum(MC_DATUM);

    char weightStr[12];
    dtostrf(weight, 5, 1, weightStr);
    strcat(weightStr, " g");
    M5.Lcd.drawString(weightStr, SCREEN_WIDTH / 2, TITLE_HEIGHT + 28);

    // === 状态栏 ===
    _drawStatusBar(flowRate, timer);

    // === 状态指示器 ===
    _drawStatusIndicators(timer);

    // === 迷你曲线 ===
    _drawMiniCurve(flowCalc, 0, 97, SCREEN_WIDTH, 36);
}

void DisplayModule::_drawWeightCurvePage(FlowCalculator* flowCalc) {
    int count = flowCalc->getHistoryCount();
    if (count < 2) return;

    // 曲线区域
    int curveX = 35;
    int curveY = TITLE_HEIGHT + 5;
    int curveW = SCREEN_WIDTH - 40;
    int curveH = SCREEN_HEIGHT - TITLE_HEIGHT - 25;

    // 清除曲线区域
    _clearArea(curveX, curveY, curveW, curveH);

    // 绘制网格
    _drawGrid(curveX, curveY, curveW, curveH, 6, 4);

    // 绘制重量曲线
    float* weights = flowCalc->getWeightHistory();
    float minW = flowCalc->getWeightMin();
    float maxW = flowCalc->getWeightMax();

    // 添加边距
    float range = maxW - minW;
    if (range < 10) range = 10;
    minW -= range * 0.1;
    maxW += range * 0.1;

    _drawCurve(weights, count, curveX, curveY, curveW, curveH, minW, maxW, COLOR_ACCENT);

    // 绘制 Y 轴标签
    _drawAxisLabels(minW, maxW, 0, curveY, curveH);

    // 绘制时间轴
    _drawTimeAxis(flowCalc->getTimeMin(), flowCalc->getTimeMax(), curveX, SCREEN_HEIGHT - 18, curveW);
}

void DisplayModule::_drawFlowCurvePage(FlowCalculator* flowCalc) {
    int count = flowCalc->getHistoryCount();
    if (count < 2) return;

    // 曲线区域
    int curveX = 35;
    int curveY = TITLE_HEIGHT + 5;
    int curveW = SCREEN_WIDTH - 40;
    int curveH = SCREEN_HEIGHT - TITLE_HEIGHT - 25;

    // 清除曲线区域
    _clearArea(curveX, curveY, curveW, curveH);

    // 绘制网格
    _drawGrid(curveX, curveY, curveW, curveH, 6, 4);

    // 绘制流量曲线
    float* flows = flowCalc->getFlowHistory();
    float minF = flowCalc->getFlowMin();
    float maxF = flowCalc->getFlowMax();

    // 添加边距
    float range = maxF - minF;
    if (range < 5) range = 5;
    minF -= range * 0.1;
    maxF += range * 0.1;

    // 确保包含 0
    if (minF > 0) minF = 0;

    _drawCurve(flows, count, curveX, curveY, curveW, curveH, minF, maxF, COLOR_ACCENT);

    // 绘制 Y 轴标签
    _drawAxisLabels(minF, maxF, 0, curveY, curveH);

    // 绘制时间轴
    _drawTimeAxis(flowCalc->getTimeMin(), flowCalc->getTimeMax(), curveX, SCREEN_HEIGHT - 18, curveW);
}

void DisplayModule::_drawSettingsPage() {
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.setTextSize(NORMAL_FONT_SIZE);
    M5.Lcd.setTextDatum(TL_DATUM);

    M5.Lcd.drawString(F("Settings"), 10, TITLE_HEIGHT + 10);
    M5.Lcd.drawString(F("Press '1' to return"), 10, TITLE_HEIGHT + 40);
}

void DisplayModule::_drawMainBackground() {
    // 标题栏背景
    M5.Lcd.fillRect(0, 0, SCREEN_WIDTH, TITLE_HEIGHT, COLOR_BG);

    // 标题
    _drawTitle(F("Pour Over Scale"));

    // 分隔线
    M5.Lcd.drawFastHLine(0, TITLE_HEIGHT, SCREEN_WIDTH, COLOR_TEXT_DIM);
    M5.Lcd.drawFastHLine(0, 75, SCREEN_WIDTH, COLOR_TEXT_DIM);
    M5.Lcd.drawFastHLine(0, 95, SCREEN_WIDTH, COLOR_TEXT_DIM);
}

void DisplayModule::_drawCurveBackground(const __FlashStringHelper* title, const __FlashStringHelper* yLabel) {
    // 标题栏
    _drawTitle(title);

    // Y 轴标签
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.drawString(yLabel, 5, TITLE_HEIGHT + 5);
}

void DisplayModule::_drawTitle(const __FlashStringHelper* title) {
    M5.Lcd.setTextColor(COLOR_ACCENT, COLOR_BG);
    M5.Lcd.setTextSize(NORMAL_FONT_SIZE);
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.drawString(title, 5, 3);

    // 咖啡图标
    M5.Lcd.drawString(F("☕"), SCREEN_WIDTH - 20, 3);
}

void DisplayModule::_drawStatusBar(float flowRate, TimerModule* timer) {
    // 清除状态栏区域
    _clearArea(0, 77, SCREEN_WIDTH, 16);

    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextDatum(TL_DATUM);

    // 流量
    char flowStr[16];
    snprintf(flowStr, sizeof(flowStr), "Flow: %.1f g/s", flowRate);
    M5.Lcd.drawString(flowStr, 5, 80);

    // 计时
    M5.Lcd.setTextDatum(TR_DATUM);
    String timeStr = F("Time: ") + timer->getFormattedTime();
    M5.Lcd.drawString(timeStr, SCREEN_WIDTH - 5, 80);
}

void DisplayModule::_drawMiniCurve(FlowCalculator* flowCalc, int x, int y, int w, int h) {
    int count = flowCalc->getHistoryCount();
    if (count < 2) return;

    // 清除曲线区域
    _clearArea(x, y, w, h);

    // 绘制重量迷你曲线
    float* weights = flowCalc->getWeightHistory();
    float minW = flowCalc->getWeightMin();
    float maxW = flowCalc->getWeightMax();

    float range = maxW - minW;
    if (range < 5) range = 5;
    minW -= range * 0.1;
    maxW += range * 0.1;

    _drawCurve(weights, count, x, y, w, h, minW, maxW, COLOR_ACCENT);
}

void DisplayModule::_drawCurve(float* data, int count, int x, int y, int w, int h,
                                float minVal, float maxVal, uint16_t color) {
    if (count < 2) return;

    float range = maxVal - minVal;
    if (range < 0.01f) range = 1.0f;  // 防止除零

    // 预计算缩放因子
    float yScale = h / range;

    for (int i = 0; i < count - 1; i++) {
        int x1 = x + (i * w) / count;
        int x2 = x + ((i + 1) * w) / count;
        int y1 = y + h - (int)((data[i] - minVal) * yScale);
        int y2 = y + h - (int)((data[i + 1] - minVal) * yScale);

        // 限制在区域内
        y1 = constrain(y1, y, y + h);
        y2 = constrain(y2, y, y + h);

        M5.Lcd.drawLine(x1, y1, x2, y2, color);
    }
}

void DisplayModule::_drawGrid(int x, int y, int w, int h, int xDivisions, int yDivisions) {
    M5.Lcd.drawRect(x, y, w, h, COLOR_TEXT_DIM);

    // 垂直网格线（虚线）
    for (int i = 1; i < xDivisions; i++) {
        int lineX = x + (i * w) / xDivisions;
        for (int lineY = y; lineY < y + h; lineY += 4) {
            M5.Lcd.drawPixel(lineX, lineY, COLOR_TEXT_DIM);
        }
    }

    // 水平网格线（虚线）
    for (int i = 1; i < yDivisions; i++) {
        int lineY = y + (i * h) / yDivisions;
        for (int lineX = x; lineX < x + w; lineX += 4) {
            M5.Lcd.drawPixel(lineX, lineY, COLOR_TEXT_DIM);
        }
    }
}

void DisplayModule::_drawAxisLabels(float minVal, float maxVal, int x, int y, int h) {
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextDatum(TR_DATUM);

    // 最大值
    char label[8];
    snprintf(label, sizeof(label), "%.0f", maxVal);
    M5.Lcd.drawString(label, x + 28, y);

    // 最小值
    snprintf(label, sizeof(label), "%.0f", minVal);
    M5.Lcd.drawString(label, x + 28, y + h - 8);
}

void DisplayModule::_drawTimeAxis(unsigned long timeMin, unsigned long timeMax, int x, int y, int w) {
    M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextDatum(TC_DATUM);

    // 时间范围（秒）
    float startSec = timeMin / 1000.0f;
    float endSec = timeMax / 1000.0f;

    char label[8];

    // 起始时间
    snprintf(label, sizeof(label), "%.0fs", startSec);
    M5.Lcd.drawString(label, x, y);

    // 结束时间
    snprintf(label, sizeof(label), "%.0fs", endSec);
    M5.Lcd.drawString(label, x + w, y);
}

void DisplayModule::_drawPageIndicator() {
    // 在右上角绘制页面指示器
    int indicatorY = 5;
    int indicatorX = SCREEN_WIDTH - 35;

    for (int i = 0; i < 4; i++) {
        uint16_t color = (i == _currentPage) ? COLOR_ACCENT : COLOR_TEXT_DIM;
        M5.Lcd.fillCircle(indicatorX + i * 8, indicatorY, 3, color);
    }
}

void DisplayModule::_drawStatusIndicators(TimerModule* timer) {
    // 在状态栏下方绘制状态指示器
    int indicatorY = 93;
    int indicatorX = 5;

    // 计时状态指示器
    if (timer->isRunning()) {
        M5.Lcd.fillCircle(indicatorX, indicatorY, 3, COLOR_SUCCESS);
        M5.Lcd.setTextColor(COLOR_SUCCESS, COLOR_BG);
    } else {
        M5.Lcd.fillCircle(indicatorX, indicatorY, 3, COLOR_TEXT_DIM);
        M5.Lcd.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    }
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.drawString(timer->isRunning() ? "RUNNING" : "STOPPED", indicatorX + 8, indicatorY - 4);
}

void DisplayModule::_clearArea(int x, int y, int w, int h) {
    M5.Lcd.fillRect(x, y, w, h, COLOR_BG);
}
