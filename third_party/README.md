# 第三方源码

## TagLib

- 路径：`third_party/taglib/`
- 版本：v2.0.2（与 upstream [taglib/taglib](https://github.com/taglib/taglib) 一致）
- 用途：本地音频元数据与内嵌封面读取（通过 C 绑定 `tag_c`）
- 构建：由根目录 `CMakeLists.txt` 以**动态库**方式编译，无需网络拉取或 vcpkg

升级 TagLib 时，请替换 `third_party/taglib/` 目录内容并重新验证编译与播放/扫库功能。
