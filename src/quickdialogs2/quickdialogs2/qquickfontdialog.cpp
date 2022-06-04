/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QQUICKFONTDIALOG_CPP
#define QQUICKFONTDIALOG_CPP

#include "qquickfontdialog_p.h"

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype FontDialog
    \inherits Dialog
//!     \instantiates QQuickFontDialog
    \inqmlmodule QtQuick.Dialogs
    \since 6.2
    \brief A font dialog.

    The FontDialog type provides a QML API for font dialogs.

    \image qtquickdialogs-fontdialog-gtk.png

    To show a font dialog, construct an instance of FontDialog, set the
    desired properties, and call \l {Dialog::}{open()}. The \l currentFont
    property can be used to determine the currently selected font in the
    dialog. The \l selectedFont property is updated only after the final selection
    has been made by accepting the dialog.

    \code
    MenuItem {
        text: "Font"
        onTriggered: fontDialog.open()
    }

    FontDialog {
        id: fontDialog
        currentFont.family: document.font
    }

    MyDocument {
        id: document
        font: fontDialog.selectedFont
    }
    \endcode

    \section2 Availability

    A native platform font dialog is currently available on the following platforms:

    \list
    \li macOS
    \li Linux (when running with the GTK+ platform theme)
    \endlist

    \include includes/fallback.qdocinc
*/

Q_LOGGING_CATEGORY(lcFontDialog, "qt.quick.dialogs.fontdialog")

QQuickFontDialog::QQuickFontDialog(QObject *parent)
    : QQuickAbstractDialog(QPlatformTheme::FontDialog, parent),
      m_options(QFontDialogOptions::create())
{
}

/*!
    \qmlproperty font QtQuick.Dialogs::FontDialog::currentFont

    This property holds the currently selected font in the dialog.

    Unlike the \l selectedFont property, the \c currentFont property is updated
    while the user is selecting fonts in the dialog, even before the final
    selection has been made.

    \sa selectedFont
*/

QFont QQuickFontDialog::currentFont() const
{
    if (QPlatformFontDialogHelper *fontDialog = qobject_cast<QPlatformFontDialogHelper *>(handle()))
        return fontDialog->currentFont();
    return QFont();
}

void QQuickFontDialog::setCurrentFont(const QFont &font)
{
    if (QPlatformFontDialogHelper *fontDialog =
        qobject_cast<QPlatformFontDialogHelper *>(handle()))
            fontDialog->setCurrentFont(font);
}

/*!
    \qmlproperty font QtQuick.Dialogs::FontDialog::selectedFont

    This property holds the final accepted font.

    Unlike the \l currentFont property, the \c selectedFont property is not updated
    while the user is selecting fonts in the dialog, but only after the final
    selection has been made. That is, when the user has clicked \uicontrol Open
    to accept a font. Alternatively, the \l {Dialog::}{accepted()} signal
    can be handled to get the final selection.

    \sa currentFont, {Dialog::}{accepted()}
*/

QFont QQuickFontDialog::selectedFont() const
{
    return m_selectedFont;
}

void QQuickFontDialog::setSelectedFont(const QFont &font)
{
    if (m_selectedFont == font)
        return;

    m_selectedFont = font;
    emit selectedFontChanged();
}

/*!
    \qmlproperty flags QtQuick.Dialogs::FontDialog::options

    This property holds the various options that affect the look and feel of the dialog.

    By default, all options are disabled.

    Options should be set before showing the dialog. Setting them while the dialog is
    visible is not guaranteed to have an immediate effect on the dialog (depending on
    the option and on the platform).

    Available options:
    \value FontDialog.ScalableFonts Show scalable fonts.
    \value FontDialog.NonScalableFonts Show non-scalable fonts.
    \value FontDialog.MonospacedFonts Show monospaced fonts.
    \value FontDialog.ProportionalFonts Show proportional fonts.
    \value FontDialog.NoButtons Don't display \uicontrol Open and \uicontrol Cancel buttons (useful
   for "live dialogs").
*/

QFontDialogOptions::FontDialogOptions QQuickFontDialog::options() const
{
    return m_options->options();
}

void QQuickFontDialog::setOptions(QFontDialogOptions::FontDialogOptions options)
{
    if (options == m_options->options())
        return;

    m_options->setOptions(options);
    emit optionsChanged();
}

void QQuickFontDialog::resetOptions()
{
    setOptions({});
}

bool QQuickFontDialog::useNativeDialog() const
{
    return QQuickAbstractDialog::useNativeDialog()
            && !(m_options->testOption(QFontDialogOptions::DontUseNativeDialog));
}

void QQuickFontDialog::onCreate(QPlatformDialogHelper *dialog)
{
    if (QPlatformFontDialogHelper *fontDialog = qobject_cast<QPlatformFontDialogHelper *>(dialog)) {
        connect(fontDialog, &QPlatformFontDialogHelper::currentFontChanged, this,
                &QQuickFontDialog::currentFontChanged);
        connect(fontDialog, &QPlatformFontDialogHelper::fontSelected, this,
                &QQuickFontDialog::setSelectedFont);
        fontDialog->setOptions(m_options);
    }
}

void QQuickFontDialog::onShow(QPlatformDialogHelper *dialog)
{
    m_options->setWindowTitle(title());
    if (QPlatformFontDialogHelper *fontDialog = qobject_cast<QPlatformFontDialogHelper *>(dialog))
        fontDialog->setOptions(m_options); // setOptions only assigns a member and isn't virtual
}

void QQuickFontDialog::accept()
{
    if (auto fontDialog = qobject_cast<QPlatformFontDialogHelper *>(handle()))
        setSelectedFont(fontDialog->currentFont());
    QQuickAbstractDialog::accept();
}

QT_END_NAMESPACE

#endif // QQUICKFONTDIALOG_CPP
