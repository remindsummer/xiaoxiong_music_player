# 小熊音乐播放器 V1 实现计划

> **面向 AI 代理的工作者：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 在现有 Qt + QML 工程上实现桌面三端可用的本地音乐播放器 V1（曲库、播放、列表、搜索、歌词、设置、应用内快捷键），并满足既定性能与稳定性指标。

**架构：** 采用“C++ 服务层 + QML 表现层”的分层结构。媒体能力由 `Qt Multimedia` 提供，曲库/播放列表/设置使用本地持久化，歌词采用“本地优先 + 在线补全 + 本地缓存”的策略。通过 ViewModel 暴露状态给 QML，避免 UI 直接操作底层存储与扫描逻辑。

**技术栈：** C++17、Qt 6（Core/Gui/Quick/Multimedia/Network/Sql）、QML、CMake、Qt Test（单元测试）。

---

## 0. 文件结构规划（先定边界）

### 新增文件

- `src/app/ApplicationController.h`
- `src/app/ApplicationController.cpp`
- `src/player/PlaybackService.h`
- `src/player/PlaybackService.cpp`
- `src/library/LibraryScanner.h`
- `src/library/LibraryScanner.cpp`
- `src/library/LibraryRepository.h`
- `src/library/LibraryRepository.cpp`
- `src/playlist/PlaylistService.h`
- `src/playlist/PlaylistService.cpp`
- `src/lyrics/LyricParser.h`
- `src/lyrics/LyricParser.cpp`
- `src/lyrics/LyricService.h`
- `src/lyrics/LyricService.cpp`
- `src/settings/SettingsService.h`
- `src/settings/SettingsService.cpp`
- `src/search/SearchService.h`
- `src/search/SearchService.cpp`
- `src/models/Track.h`
- `src/models/Playlist.h`
- `src/models/LyricLine.h`
- `qml/pages/LibraryPage.qml`
- `qml/pages/PlaylistsPage.qml`
- `qml/pages/NowPlayingPage.qml`
- `qml/pages/SettingsPage.qml`
- `qml/components/TrackTable.qml`
- `qml/components/PlaybackBar.qml`
- `qml/components/LyricsPanel.qml`
- `qml/components/SearchBar.qml`
- `tests/unit/test_lyric_parser.cpp`
- `tests/unit/test_search_service.cpp`
- `tests/unit/test_playlist_service.cpp`
- `docs/testing/manual-test-checklist-v1.md`

### 修改文件

- `CMakeLists.txt`
- `src/main.cpp`
- `qml/Main.qml`

### 职责边界

- `player/*`：播放状态机、进度、音量、播放模式与异常跳过。
- `library/*`：目录扫描、元数据提取、曲库存储与读取。
- `playlist/*`：歌单 CRUD 与排序持久化。
- `lyrics/*`：LRC 解析、本地匹配、在线拉取、缓存与同步。
- `settings/*`：主题/语言/目录/联网开关。
- `search/*`：曲库检索与结果排序。
- `qml/*`：页面布局与交互绑定，不直接包含业务存储逻辑。

---

## 任务 1：搭建分层骨架与主导航

**文件：**
- 创建：`src/app/ApplicationController.h`、`src/app/ApplicationController.cpp`
- 创建：`qml/pages/LibraryPage.qml`、`qml/pages/PlaylistsPage.qml`、`qml/pages/NowPlayingPage.qml`、`qml/pages/SettingsPage.qml`
- 修改：`src/main.cpp`、`qml/Main.qml`、`CMakeLists.txt`
- 测试：启动检查（手工）

- [ ] **步骤 1：在 CMake 中加入新目录与模块依赖**
  - 增加 `Qt6::Network`、`Qt6::Sql`、`Qt6::Test`（测试目标用）；
  - 将 `src/*` 新增源文件与 `qml/pages/*` 注册到构建系统。

- [ ] **步骤 2：创建 `ApplicationController` 统一对外接口**
  - 在 C++ 中聚合 `PlaybackService`、`LibraryRepository`、`PlaylistService`、`SettingsService`；
  - 以 `QObject` 属性和槽函数暴露给 QML。

- [ ] **步骤 3：把 `Main.qml` 改为四主区导航框架**
  - 提供“曲库 / 播放列表 / 正在播放 / 设置”入口；
  - 先实现空页面容器与路由切换，不填充业务细节。

- [ ] **步骤 4：编译并验证壳层稳定**
  - 运行：`cmake --build build --config Debug`
  - 运行：`.\build\Debug\xxMusic.exe`
  - 预期：应用可启动，四主区可切换，无崩溃。

- [ ] **步骤 5：提交**
  - `git add CMakeLists.txt src/main.cpp qml/Main.qml src/app qml/pages`
  - `git commit -m "feat: 搭建 V1 分层骨架与四主区导航（任务 1）"`

---

## 任务 2：实现播放服务与播放控制条

