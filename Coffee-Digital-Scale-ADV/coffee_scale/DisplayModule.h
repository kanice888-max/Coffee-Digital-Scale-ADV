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
    void showMessage(const __FlashStringHelper* message, int durationMs = 0);
    void setBrewParams(float ratio, float dose, float target);
    void setCalibrationFactor(float factor);
    void setTargetReached(bool reached);
    void setSettingsInputState(uint8_t selectedIndex, bool editing, const char* inputText);
    void markMainPageDirty();

private:
    Page _currentPage;
    unsigned long _lastUpdateTime;
    unsigned long _lastCurveUpdateTime;
    unsigned long _lastMiniCurveUpdateTime;

    void _drawMainPage(float weight, float flowRate, TimerModule* timer, FlowCalculator* flowCalc);
    void _drawWeightCurvePage(FlowCalculator* flowCalc);
    void _drawFlowCurvePage(FlowCalculator* flowCalc);
    void _drawSettingsPage();

    void _drawMainBackground();
    void _drawCurveBackground(const __FlashStringHelper* title);

    void _drawBottomHints();
    void _drawGrid(int x, int y, int w, int h);
    void _drawAxisLabels(float minVal, float maxVal, int x, int y, int h);
    void _drawTimeAxis(unsigned long timeMin, unsigned long timeMax, int x, int y, int w);
    void _drawPageIndicator();
    void _clearArea(int x, int y, int w, int h);

    // 冲煮参数缓存
    float _brewRatio;
    float _brewDose;
    float _brewTarget;
    float _calibrationFactor;
    bool _targetReached;
    float _lastWeightShown;
    float _lastFlowShown;
    unsigned long _lastElapsedShown;
    bool _lastTargetShown;
    bool _lastRunningShown;
    bool _mainPageDirty;
    bool _pageDirty;
    uint8_t _settingsSelectedIndex;
    bool _settingsEditing;
    bool _settingsPageDirty;
    char _settingsInputText[16];
    int _lastMiniCurveCount;
};

#endif
