// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

import QtQuick
import QtQuick.NativeStyle as NativeStyle

NativeStyle.DefaultDial {
    id: control
    font.pixelSize: background.styleFont(control).pixelSize
}
