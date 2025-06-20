# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmlcppcodegen LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

add_subdirectory(data)

qt_internal_add_test(tst_qmlcppcodegen
    SOURCES
        tst_qmlcppcodegen.cpp
    LIBRARIES
        Qt::Qml
        Qt::QmlPrivate
        Qt::QmlMetaPrivate
        Qt::GuiPrivate
        codegen_test_module
        codegen_test_moduleplugin
        codegen_test_hidden
        codegen_test_hiddenplugin
        codegen_test_stringbuilder
        codegen_test_stringbuilderplugin
        confused_test_module
        confused_test_moduleplugin
        with_subdir_test_module
        with_subdir_test_moduleplugin
    DEFINES
        QT_NO_CAST_FROM_ASCII
)

qt_internal_add_test(tst_qmlcppcodegen_interpreted
    SOURCES
        tst_qmlcppcodegen.cpp
    LIBRARIES
        Qt::Qml
        Qt::QmlPrivate
        Qt::QmlMetaPrivate
        Qt::GuiPrivate
        codegen_test_module
        codegen_test_moduleplugin
        codegen_test_hidden
        codegen_test_hiddenplugin
        codegen_test_stringbuilder
        codegen_test_stringbuilderplugin
        confused_test_module
        confused_test_moduleplugin
        with_subdir_test_module
        with_subdir_test_moduleplugin
    DEFINES
        QT_TEST_FORCE_INTERPRETER
)
