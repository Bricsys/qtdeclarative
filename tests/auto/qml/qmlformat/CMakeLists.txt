# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmlformat Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmlformat LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qmlformat
    SOURCES
        tst_qmlformat.cpp
        tst_qmlformat_base.h
    DEFINES
        SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
    LIBRARIES
        Qt::Core
        Qt::QmlDomPrivate
        Qt::TestPrivate
        Qt::QuickTestUtilsPrivate
        Qt::QmlFormatPrivate
    TESTDATA ${test_data}
    TIMEOUT 3000
)

qt_internal_add_test(tst_qmlformat_cli
    SOURCES
        tst_qmlformat_cli.cpp
        tst_qmlformat_base.h
    DEFINES
        SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
    LIBRARIES
        Qt::Core
        Qt::QmlDomPrivate
        Qt::TestPrivate
        Qt::QuickTestUtilsPrivate
        Qt::QmlFormatPrivate
    TESTDATA ${test_data}
    TIMEOUT 3000
)

add_dependencies(tst_qmlformat_cli Qt::qmlformat)

qt_internal_extend_target(tst_qmlformat CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qmlformat CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

qt_internal_extend_target(tst_qmlformat_cli CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)
qt_internal_extend_target(tst_qmlformat_cli CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
