#ifndef DISPLAY_MODULE_H
#define DISPLAY_MODULE_H

#include <M5Unified.h>
#include "config.h"
#include "FlowCalculator.h"
#include "TimerModule.h"

class DisplayModule {
public:
    DisplayModule();

    void init();
    enum Page { PAGE_MAIN, PAGE_WEIGHT_CURVE, PAGE_FLOW_CURVE, PAGE_SETTINGS };
    void setPage(Page page);
    Page getCurrentPage();
    void update(float weight, float flowRate, TimerModule* timer, FlowCalculator* flowCalc);
    void handleInput();
    void showMessage(const String& message, int durationMs = 1000);

private:
    Page _currentPage;
    unsigned long _lastUpdateTime;
    unsigned long _lastCurveUpdateTime;

    void _drawMainPage(float weight, float flowRate, TimerModule* timer, FlowCalculator* flowCalc);
    void _drawWeightCurvePage(FlowCalculator* flowCalc);
    void _drawFlowCurvePage(FlowCalculator* flowCalc);
    void _drawSettingsPage();

    void _drawMainBackground();
    void _drawCurveBackground(const __FlashStringHelper* title);

    void _drawTitle(const __FlashStringHelper* title);
    void _drawStatusBar(float flowRate, TimerModule* timer);
    void _drawStatusIndicators(TimerModule* timer);
    void _drawMiniCurve(FlowCalculator* flowCalc, int x, int y, int w, int h);
    void _drawGrid(int x, int y, int w, int h);
    void _drawAxisLabels(float minVal, float maxVal, int x, int y, int h);
    void _drawTimeAxis(unsigned long timeMin, unsigned long timeMax, int x, int y, int w);
    void _drawPageIndicator();
    void _clearArea(int x, int y, int w, int h);
};

#endif
