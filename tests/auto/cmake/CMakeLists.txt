# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# This is an automatic test for the CMake configuration files.
# To run it manually,
# 1) mkdir build   # Create a build directory
# 2) cd build
# 3) # Run cmake on this directory
#    `$qt_prefix/bin/qt-cmake ..` or `cmake -DCMAKE_PREFIX_PATH=/path/to/qt ..`
# 4) ctest         # Run ctest

cmake_minimum_required(VERSION 3.16)
project(qtdeclarative_cmake_tests)
enable_testing()

set(required_packages Core TestInternalsPrivate)
set(optional_packages Qml Quick)

# Setup the test when called as a completely standalone project.
if(TARGET Qt6::Core)
    # Tests are built as part of the qtsensors build tree.
    # Setup paths so that the Qt packages are found.
    qt_internal_set_up_build_dir_package_paths()
endif()

find_package(Qt6 REQUIRED COMPONENTS ${required_packages})
find_package(Qt6 OPTIONAL_COMPONENTS ${optional_packages})

# Setup common test variables which were previously set by ctest_testcase_common.prf.
set(CMAKE_MODULES_UNDER_TEST "${required_packages}" "${optional_packages}")

foreach(qt_package ${CMAKE_MODULES_UNDER_TEST})
    set(package_name "${QT_CMAKE_EXPORT_NAMESPACE}${qt_package}")
    if(${package_name}_FOUND)
        set(CMAKE_${qt_package}_MODULE_MAJOR_VERSION "${${package_name}_VERSION_MAJOR}")
        set(CMAKE_${qt_package}_MODULE_MINOR_VERSION "${${package_name}_VERSION_MINOR}")
        set(CMAKE_${qt_package}_MODULE_PATCH_VERSION "${${package_name}_VERSION_PATCH}")
    endif()
endforeach()

include("${_Qt6CTestMacros}")

if(TARGET Qt::Quick)
    set(deploy_args
        test_qml_app_deployment
        BINARY "${CMAKE_CTEST_COMMAND}"
        BINARY_ARGS "-V"
        # Need to explicitly specify a writable install prefix.
        BUILD_OPTIONS
            -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/test_qml_app_deployment_installed
        NO_RUN_ENVIRONMENT_PLUGIN_PATH
    )

    # For now, the test should only pass on
    # - Windows shared / static builds
    # - macOS shared / static builds
    #-  Linux desktop shared / static builds
    # - iOS static builds
    # It should fail on other platforms, because there is no support for runtime dependency
    # deployment on those platforms.
    # In the static builds case, the runtime dependencies are just skipped, but the test should
    # still pass.
    if(WIN32 OR APPLE
             OR (UNIX AND NOT APPLE AND NOT ANDROID AND NOT CMAKE_CROSSCOMPILING))
        _qt_internal_test_expect_pass(${deploy_args})
    else()
        _qt_internal_test_expect_fail(${deploy_args})
    endif()
endif()

# We only support a limited subset of cmake tests when targeting iOS:
# - Only those that use qt_add_executable (but not add_executable), and
# - don't try to run the built binaries via BINARY_ARGS option, and
# - don't use internal API like qt_internal_add_*
#
# We can't run binaries in the simulator or on-device, but we at least
# want build coverage (app linking succeeds).
if(IOS)
    return()
endif()

set(module_includes "")

if(TARGET Qt::Qml)
    list(APPEND module_includes
        Qml QQmlEngine
    )
endif()

if(TARGET Qt::Quick)
    list(APPEND module_includes
        Quick QQuickWindow
    )
endif()

if(TARGET Qt::QuickControls2)
    list(APPEND module_includes
        QuickControls2 QQuickStyle
    )
endif()

_qt_internal_test_module_includes(${module_includes})

