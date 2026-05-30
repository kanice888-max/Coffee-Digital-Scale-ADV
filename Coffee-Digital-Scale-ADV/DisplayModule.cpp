#include "DisplayModule.h"

DisplayModule::DisplayModule() {
    _currentPage = PAGE_MAIN;
    _lastPage = PAGE_MAIN;
    _lastUpdateTime = 0;
    _lastCurveUpdateTime = 0;
}

void DisplayModule::init() {
    M5.Lcd.setRotation(1);  // 横屏模式
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
                _drawCurveBackground(F("Weight"), F("g"));
                break;
            case PAGE_FLOW_CURVE:
                _drawCurveBackground(F("Flow"), F("g/s"));
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
    // 按钮 A 切换页面
    if (M5.BtnA.wasPressed()) {
        Page nextPage = (Page)((_currentPage + 1) % 4);
        setPage(nextPage);
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
            _drawCurveBackground(F("Weight"), F("g"));
            break;
        case PAGE_FLOW_CURVE:
            _drawCurveBackground(F("Flow"), F("g/s"));
            break;
        case PAGE_SETTINGS:
            _drawSettingsPage();
            break;
    }
    _drawPageIndicator();
}

void DisplayModule::_drawMainPage(float weight, float flowRate, TimerModule* timer, FlowCalculator* flowCalc) {
    // 清除重量显示区域
    _clearArea(0, TITLE_HEIGHT, SCREEN_WIDTH, 50);

    // 显示重量（大字体）
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.setTextSize(WEIGHT_FONT_SIZE);
    M5.Lcd.setTextDatum(MC_DATUM);

    // 使用 F() 宏减少内存使用
    char weightStr[12];
    dtostrf(weight, 5, 1, weightStr);
    strcat(weightStr, " g");
    M5.Lcd.drawString(weightStr, SCREEN_WIDTH / 2, TITLE_HEIGHT + 25);

    // 更新状态栏
    _drawStatusBar(flowRate, timer);

    // 更新迷你曲线
    _drawMiniCurve(flowCalc, 0, 95, SCREEN_WIDTH, 40);
}

void DisplayModule::_drawWeightCurvePage(FlowCalculator* flowCalc) {
    int count = flowCalc->getHistoryCount();
    if (count < 2) return;

    // 清除曲线区域
    _clearArea(30, TITLE_HEIGHT + 5, SCREEN_WIDTH - 35, SCREEN_HEIGHT - TITLE_HEIGHT - 25);

    // 绘制网格
    _drawGrid(30, TITLE_HEIGHT + 5, SCREEN_WIDTH - 35, SCREEN_HEIGHT - TITLE_HEIGHT - 25, 5, 4);

    // 绘制重量曲线
    float* weights = flowCalc->getWeightHistory();
    float minW = flowCalc->getWeightMin();
    float maxW = flowCalc->getWeightMax();

    // 添加一些边距
    float range = maxW - minW;
    if (range < 10) range = 10;
    minW -= range * 0.1;
    maxW += range * 0.1;

    _drawCurve(weights, count, 30, TITLE_HEIGHT + 5,
               SCREEN_WIDTH - 35, SCREEN_HEIGHT - TITLE_HEIGHT - 25,
               minW, maxW, COLOR_ACCENT);

    // 绘制 Y 轴标签
    _drawAxisLabels(minW, maxW, 0, TITLE_HEIGHT + 5, SCREEN_HEIGHT - TITLE_HEIGHT - 25);

    // 绘制时间轴
    _drawTimeAxis(flowCalc->getTimeMin(), flowCalc->getTimeMax(),
                  30, SCREEN_HEIGHT - 20, SCREEN_WIDTH - 35);
}

