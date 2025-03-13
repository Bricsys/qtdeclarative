// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml
import Test

BindablePoint {
    id: root
    property int changes: 0
    property alias aa: root.point.x
    onAaChanged: ++changes
}