**文件：**
- 创建：`src/player/PlaybackService.h`、`src/player/PlaybackService.cpp`
- 创建：`qml/components/PlaybackBar.qml`
- 修改：`qml/pages/NowPlayingPage.qml`、`src/app/ApplicationController.*`
- 测试：`docs/testing/manual-test-checklist-v1.md`（新增播放条目）

- [ ] **步骤 1：实现 `PlaybackService`**
  - 封装 `QMediaPlayer` + `QAudioOutput`；
  - 暴露播放状态、位置、时长、音量、播放模式；
  - 提供播放、暂停、停止、上一首、下一首、seek 接口。

- [ ] **步骤 2：处理异常文件跳过**
  - 监听媒体错误；
  - 记录失败曲目并跳转下一曲；
  - 向 UI 提供可见错误消息。

- [ ] **步骤 3：落地 `PlaybackBar.qml`**
  - 包含播放控制按钮、进度条、时间显示、音量与模式切换；
  - 与 `ApplicationController` 绑定，完成控制闭环。

- [ ] **步骤 4：手工验证播放控制**
  - 运行应用并加载本地文件；
  - 验证播放/暂停/停止/拖拽/切歌/音量/模式切换。

- [ ] **步骤 5：提交**
  - `git add src/player qml/components/PlaybackBar.qml qml/pages/NowPlayingPage.qml src/app`
  - `git commit -m "feat: 实现播放服务与控制条（任务 2）"`

---

## 任务 3：实现本地曲库扫描与持久化

**文件：**
- 创建：`src/library/LibraryScanner.*`、`src/library/LibraryRepository.*`、`src/models/Track.h`
- 修改：`src/app/ApplicationController.*`、`qml/pages/LibraryPage.qml`
- 测试：新增手工扫描测试条目

- [ ] **步骤 1：实现扫描器 `LibraryScanner`**
  - 支持多目录；
  - 提取最小元数据（路径、歌名、歌手、专辑、时长）；
  - 支持全量扫描与增量扫描模式。

- [ ] **步骤 2：实现仓储 `LibraryRepository`**
  - 使用 SQLite 持久化曲库索引；
  - 支持按路径去重更新；
  - 应用启动时快速载入缓存索引。

- [ ] **步骤 3：连接 UI**
  - `LibraryPage` 提供目录添加、扫描按钮、曲目表格展示；
  - 扫描进度与错误可见。

- [ ] **步骤 4：手工验证曲库恢复**
  - 扫描后退出重启；
  - 预期：曲库无需重扫即可恢复展示。

- [ ] **步骤 5：提交**
  - `git add src/library src/models/Track.h qml/pages/LibraryPage.qml src/app`
  - `git commit -m "feat: 实现本地曲库扫描与持久化（任务 3）"`

---

## 任务 4：实现搜索与筛选

**文件：**
- 创建：`src/search/SearchService.*`、`qml/components/SearchBar.qml`
- 修改：`qml/pages/LibraryPage.qml`、`src/app/ApplicationController.*`
- 测试：`tests/unit/test_search_service.cpp`

- [ ] **步骤 1：实现 `SearchService`**
  - 支持歌名/歌手/专辑多字段搜索；
  - 统一搜索入口与排序规则。

- [ ] **步骤 2：接入 `SearchBar` 与曲库页面**
  - 输入即时触发查询（加轻量去抖）；
  - 支持结果“立即播放”“加入播放列表”。

- [ ] **步骤 3：编写并运行单元测试**
  - 运行：`ctest --test-dir build -C Debug --output-on-failure`
  - 重点覆盖：大小写、空关键词、多字段匹配。

- [ ] **步骤 4：提交**
  - `git add src/search qml/components/SearchBar.qml qml/pages/LibraryPage.qml tests/unit/test_search_service.cpp src/app`
  - `git commit -m "feat: 实现曲库搜索与筛选（任务 4）"`

---

## 任务 5：实现播放列表管理

**文件：**
- 创建：`src/playlist/PlaylistService.*`、`src/models/Playlist.h`
- 修改：`qml/pages/PlaylistsPage.qml`、`src/app/ApplicationController.*`
- 测试：`tests/unit/test_playlist_service.cpp`

- [ ] **步骤 1：实现播放列表服务**
  - 支持创建、重命名、删除；
  - 支持添加/移除曲目与排序更新；
  - 数据持久化到 SQLite。

- [ ] **步骤 2：接入播放列表页面**
  - 实现歌单列表与歌单内曲目视图；
  - 支持从曲库结果添加到目标歌单。

- [ ] **步骤 3：测试与手工验证**
  - 单元测试覆盖 CRUD 与排序；
  - 重启后验证播放列表恢复。

- [ ] **步骤 4：提交**
  - `git add src/playlist src/models/Playlist.h qml/pages/PlaylistsPage.qml tests/unit/test_playlist_service.cpp src/app`
  - `git commit -m "feat: 实现播放列表管理（任务 5）"`

