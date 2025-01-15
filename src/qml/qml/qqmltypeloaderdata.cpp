// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmltypeloaderdata_p.h"

#include <private/qqmldirdata_p.h>
#include <private/qqmlprofiler_p.h>
#include <private/qqmlscriptblob_p.h>
#include <private/qqmltypedata_p.h>

QT_BEGIN_NAMESPACE

QQmlTypeLoaderLockedData::QQmlTypeLoaderLockedData(QQmlEngine *engine)
    : m_engine(engine)
{
}

QT_END_NAMESPACE
