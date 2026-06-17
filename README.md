# xiaoxiong_music_player

小熊音乐播放器初始框架。当前基于 **Qt 6 + QML + CMake**，仅包含一个可运行的空白窗口，用于验证工具链与构建流程。

## 环境要求

| 依赖 | 版本 / 说明 |
|------|-------------|
| Qt | 6.9.3，MSVC 2022 64-bit（默认路径 `C:/Qt/6.9.3/msvc2022_64`） |
| 编译器 | Visual Studio 2022（含「使用 C++ 的桌面开发」） |
| CMake | 4.3.3+ |
| 系统 | Windows 10/11 x64 |

## 项目结构

```
xiaoxiong_music_player/
├── CMakeLists.txt      # CMake 构建配置
├── src/main.cpp        # 程序入口
├── qml/Main.qml        # 主界面
├── .vscode/            # Cursor / VS Code 任务与调试配置
└── build/              # 构建输出（已 gitignore，需本地生成）
```

编译产物：`build/Debug/xxMusic.exe`

## 推荐扩展（Cursor / VS Code）

打开项目后按提示安装，或手动安装：

- **CMake Tools** — CMake 配置与构建
- **clangd** — C++ 代码补全与诊断
- **CodeLLDB** — 调试（推荐）
- **C/C++** — 可选，配合调试器使用

---

## 命令行使用

在项目根目录打开终端（PowerShell）。

### 1. 仅 CMake 配置（不编译）

首次 clone 或修改 `CMakeLists.txt` 后执行：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/6.9.3/msvc2022_64
```

### 2. 仅编译（不运行）

```powershell
cmake --build build --config Debug
```

编译成功后会自动执行 `windeployqt`，将 Qt / QML 依赖复制到 `build/Debug/`。

### 3. 编译并运行

```powershell
cmake --build build --config Debug
.\build\Debug\xxMusic.exe
```

`cmake --build` 为增量构建，无代码改动时几乎瞬间完成。

### 4. 编译并调试

命令行调试需使用 Visual Studio 打开生成的解决方案，或使用 Cursor 的调试功能（见下文）。

### 手动部署 Qt 依赖（通常不需要）

若运行提示缺少 DLL，可手动执行：

```powershell
C:\Qt\6.9.3\msvc2022_64\bin\windeployqt.exe --debug --qmldir qml build\Debug\xxMusic.exe
```

### 从零开始（删除 build 后）

```powershell
Remove-Item -Recurse -Force build
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/6.9.3/msvc2022_64
cmake --build build --config Debug
.\build\Debug\xxMusic.exe
```

---

## Cursor / VS Code 使用

### 任务（Tasks）

按 `Ctrl+Shift+P`，输入 `Tasks: Run Task`，选择对应任务：

| 任务 | 作用 |
|------|------|
| **CMake: Configure** | 仅生成 CMake 工程，不编译 |
| **CMake: Build** | 仅编译 |
| **Build and Run** | 编译后启动程序（默认构建任务） |
| **Run xxMusic** | 直接运行，不调用 cmake |
| **Qt: Deploy** | 手动执行 windeployqt（一般不必，编译时已自动部署） |

快捷方式：

- `Ctrl+Shift+P` → `Tasks: Run Build Task` → **Build and Run**
- 若快捷键冲突，可在键盘快捷方式中将 `Run Build Task` 绑定到 `Ctrl+Shift+B`

### CMake Tools 状态栏

底部状态栏可操作：

1. **Configure** — 等价于 `CMake: Configure`
2. **Build** — 等价于 `CMake: Build`
3. 构建类型选 **Debug**，Kit 选 **Visual Studio 2022 amd64**

### 调试（Run and Debug）

1. 打开 `src/main.cpp`，点击行号左侧设置断点
2. 左侧「运行和调试」面板，选择 **Debug (CodeLLDB)**（推荐）
3. 按 **F5** 开始调试（会先增量编译，再启动调试器）

| 配置 | 说明 |
|------|------|
| **Debug (CodeLLDB)** | 使用 CodeLLDB 调试，推荐 |
| **Debug (MSVC)** | 使用 MSVC 调试器，需安装 C/C++ 扩展 |

---

## 常见问题

**找不到 Qt**

确认 Qt 安装路径，并同步修改 `CMakeLists.txt`、`.vscode/settings.json`、`.vscode/tasks.json`、`.clangd` 中的路径。

**运行提示缺少 `Qt6Qmld.dll` 等**

重新编译一次（触发 POST_BUILD 的 windeployqt），或手动执行上文「手动部署」命令。

**代码编辑区 Qt 头文件报红**

先完成一次编译，再执行 `Ctrl+Shift+P` → `clangd: Restart language server`。

**`Ctrl+Shift+B` 打开了浏览器**

Cursor 快捷键冲突。在键盘快捷方式中移除浏览器绑定，将 `Run Build Task` 设为 `Ctrl+Shift+B`。

---

## 修改 Qt 路径

若 Qt 未安装在默认路径，需更新以下文件中的 `C:/Qt/6.9.3/msvc2022_64`：

- `CMakeLists.txt`
- `.vscode/settings.json`
- `.vscode/tasks.json`
- `.clangd`
- `.vscode/c_cpp_properties.json`
