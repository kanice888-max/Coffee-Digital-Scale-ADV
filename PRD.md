# PRD: Cardputer-ADV 手冲咖啡秤固件

## Problem Statement

作为手冲咖啡爱好者，我需要一个实时显示冲煮过程数据的工具，以便精确控制注水量、流速和时间，从而提升咖啡萃取的一致性和品质。目前市面上的专业咖啡秤价格高昂，而我希望利用 M5Stack Cardputer-adv 硬件平台，配合 HX711 称重模块，打造一款低成本、可定制的手冲咖啡秤。

## Solution

基于 M5Stack Cardputer-adv（ESP32-S3）开发一款 Arduino 固件，连接 HX711 称重模块和压力传感器，提供以下核心能力：

- **实时重量显示**：高精度称重，支持自动去皮
- **流量/速度曲线**：实时计算并绘制注水流量变化
- **自动计时**：去皮后检测到重量变化自动开始计时
- **数据记录**：将每次冲煮数据保存到 SD 卡，便于后续分析

## User Stories

1. As a 手冲咖啡用户, I want 将咖啡壶放在秤上并自动去皮, so that 我可以立即开始冲煮而无需手动操作
2. As a 手冲咖啡用户, I want 实时查看当前注水重量（精确到 0.1g）, so that 我可以精确控制注水量
3. As a 手冲咖啡用户, I want 在开始注水时自动启动计时器, so that 我不需要分心去按按钮
4. As a 手冲咖啡用户, I want 查看当前的注水流量（g/s）, so that 我可以调整注水速度以达到理想的萃取效果
5. As a 手冲咖啡用户, I want 查看重量随时间变化的曲线图, so that 我可以直观了解冲煮过程的节奏
6. As a 手冲咖啡用户, I want 查看流量随时间变化的曲线图, so that 我可以分析注水模式的一致性
7. As a 手冲咖啡用户, I want 使用已知重量的砝码校准传感器, so that 称重结果准确可靠
8. As a 手冲咖啡用户, I want 在设置界面调整校准因子, so that 我可以随时重新校准设备
9. As a 手冲咖啡用户, I want 将冲煮数据自动保存到 SD 卡, so that 我可以事后分析每次冲煮的数据
10. As a 手冲咖啡用户, I want 数据以 CSV 格式保存, so that 我可以用 Excel 或其他工具进行分析
11. As a 手冲咖啡用户, I want 查看当前冲煮的总时间, so that 我可以控制整体冲煮时长
12. As a 手冲咖啡用户, I want 在重量归零时自动重置计时器, so that 每次冲煮都是独立的计时
13. As a 手冲咖啡用户, I want 使用物理按键切换界面（主界面/曲线/设置）, so that 我可以方便地在不同视图间切换
14. As a 手冲咖啡用户, I want 在小屏幕上看到清晰的大字体重量数字, so that 我在冲煮时能快速读取数据
15. As a 手冲咖啡用户, I want 重量读数经过滤波处理, so that 显示的数值不会因传感器噪声而跳动
16. As a 手冲咖啡用户, I want 流量计算使用滑动窗口平均, so that 流量显示平滑且有参考价值
17. As a 手冲咖啡用户, I want 设备启动时自动执行去皮, so that 我无需在每次开机后手动去皮
18. As a 手冲咖啡用户, I want 在主界面同时看到重量、流量和时间, so that 我无需切换界面就能获取所有关键信息
19. As a 手冲咖啡用户, I want 曲线图的时间轴和数值轴自动缩放, so that 无论冲煮时长如何都能完整显示曲线
20. As a 手冲咖啡用户, I want 设置重量变化阈值来触发自动计时, so that 我可以根据不同场景调整灵敏度

## Implementation Decisions

### 硬件平台
- **MCU**: ESP32-S3（M5Stack Cardputer-adv）
- **屏幕**: 1.14寸 IPS TFT，135×240 像素，ST7789V2 驱动
- **称重模块**: HX711 24-bit ADC + 压力传感器
- **HX711 接线**: VCC→+5V, GND→GND, DT/DOUT→G2, SCK/CLK→G1
- **存储**: MicroSD 卡（用于数据记录）

### 开发框架
- **Arduino** + M5Unified 库
- **HX711_ADC** 库（而非 HX711 库，提供更稳定的读数和内置滤波）
- **TFT_eSPI** 屏幕驱动（M5Stack 内置）

### 模块划分

#### 1. WeightSensor 模块
- 封装 HX711 初始化、校准、读取
- 提供滤波后的稳定重量值
- 接口：`init()`, `tare()`, `getWeight()`, `setCalibrationFactor()`, `calibrateWithKnownWeight()`
- 深度模块：内部实现移动平均滤波，对外只暴露干净的重量接口

