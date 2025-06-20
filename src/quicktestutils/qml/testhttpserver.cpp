// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "testhttpserver_p.h"
#include <QTcpSocket>
#include <QDebug>
#include <QFile>
#include <QTimer>
#include <QTest>
#include <QQmlFile>

QT_BEGIN_NAMESPACE

/*!
\internal
\class TestHTTPServer
\brief provides a very, very basic HTTP server for testing.

Inside the test case, an instance of TestHTTPServer should be created, with the
appropriate port to listen on.  The server will listen on the localhost interface.

Directories to serve can then be added to server, which will be added as "roots".
Each root can be added as a Normal, Delay or Disconnect root.  Requests for files
within a Normal root are returned immediately.  Request for files within a Delay
root are delayed for 500ms, and then served.  Requests for files within a Disconnect
directory cause the server to disconnect immediately.  A request for a file that isn't
found in any root will return a 404 error.

If you have the following directory structure:

\code
disconnect/disconnectTest.qml
files/main.qml
files/Button.qml
files/content/WebView.qml
slowFiles/slowMain.qml
\endcode
it can be added like this:
\code
TestHTTPServer server;
QVERIFY2(server.listen(14445), qPrintable(server.errorString()));
server.serveDirectory("disconnect", TestHTTPServer::Disconnect);
server.serveDirectory("files");
server.serveDirectory("slowFiles", TestHTTPServer::Delay);
\endcode

The following request urls will then result in the appropriate action:
\table
\header \li URL \li Action
\row \li http://localhost:14445/disconnectTest.qml \li Disconnection
\row \li http://localhost:14445/main.qml \li main.qml returned immediately
\row \li http://localhost:14445/Button.qml \li Button.qml returned immediately
\row \li http://localhost:14445/content/WebView.qml \li content/WebView.qml returned immediately
\row \li http://localhost:14445/slowMain.qml \li slowMain.qml returned after 500ms
\endtable
*/

static QList<QByteArrayView> ignoredHeaders = {
    "HTTP2-Settings", // We ignore this
    "Upgrade", // We ignore this as well
};

static QUrl localHostUrl(quint16 port)
{
    QUrl url;
    url.setScheme(QStringLiteral("http"));
    url.setHost(QStringLiteral("127.0.0.1"));
    url.setPort(port);
    return url;
}

TestHTTPServer::TestHTTPServer()
    : m_state(AwaitingHeader)
{
    QObject::connect(&m_server, &QTcpServer::newConnection, this, &TestHTTPServer::newConnection);
}

bool TestHTTPServer::listen()
{
    return m_server.listen(QHostAddress::LocalHost, 0);
}

QUrl TestHTTPServer::baseUrl() const
{
    return localHostUrl(m_server.serverPort());
}

quint16 TestHTTPServer::port() const
{
    return m_server.serverPort();
}

QUrl TestHTTPServer::url(const QString &documentPath) const
{
    return baseUrl().resolved(documentPath);
}

QString TestHTTPServer::urlString(const QString &documentPath) const
{
    return url(documentPath).toString();
}

QString TestHTTPServer::errorString() const
{
    return m_server.errorString();
}

bool TestHTTPServer::serveDirectory(const QString &dir, Mode mode)
{
    m_directories.append(std::make_pair(dir, mode));
    return true;
}

/*
   Add an alias, so that if filename is requested and does not exist,
   alias may be returned.
*/
void TestHTTPServer::addAlias(const QString &filename, const QString &alias)
{
    m_aliases.insert(filename, alias);
}

void TestHTTPServer::addRedirect(const QString &filename, const QString &redirectName)
{
    m_redirects.insert(filename, redirectName);
}

void TestHTTPServer::registerFileNameForContentSubstitution(const QString &fileName)
{
    m_contentSubstitutedFileNames.insert(fileName);
}

