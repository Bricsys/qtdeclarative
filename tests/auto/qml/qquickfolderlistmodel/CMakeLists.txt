# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qquickfolderlistmodel.pro.

#####################################################################
## tst_qquickfolderlistmodel Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquickfolderlistmodel LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickfolderlistmodel
    SOURCES
        tst_qquickfolderlistmodel.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
        Qt::QuickTestUtilsPrivate
        Qt::LabsFolderListModelPrivate

    TESTDATA ${test_data}
)

# Resources:
set(introspect_resource_files
    "data/txtdir/hello.txt"
)

# On platforms where TESTDATA is embedded into resources (Android, iOS and
# INTEGRITY), the QT_RESOURCE_ALIAS property was previously set when processing
# TESTDATA, so we need to reset it. Otherwise the wrong alias will be written
# to introspect.qrc, and the related test will fail.
set_source_files_properties(${introspect_resource_files} PROPERTIES
    QT_RESOURCE_ALIAS "NOTFOUND")

qt_internal_add_resource(tst_qquickfolderlistmodel "introspect"
    PREFIX
        "/myprefix/subdir"
    BASE
        "data"
    FILES
        ${introspect_resource_files}
)


## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquickfolderlistmodel CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qquickfolderlistmodel CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
