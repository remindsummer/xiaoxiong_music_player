cmake_minimum_required(VERSION 4.3.3)

project(xiaoxiong_music_player LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 生成 compile_commands.json，供 clangd 等工具做代码分析
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Qt 安装路径解析优先级：
#   1) 命令行 -DCMAKE_PREFIX_PATH=...（最高优先级，换机器时推荐）
#   2) 环境变量 QT_DIR（例如 setx QT_DIR C:/Qt/6.9.3/msvc2022_64）
#   3) 下方默认值（本机默认安装路径，仅作兜底）
if(NOT CMAKE_PREFIX_PATH)
    if(DEFINED ENV{QT_DIR})
        set(CMAKE_PREFIX_PATH "$ENV{QT_DIR}")
    else()
        set(CMAKE_PREFIX_PATH "C:/Qt/6.9.3/msvc2022_64")
    endif()
endif()
message(STATUS "使用的 Qt 路径 (CMAKE_PREFIX_PATH): ${CMAKE_PREFIX_PATH}")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg Quick QuickControls2 Multimedia Network LinguistTools)

# ===== TagLib（vendored，动态库）=====
set(TAGLIB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/taglib")
if(NOT EXISTS "${TAGLIB_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "缺少 vendored TagLib 源码：${TAGLIB_SOURCE_DIR}")
endif()

set(_xxmusic_saved_build_shared_libs "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS ON)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_BINDINGS ON CACHE BOOL "" FORCE)
add_subdirectory("${TAGLIB_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/third_party/taglib" EXCLUDE_FROM_ALL)
set(BUILD_SHARED_LIBS "${_xxmusic_saved_build_shared_libs}")

if(NOT TARGET tag_c)
    message(FATAL_ERROR "TagLib C 绑定目标 tag_c 未生成，请确认 third_party/taglib 完整且 BUILD_BINDINGS=ON")
endif()
if(NOT TARGET TagLib::tag_c)
    target_include_directories(tag_c INTERFACE
        "$<BUILD_INTERFACE:${TAGLIB_SOURCE_DIR}/bindings/c>"
    )
    add_library(TagLib::tag_c ALIAS tag_c)
endif()

# 启用 Qt 标准 CMake 行为（AUTOMOC、QML 模块等）
qt_standard_project_setup()

# QML 文件在子目录 qml/ 下时，使用旧策略避免额外 qmldir 要求
qt_policy(SET QTP0004 OLD)

# 可执行目标，编译产物为 xxMusic.exe
qt_add_executable(xxMusic
    # ===== App 启动与桥接层 =====
    src/main.cpp
    src/app/application_bootstrap.cpp
    src/app/application_bootstrap.h
    src/app/tray_service.cpp
    src/app/tray_service.h
    src/app/single_instance_guard.cpp
    src/app/single_instance_guard.h
    src/ui_bridge/application_controller.cpp
    src/ui_bridge/application_controller.h

    # ===== 播放核心模块 =====
    src/player/playback_service.cpp
    src/player/playback_service.h
    src/player/media_session_integration.cpp
    src/player/media_session_integration.h
    src/player/audio_spectrum_analyzer.cpp
    src/player/audio_spectrum_analyzer.h
    src/player/audio_visualizer_service.cpp
    src/player/audio_visualizer_service.h

    # ===== 曲库模块 =====
    src/library/library_scanner.cpp
    src/library/library_scanner.h
    src/library/library_repository.cpp
    src/library/library_repository.h
    src/library/library_metadata_resolver.cpp
    src/library/library_metadata_resolver.h
    src/library/audio_tag_reader.cpp
    src/library/audio_tag_reader.h

    # ===== 播放列表模块 =====
    src/playlist/playlist_service.cpp
    src/playlist/playlist_service.h

    # ===== 歌词模块 =====
    src/lyrics/lyric_service.cpp
    src/lyrics/lyric_service.h
    src/lyrics/lyric_parser.cpp
    src/lyrics/lyric_parser.h

    # ===== 搜索模块 =====
    src/search/search_service.cpp
    src/search/search_service.h

    # ===== 网络 / Meting API =====
    src/network/meting_api.cpp
    src/network/meting_api.h
    src/network/meting_stream_resolver.cpp
    src/network/meting_stream_resolver.h

    # ===== 封面缓存 =====
    src/artwork/cover_art_service.cpp
    src/artwork/cover_art_service.h
    src/artwork/library_cover_prefetcher.cpp
    src/artwork/library_cover_prefetcher.h

    # ===== 在线下载 =====
    src/download/online_download_service.cpp
    src/download/online_download_service.h

    # ===== 设置模块 =====
    src/settings/settings_service.cpp
    src/settings/settings_service.h

    # ===== 国际化 =====
    src/i18n/ui_translation_service.cpp
    src/i18n/ui_translation_service.h

    # ===== 数据模型占位 =====
    src/models/track_model.h
    src/models/lyric_line.h
    src/models/playlist_model.h
)

# Debug 构建使用控制台子系统，便于 qDebug()/std::cout 输出到终端
set_target_properties(xxMusic PROPERTIES
    WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
    AUTOUIC OFF
)

# 将 QML 注册为内嵌模块，运行时通过 qrc:/XiaoxiongMusic/... 加载
qt_add_qml_module(xxMusic
    URI XiaoxiongMusic
    VERSION 1.0
    QML_FILES
        # ===== QML 壳层入口 =====
        qml/Main.qml

        # ===== QML 页面分区 =====
        qml/pages/LibraryPage.qml
        qml/pages/PlaylistPage.qml
        qml/pages/PlaylistsPage.qml
        qml/pages/FavoritesPage.qml
        qml/pages/SettingsPage.qml

        # ===== QML 组件分区 =====
        qml/components/PlaybackBar.qml
        qml/components/MarqueeLabel.qml
        qml/components/RoundedCover.qml
        qml/components/ThemedSlider.qml
        qml/components/GlassPanel.qml
        qml/components/PrimaryButton.qml
        qml/components/GhostButton.qml
        qml/components/CloseChoiceDialog.qml
        qml/components/ThemedCheckBox.qml
        qml/components/DangerButton.qml
        qml/components/ThemedTextField.qml
        qml/components/ThemedComboBox.qml
        qml/components/TrackListDelegate.qml
        qml/components/HiResBadge.qml
        qml/components/TrackMetadataCard.qml
        qml/components/SpectrumVisualizer.qml
        qml/components/ImmersiveLyricsPanel.qml
        qml/components/SearchBar.qml
        qml/components/LyricsPanel.qml
        qml/components/LyricsContextMenu.qml
        qml/components/QueuePanel.qml
        qml/components/NowPlayingOverlay.qml

        # ===== QML 主题 Token =====
        qml/theme/Theme.qml

    RESOURCES
        # ===== 图标资源（SVG，可通过 icon.color 着色）=====
        assets/icons/play.svg
        assets/icons/pause.svg
        assets/icons/prev.svg
        assets/icons/next.svg
        assets/icons/mode_sequential.svg
        assets/icons/mode_repeat_one.svg
        assets/icons/mode_repeat_all.svg
        assets/icons/mode_shuffle.svg
        assets/icons/playlist_add.svg
        assets/icons/volume.svg
        assets/icons/volume_mute.svg
        assets/icons/heart.svg
        assets/icons/heart_filled.svg
        assets/icons/lyrics.svg
        assets/icons/queue.svg
        assets/icons/music_note.svg
        assets/icons/tray_icon.svg
        assets/icons/download.svg
        assets/icons/back.svg
)

qt_add_translations(xxMusic
    TS_FILES
        i18n/xiaoxiong_zh_CN.ts
        i18n/xiaoxiong_en_US.ts
    RESOURCE_PREFIX
        /i18n
)

qt_add_resources(xxMusic "app_translations"
    PREFIX "/i18n"
    FILES
        i18n/xiaoxiong_zh_CN.qm
        i18n/xiaoxiong_en_US.qm
)

target_link_libraries(xxMusic PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
    Qt6::Quick   # QML 界面必需
    Qt6::QuickControls2 # QQuickStyle 设置控件样式
    Qt6::Multimedia
    Qt6::Network # 在线歌词（LRCLIB）所需
    TagLib::tag_c
)

if(WIN32)
    target_sources(xxMusic PRIVATE
        src/app/windows_app_identity.cpp
        src/app/windows_app_identity.h
        src/player/windows_media_session_integration.cpp
        src/player/windows_media_session_integration.h
        resources/xxmusic.rc
    )
    target_link_libraries(xxMusic PRIVATE windowsapp propsys shell32 ole32)
endif()

# 定位 windeployqt（随 Qt 一起安装，位于 Qt 的 bin 目录）
# 直接对 exe 调用 windeployqt 会把所有依赖平铺部署到 exe 同目录，
# 避免 qt_generate_deploy_qml_app_script 把核心 DLL 放进 bin/ 子目录导致运行时找不到。
find_program(WINDEPLOYQT_EXECUTABLE
    NAMES windeployqt
    HINTS "${CMAKE_PREFIX_PATH}/bin"
)

if(WIN32)
    add_custom_command(TARGET xxMusic POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:tag>"
            "$<TARGET_FILE:tag_c>"
            "$<TARGET_FILE_DIR:xxMusic>"
        COMMENT "Copying TagLib DLLs next to xxMusic..."
        VERBATIM
    )
endif()

if(WINDEPLOYQT_EXECUTABLE)
    # 每次编译后自动把 Qt / QML 依赖平铺部署到 exe 同目录（如 build/Debug/）
    add_custom_command(TARGET xxMusic POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            $<IF:$<CONFIG:Debug>,--debug,--release>
            --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/qml"
            --no-translations
            --no-system-d3d-compiler
            "$<TARGET_FILE:xxMusic>"
        COMMENT "Deploying Qt and QML dependencies via windeployqt..."
        VERBATIM
    )
else()
    message(WARNING
        "未找到 windeployqt，跳过自动部署。运行 exe 前请手动执行 windeployqt，"
        "或确认 CMAKE_PREFIX_PATH 指向正确的 Qt 安装目录。")
endif()

# ===== 测试分区占位 =====
include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)

    qt_add_executable(test_lyric_parser
        tests/unit/test_lyric_parser.cpp
        src/lyrics/lyric_parser.cpp
    )
    target_link_libraries(test_lyric_parser PRIVATE
        Qt6::Core
        Qt6::Test
    )
    add_test(
        NAME test_lyric_parser
        COMMAND ${CMAKE_COMMAND} -E env
            "PATH=${CMAKE_PREFIX_PATH}/bin$<SEMICOLON>$<TARGET_FILE_DIR:xxMusic>/bin$<SEMICOLON>$<TARGET_FILE_DIR:test_lyric_parser>$<SEMICOLON>$ENV{PATH}"
            $<TARGET_FILE:test_lyric_parser>
    )

    qt_add_executable(test_playlist_service
        tests/unit/test_playlist_service.cpp
        src/playlist/playlist_service.cpp
    )
    target_link_libraries(test_playlist_service PRIVATE
        Qt6::Core
        Qt6::Test
    )
    add_test(
        NAME test_playlist_service
        COMMAND ${CMAKE_COMMAND} -E env
            "PATH=${CMAKE_PREFIX_PATH}/bin$<SEMICOLON>$<TARGET_FILE_DIR:xxMusic>/bin$<SEMICOLON>$<TARGET_FILE_DIR:test_playlist_service>$<SEMICOLON>$ENV{PATH}"
            $<TARGET_FILE:test_playlist_service>
    )

    qt_add_executable(test_audio_spectrum_analyzer
        tests/unit/test_audio_spectrum_analyzer.cpp
        src/player/audio_spectrum_analyzer.cpp
    )
    target_link_libraries(test_audio_spectrum_analyzer PRIVATE
        Qt6::Core
        Qt6::Test
    )
    add_test(
        NAME test_audio_spectrum_analyzer
        COMMAND ${CMAKE_COMMAND} -E env
            "PATH=${CMAKE_PREFIX_PATH}/bin$<SEMICOLON>$<TARGET_FILE_DIR:xxMusic>/bin$<SEMICOLON>$<TARGET_FILE_DIR:test_audio_spectrum_analyzer>$<SEMICOLON>$ENV{PATH}"
            $<TARGET_FILE:test_audio_spectrum_analyzer>
    )
endif()
