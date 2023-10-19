# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial

qt_internal_add_plugin(QmlLintQuickPlugin
    OUTPUT_NAME quicklintplugin
    PLUGIN_TYPE qmllint
    SOURCES
        quicklintplugin.h
        quicklintplugin.cpp
    LIBRARIES
        Qt::QmlCompilerPrivate
)
