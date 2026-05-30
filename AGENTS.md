# AFK Agent 工作规范

本文件指导自主代理独立完成 issues。开始前先读 CLAUDE.md 了解项目上下文。

## 工作原则

### 不假设，不猜测

- 引脚号、库行为、屏幕尺寸都是确定的，从 CLAUDE.md 获取
- 如果某行为不确定，查 HX711_ADC / M5Unified 文档
- 如果有多种实现方式，在 PR 中说明选择理由

### 最小代码

- 只实现 Issue Acceptance criteria 中要求的功能
- 不添加"以防万一"的配置项或抽象层
- 不为单次使用创建工具函数
- 如果 200 行能变成 50 行，简化它

### 精准修改

- 只改必须改的，不"顺手改进"其他代码
- 匹配现有代码风格
- 你引入的不再使用的代码，要清理掉

### 目标驱动

- 每个 Issue 有明确的 Acceptance criteria
- 实现后逐条核对是否满足
- 编译通过 ≠ 功能正确，要验证行为

## 工作流程

```
1. 阅读 Issue 全文（What to build + Acceptance criteria）
2. 检查依赖（Blocked by 的 issues 是否已完成）
3. 实现代码（先骨架后填充，增量编译）
4. 验收检查（逐条核对 Acceptance criteria）
5. 代码审查（检查禁止事项清单）
6. 提交 PR（使用规范的 commit message）
```

## 模块实现规范

### WeightSensor

- `getWeight()` 返回滤波后的值，不暴露原始 ADC
- 移动平均窗口大小 = FILTER_WINDOW_SIZE
- `setCalibrationFactor()` 必须立即生效

### FlowCalculator

- 使用环形缓冲区存储历史数据，大小 = HISTORY_BUFFER_SIZE
- 流量计算使用滑动窗口，窗口 = FLOW_WINDOW_MS
- `update()` 每次调用记录一个数据点

### TimerModule

- 时间精度毫秒级，使用 `millis()`
- `checkAutoStart()` 基于重量阈值判断
- 自动启动阈值 = AUTO_START_THRESHOLD，重置阈值 = RESET_THRESHOLD

### DisplayModule

- 不直接访问其他模块内部状态
- 只通过公开接口获取数据
- 屏幕刷新只更新变化区域，不全屏重绘

### StorageModule

- CSV 格式：`timestamp_ms,weight_g,flow_rate_g_per_s,elapsed_s`
- 文件名：`brew_YYYYMMDD_HHMMSS.csv`
- 校准参数保存为 JSON

## 禁止事项

- ❌ 添加 PRD Out of Scope 中的功能（BLE、WiFi、触摸屏等）
- ❌ 修改已通过测试的其他模块
- ❌ 引入新的依赖库（除非 Issue 明确要求）
- ❌ 在主循环中使用 `delay()`
- ❌ 忽略编译警告
- ❌ 硬编码校准因子
- ❌ 无限增长的数据结构
- ❌ 全屏重绘

## 编译检查清单

提交 PR 前必须通过：

- [ ] 编译成功，无错误
- [ ] 无编译警告
- [ ] 所有新函数在 .h 文件中有声明
- [ ] 没有未使用的 #include
- [ ] 没有未使用的变量
- [ ] 主循环中没有 `delay()`
- [ ] 动态内存分配有边界检查

## Commit 规范

```
feat: [Issue #X] 简短描述

详细说明（可选）

Closes #X
```

示例：
```
feat: [Issue #2] 实现 WeightSensor 模块

- 添加移动平均滤波
- 实现 tare 功能
- 屏幕显示滤波后的重量

Closes #2
```

## 好代码 vs 坏代码

**好：**
```cpp
float getWeight();  // 明确的接口
#define AUTO_START_THRESHOLD 0.5f  // 有意义的常量

void loop() {
    unsigned long now = millis();
    if (now - lastUpdate >= UPDATE_INTERVAL) {
        updateWeight();
        lastUpdate = now;
    }
}
```

**坏：**
```cpp
float raw_adc_value;  // 暴露内部实现
if (weight > 0.5) { ... }  // 魔法数字

void loop() {
    updateWeight();
    delay(100);  // 阻塞循环
}
```
