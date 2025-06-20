// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlsettings_p.h"
#include <qcoreevent.h>
#include <qcoreapplication.h>
#include <qloggingcategory.h>
#include <qsettings.h>
#include <qpointer.h>
#include <qjsvalue.h>
#include <qqmlinfo.h>
#include <qdebug.h>
#include <qhash.h>

QT_BEGIN_NAMESPACE

/*!
    \qmlmodule Qt.labs.settings 1.0
    \title Qt Labs Settings QML Types
    \ingroup qmlmodules
    \deprecated [6.5] Use \l [QML] {QtCore::}{Settings} from Qt Qml Core instead.
    \brief Provides persistent platform-independent application settings.

    To use this module, import the module with the following line:

    \code
    import Qt.labs.settings
    \endcode
*/

/*!
    \qmltype Settings
//!    \nativetype QQmlSettingsLabs
    \inqmlmodule Qt.labs.settings
    \ingroup settings
    \deprecated [6.5] Use \l [QML] {QtCore::}{Settings} from Qt Qml Core instead.
    \brief Provides persistent platform-independent application settings.

    The Settings type provides persistent platform-independent application settings.

    \note This type is made available by importing the \b Qt.labs.settings module.
    \e {Types in the Qt.labs module are not guaranteed to remain compatible
    in future versions.}

    Users normally expect an application to remember its settings (window sizes
    and positions, options, etc.) across sessions. The Settings type enables you
    to save and restore such application settings with the minimum of effort.

    Individual setting values are specified by declaring properties within a
    Settings element. All \l {QML Value Types}{value type} properties are
    supported. The recommended approach is to use property aliases in order
    to get automatic property updates both ways. The following example shows
    how to use Settings to store and restore the geometry of a window.

    \qml
    import QtQuick.Window
    import Qt.labs.settings

    Window {
        id: window

        width: 800
        height: 600

        Settings {
            property alias x: window.x
            property alias y: window.y
            property alias width: window.width
            property alias height: window.height
        }
    }
    \endqml

    At first application startup, the window gets default dimensions specified
    as 800x600. Notice that no default position is specified - we let the window
    manager handle that. Later when the window geometry changes, new values will
    be automatically stored to the persistent settings. The second application
    run will get initial values from the persistent settings, bringing the window
    back to the previous position and size.

    A fully declarative syntax, achieved by using property aliases, comes at the
    cost of storing persistent settings whenever the values of aliased properties
    change. Normal properties can be used to gain more fine-grained control over
    storing the persistent settings. The following example illustrates how to save
    a setting on component destruction.

    \qml
    import QtQuick
    import Qt.labs.settings

    Item {
        id: page

        state: settings.state

        states: [
            State {
                name: "active"
                // ...
            },
            State {
                name: "inactive"
                // ...
            }
        ]

        Settings {
            id: settings
            property string state: "active"
        }

        Component.onDestruction: {
            settings.state = page.state
        }
    }
    \endqml

    Notice how the default value is now specified in the persistent setting property,
    and the actual property is bound to the setting in order to get the initial value
    from the persistent settings.

    \section1 Application Identifiers

    Application specific settings are identified by providing application
    \l {QCoreApplication::applicationName}{name},
    \l {QCoreApplication::organizationName}{organization} and
    \l {QCoreApplication::organizationDomain}{domain}, or by specifying
    \l fileName.

    \code
    #include <QGuiApplication>
    #include <QQmlApplicationEngine>

    int main(int argc, char *argv[])
    {
        QGuiApplication app(argc, argv);
        app.setOrganizationName("Some Company");
        app.setOrganizationDomain("somecompany.com");
        app.setApplicationName("Amazing Application");

        QQmlApplicationEngine engine("main.qml");
        return app.exec();
    }
    \endcode

    These are typically specified in C++ in the beginning of \c main(),
    but can also be controlled in QML via the following properties:
    \list
        \li \l {Qt::application}{Qt.application.name},
        \li \l {Qt::application}{Qt.application.organization} and
        \li \l {Qt::application}{Qt.application.domain}.
    \endlist

    \section1 Categories

    Application settings may be divided into logical categories by specifying
    a category name via the \l category property. Using logical categories not
    only provides a cleaner settings structure, but also prevents possible
    conflicts between setting keys.

    If several categories are required, use several Settings objects, each with
    their own category:

    \qml
    Item {
        id: panel

        visible: true

        Settings {
            category: "OutputPanel"
            property alias visible: panel.visible
            // ...
        }

        Settings {
            category: "General"
            property alias fontSize: fontSizeSpinBox.value
            // ...
        }
    }
    \endqml

    Instead of ensuring that all settings in the application have unique names,
    the settings can be divided into unique categories that may then contain
    settings using the same names that are used in other categories - without
    a conflict.

    \section1 Notes

    The current implementation is based on \l QSettings. This imposes certain
    limitations, such as missing change notifications. Writing a setting value
    using one instance of Settings does not update the value in another Settings
    instance, even if they are referring to the same setting in the same category.

    The information is stored in the system registry on Windows, and in XML
    preferences files on \macos. On other Unix systems, in the absence of a
    standard, INI text files are used. See \l QSettings documentation for
    more details.

    \sa {QtCore::}{Settings}, QSettings
*/

