# Xiaoxiong Music Player（小熊音乐播放器）

基于 **Qt 6 + QML + C++17** 的跨平台桌面音乐播放器。支持本地曲库扫描、在线搜索与播放、歌词同步、封面缓存、播放列表与「我喜欢」歌单等功能。

## 功能概览

### 本地播放

- 递归扫描目录，支持 `mp3`、`flac`、`wav`、`m4a`、`aac`、`ogg`、`wma`、`ape`
- 曲库索引持久化，内嵌元数据解析（标题、艺术家、专辑、时长）
- 播放 / 暂停、进度拖动、音量调节
- 播放模式：顺序、列表循环、单曲循环、随机
- 播放队列管理（侧栏抽屉）

### 在线能力（Meting API）

- 在线搜索歌曲（网易云等平台，经 Meting 镜像）
- 在线流媒体播放与 URL 解析
- 在线歌词检索（Meting 搜索 + LRC 下载）
- 在线封面拉取与本地缓存
- 在线歌曲下载到本地目录（默认 `{Music}/Downloads`）

### 歌词

- 优先加载同名 `.lrc` sidecar 文件
- 其次读取本地歌词缓存
- 无本地歌词时自动在线检索；成功后写入缓存，下次无需再搜
- 播放浮层内歌词高亮、点击跳转进度
- 支持手动选择歌词文件

### 播放列表与收藏

- 自定义播放列表（创建、重命名、删除、增删曲目）
- 系统歌单 **「我喜欢」**（红心收藏，持久化）
- 从曲库 / 在线结果加入播放列表或收藏

### 界面

- 侧栏导航：**曲库 · 播放列表 · 我喜欢 · 设置**
- 底部播放控制栏；点击封面或歌词按钮进入 **Now Playing** 浮层（封面 + 歌词）
- 主题 Token 体系（`qml/theme/Theme.qml`），SVG 图标可着色

### 设置

- 主题 / 语言（预留）、默认扫描目录、在线下载目录
- Meting API 镜像列表（每行一个，失败时按顺序切换）
- 配置保存在 `settings.json`，与曲库、歌单同目录

---

## 环境要求

| 依赖 | 说明 |
|------|------|
| **Qt** | 6.x（开发环境示例：6.9.3），需安装 **Multimedia**、**Quick**、**QuickControls2**、**Network** 组件 |
| **CMake** | 4.3.3+ |
| **C++ 编译器** | 支持 C++17（Windows：Visual Studio 2022 x64；Linux/macOS：对应 Qt Kit 的编译器） |
| **系统** | 主要在 Windows 10/11 x64 开发与测试；架构上可移植至 Linux / macOS |

---

## 项目结构

```
xiaoxiong_music_player/
├── CMakeLists.txt          # 构建与 QML 模块配置
├── assets/icons/           # SVG 图标（源码，需纳入 Git）
├── qml/
│   ├── Main.qml            # 主窗口与导航
│   ├── pages/              # 曲库、播放列表、我喜欢、设置等页面
│   ├── components/         # 播放栏、歌词面板、队列、Now Playing 浮层
│   └── theme/Theme.qml     # 设计 Token
├── src/
│   ├── main.cpp
│   ├── ui_bridge/          # QML ↔ C++ 桥接（ApplicationController）
│   ├── player/             # 播放核心
│   ├── library/            # 扫描、曲库、元数据
│   ├── playlist/           # 播放列表与收藏
│   ├── lyrics/             # LRC 解析与歌词服务
│   ├── search/             # 本地过滤 + 在线搜索
│   ├── network/            # Meting API 与流 URL 解析
│   ├── artwork/            # 封面缓存与扫描后预拉
│   ├── download/           # 在线下载
│   └── settings/           # settings.json 读写
├── tests/unit/             # 单元测试
└── build/                  # 构建输出（已 gitignore）
```

编译产物：`build/Debug/xxMusic.exe`（Debug 为控制台子系统，便于查看 `qDebug` 输出）

---

## 构建与运行

### Windows（PowerShell）

**首次配置：**

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/6.9.3/msvc2022_64
```

**编译并运行：**

```powershell
cmake --build build --config Debug
.\build\Debug\xxMusic.exe
```

编译成功后 **POST_BUILD** 会自动执行 `windeployqt`，将 Qt / QML 依赖部署到 `build/Debug/`。

**运行单元测试：**

```powershell
cd build
ctest -C Debug --output-on-failure
```

### 指定 Qt 路径

优先级（高 → 低）：

1. 命令行 `-DCMAKE_PREFIX_PATH=...`（推荐）
2. 环境变量 `QT_DIR`
3. `CMakeLists.txt` 中的默认路径

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=D:/your/Qt/6.9.3/msvc2022_64
```

