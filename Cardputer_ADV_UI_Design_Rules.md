# Cardputer ADV 界面设计规则

> 适用对象：M5Stack Cardputer ADV 固件 UI / 小工具界面 / 传感器界面 / 菜单系统  
> 推荐技术栈：PlatformIO + Arduino Framework + M5Cardputer + M5GFX / M5Canvas  
> 设计定位：不要把 Cardputer ADV 当成“小手机”，而要当成“带键盘的小型工具终端”。

---

## 1. 设备与界面基础限制

Cardputer ADV 的界面开发不是网页前端开发，而是嵌入式屏幕 UI 开发。

不能直接使用：

- HTML
- CSS
- React
- Vue
- DOM
- Tauri 前端界面
- 浏览器布局逻辑

通常使用：

- C++ / Arduino
- M5Cardputer
- M5GFX
- M5Canvas
- 少量 bitmap / 图标资源

核心限制：

```txt
屏幕尺寸：240 × 135 px
显示面积很小
Flash / RAM 有限
不适合复杂动效
不适合大量图片
不适合复杂中文字体
```

---

## 2. 产品定位

Cardputer ADV 更适合做：

- 状态显示器
- 传感器仪表盘
- 小型菜单系统
- 串口 / 网络 / LoRa 工具
- 称重工具
- 红外遥控器
- 小型游戏
- 命令行风格工具
- 硬件控制面板

不适合做：

- 手机 App 式复杂界面
- 多层弹窗系统
- 大量文字阅读
- 复杂列表管理
- 玻璃拟态 / 毛玻璃 UI
- 大量阴影、模糊和透明叠层
- 高帧率复杂动效

设计原则：

```txt
少即是多
信息优先
按键明确
状态清晰
每屏只做一件事
```

---

## 3. 推荐整体布局

建议使用固定三段式布局：

```txt
┌────────────────────────┐
│ 顶部状态栏 / 标题区       │ 16 ~ 20 px
├────────────────────────┤
│ 主内容区                 │ 90 ~ 100 px
├────────────────────────┤
│ 底部按键提示区            │ 16 ~ 20 px
└────────────────────────┘
```

### 顶部区域

用于显示：

- 当前应用标题
- 连接状态
- 电池状态
- 模式状态
- 简短错误提示

示例：

```txt
Weight Scale        BAT 86%
```

### 主内容区域

用于显示当前页面最重要的信息，例如：

```txt
128.4 g
```

或：

```txt
LoRa Ready
Signal: -68 dBm
```

### 底部区域

用于显示键盘操作提示：

```txt
Enter: OK     G0: Back
```

或：

```txt
←/→ Select    Enter Run
```

---

## 4. 信息层级规则

每个界面最多分为 3 个信息层级：

### 一级信息

当前屏幕最重要的内容。

例如：

- 重量数值
- 当前菜单项
- 连接状态
- 运行结果

建议使用较大字号或居中显示。

### 二级信息

辅助说明。

例如：

- 单位
- 模式
- 传感器状态
- 当前页码

### 三级信息

操作提示或状态脚注。

例如：

- Enter: Confirm
- G0: Back
- Fn + ← / →: Switch

规则：

```txt
一屏只突出一个核心信息
不要同时让多个内容抢视觉焦点
底部提示尽量固定
错误信息要短
```

---

## 5. 字体与文字规则

### 推荐文字风格

- 英文和数字优先
- 中文尽量短
- 少用长句
- 多用符号和缩写
- 重要数字要大
- 操作提示要固定在底部

### 中文使用建议

中文字体在嵌入式设备中很占空间。建议：

- 高频固定文案可转 bitmap
- 中文词汇尽量控制在少量
- 功能菜单可用英文或中英混合
- 不要加载完整中文字体库，除非确实需要

示例：

```txt
推荐：
Weight
128.4 g
Enter: Tare

不推荐：
当前重量测量结果为 128.4 克，请按确认键进行去皮操作
```

---

## 6. 颜色规则

屏幕小，颜色不宜太多。

建议每个应用使用：

```txt
背景色：1 个
主色：1 个
辅助色：1 个
警告色：1 个
文字色：1 ~ 2 个
```

### 推荐用法

- 深色背景 + 高对比文字
- 主操作用高亮色
- 当前选中项使用反色或色块
- 错误状态使用醒目颜色
- 不依赖微弱阴影区分层级

