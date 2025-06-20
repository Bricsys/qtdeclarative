// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QStringList>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>
#include <QDebug>

class MyChildPluginType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue)

public:
    MyChildPluginType(QObject *parent=nullptr) : QObject(parent)
    {
        qWarning("child import worked");
    }

    int value() const { return v; }
    void setValue(int i) { v = i; }

private:
    int v;
};


class MyChildPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    MyChildPlugin() { qWarning("child plugin created"); }

    void registerTypes(const char *uri) override
    {
        Q_ASSERT(QLatin1String(uri) == "org.qtproject.AutoTestQmlPluginType.ChildPlugin");
        qmlRegisterType<MyChildPluginType>(uri, 1, 0, "MyChildPluginType");
    }
};

#include "childplugin.moc"
