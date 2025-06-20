// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>
#include <QDir>
#include <QDebug>

class MyPluginSingleton : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char *uri) override
    {
        Q_ASSERT(QLatin1String(uri) == "org.qtproject.ModuleWithQmlSingleton");
        qmlRegisterModule(uri, 1, 0);
    }
};

#include "plugin.moc"
