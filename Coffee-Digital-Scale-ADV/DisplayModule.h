#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

#include <M5Unified.h>
#include "config.h"
#include "FlowCalculator.h"
#include "TimerModule.h"

class DisplayModule {
public:
    DisplayModule();

    // 初始化
    void init();

    // 页面切换
    enum Page { PAGE_MAIN, PAGE_WEIGHT_CURVE, PAGE_FLOW_CURVE, PAGE_SETTINGS };
    void setPage(Page page);
    Page getCurrentPage();

    // 更新显示
    void update(float weight, float flowRate, TimerModule* timer, FlowCalculator* flowCalc);

    // 输入处理
    void handleInput();

    // 显示消息
    void showMessage(const char* message, int durationMs = 1000);

private:
    Page _currentPage;
    Page _lastPage;
    unsigned long _lastUpdateTime;
    unsigned long _lastCurveUpdateTime;

    // 页面渲染
    void _drawMainPage(float weight, float flowRate, TimerModule* timer, FlowCalculator* flowCalc);
    void _drawWeightCurvePage(FlowCalculator* flowCalc);
    void _drawFlowCurvePage(FlowCalculator* flowCalc);
    void _drawSettingsPage();

    // 背景绘制（只在页面切换时调用）
    void _drawMainBackground();
    void _drawCurveBackground(const char* title, const char* yLabel);

    // UI 组件
    void _drawTitle(const char* title);
    void _drawStatusBar(float flowRate, TimerModule* timer);
    void _drawMiniCurve(FlowCalculator* flowCalc, int x, int y, int w, int h);
    void _drawCurve(float* data, int count, int x, int y, int w, int h,
                    float minVal, float maxVal, uint16_t color);
    void _drawGrid(int x, int y, int w, int h, int xDivisions, int yDivisions);
    void _drawAxisLabels(float minVal, float maxVal, int x, int y, int h, const char* unit);
    void _drawTimeAxis(unsigned long timeMin, unsigned long timeMax, int x, int y, int w);

    // 页面指示器
    void _drawPageIndicator();

    // 清除区域
    void _clearArea(int x, int y, int w, int h);
};

#endif
