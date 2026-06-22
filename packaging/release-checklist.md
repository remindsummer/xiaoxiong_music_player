# Release 打包前验证清单

在生成安装包之前，请逐项确认。

## 1. 编译 Release

```powershell
cmake --build build --config Release --target xxMusic --parallel
```

产物目录：`build/Release/`

## 2. windeployqt 完整性

`build/Release/` 中必须存在（POST_BUILD 已自动部署）：

| 路径 | 说明 |
|------|------|
| `xxMusic.exe` | 主程序 |
| `tag.dll`、`tag_c.dll` | TagLib 动态库（元数据 / 内嵌封面） |
| `Qt6Core.dll`、`Qt6Gui.dll`、`Qt6Quick.dll` 等 | Qt 核心 DLL |
| `platforms/qwindows.dll` | Windows 平台插件 |
| `qml/` | QML 运行时 |
| `multimedia/` | 音频播放 |
| `imageformats/` | 图片格式（封面） |
| `tls/` | HTTPS（在线 API） |
| `styles/` | Quick Controls 样式 |

**不应**打入安装包的构建中间文件：`xxMusic.lib`、`xxMusic.exp`、`*.pdb`（Inno 脚本已排除）。

## 3. 本机冒烟测试

- [ ] 双击 `build/Release/xxMusic.exe` 能启动
- [ ] 本地曲库扫描与播放正常
- [ ] 在线搜索 / 播放 / 歌词正常
- [ ] 托盘图标与系统媒体控件正常

## 4. 干净环境测试（发版前强烈建议）

在未安装 Qt / Visual Studio 的 Windows 10/11 x64 虚拟机或另一台电脑上：

- [ ] 已安装 **Microsoft Visual C++ 2015–2022 Redistributable (x64)**，或安装包已捆绑并成功安装
- [ ] 运行安装程序并完成安装
- [ ] 从开始菜单启动，功能与上一节一致
- [ ] 「设置 → 应用」可卸载
- [ ] 卸载后 `%LOCALAPPDATA%\Xiaoxiong\XiaoxiongMusicPlayer\` 默认仍存在（未勾选删除数据时）

## 5. 生成安装包

见 [packaging/inno/README.md](inno/README.md)。
