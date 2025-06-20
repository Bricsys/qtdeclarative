# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(customrendernode LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core GuiPrivate Qml Quick ShaderTools)

qt_standard_project_setup(REQUIRES 6.9)

qt_add_executable(customrendernode WIN32 MACOSX_BUNDLE
    main.cpp
    customrender.cpp customrender.h
)

target_link_libraries(customrendernode PRIVATE
    Qt6::Core
    Qt6::GuiPrivate
    Qt6::Qml
    Qt6::Quick
)

qt_add_qml_module(customrendernode
    URI SceneGraphRendering
    QML_FILES
        main.qml
    RESOURCE_PREFIX /scenegraph/customrendernode
    NO_RESOURCE_TARGET_PATH
)

qt6_add_shaders(customrendernode "shaders"
    # gl_VertexId is in earliest OpenGL ES 3.00, and GLSL 130
    GLSL "300 es,130"
    PREFIX
        "/scenegraph/customrendernode"
    FILES
        "shaders/customrender.vert"
        "shaders/customrender.frag"
)

install(TARGETS customrendernode
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET customrendernode
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