bool TestHTTPServer::wait(const QUrl &expect, const QUrl &reply, const QUrl &body)
{
    m_state = AwaitingHeader;
    m_data.clear();

    QFile expectFile(QQmlFile::urlToLocalFileOrQrc(expect));
    if (!expectFile.open(QIODevice::ReadOnly))
        return false;

    QFile replyFile(QQmlFile::urlToLocalFileOrQrc(reply));
    if (!replyFile.open(QIODevice::ReadOnly))
        return false;

    m_bodyData = QByteArray();
    if (body.isValid()) {
        QFile bodyFile(QQmlFile::urlToLocalFileOrQrc(body));
        if (!bodyFile.open(QIODevice::ReadOnly))
            return false;
        m_bodyData = bodyFile.readAll();
    }

    const QByteArray serverHostUrl
        = QByteArrayLiteral("127.0.0.1:")+ QByteArray::number(m_server.serverPort());

    QByteArray line;
    bool headers_done = false;
    while (!(line = expectFile.readLine()).isEmpty()) {
        line.replace('\r', "");
        if (headers_done) {
            m_waitData.body.append(line);
        } else if (line.at(0) == '\n') {
            headers_done = true;
        } else if (line.endsWith("{{Ignore}}\n")) {
            m_waitData.headerPrefixes.append(line.left(line.size() - strlen("{{Ignore}}\n")));
        } else {
            line.replace("{{ServerHostUrl}}", serverHostUrl);
            m_waitData.headerExactMatches.append(line);
        }
    }

    m_replyData = replyFile.readAll();

    if (!m_replyData.endsWith('\n'))
        m_replyData.append('\n');
    m_replyData.append("Content-length: ");
    m_replyData.append(QByteArray::number(m_bodyData.size()));
    m_replyData.append("\n\n");

    for (int ii = 0; ii < m_replyData.size(); ++ii) {
        if (m_replyData.at(ii) == '\n' && (!ii || m_replyData.at(ii - 1) != '\r')) {
            m_replyData.insert(ii, '\r');
            ++ii;
        }
    }
    m_replyData.append(m_bodyData);

    return true;
}

bool TestHTTPServer::hasFailed() const
{
    return m_state == Failed;
}

void TestHTTPServer::newConnection()
{
    QTcpSocket *socket = m_server.nextPendingConnection();
    if (!socket)
        return;

    if (!m_directories.isEmpty())
        m_dataCache.insert(socket, QByteArray());

    QObject::connect(socket, &QAbstractSocket::disconnected, this, &TestHTTPServer::disconnected);
    QObject::connect(socket, &QIODevice::readyRead, this, &TestHTTPServer::readyRead);
}

void TestHTTPServer::disconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket)
        return;

    m_dataCache.remove(socket);
    for (int ii = 0; ii < m_toSend.size(); ++ii) {
        if (m_toSend.at(ii).first == socket) {
            m_toSend.removeAt(ii);
            --ii;
        }
    }
    socket->disconnect();
    socket->deleteLater();
}

void TestHTTPServer::readyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket || socket->state() == QTcpSocket::ClosingState)
        return;

    if (!m_directories.isEmpty()) {
        serveGET(socket, socket->readAll());
        return;
    }

    if (m_state == Failed || (m_waitData.body.isEmpty() && m_waitData.headerExactMatches.size() == 0)) {
        qWarning() << "TestHTTPServer: Unexpected data" << socket->readAll();
        return;
    }

    if (m_state == AwaitingHeader) {
        QByteArray line;
        while (!(line = socket->readLine()).isEmpty()) {
            line.replace('\r', "");
            if (line.at(0) == '\n') {
                m_state = AwaitingData;
                m_data += socket->readAll();
                break;
            } else {
                bool prefixFound = false;
                for (const QByteArray &prefix : m_waitData.headerPrefixes) {
                    if (line.startsWith(prefix)) {
                        prefixFound = true;
                        break;
                    }
                }
                for (QByteArrayView ignore : ignoredHeaders) {
                    if (line.startsWith(ignore)) {
                        prefixFound = true;
                        break;
                    }
                }

                if (!prefixFound && !m_waitData.headerExactMatches.contains(line)) {
                    qWarning() << "TestHTTPServer: Unexpected header:" << line
                        << "\nExpected exact headers: " << m_waitData.headerExactMatches
                        << "\nExpected header prefixes: " << m_waitData.headerPrefixes;
                    m_state = Failed;
                    socket->disconnectFromHost();
                    return;
                }
            }
        }
    }  else {
        m_data += socket->readAll();
    }

    if (!m_data.isEmpty() || m_waitData.body.isEmpty()) {
        if (m_waitData.body != m_data) {
            qWarning() << "TestHTTPServer: Unexpected data" << m_data << "\nExpected: " << m_waitData.body;
            m_state = Failed;
        } else {
            socket->write(m_replyData);
        }
        socket->disconnectFromHost();
    }
}

