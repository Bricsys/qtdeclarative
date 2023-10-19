// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial
//! [define]
import QtQuick
import QtQuick.Controls

Button {
    width: 50; height: 50
    onClicked: { width = 100; }
}
//! [define]