### 避免

- 大面积渐变
- 复杂透明叠层
- 低对比文字
- 多个相近颜色
- 依赖毛玻璃 / 模糊区分层级

---

## 7. 图标与图形规则

适合使用：

- 线性小图标
- 简单 bitmap
- 像素风图标
- 几何图形
- 小型状态符号

不适合使用：

- 高精度插画
- 大面积贴图
- 复杂 3D 图标
- 大量透明 PNG
- 多帧序列动画

图标尺寸建议：

```txt
状态栏图标：8 ~ 14 px
菜单图标：16 ~ 24 px
主视觉图标：32 ~ 64 px
```

---

## 8. 动效规则

ADV 可以做动效，但要克制。

适合：

- 菜单滑动
- 选中项反色
- 闪烁提示
- 简单 loading
- 进度条
- 数字变化
- 小图标帧动画

不适合：

- 毛玻璃模糊
- 大阴影
- 多层透明动画
- 高斯模糊
- 大量粒子
- 高帧率复杂缓动
- 大面积图片序列帧

建议动效时长：

```txt
菜单切换：100 ~ 200 ms
提示闪烁：300 ~ 600 ms
Loading 刷新：100 ~ 300 ms
错误提示停留：1000 ~ 2000 ms
```

---

## 9. 按键交互规则

Cardputer ADV 有实体键盘，所以界面应该围绕按键设计。

### 常用按键映射建议

```txt
Enter：确认 / 执行
G0 / BtnA：返回 / 退出
← / →：左右切换
↑ / ↓：上下选择
Fn：辅助功能
Esc：取消，可选
数字键：快捷选择
字母键：输入或快捷命令
```

### 页面设计要求

每个页面都应该明确告诉用户：

- 如何确认
- 如何返回
- 如何切换
- 当前是否可输入
- 当前是否正在运行任务

底部提示示例：

```txt
Enter: Start     G0: Back
```

```txt
←/→ Mode     Enter OK
```

```txt
Fn: Menu     G0 Exit
```

---

## 10. 页面结构建议

### 主菜单页

适合卡片式或列表式。

```txt
TOOLS
> Weight Scale
  LoRa Msg
  IR Remote

Enter: Run     G0: Back
```

### 仪表盘页

适合传感器或状态工具。

```txt
Weight Scale        BAT 86%

        128.4 g

Enter: Tare     G0: Back
```

### 设置页

设置项不要太多，一屏 3 ~ 5 个即可。

```txt
SETTINGS
> Unit: g
  Auto Tare: Off
  Brightness: 80%

Enter: Edit     G0: Back
```

### 运行中页面

必须明确显示状态。

```txt
Sending...

LoRa Packet #12

G0: Cancel
```

### 错误页面

错误文案要短。

```txt
ERROR

Sensor not found

G0: Back
```

---

## 11. 绘制实现规则

不要频繁全屏重绘，尽量只更新变化区域。

推荐：

- 使用 `M5Canvas` 做局部缓冲
- 页面状态变化时再重绘
- 数值变化区域单独刷新
- 控制刷新频率
- 避免长时间阻塞

基础结构建议：

```cpp
void loop() {
  M5Cardputer.update();

  handleInput();
  updateState();
  renderUI();
}
```

不要大量使用：

```cpp
delay(2000);
```

建议改成基于 `millis()` 的非阻塞更新。

---

## 12. 性能与资源规则

### 图片资源

- 全屏图片尽量少
- 不要大量使用 PNG
- 能用代码画的就不要用图
- 图标尽量转小 bitmap
- 避免长动画序列帧

### 字体资源

- 不要塞完整中文字体
- 固定中文可转图片
- 数字和英文使用内置字体
- 大字号只用于核心信息

### 刷新策略

- 不要每帧全屏 fill
- 不要无意义高频刷新
- 动画需要限制帧率
- 静态页面只在状态变化时重绘

---

## 13. 推荐 UI 风格方向

适合 ADV 的风格：

- 极简工具 UI
- 像素风
- 终端风
- 复古掌机风
- 单色高对比风
- 工业仪表风
- 简洁状态卡片
- 小型控制面板

不建议：

- 手机 App 风
- 桌面端玻璃拟态
- 复杂品牌视觉
- 大量插画风 UI
- 过多装饰性元素

---

