cmake_minimum_required(VERSION 3.19)
project(simple-video-player LANGUAGES CXX)

set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/app.rc")

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Multimedia MultimediaWidgets)

qt_standard_project_setup()

qt_add_executable(simple-video-player
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.h
    mainwindow.cpp
    playlistdialog.h
    playlistdialog.cpp
    ${app_icon_resource_windows}
)

target_link_libraries(simple-video-player
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt6::Multimedia
        Qt6::MultimediaWidgets
)

install(TARGETS simple-video-player
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET simple-video-player
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
