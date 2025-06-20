// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QStringList>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>
#include <QDebug>

class FloorPluginType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int value READ value);

public:
    int value() const { return 16; }
};

class MyMixedPluginVersion : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char *uri) override
    {
        Q_ASSERT(QLatin1String(uri) == "org.qtproject.AutoTestQmlVersionPluginType");
        qmlRegisterType<FloorPluginType>(uri, 1, 4, "Floor");
    }
};

#include "plugin.moc"
