# VC++ 运行库（可选捆绑）

Qt MSVC 版依赖 **Microsoft Visual C++ 2015–2022 Redistributable (x64)**。

## 下载

从微软官方下载 `vc_redist.x64.exe`：

https://learn.microsoft.com/zh-cn/cpp/windows/latest-supported-vc-redist

将下载的文件放入本目录：

```
packaging/redist/vc_redist.x64.exe
```

> 该文件体积较大，**不要提交到 Git**。打包脚本检测到后会自动捆绑并在安装时静默安装。

## 未放置 redist 时

安装程序仍可生成，但会在本机缺少运行库时提示；目标用户需自行安装 VC++ Redistributable。
