# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(threadedfetchmore LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appthreadedfetchmore WIN32 MACOSX_BUNDLE
    main.cpp
)

qt_add_qml_module(appthreadedfetchmore
    URI threadedfetchmore
    QML_FILES
        ContactBookDelegate.qml
        ThreadedFetchMore.qml
    SOURCES
        fetchworker.h
        fetchworker.cpp
        threadedfetchmoremodel.h
        threadedfetchmoremodel.cpp
)

target_link_libraries(appthreadedfetchmore
    PRIVATE Qt6::Quick
)

include(GNUInstallDirs)
install(TARGETS appthreadedfetchmore
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET appthreadedfetchmore
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