bool TestHTTPServer::reply(QTcpSocket *socket, const QByteArray &fileNameIn)
{
    const QString fileName = QLatin1String(fileNameIn);
    if (m_redirects.contains(fileName)) {
        const QByteArray response
            = "HTTP/1.1 302 Found\r\nContent-length: 0\r\nContent-type: text/html; charset=UTF-8\r\nLocation: "
              + m_redirects.value(fileName).toUtf8() + "\r\n\r\n";
        socket->write(response);
        return true;
    }

    for (int ii = 0; ii < m_directories.size(); ++ii) {
        const QString &dir = m_directories.at(ii).first;
        const Mode mode = m_directories.at(ii).second;

        QString dirFile = dir + QLatin1Char('/') + fileName;

        if (!QFile::exists(dirFile)) {
            const QHash<QString, QString>::const_iterator it = m_aliases.constFind(fileName);
            if (it != m_aliases.constEnd())
                dirFile = dir + QLatin1Char('/') + it.value();
        }

        QFile file(dirFile);
        if (file.open(QIODevice::ReadOnly)) {

            if (mode == Disconnect)
                return true;

            QByteArray data = file.readAll();
            if (m_contentSubstitutedFileNames.contains(QLatin1Char('/') + fileName))
                data.replace(QByteArrayLiteral("{{ServerBaseUrl}}"), baseUrl().toString().toUtf8());

            QByteArray response
                = "HTTP/1.0 200 OK\r\nContent-type: text/html; charset=UTF-8\r\nContent-length: ";
            response += QByteArray::number(data.size());
            response += "\r\n\r\n";
            response += data;

            if (mode == Delay) {
                m_toSend.append(std::make_pair(socket, response));
                QTimer::singleShot(500, this, &TestHTTPServer::sendOne);
                return false;
            }

            if (response.length() <= m_chunkSize) {
                socket->write(response);
                return true;
            }

            socket->write(response.left(m_chunkSize));
            for (qsizetype offset = m_chunkSize, end = response.length(); offset < end;
                 offset += m_chunkSize) {
                m_toSend.append(std::make_pair(socket, response.mid(offset, m_chunkSize)));
            }

            QTimer::singleShot(1, this, &TestHTTPServer::sendChunk);
            return false;
        }
    }

    socket->write("HTTP/1.0 404 Not found\r\nContent-type: text/html; charset=UTF-8\r\n\r\n");

    return true;
}

void TestHTTPServer::sendDelayedItem()
{
    sendOne();
}

void TestHTTPServer::sendOne()
{
    if (!m_toSend.isEmpty()) {
        m_toSend.first().first->write(m_toSend.first().second);
        m_toSend.first().first->close();
        m_toSend.removeFirst();
    }
}

void TestHTTPServer::sendChunk()
{
    const auto chunk = m_toSend.takeFirst();
    chunk.first->write(chunk.second);
    if (m_toSend.isEmpty())
        chunk.first->close();
    else
        QTimer::singleShot(1, this, &TestHTTPServer::sendChunk);
}

void TestHTTPServer::serveGET(QTcpSocket *socket, const QByteArray &data)
{
    const QHash<QTcpSocket *, QByteArray>::iterator it = m_dataCache.find(socket);
    if (it == m_dataCache.end())
        return;

    QByteArray &total = it.value();
    total.append(data);

    if (total.contains("\n\r\n")) {
        bool close = true;
        if (total.startsWith("GET /")) {
            const int space = total.indexOf(' ', 4);
            if (space != -1)
                close = reply(socket, total.mid(5, space - 5));
        }
        m_dataCache.erase(it);
        if (close)
            socket->disconnectFromHost();
    }
}

ThreadedTestHTTPServer::ThreadedTestHTTPServer(const QString &dir, TestHTTPServer::Mode mode) :
    m_port(0)
{
    m_dirs[dir] = mode;
    start();
}

ThreadedTestHTTPServer::ThreadedTestHTTPServer(const QHash<QString, TestHTTPServer::Mode> &dirs) :
    m_dirs(dirs), m_port(0)
{
    start();
}

ThreadedTestHTTPServer::~ThreadedTestHTTPServer()
{
    quit();
    wait();
}

QUrl ThreadedTestHTTPServer::baseUrl() const
{
    return localHostUrl(m_port);
}

QUrl ThreadedTestHTTPServer::url(const QString &documentPath) const
{
    return baseUrl().resolved(documentPath);
}

QString ThreadedTestHTTPServer::urlString(const QString &documentPath) const
{
    return url(documentPath).toString();
}

void ThreadedTestHTTPServer::run()
{
    TestHTTPServer server;
    {
        QMutexLocker locker(&m_mutex);
        QVERIFY2(server.listen(), qPrintable(server.errorString()));
        m_port = server.port();
        for (QHash<QString, TestHTTPServer::Mode>::ConstIterator i = m_dirs.constBegin();
             i != m_dirs.constEnd(); ++i) {
            server.serveDirectory(i.key(), i.value());
        }
        m_condition.wakeAll();
    }
    exec();
}

void ThreadedTestHTTPServer::start()
{
    QMutexLocker locker(&m_mutex);
    QThread::start();
    m_condition.wait(&m_mutex);
}

QT_END_NAMESPACE

#include "moc_testhttpserver_p.cpp"
