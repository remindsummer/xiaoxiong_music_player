# Inno Setup 安装包

将 Release 版 **小熊音乐播放器** 打包为 Windows 用户级安装程序（无需管理员）。

## 前置条件

1. 已完成 [Release 打包前验证](../release-checklist.md)
2. 安装 [Inno Setup 6](https://jrsoftware.org/isinfo.php)（默认路径 `C:\Program Files (x86)\Inno Setup 6\ISCC.exe`）
3. （可选）将 `vc_redist.x64.exe` 放入 [`../redist/`](../redist/)（见 [redist/README.md](../redist/README.md)）

## 一键打包（VS Code / Cursor）

**Tasks: Run Task** → **Package: Build Release Installer**

会先编译 Release，再调用 `ISCC.exe` 生成安装包。

## 手动打包

```powershell
# 1. Release 编译
cmake --build build --config Release --target xxMusic --parallel

# 2. 生成安装包
& "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" packaging\inno\xiaoxiong_music_player.iss
```

## 输出

```
packaging/inno/output/小熊音乐播放器_1.0.0_Setup.exe
```

将此 **单个 exe** 分发给用户即可。

## 安装行为

| 项 | 说明 |
|----|------|
| 安装位置 | `%LOCALAPPDATA%\Programs\Xiaoxiong Music Player\` |
| 权限 | 当前用户（`PrivilegesRequired=lowest`） |
| 快捷方式 | 开始菜单；可选桌面 |
| AUMID | `Xiaoxiong.XiaoxiongMusicPlayer`（系统媒体控件识别） |
| 用户数据 | 默认保留在 `%APPDATA%\Xiaoxiong\XiaoxiongMusicPlayer\` |
| 卸载 | 设置 → 应用；可选删除个人数据 |

## 修改版本号

编辑 `xiaoxiong_music_player.iss` 中的 `#define MyAppVersion`，并与 [`resources/xxmusic.rc`](../../resources/xxmusic.rc) 保持一致。
