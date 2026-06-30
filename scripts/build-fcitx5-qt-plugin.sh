#!/usr/bin/env bash
# 为 Qt 在线安装器版本（如 6.9.3）编译 fcitx5 输入法插件。
# Ubuntu apt 自带的 fcitx5-frontend-qt6 仅匹配系统 Qt 6.4，无法用于自装 Qt。
#
# 源码优先级（无需访问 GitHub 也可构建）：
#   1) FCITX5_QT_SOURCE_DIR 环境变量
#   2) 项目内 third_party/fcitx5-qt（已 vendored）
#   3) /tmp/fcitx5-qt（若曾成功克隆过）
#   4) 网络下载（GitHub / 镜像 tarball，最后手段）
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

QT_DIR="${QT_DIR:-$HOME/Qt/6.9.3/gcc_64}"
FCITX5_QT_TAG="${FCITX5_QT_TAG:-5.1.4}"
WORK_DIR="${WORK_DIR:-/tmp/fcitx5-qt-build}"
INSTALL_TO_QT="${INSTALL_TO_QT:-1}"
SOURCE_DIR="${FCITX5_QT_SOURCE_DIR:-}"

if [[ ! -x "$QT_DIR/bin/qmake6" ]]; then
    echo "错误: 未找到 Qt，请设置 QT_DIR（当前: $QT_DIR）" >&2
    exit 1
fi

echo "==> 检查构建依赖..."
missing=()
for pkg in libxkbcommon-dev libfcitx5core-dev libfcitx5utils-dev libfcitx5config-dev libfcitx5-qt6-dev gettext; do
    if ! dpkg -s "$pkg" >/dev/null 2>&1; then
        missing+=("$pkg")
    fi
done
if ((${#missing[@]} > 0)); then
    echo "请先安装: sudo apt install -y ${missing[*]} extra-cmake-modules" >&2
    exit 1
fi

ECM_PREFIX=""
if [[ -f "$HOME/.local/share/ECM/cmake/ECMConfig.cmake" ]]; then
    ECM_PREFIX="$HOME/.local/share/ECM"
elif [[ -d /usr/share/ECM/cmake ]]; then
    ECM_PREFIX="/usr/share/ECM"
else
    echo "请先安装: sudo apt install -y extra-cmake-modules" >&2
    exit 1
fi

resolve_source_dir() {
    if [[ -n "$SOURCE_DIR" && -f "$SOURCE_DIR/CMakeLists.txt" ]]; then
        echo "$SOURCE_DIR"
        return 0
    fi
    if [[ -f "$PROJECT_ROOT/third_party/fcitx5-qt/CMakeLists.txt" ]]; then
        echo "$PROJECT_ROOT/third_party/fcitx5-qt"
        return 0
    fi
    if [[ -f "/tmp/fcitx5-qt/CMakeLists.txt" ]]; then
        echo "/tmp/fcitx5-qt"
        return 0
    fi
    if [[ -f "$WORK_DIR/fcitx5-qt/CMakeLists.txt" ]]; then
        echo "$WORK_DIR/fcitx5-qt"
        return 0
    fi
    return 1
}

download_source() {
    mkdir -p "$WORK_DIR"
    local dest="$WORK_DIR/fcitx5-qt"
    local tarball="$WORK_DIR/fcitx5-qt-${FCITX5_QT_TAG}.tar.gz"
    local urls=(
        "https://github.com/fcitx/fcitx5-qt/archive/refs/tags/${FCITX5_QT_TAG}.tar.gz"
        "https://ghfast.top/https://github.com/fcitx/fcitx5-qt/archive/refs/tags/${FCITX5_QT_TAG}.tar.gz"
        "https://mirror.ghproxy.com/https://github.com/fcitx/fcitx5-qt/archive/refs/tags/${FCITX5_QT_TAG}.tar.gz"
    )

    echo "==> 本地无源码，尝试下载 fcitx5-qt ${FCITX5_QT_TAG} ..."
    rm -rf "$dest" "$tarball"

    for url in "${urls[@]}"; do
        echo "    尝试: $url"
        if curl -fsSL --connect-timeout 30 --retry 2 -o "$tarball" "$url"; then
            echo "    下载成功"
            tar -xzf "$tarball" -C "$WORK_DIR"
            mv "$WORK_DIR/fcitx5-qt-${FCITX5_QT_TAG}" "$dest"
            rm -f "$tarball"
            return 0
        fi
        echo "    失败，换下一个镜像..."
    done

    echo "==> 尝试 git clone ..."
    local git_urls=(
        "https://github.com/fcitx/fcitx5-qt.git"
        "https://gitclone.com/github.com/fcitx/fcitx5-qt.git"
    )
    for url in "${git_urls[@]}"; do
        echo "    尝试: $url"
        if git clone --depth 1 --branch "$FCITX5_QT_TAG" "$url" "$dest"; then
            return 0
        fi
        rm -rf "$dest"
    done

    echo "错误: 无法获取 fcitx5-qt 源码。" >&2
    echo "GitHub 仓库地址: https://github.com/fcitx/fcitx5-qt" >&2
    echo "若网络受限，请手动下载 ${FCITX5_QT_TAG} 源码压缩包，解压到 third_party/fcitx5-qt 后重试。" >&2
    return 1
}

if ! SOURCE_DIR="$(resolve_source_dir)"; then
    download_source
    SOURCE_DIR="$(resolve_source_dir)"
fi

echo "==> 使用源码: $SOURCE_DIR"

cmake -S "$SOURCE_DIR" -B "$WORK_DIR/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$QT_DIR${ECM_PREFIX:+;$ECM_PREFIX}" \
    -DENABLE_QT6=ON \
    -DENABLE_QT5=OFF \
    -DENABLE_QT4=OFF \
    -DBUILD_ONLY_PLUGIN=ON \
    -DENABLE_QT6_WAYLAND_WORKAROUND=OFF

cmake --build "$WORK_DIR/build" --parallel

PLUGIN_SRC="$WORK_DIR/build/qt6/platforminputcontext/libfcitx5platforminputcontextplugin.so"
if [[ ! -f "$PLUGIN_SRC" ]]; then
    PLUGIN_SRC="$(find "$WORK_DIR/build" -name 'libfcitx5platforminputcontextplugin.so' | head -1)"
fi
if [[ ! -f "$PLUGIN_SRC" ]]; then
    echo "错误: 未找到编译产物 libfcitx5platforminputcontextplugin.so" >&2
    exit 1
fi

if [[ "$INSTALL_TO_QT" == "1" ]]; then
    DEST="$QT_DIR/plugins/platforminputcontexts"
    mkdir -p "$DEST"
    cp -f "$PLUGIN_SRC" "$DEST/"
    echo "已安装到: $DEST/libfcitx5platforminputcontextplugin.so"
else
    DEST="$PROJECT_ROOT/build/platforminputcontexts"
    mkdir -p "$DEST"
    cp -f "$PLUGIN_SRC" "$DEST/"
    echo "已复制到: $DEST/libfcitx5platforminputcontextplugin.so"
fi

echo "完成。请重新启动 xxMusic，文本框中应可正常使用 fcitx5 中文输入。"
