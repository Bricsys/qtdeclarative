# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(qml-i18n LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml LinguistTools)

qt_standard_project_setup(REQUIRES 6.8 I18N_TRANSLATED_LANGUAGES en en_AU fr)

qt_add_executable(qmli18n WIN32 MACOSX_BUNDLE main.cpp)

target_link_libraries(qmli18n PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

qt_add_qml_module(qmli18n
    URI Translated
    QML_FILES
        Main.qml
)

#![0]
qt_add_translations(qmli18n
    RESOURCE_PREFIX /qt/qml/Translated/i18n
    TS_FILE_BASE qml
    TS_FILE_DIR i18n
)
#![0]

install(TARGETS qmli18n
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET qmli18n
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