Q_STATIC_LOGGING_CATEGORY(lcSettings, "qt.labs.settings")

static const int settingsWriteDelay = 500;

class QQmlSettingsLabsPrivate
{
    Q_DECLARE_PUBLIC(QQmlSettingsLabs)

public:
    QQmlSettingsLabsPrivate();

    QSettings *instance() const;

    void init();
    void reset();

    void load();
    void store();

    void _q_propertyChanged();
    QVariant readProperty(const QMetaProperty &property) const;

    QQmlSettingsLabs *q_ptr = nullptr;
    int timerId = 0;
    bool initialized = false;
    QString category;
    QString fileName;
    mutable QPointer<QSettings> settings;
    QHash<const char *, QVariant> changedProperties;
};

QQmlSettingsLabsPrivate::QQmlSettingsLabsPrivate() {}

QSettings *QQmlSettingsLabsPrivate::instance() const
{
    if (!settings) {
        QQmlSettingsLabs *q = const_cast<QQmlSettingsLabs*>(q_func());
        settings = fileName.isEmpty() ? new QSettings(q) : new QSettings(fileName, QSettings::IniFormat, q);
        if (settings->status() != QSettings::NoError) {
            // TODO: can't print out the enum due to the following error:
            // error: C2666: 'QQmlInfo::operator <<': 15 overloads have similar conversions
            qmlWarning(q) << "Failed to initialize QSettings instance. Status code is: " << int(settings->status());

            if (settings->status() == QSettings::AccessError) {
                QVector<QString> missingIdentifiers;
                if (QCoreApplication::organizationName().isEmpty())
                    missingIdentifiers.append(QLatin1String("organizationName"));
                if (QCoreApplication::organizationDomain().isEmpty())
                    missingIdentifiers.append(QLatin1String("organizationDomain"));
                if (QCoreApplication::applicationName().isEmpty())
                    missingIdentifiers.append(QLatin1String("applicationName"));

                if (!missingIdentifiers.isEmpty())
                    qmlWarning(q) << "The following application identifiers have not been set: " << missingIdentifiers;
            }
            return settings;
        }

        if (!category.isEmpty())
            settings->beginGroup(category);
        if (initialized)
            q->d_func()->load();
    }
    return settings;
}

void QQmlSettingsLabsPrivate::init()
{
    if (!initialized) {
        qCDebug(lcSettings) << "QQmlSettingsLabs: stored at" << instance()->fileName();
        load();
        initialized = true;
    }
}

void QQmlSettingsLabsPrivate::reset()
{
    if (initialized && settings && !changedProperties.isEmpty())
        store();
    delete settings;
}

void QQmlSettingsLabsPrivate::load()
{
    Q_Q(QQmlSettingsLabs);
    const QMetaObject *mo = q->metaObject();
    const int offset = mo->propertyOffset();
    const int count = mo->propertyCount();

    // don't save built-in properties if there aren't any qml properties
    if (offset == 1)
        return;

    for (int i = offset; i < count; ++i) {
        QMetaProperty property = mo->property(i);
        const QString propertyName = QString::fromUtf8(property.name());

        const QVariant previousValue = readProperty(property);
        const QVariant currentValue = instance()->value(propertyName,
                                                        previousValue);

        if (!currentValue.isNull() && (!previousValue.isValid()
                || (currentValue.canConvert(previousValue.metaType())
                    && previousValue != currentValue))) {
            property.write(q, currentValue);
            qCDebug(lcSettings) << "QQmlSettingsLabs: load" << property.name() << "setting:" << currentValue << "default:" << previousValue;
        }

        // ensure that a non-existent setting gets written
        // even if the property wouldn't change later
        if (!instance()->contains(propertyName))
            _q_propertyChanged();

        // setup change notifications on first load
        if (!initialized && property.hasNotifySignal()) {
            static const int propertyChangedIndex = mo->indexOfSlot("_q_propertyChanged()");
            QMetaObject::connect(q, property.notifySignalIndex(), q, propertyChangedIndex);
        }
    }
}

void QQmlSettingsLabsPrivate::store()
{
    QHash<const char *, QVariant>::const_iterator it = changedProperties.constBegin();
    while (it != changedProperties.constEnd()) {
        instance()->setValue(QString::fromUtf8(it.key()), it.value());
        qCDebug(lcSettings) << "QQmlSettingsLabs: store" << it.key() << ":" << it.value();
        ++it;
    }
    changedProperties.clear();
}

