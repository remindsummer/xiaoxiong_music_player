cmake_minimum_required(VERSION 4.3.3)

project(xiaoxiong_music_player LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 生成 compile_commands.json，供 clangd 等工具做代码分析
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Qt 安装路径（换机器时需同步修改）
set(CMAKE_PREFIX_PATH "C:/Qt/6.9.3/msvc2022_64")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick)

# 启用 Qt 标准 CMake 行为（AUTOMOC、QML 模块等）
qt_standard_project_setup()

# QML 文件在子目录 qml/ 下时，使用旧策略避免额外 qmldir 要求
qt_policy(SET QTP0004 OLD)

# 可执行目标，编译产物为 xxMusic.exe
qt_add_executable(xxMusic
    src/main.cpp
)

# Debug 构建使用控制台子系统，便于 qDebug()/std::cout 输出到终端
set_target_properties(xxMusic PROPERTIES
    WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
)

# 将 QML 注册为内嵌模块，运行时通过 qrc:/XiaoxiongMusic/... 加载
qt_add_qml_module(xxMusic
    URI XiaoxiongMusic
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

target_link_libraries(xxMusic PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick   # QML 界面必需
)

# 生成 Qt 官方 QML 应用部署脚本（分析 QML import 并调用 windeployqt）
qt_generate_deploy_qml_app_script(
    TARGET xxMusic
    OUTPUT_SCRIPT deploy_script
)

# 每次编译后自动部署 DLL / QML 到 exe 同目录（build/Debug/）
# Windows 上必须指定部署前缀，否则会错误部署到项目根目录
add_custom_command(TARGET xxMusic POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        -DCMAKE_INSTALL_PREFIX=$<TARGET_FILE_DIR:xxMusic>
        -DQT_DEPLOY_PREFIX=$<TARGET_FILE_DIR:xxMusic>
        -P ${deploy_script}
    COMMENT "Deploying Qt and QML dependencies..."
)