---

## 任务 6：实现歌词系统（本地优先 + 在线补全）

**文件：**
- 创建：`src/lyrics/LyricParser.*`、`src/lyrics/LyricService.*`、`src/models/LyricLine.h`、`qml/components/LyricsPanel.qml`
- 修改：`qml/pages/NowPlayingPage.qml`、`src/app/ApplicationController.*`
- 测试：`tests/unit/test_lyric_parser.cpp`

- [ ] **步骤 1：实现 LRC 解析器**
  - 解析时间标签与文本；
  - 输出按时间有序的歌词行。

- [ ] **步骤 2：实现本地歌词匹配**
  - 优先匹配同名 `.lrc`；
  - 提供手动指定歌词文件接口。

- [ ] **步骤 3：实现在线检索与缓存**
  - 联网开关开启时允许请求；
  - 请求成功写入本地缓存；
  - 失败不自动重试，仅暴露手动重试入口。

- [ ] **步骤 4：接入歌词面板**
  - 根据播放进度高亮当前行并滚动；
  - 无歌词时显示可理解占位与检索动作。

- [ ] **步骤 5：测试与提交**
  - `ctest --test-dir build -C Debug --output-on-failure`
  - `git add src/lyrics src/models/LyricLine.h qml/components/LyricsPanel.qml qml/pages/NowPlayingPage.qml tests/unit/test_lyric_parser.cpp src/app`
  - `git commit -m "feat: 实现歌词系统与本地缓存（任务 6）"`

---

## 任务 7：实现设置中心与应用内快捷键

**文件：**
- 创建：`src/settings/SettingsService.*`
- 修改：`qml/pages/SettingsPage.qml`、`src/app/ApplicationController.*`、`qml/Main.qml`
- 测试：手工验证 + 冒烟测试清单更新

- [ ] **步骤 1：实现设置服务**
  - 使用 `QSettings` 保存主题、语言、默认扫描目录、歌词联网开关；
  - 应用启动时加载设置并分发到各服务。

- [ ] **步骤 2：实现应用内快捷键（固定预设）**
  - 支持：播放/暂停、上一首、下一首、聚焦搜索框；
  - 在设置页展示快捷键说明，不提供编辑入口。

- [ ] **步骤 3：手工验证**
  - 重启后设置生效；
  - 快捷键仅在应用内触发，且行为正确。

- [ ] **步骤 4：提交**
  - `git add src/settings qml/pages/SettingsPage.qml qml/Main.qml src/app`
  - `git commit -m "feat: 实现设置中心与应用内快捷键（任务 7）"`

---

## 任务 8：统一回归、性能验证与收尾

**文件：**
- 创建：`docs/testing/manual-test-checklist-v1.md`（若前序未创建）
- 修改：必要的缺陷修复文件
- 测试：完整回归 + 性能测量记录

- [ ] **步骤 1：执行完整验证**
  - 构建：`cmake --build build --config Debug`
  - 测试：`ctest --test-dir build -C Debug --output-on-failure`
  - 手工：按 `manual-test-checklist-v1.md` 全量走查核心流程。

- [ ] **步骤 2：性能目标验收（2,000 首）**
  - 测冷启动、搜索首屏、切歌延迟；
  - 记录实测数据并对比目标：`<= 3 s`、`<= 200 ms`、`<= 300 ms`。

- [ ] **步骤 3：修复阻断问题并复测**
  - 若有失败项，先修复再回归；
  - 复测通过后再进入收尾。

- [ ] **步骤 4：最终提交**
  - `git add .`
  - `git commit -m "chore: 完成音乐播放器 V1 回归与验收（任务 8）"`

---

## 里程碑与完成定义

### 里程碑

1. M1：导航壳层 + 播放基础闭环可用（任务 1-2）
2. M2：曲库、搜索、播放列表可用（任务 3-5）
3. M3：歌词、设置、快捷键可用（任务 6-7）
4. M4：回归与性能验收通过（任务 8）

### 完成定义（Definition of Done）

- 所有“必须功能”完成并通过回归。
- 离线模式下核心能力完整可用。
- 在线歌词失败仅支持手动重试，行为符合需求文档。
- 三端核心流程无阻断崩溃。
- 性能指标达到 V1 目标或有明确偏差说明与后续计划。

---

## 风险与应对

1. **跨平台媒体后端差异风险**
   - 应对：抽象 `PlaybackService`，在服务层处理平台差异，避免 UI 分支逻辑膨胀。

2. **曲库扫描性能与 UI 阻塞风险**
   - 应对：扫描任务异步化，采用增量更新与批量写库。

3. **歌词在线来源不稳定风险**
   - 应对：先保障本地歌词体验，在线仅作补全，失败不阻断播放。

4. **需求蔓延风险**
   - 应对：严格执行 V1 范围边界，新增能力转入后续版本 backlog。