### Linux / macOS（参考）

安装 Qt 6 对应 Kit 后，使用 Ninja 或系统生成器配置即可，例如：

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
cmake --build build
./build/xxMusic   # 可执行文件名因平台而异
```

> Windows 以外的打包、部署步骤尚未在 CI 中验证，需自行配置 Qt 运行时路径。

---

## 数据与配置存储

应用名：`XiaoxiongMusicPlayer`，组织名：`Xiaoxiong`。

| 文件 / 目录 | 位置（Windows 示例） | 说明 |
|-------------|----------------------|------|
| `settings.json` | `%LOCALAPPDATA%\Xiaoxiong\XiaoxiongMusicPlayer\` | 用户设置（含 Meting 镜像） |
| `library.json` | 同上 | 曲库索引 |
| `playlists.json` | 同上 | 播放列表与「我喜欢」 |
| `lyrics_cache/` | 同上 | 在线歌词缓存 |
| `cache/` | 同上 | 封面等缓存 |

**跨平台说明：** 默认扫描目录在**首次运行**时通过 `QStandardPaths::MusicLocation` 解析（Windows 为 `~/Music`，Linux 通常为 `~/Music`）。若 `settings.json` 中已保存绝对路径（例如在 Windows 写入的 `C:/Users/...`），换平台后需手动修改或点「恢复默认」。

**settings.json 示例：**

```json
{
  "theme": "system",
  "language": "zh_CN",
  "defaultScanDirectory": "C:/Users/you/Music",
  "defaultDownloadDirectory": "C:/Users/you/Music/Downloads",
  "lyricOnlineEnabled": true,
  "metingApiBases": [
    "https://meting.elysium-stack.cn/api",
    "https://api.injahow.cn/meting"
  ]
}
```

首次启动若存在旧版注册表配置，会自动迁移到 `settings.json`。

---

## 快捷键

| 快捷键 | 功能 |
|--------|------|
| `Ctrl+P` | 播放 / 暂停 |
| `Ctrl+Left` | 上一首 |
| `Ctrl+Right` | 下一首 |
| `Ctrl+F` | 聚焦曲库搜索框 |

---

## Cursor / VS Code

推荐扩展：**CMake Tools**、**clangd**、**CodeLLDB**（调试）。

常用任务（`Ctrl+Shift+P` → `Tasks: Run Task`）：

| 任务 | 作用 |
|------|------|
| **CMake: Configure** | 生成 CMake 工程 |
| **CMake: Build** | 编译 |
| **Build and Run** | 编译并启动 |
| **Qt: Deploy** | 手动 windeployqt（通常不必，编译已自动部署） |

调试：选择 **Debug (CodeLLDB)**，在 C++ 源码中设断点后按 **F5**。

---

## 架构简述

```
QML (pages / components)
        ↕  appController 上下文属性
ApplicationController
        ↕
PlaybackService · LibraryRepository · PlaylistService
LyricService · SearchService · CoverArtService
OnlineDownloadService · SettingsService
        ↕
MetingApi / MetingStreamResolver · LibraryScanner · LyricParser
```

QML 通过 `qt_add_qml_module` 内嵌为 `qrc:/XiaoxiongMusic/...`；图标等资源位于源码 `assets/icons/`，构建时复制到 `build/XiaoxiongMusic/assets/icons/`（构建产物，无需提交）。

---

## 常见问题

**找不到 Qt**

确认 Qt 安装路径，configure 时传入 `-DCMAKE_PREFIX_PATH`，或设置环境变量 `QT_DIR`。

**运行提示缺少 `Qt6Qml.dll` 等**

重新编译（触发 windeployqt），或手动执行：

```powershell
C:\Qt\6.9.3\msvc2022_64\bin\windeployqt.exe --debug --qmldir qml build\Debug\xxMusic.exe
```

**C++ 头文件报红（clangd）**

先完成一次编译，再执行 `clangd: Restart language server`。编辑器 Qt 路径见 `.vscode/settings.json`、`.clangd`。

**在线歌词 / 搜索失败**

在 **设置 → Meting API** 中更换可用镜像并保存；Meting 公益镜像可能变动，以实际可用地址为准。

**图标不显示**

确认源码目录 `assets/icons/` 已提交并在 `CMakeLists.txt` 的 `RESOURCES` 中注册。

---

## 测试

当前单元测试：

- `test_lyric_parser` — LRC 时间轴解析
- `test_playlist_service` — 播放列表 CRUD 与系统歌单「我喜欢」

```powershell
cmake --build build --config Debug
cd build && ctest -C Debug --output-on-failure
```

---

## 许可证

GPLv3
