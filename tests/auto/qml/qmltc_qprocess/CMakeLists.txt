# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmltc_qprocess LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qmltc_qprocess
    SOURCES
        tst_qmltc_qprocess.cpp
    LIBRARIES
        Qt::Core
        Qt::Qml
        Qt::QuickTestUtilsPrivate
)

# special setup for singleton files:
set_source_files_properties(data/SingletonThing.qml data/singletonUncreatable.qml
    PROPERTIES QT_QML_SINGLETON_TYPE true)

qt_policy(SET QTP0001 NEW)

qt6_add_qml_module(tst_qmltc_qprocess
    VERSION 1.0
    URI QmltcQProcessTests
    SOURCES
        cpptypes/testtype.h
        cpptypes/typewithrequiredproperty.h
    DEPENDENCIES
        QtQuick/auto
    QML_FILES
        data/dummy.qml
        data/inlineComponentInvalidAlias.qml
        data/SingletonThing.qml
        data/erroneousFile.qml
        data/invalidAliasRevision.qml
        data/ComponentType.qml
        data/inlineComponentWithEnum.qml
        data/singletonUncreatable.qml
        data/uncreatable.qml
        data/invalidSignalHandlers.qml
        data/QmlBaseFromAnotherModule.qml
        data/invalidTypeAnnotation.qml
        data/constructFromString.qml
        data/unboundRequiredPropertyInInlineComponent.qml
        data/componentDefinitionInnerRequiredProperty.qml
        data/componentDefinitionInnerRequiredPropertyFromOutside.qml
        data/innerLevelRequiredProperty.qml
        data/customParsed.qml
    NO_GENERATE_EXTRA_QMLDIRS
)

set(common_libraries
    Qt::Core
    Qt::Qml
    Qt::QmlPrivate
    Qt::QuickPrivate
    Qt::TestPrivate
    Qt::Gui # QColor, QMatrix4x4, ...
)

# use PUBLIC everywhere to simplify the build of the test binary
target_include_directories(tst_qmltc_qprocess PUBLIC cpptypes/)
target_link_libraries(tst_qmltc_qprocess PUBLIC ${common_libraries})

add_dependencies(tst_qmltc_qprocess Qt::qmltc)

# fetch --resource arguments manually (mimics the logic of qmltc compilation
# command)
_qt_internal_genex_getjoinedproperty(qrc_args tst_qmltc_qprocess
    _qt_generated_qrc_files "" "_::_"
)
target_compile_definitions(tst_qmltc_qprocess PRIVATE
    TST_QMLTC_QPROCESS_RESOURCES="${qrc_args}"
    QT_NO_CAST_FROM_ASCII
)

qt_internal_extend_target(tst_qmltc_qprocess CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qmltc_qprocess CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
