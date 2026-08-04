# Minesweeper（扫雷）

基于 **C++ / EasyX 图形库** 实现的经典扫雷游戏，具备完整的 GUI 界面、三种难度、自动展开与快捷展开等特性。

---

## ✨ 功能特性

- **三种难度**
  - 初级：9 × 9，10 颗雷
  - 中级：16 × 16，40 颗雷
  - 高级：30 × 16，99 颗雷
- **首次点击安全**：第一下点击的格子及其周围 3×3 范围内保证无雷，开局不会被秒杀。
- **左键翻开 / 右键标记**：标准扫雷交互。
- **空白区域自动展开（Flood Fill）**：点到 0 时自动连锁翻开相邻空白区。
- **快捷展开（Chording）**：在已翻开的数字格上点击，当周围旗数等于该数字时，自动翻开其余安全格。
- **计时器与剩余雷数**：顶部信息栏实时显示已用时长（上限 999 秒）与剩余雷数。
- **胜 / 负判定**：翻开所有安全格即胜（并自动补旗）；踩雷即负，并揭示所有未标记的雷。
- **返回 / 重玩按钮**：随时返回难度选择或重开本局。

---

## 🛠 技术栈

| 类别 | 说明 |
| --- | --- |
| **编程语言** | C++20（`LanguageStandard=stdcpp20`） |
| **图形界面** | [EasyX](https://easyx.cn/) 图形库（基于 Win32 GDI 封装，提供 `graphics.h`） |
| **构建系统** | MSBuild（Visual Studio 解决方案 / 项目文件） |
| **编译器 / 平台工具集** | MSVC `v145`（对应 Visual Studio 2019 / 2022） |
| **目标平台** | Windows（Win32 与 x64 两个平台配置），Windows SDK 10.0 |
| **字符集** | Unicode（Win32）/ MultiByte（x64 Debug） |
| **子系统** | 控制台（`/SUBSYSTEM:CONSOLE`）+ EasyX 独立绘图窗口 |
| **核心标准库** | `<vector>`、`<string>`、`<algorithm>`、`<random>`（Mersenne Twister `mt19937` 随机数）、`<chrono>`、`<ctime>`、`<conio.h>` |

> 随机数采用 `std::mt19937` 并以 `std::chrono::steady_clock` 高精度时钟播种，保证每局布局不可预测。

---

## 📋 运行环境要求

- **操作系统**：Windows 10 / 11（x64 或 x86）
- **开发环境**：Visual Studio 2019 或 2022（需安装「使用 C++ 的桌面开发」工作负载）
- **EasyX 图形库**：需从 [easyx.cn](https://easyx.cn/) 下载并安装（安装程序会自动将头文件与库注入 VS 的 MSVC 目录）

---

## 🚀 构建与运行

1. 安装 **Visual Studio 2019/2022** 与 **EasyX**（见上方要求）。
2. 用 Visual Studio 打开 `Minesweeper.slnx`。
3. 选择配置（建议 `Debug | x64` 或 `Debug | Win32`）。
4. 执行 **生成 → 重新生成解决方案**（首次构建会刷新 `.vs` 缓存与中间产物）。
5. 按 **Ctrl + F5**（开始执行不调试）或 **F5** 运行。

> 若 Visual Studio 此前打开过旧项目，建议先关闭再重新打开 `Minesweeper.slnx`，避免 `.vs` 缓存被旧进程覆盖。

构建产物位于：
- 可执行文件：`x64\Debug\Minesweeper.exe`（或对应平台 / 配置目录下）
- 中间文件：`Minesweeper\`（由 `$(ProjectName)` 自动派生）

---

## 🎮 操作说明

| 操作 | 效果 |
| --- | --- |
| **鼠标左键** | 翻开格子；点击已翻开的数字可触发快捷展开 |
| **鼠标右键** | 标记 / 取消标记旗子（仅棋盘区域，信息栏不响应） |
| **返回按钮**（左上） | 回到难度选择界面 |
| **重玩按钮**（右上） | 以当前难度重新开始 |
| **ESC 键** | 退出游戏 |

---

## 📁 项目结构

```
Minesweeper/
├── Minesweeper.cpp            # 游戏全部源码（单文件实现）
├── Minesweeper.slnx           # 解决方案文件
├── Minesweeper.vcxproj        # 项目配置（MSBuild）
├── Minesweeper.vcxproj.filters# 解决方案资源管理器筛选器
├── Minesweeper.vcxproj.user   # 用户级本地配置（自动派生）
├── x64\Debug\                 # 构建输出目录（含 Minesweeper.exe）（自动派生）  
└── Minesweeper\               # 中间构建目录（自动派生）
```

---

## 🧩 实现要点

- **`Minesweeper` 类**封装了棋盘状态（`board`）、难度、计时与全部渲染逻辑，职责清晰。
- **安全布雷**（`generateMines`）：在玩家首次点击后，于排除 3×3 安全区后的候选格中均匀随机布雷。
- **空白展开**（`revealCell`）：使用迭代式 BFS 队列，对相邻雷数为 0 的区域做连锁翻开，避免递归过深。
- **双缓冲绘图**（`BeginBatchDraw` / `EndBatchDraw`）：减少闪烁，提升交互流畅度。
- **局部重绘**：计时器仅重绘顶部信息栏，降低每帧开销。

---

## ❓ 常见问题

- **编译报错“无法打开 graphics.h”**：未安装 EasyX，请从 [easyx.cn](https://easyx.cn/) 安装对应 VS 版本的库。
- **窗口一闪而过**：请使用「开始执行(不调试)」Ctrl+F5，或在代码中保留控制台读取；本作已通过菜单循环保持窗口常驻。
- **为什么用控制台子系统**：项目以 `_CONSOLE` 链接，但图形界面由 EasyX 单独创建窗口，二者互不冲突。

---

*本项目为 C++ 课程实践作品，使用 EasyX 完成 GUI 设计与扫雷核心逻辑。*