void QQmlSettingsLabsPrivate::_q_propertyChanged()
{
    Q_Q(QQmlSettingsLabs);
    const QMetaObject *mo = q->metaObject();
    const int offset = mo->propertyOffset();
    const int count = mo->propertyCount();
    for (int i = offset; i < count; ++i) {
        const QMetaProperty &property = mo->property(i);
        const QVariant value = readProperty(property);
        changedProperties.insert(property.name(), value);
        qCDebug(lcSettings) << "QQmlSettingsLabs: cache" << property.name() << ":" << value;
    }
    if (timerId != 0)
        q->killTimer(timerId);
    timerId = q->startTimer(settingsWriteDelay);
}

QVariant QQmlSettingsLabsPrivate::readProperty(const QMetaProperty &property) const
{
    Q_Q(const QQmlSettingsLabs);
    QVariant var = property.read(q);
    if (var.metaType() == QMetaType::fromType<QJSValue>())
        var = var.value<QJSValue>().toVariant();
    return var;
}

QQmlSettingsLabs::QQmlSettingsLabs(QObject *parent)
    : QObject(parent), d_ptr(new QQmlSettingsLabsPrivate)
{
    Q_D(QQmlSettingsLabs);
    d->q_ptr = this;
}

QQmlSettingsLabs::~QQmlSettingsLabs()
{
    Q_D(QQmlSettingsLabs);
    d->reset(); // flush pending changes
}

/*!
    \qmlproperty string Settings::category

    This property holds the name of the settings category.

    Categories can be used to group related settings together.
*/
QString QQmlSettingsLabs::category() const
{
    Q_D(const QQmlSettingsLabs);
    return d->category;
}

void QQmlSettingsLabs::setCategory(const QString &category)
{
    Q_D(QQmlSettingsLabs);
    if (d->category != category) {
        d->reset();
        d->category = category;
        if (d->initialized)
            d->load();
    }
}

/*!
    \qmlproperty string Settings::fileName

    This property holds the path to the settings file. If the file doesn't
    already exist, it is created.

    \since Qt 5.12

    \sa QSettings::fileName, QSettings::IniFormat
*/
QString QQmlSettingsLabs::fileName() const
{
    Q_D(const QQmlSettingsLabs);
    return d->fileName;
}

void QQmlSettingsLabs::setFileName(const QString &fileName)
{
    Q_D(QQmlSettingsLabs);
    if (d->fileName != fileName) {
        d->reset();
        d->fileName = fileName;
        if (d->initialized)
            d->load();
    }
}

/*!
   \qmlmethod var Settings::value(string key, var defaultValue)

   Returns the value for setting \a key. If the setting doesn't exist,
   returns \a defaultValue.

   \since Qt 5.12

   \sa QSettings::value
*/
QVariant QQmlSettingsLabs::value(const QString &key, const QVariant &defaultValue) const
{
    Q_D(const QQmlSettingsLabs);
    return d->instance()->value(key, defaultValue);
}

/*!
   \qmlmethod Settings::setValue(string key, var value)

   Sets the value of setting \a key to \a value. If the key already exists,
   the previous value is overwritten.

   \since Qt 5.12

   \sa QSettings::setValue
*/
void QQmlSettingsLabs::setValue(const QString &key, const QVariant &value)
{
    Q_D(const QQmlSettingsLabs);
    d->instance()->setValue(key, value);
    qCDebug(lcSettings) << "QQmlSettingsLabs: setValue" << key << ":" << value;
}

/*!
   \qmlmethod Settings::sync()

    Writes any unsaved changes to permanent storage, and reloads any
    settings that have been changed in the meantime by another
    application.

    This function is called automatically from QSettings's destructor and
    by the event loop at regular intervals, so you normally don't need to
    call it yourself.

   \sa QSettings::sync
*/
void QQmlSettingsLabs::sync()
{
    Q_D(QQmlSettingsLabs);
    d->instance()->sync();
}

void QQmlSettingsLabs::classBegin()
{
}

void QQmlSettingsLabs::componentComplete()
{
    Q_D(QQmlSettingsLabs);
    d->init();
    qmlWarning(this) << "The Settings type from Qt.labs.settings is deprecated"
                        " and will be removed in a future release. Please use "
                        "the one from QtCore instead.";
}

void QQmlSettingsLabs::timerEvent(QTimerEvent *event)
{
    Q_D(QQmlSettingsLabs);
    if (event->timerId() == d->timerId) {
        killTimer(d->timerId);
        d->timerId = 0;

        d->store();
    }
    QObject::timerEvent(event);
}

QT_END_NAMESPACE

#include "moc_qqmlsettings_p.cpp"
