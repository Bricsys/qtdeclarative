# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmllint_benchmark LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_benchmark(tst_qmllint_benchmark
    SOURCES
        tst_qmllint_benchmark.cpp
    DEFINES
        SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
    LIBRARIES
        Qt::QmlCompilerPrivate
        Qt::Test
)
