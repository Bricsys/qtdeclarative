# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from moduleWithQmlSingleton.pro.

#####################################################################
## moduleWithQmlSingleton Generic Library:
#####################################################################

qt_internal_add_cmake_library(moduleWithQmlSingleton
    MODULE
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/ModuleWithQmlSingleton"
    SOURCES
        plugin.cpp
    MOC_OPTIONS "-Muri=org.qtproject.ModuleWithQmlSingleton"
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
)

get_target_property(target_type moduleWithQmlSingleton TYPE)
if(target_type STREQUAL STATIC_LIBRARY)
    target_compile_definitions(moduleWithQmlSingleton PRIVATE QT_STATICPLUGIN)
endif()

#### Keys ignored in scope 1:.:.:moduleWithQmlSingleton.pro:<TRUE>:
# COPIES = "importfiles" "subfiles"
# IMPORT_FILES = "qmldir" "MySingleton.qml" "MySingleton2.qml"
# TEMPLATE = "lib"
# subfiles.files = "internal/InternalType.qml"
# subfiles.path = "$$DESTDIR/internal"

# special case begin
file(COPY qmldir MySingleton.qml MySingleton2.qml
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/ModuleWithQmlSingleton"
)
# special case end

qt_autogen_tools_initial_setup(moduleWithQmlSingleton)