if(TARGET Qt::Qml)
    if(NOT CMAKE_CROSSCOMPILING)
        _qt_internal_test_expect_pass(tooling_imports TESTNAME cmake_tooling_imports BINARY cmake_test)
        set_tests_properties(cmake_tooling_imports PROPERTIES
            FAIL_REGULAR_EXPRESSION "(Info|Warning|Error): [a-zA-Z]*\\.qml:[0-9]"
        )
    endif()
    # TODO: Plugin targets are not available in shared builds at the moment QTBUG-94066
    if(NOT QT6_IS_SHARED_LIBS_BUILD)
        _qt_internal_test_expect_pass(test_plugins)
    endif()
    _qt_internal_test_expect_pass(empty_qmldir)
    If(NOT ANDROID) # QML only project cannot run on Android with C++ enty point
        _qt_internal_test_expect_pass(qmlquery)
    endif()
    _qt_internal_test_expect_fail(test_internal_singleton)

    # Make sure that we don't attempt to run binary when crosscompiling
    if(NOT CMAKE_CROSSCOMPILING)
        set(test_static_qml_module_extra_args BINARY test_static_qml_module)
    else()
        set(test_static_qml_module_extra_args "")
    endif()
    _qt_internal_test_expect_pass(test_static_qml_module ${test_static_qml_module_extra_args})
    _qt_internal_test_expect_pass(test_javascript_files TESTNAME cmake_test_javascript_files)
    set_tests_properties(cmake_test_javascript_files PROPERTIES
        FAIL_REGULAR_EXPRESSION "(Good\.js|good\.js|Included\.js|Excluded\.js|Good\.mjs) is not an ECMAScript module"
        PASS_REGULAR_EXPRESSION "Bad\.js is not an ECMAScript module"
    )

    _qt_internal_test_expect_fail(test_invalid_qml_uri)

    set(test_duplicate_files_extra_args "")
    if(NOT CMAKE_CROSSCOMPILING)
        list(APPEND test_duplicate_files_extra_args BINARY main)
    endif()
    _qt_internal_test_expect_pass(test_duplicate_files
        TESTNAME test_duplicate_files_qml_files
        BUILD_OPTIONS -Drun=case_qml_files
        ${test_duplicate_files_extra_args}
    )
    _qt_internal_test_expect_pass(test_duplicate_files
        TESTNAME test_duplicate_files_resources
        BUILD_OPTIONS -Drun=case_resources
        ${test_duplicate_files_extra_args}
    )
endif()

if(TARGET Qt::Quick)
    if(NOT CMAKE_CROSSCOMPILING)
        _qt_internal_test_expect_pass(shared_qml_module
            BINARY "${CMAKE_CTEST_COMMAND}"
            BINARY_ARGS "-V"
        )
        _qt_internal_test_expect_pass(qtquickcompiler BINARY qqc_test)
        if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.21")
            _qt_internal_test_expect_pass(test_common_import_path
                TESTNAME cmake_test_common_import_path
                BINARY cmake_test
            )
        endif()
        if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
            _qt_internal_test_expect_pass(test_generate_qmlls_ini BINARY tst_generate_qmlls_ini)
            _qt_internal_test_expect_pass(test_aotstats_integration BINARY tst_aotstats_integration)
            _qt_internal_test_expect_pass(test_target_qml_sources_in_different_scope)
        endif()
        _qt_internal_test_expect_pass(
                   test_link_qml_module_without_target
            BINARY test_link_qml_module_without_target)
    endif()
    if(NOT QT6_IS_SHARED_LIBS_BUILD)
        _qt_internal_test_expect_pass(test_import_static_shapes_plugin_resources
            BINARY "${CMAKE_CTEST_COMMAND}"
            BINARY_ARGS "-V"
        )
        if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.21")
            _qt_internal_test_expect_pass(qmlimportscanner
                TESTNAME cmake_qmlimportscanner
                BINARY qis_test
            )
        endif()
    endif()
endif()

add_subdirectory(RunCMake)
