// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma Strict

import QtQml
import TestTypes as T

T.UsingUserObject {
    id: self
    property int valA: val.a
    property int myA: a
    property int myA2: self.a

    function twiddle() {
        val.a = 55
        a = 57
        self.a = 59
    }
}