## 14. 给 Codex 的开发规则

可以在项目根目录创建 `AGENTS.md`，让 Codex 按这些规则开发：

```md
# Cardputer ADV UI Development Rules

This is a M5Stack Cardputer ADV firmware project.

Use:
- PlatformIO
- Arduino framework
- M5Cardputer
- M5GFX / M5Canvas

Hardware constraints:
- Display: 240x135 px
- Small embedded device, no HTML/CSS/React
- Keep UI simple and readable
- Avoid large images, heavy animation, blur, shadows, and large fonts
- Avoid loading full Chinese font libraries unless absolutely required
- Prefer simple shapes, short labels, icons, and high-contrast layouts

Input rules:
- Always call M5Cardputer.update() in loop()
- Avoid blocking delay()
- Top G0 button is BtnA
- Front keyboard uses M5Cardputer.Keyboard
- Enter confirms
- G0 / BtnA goes back or exits
- Show key hints at the bottom of each screen

UI layout rules:
- Use a top status/title bar
- Use a large central content area
- Use a bottom key-hint bar
- Each screen should have one primary purpose
- Do not overfill the 240x135 display
- Keep text short

Rendering rules:
- Use M5Canvas when useful
- Redraw only when state changes
- Avoid unnecessary full-screen refresh
- Keep animations short and simple

Code structure:
- Keep UI, input, and app logic separated
- Each app should be a separate state or module
- Keep main.cpp simple
- After changes, run PlatformIO build and fix compile errors
```

---

## 15. Codex 提示词模板

### 生成基础 UI 框架

```txt
这是一个 M5Stack Cardputer ADV 项目。请使用 PlatformIO + Arduino framework + M5Cardputer + M5GFX 创建一个小屏 UI 框架。

要求：
1. 屏幕尺寸按 240x135 设计
2. 使用顶部标题栏、中间主内容区、底部按键提示栏
3. Enter 作为确认键
4. G0 / BtnA 作为返回键
5. 主循环必须调用 M5Cardputer.update()
6. 不要使用 delay() 做长时间阻塞
7. UI、输入、页面状态要拆分模块
8. main.cpp 保持简洁
9. 完成后运行 pio run，并根据编译错误修复
```

### 生成称重工具界面

```txt
请为 Cardputer ADV 生成一个称重工具界面。

界面要求：
1. 顶部显示 Weight Scale 和电池状态占位
2. 中间大字号显示重量数值，例如 128.4 g
3. 底部显示 Enter: Tare 和 G0: Back
4. Enter 触发 tare 去皮
5. G0 返回主菜单
6. 不要使用复杂图片、阴影、模糊或动画
7. 保持 240x135 小屏幕可读性
8. 代码要模块化
```

### 生成菜单系统

```txt
请为 Cardputer ADV 生成一个简单菜单系统。

要求：
1. 一屏最多显示 4 个菜单项
2. 当前选中项用反色或色块显示
3. 上下键切换选项
4. Enter 进入应用
5. G0 返回
6. 底部始终显示按键提示
7. 不要做复杂动效
8. 使用 M5Cardputer.Keyboard 和 BtnA
```

---

## 16. 界面检查清单

开发完成后，用下面清单检查：

```txt
[ ] 一屏是否只突出一个主要信息？
[ ] 是否能在 240x135 上清楚阅读？
[ ] 底部是否有明确按键提示？
[ ] G0 / BtnA 是否能返回或退出？
[ ] Enter 是否用于确认？
[ ] 是否避免了复杂阴影、模糊和透明？
[ ] 是否没有使用大量图片资源？
[ ] 是否避免加载完整中文字体？
[ ] 是否在 loop() 中调用 M5Cardputer.update()？
[ ] 是否避免了长时间 delay()？
[ ] 是否只在必要时重绘界面？
[ ] main.cpp 是否足够简洁？
[ ] UI、输入、业务逻辑是否拆分？
```

---

## 17. 总结

Cardputer ADV 的界面设计重点不是“炫”，而是：

```txt
小屏清晰
状态明确
按键直接
反馈及时
资源克制
```

最推荐的界面方向是：

```txt
极简仪表盘
工具终端
状态卡片
小型菜单系统
像素风控制面板
```

最不推荐的方向是：

```txt
手机 App 化
复杂动效化
玻璃拟态化
图片堆叠化
长文本阅读化
```
