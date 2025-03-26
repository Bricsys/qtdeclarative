// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickbasictheme_p.h"

#include <QtQuickTemplates2/private/qquicktheme_p.h>
#include <QtQuickControls2/private/qquickstyle_p.h>

QT_BEGIN_NAMESPACE

void QQuickBasicTheme::initialize(QQuickTheme *theme)
{
    QPalette systemPalette;

    const bool isDarkSystemTheme = QQuickStylePrivate::isDarkSystemTheme();

    const QRgb base(isDarkSystemTheme ? 0xFF000000 : 0xFFFFFFFF);
    const QRgb disabledBase(isDarkSystemTheme ? 0xFF292929 : 0xFFD6D6D6);
    const QRgb button(isDarkSystemTheme ? 0xFF2F2F2F : 0xFFE0E0E0);
    const QRgb buttonText(isDarkSystemTheme ? 0xFFD4D6D8 : 0xFF26282A);
    const QRgb disabledButtonText(isDarkSystemTheme ? 0x4DD4D6D8 : 0x4D26282A);
    const QRgb brightText(isDarkSystemTheme ? 0xFF000000 : 0xFFFFFFFF);
    const QRgb disabledBrightText(isDarkSystemTheme ? 0x4D000000 : 0x4DFFFFFF);
    const QRgb dark(isDarkSystemTheme ? 0xFFC8C9CB : 0xFF353637);
    const QRgb highlight(isDarkSystemTheme ? 0xFF0D69F2 : 0xFF0066FF);
    const QRgb disabledHighlight(isDarkSystemTheme ? 0xFF01060F : 0xFFF0F6FF);
    const QRgb highlightedText(isDarkSystemTheme ? 0xFFFDFDFD : 0xFF090909);
    const QRgb light(isDarkSystemTheme ? 0xFF1A1A1A : 0xFFF6F6F6);
    const QRgb link(isDarkSystemTheme ? 0xFF2F86B1 : 0xFF45A7D7);
    const QRgb mid(isDarkSystemTheme ? 0xFF626262 : 0xFFBDBDBD);
    const QRgb midlight(isDarkSystemTheme ? 0xFF2C2C2C : 0xFFE4E4E4);
    const QRgb text(isDarkSystemTheme ? 0xFFEFF0F2 : 0xFF353637);
    const QRgb disabledText(isDarkSystemTheme ? 0x7FC8C9CB : 0x7F353637);
    const QRgb shadow(isDarkSystemTheme ? 0xFF28282A : 0xFF28282A);
    const QRgb toolTipBase(isDarkSystemTheme ? 0xFF000000 : 0xFFFFFFFF);
    const QRgb toolTipText(isDarkSystemTheme ? 0xFFFFFFFF : 0xFF000000);
    const QRgb window(isDarkSystemTheme ? 0xFF000000 : 0xFFFFFFFF);
    const QRgb windowText(isDarkSystemTheme ? 0xFFD4D6D8 : 0xFF26282A);
    const QRgb disabledWindowText(isDarkSystemTheme ? 0xFF3F4040 : 0xFFBDBEBF);
    const QRgb placeholderText(isDarkSystemTheme ? 0x88C8C9CB : 0x88353637);

    systemPalette.setColor(QPalette::Base, base);
    systemPalette.setColor(QPalette::Disabled, QPalette::Base, disabledBase);

    systemPalette.setColor(QPalette::Button, button);

    systemPalette.setColor(QPalette::ButtonText, buttonText);
    systemPalette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledButtonText);

    systemPalette.setColor(QPalette::BrightText, brightText);
    systemPalette.setColor(QPalette::Disabled, QPalette::BrightText, disabledBrightText);

    systemPalette.setColor(QPalette::Dark, dark);

    systemPalette.setColor(QPalette::Highlight, highlight);
    systemPalette.setColor(QPalette::Disabled, QPalette::Highlight, disabledHighlight);

    systemPalette.setColor(QPalette::HighlightedText, highlightedText);

    systemPalette.setColor(QPalette::Light, light);

    systemPalette.setColor(QPalette::Link, link);

    systemPalette.setColor(QPalette::Mid, mid);

    systemPalette.setColor(QPalette::Midlight, midlight);

    systemPalette.setColor(QPalette::Text, text);
    systemPalette.setColor(QPalette::Disabled, QPalette::Text, disabledText);

    systemPalette.setColor(QPalette::Shadow, shadow);

    systemPalette.setColor(QPalette::ToolTipBase, toolTipBase);
    systemPalette.setColor(QPalette::ToolTipText, toolTipText);

    systemPalette.setColor(QPalette::Window, window);

    systemPalette.setColor(QPalette::WindowText, windowText);
    systemPalette.setColor(QPalette::Disabled, QPalette::WindowText, disabledWindowText);

    systemPalette.setColor(QPalette::PlaceholderText, placeholderText);

    theme->setPalette(QQuickTheme::System, systemPalette);
}

void QQuickBasicTheme::updateTheme()
{
    QQuickTheme *theme = QQuickTheme::instance();
    if (!theme)
        return;
    initialize(theme);
}

QT_END_NAMESPACE
