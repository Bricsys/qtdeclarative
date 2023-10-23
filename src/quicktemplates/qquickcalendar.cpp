// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qquickcalendar_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Calendar
    \inherits QObject
//! \instantiates QQuickCalendar
    \inqmlmodule QtQuick.Controls
    \brief A calendar namespace.

    The Calendar singleton provides miscellaneous calendar-related
    utilities.

    \include zero-based-months.qdocinc

    \sa MonthGrid, DayOfWeekRow, WeekNumberColumn
*/

QQuickCalendar::QQuickCalendar(QObject *parent) : QObject(parent)
{
}

QT_END_NAMESPACE

#include "moc_qquickcalendar_p.cpp"
