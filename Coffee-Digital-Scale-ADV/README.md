# Coffee Digital Scale - Cardputer-ADV

手冲咖啡秤固件，基于 M5Stack Cardputer-adv (ESP32-S3) 和 HX711 称重模块。

## 功能特性

- ✅ 实时重量显示（精确到 0.1g）
- ✅ 移动平均滤波，稳定读数
- ✅ 自动去皮（启动时）
- ✅ 流量实时计算（g/s）
- ✅ 自动计时（重量变化触发）
- ✅ 重量/流量历史曲线
- ✅ SD 卡数据记录（CSV 格式）
- ✅ 校准参数保存/加载

## 硬件需求

- M5Stack Cardputer-adv
- HX711 模块
- 压力传感器（称重传感器）
- MicroSD 卡

## 接线说明

```
HX711 模块          Cardputer-ADV
─────────────────────────────────
VCC (红)     →      +5V
GND (黑)     →      GND
DT/DOUT (白) →      G2
SCK/CLK (绿) →      G1
```

## 依赖库

通过 Arduino Library Manager 安装：

- **M5Unified** - M5Stack 硬件抽象层
- **HX711_ADC** - HX711 称重模块驱动
- **ArduinoJson** - JSON 配置文件处理（可选）

## 编译和烧录

1. 安装 Arduino IDE 2.x
2. 添加 M5Stack 开发板支持：
   ```
   https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json
   ```
3. 安装依赖库
4. 选择开发板：`M5Stack Arduino` → `M5Stack-Core-ESP32-S3`
5. 打开 `coffee_scale.ino`
6. 编译并上传

## 使用说明

### 基本操作

1. **开机**：设备自动初始化并去皮
2. **放置咖啡杯**：显示当前重量
3. **开始注水**：重量超过 0.5g 时自动开始计时
4. **查看曲线**：按 BtnA 切换页面
5. **结束冲煮**：移除咖啡杯，计时自动重置

### 页面切换

按 BtnA 循环切换：
- 主界面（重量 + 流量 + 时间 + 迷你曲线）
- 重量曲线页面
- 流量曲线页面
- 设置页面

### 数据记录

- 每次冲煮自动创建 CSV 文件
- 文件保存在 `/coffee_data/` 目录
- 文件名格式：`brew_XXXXX.csv`

CSV 格式：
```csv
timestamp_ms,weight_g,flow_rate_g_per_s,elapsed_s
0,0.0,0.0,0.00
100,0.1,0.5,0.10
200,0.3,1.2,0.20
...
```

### 校准

1. 进入设置页面
2. 选择校准选项
3. 放置已知重量的砝码
4. 输入砝码重量
5. 自动计算校准因子

## 配置参数

在 `config.h` 中修改：

```cpp
#define AUTO_START_THRESHOLD  0.5f   // 自动计时触发阈值 (g)
#define RESET_THRESHOLD       0.3f   // 归零重置阈值 (g)
#define FILTER_WINDOW_SIZE    5      // 移动平均窗口大小
#define HISTORY_BUFFER_SIZE   240    // 曲线数据点数
```

## 项目结构

```
Coffee-Digital-Scale-ADV/
├── coffee_scale.ino      ← 主程序
├── config.h              ← 全局配置
├── WeightSensor.h/cpp    ← 称重传感器模块
├── FlowCalculator.h/cpp  ← 流量计算模块
├── TimerModule.h/cpp     ← 计时器模块
├── DisplayModule.h/cpp   ← 显示模块
├── StorageModule.h/cpp   ← 存储模块
└── README.md             ← 本文件
```

## 故障排除

### HX711 无法初始化
- 检查接线是否正确
- 确认 G2/G1 引脚未被占用
- 尝试更换 HX711 模块

### 重量读数不稳定
- 检查传感器连接是否牢固
- 增加滤波窗口大小
- 使用屏蔽线减少干扰

### SD 卡无法识别
- 确认 SD 卡格式为 FAT32
- 检查 SD 卡是否正确插入
- 尝试更换 SD 卡

### 屏幕闪烁
- 降低刷新频率
- 减少全屏重绘
- 检查电源稳定性

## License

MIT License
