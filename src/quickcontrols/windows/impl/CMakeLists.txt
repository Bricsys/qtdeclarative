# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(qml_files
    "CheckIndicator.qml"
    "CopyAction.qml"
    "CutAction.qml"
    "DeleteAction.qml"
    "PasteAction.qml"
    "SelectAllAction.qml"
    "SwitchIndicator.qml"
    "TextEditingContextMenu.qml"
)

qt_internal_add_qml_module(QuickControls2WindowsStyleImpl
    URI "QtQuick.Controls.Windows.impl"
    VERSION "${PROJECT_VERSION}"
    PAST_MAJOR_VERSIONS 2
    CLASS_NAME QtQuickControls2WindowsStyleImplPlugin
    NO_PRIVATE_MODULE
    DEPENDENCIES
        QtQuick/auto
    PLUGIN_TARGET qtquickcontrols2windowsstyleimplplugin
    QML_FILES
        ${qml_files}
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
)