#### 2. FlowCalculator 模块
- 基于重量差和时间差计算瞬时流量
- 维护滑动窗口平均流量
- 存储历史数据点用于曲线绘制
- 接口：`update(weight, timestamp)`, `getFlowRate()`, `getFlowHistory()`, `getWeightHistory()`
- 深度模块：内部管理数据环形缓冲区，对外只提供查询接口

#### 3. TimerModule 模块
- 毫秒级精度计时
- 支持自动启动（重量变化检测）和手动控制
- 接口：`start()`, `stop()`, `reset()`, `getElapsed()`, `checkAutoStart(weight)`
- 自动计时逻辑：去皮后重量 > 阈值 → 自动启动；重量 < 归零阈值 → 自动重置

#### 4. DisplayModule 模块
- 管理多页面 UI（主界面、曲线界面、设置界面）
- 处理界面切换逻辑
- 接口：`showMainPage()`, `showCurvePage()`, `showSettingsPage()`, `handleInput()`
- 深度模块：每个页面的渲染逻辑封装在内部，对外只暴露页面切换和输入处理

#### 5. StorageModule 模块
- SD 卡数据记录
- CSV 格式写入（时间戳、重量、流量）
- 校准参数持久化
- 接口：`beginNewSession()`, `logDataPoint(weight, flow, time)`, `saveCalibration()`, `loadCalibration()`

### UI 设计决策
- **配色**: 深色背景（#1a1a1a）+ 暖白文字（#f5f5dc）+ 琥珀色曲线（#ff8c00）
- **主界面**: 同时显示重量（大字体）、流量、时间、简化曲线预览
- **曲线界面**: 上半部分重量曲线，下半部分流量曲线，时间轴自动缩放
- **界面切换**: 使用 Cardputer 物理按键

### 数据格式
- CSV 文件头：`timestamp_ms, weight_g, flow_rate_g_per_s, elapsed_s`
- 校准参数存储为 JSON 格式

### 关键阈值
- 自动计时触发阈值：0.5g（可配置）
- 归零重置阈值：0.3g（可配置）
- 滤波窗口大小：5-10 个采样
- 流量计算滑动窗口：约 1 秒

## Testing Decisions

由于这是嵌入式固件项目，测试策略如下：

### 可测试的模块
1. **FlowCalculator** — 纯数据处理模块，可在主机环境测试
   - 测试：给定重量序列，验证流量计算结果
   - 测试：滑动窗口平均的正确性
   - 测试：历史数据环形缓冲区的边界条件

2. **TimerModule** — 逻辑简单，可独立测试
   - 测试：自动启动逻辑（重量变化触发）
   - 测试：自动重置逻辑（重量归零触发）
   - 测试：手动控制的正确性

3. **StorageModule** — CSV 格式生成可测试
   - 测试：CSV 格式正确性
   - 测试：校准参数的序列化/反序列化

### 不适合自动化测试的模块
- **WeightSensor** — 依赖硬件，需要实际传感器验证
- **DisplayModule** — 依赖屏幕硬件，需要实际设备验证

### 测试方法
- 使用 Arduino 的 `AUnit` 或 `Unity` 测试框架
- FlowCalculator 和 TimerModule 可以编译为独立的测试程序，在主机上运行

## Out of Scope

- **蓝牙数据传输**：本期不实现 BLE 数据同步到手机 App
- **WiFi 功能**：不实现 OTA 更新或网络同步
- **触摸屏支持**：Cardputer-adv 无触摸屏，仅使用物理按键
- **多语言支持**：仅支持英文界面
- **电池管理**：不实现电池电量显示或低功耗模式
- **配方管理**：不存储或管理咖啡冲煮配方
- **声音提示**：不实现蜂鸣器或语音提示功能
- **自动注水控制**：不控制水泵或电磁阀

## Further Notes

### 开发顺序建议
1. **Phase 1**: WeightSensor + 基本重量显示（验证硬件连接）
2. **Phase 2**: FlowCalculator + TimerModule + 主界面（核心功能）
3. **Phase 3**: 曲线绘制 + 界面切换（UI 完善）
4. **Phase 4**: StorageModule + 数据记录（数据持久化）
5. **Phase 5**: 校准界面 + 设置界面（用户配置）

### 已知风险
- **HX711 干扰**：ESP32 的 WiFi/BLE 可能引入噪声，必要时在采样期间禁用无线
- **屏幕刷新率**：135×240 小屏幕需要优化绘制频率，避免闪烁
- **内存限制**：曲线数据需要使用环形缓冲区，避免内存溢出