void DisplayModule::_drawFlowCurvePage(FlowCalculator* flowCalc) {
    int count = flowCalc->getHistoryCount();
    if (count < 2) return;

    // 清除曲线区域
    _clearArea(30, TITLE_HEIGHT + 5, SCREEN_WIDTH - 35, SCREEN_HEIGHT - TITLE_HEIGHT - 25);

    // 绘制网格
    _drawGrid(30, TITLE_HEIGHT + 5, SCREEN_WIDTH - 35, SCREEN_HEIGHT - TITLE_HEIGHT - 25, 5, 4);

    // 绘制流量曲线
    float* flows = flowCalc->getFlowHistory();
    float minF = flowCalc->getFlowMin();
    float maxF = flowCalc->getFlowMax();

    // 添加一些边距
    float range = maxF - minF;
    if (range < 5) range = 5;
    minF -= range * 0.1;
    maxF += range * 0.1;

    // 确保包含 0
    if (minF > 0) minF = 0;

    _drawCurve(flows, count, 30, TITLE_HEIGHT + 5,
               SCREEN_WIDTH - 35, SCREEN_HEIGHT - TITLE_HEIGHT - 25,
               minF, maxF, COLOR_ACCENT);

    // 绘制 Y 轴标签
    _drawAxisLabels(minF, maxF, 0, TITLE_HEIGHT + 5, SCREEN_HEIGHT - TITLE_HEIGHT - 25);

    // 绘制时间轴
    _drawTimeAxis(flowCalc->getTimeMin(), flowCalc->getTimeMax(),
                  30, SCREEN_HEIGHT - 20, SCREEN_WIDTH - 35);
}

void DisplayModule::_drawSettingsPage() {
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.setTextSize(NORMAL_FONT_SIZE);
    M5.Lcd.setTextDatum(TL_DATUM);

    M5.Lcd.drawString(F("Settings"), 10, TITLE_HEIGHT + 10);
    M5.Lcd.drawString(F("Press BtnA to return"), 10, TITLE_HEIGHT + 40);
}

void DisplayModule::_drawMainBackground() {
    // 标题栏
    _drawTitle(F("Pour Over Scale"));

    // 分隔线
    M5.Lcd.drawLine(0, TITLE_HEIGHT, SCREEN_WIDTH, TITLE_HEIGHT, COLOR_TEXT_DIM);
    M5.Lcd.drawLine(0, 70, SCREEN_WIDTH, 70, COLOR_TEXT_DIM);
    M5.Lcd.drawLine(0, 93, SCREEN_WIDTH, 93, COLOR_TEXT_DIM);
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
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.drawString(title, SCREEN_WIDTH / 2, 2);
}

void DisplayModule::_drawStatusBar(float flowRate, TimerModule* timer) {
    // 清除状态栏区域
    _clearArea(0, 72, SCREEN_WIDTH, 20);

    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.setTextSize(NORMAL_FONT_SIZE);
    M5.Lcd.setTextDatum(TL_DATUM);

    // 流量 - 使用 F() 宏
    char flowStr[16];
    snprintf(flowStr, sizeof(flowStr), "Flow:%.1fg/s", flowRate);
    M5.Lcd.drawString(flowStr, 5, 74);

    // 计时 - 使用 F() 宏
    M5.Lcd.setTextDatum(TR_DATUM);
    String timeStr = F("Time:") + timer->getFormattedTime();
    M5.Lcd.drawString(timeStr, SCREEN_WIDTH - 5, 74);
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

    // 预计算缩放因子，避免重复除法
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
    // 在右下角绘制页面指示器
    int indicatorY = SCREEN_HEIGHT - 8;
    int indicatorX = SCREEN_WIDTH - 30;

    for (int i = 0; i < 4; i++) {
        uint16_t color = (i == _currentPage) ? COLOR_ACCENT : COLOR_TEXT_DIM;
        M5.Lcd.fillCircle(indicatorX + i * 8, indicatorY, 3, color);
    }
}

void DisplayModule::_clearArea(int x, int y, int w, int h) {
    M5.Lcd.fillRect(x, y, w, h, COLOR_BG);
}
