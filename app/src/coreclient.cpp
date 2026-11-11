// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
#include "coreclient.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>

namespace emblinkstudio
{

namespace
{

int majorOf(const QString &version)
{
    bool ok = false;
    const int major = version.section(QLatin1Char('.'), 0, 0).toInt(&ok);
    return ok ? major : -1;
}

} // namespace

CoreClient::Launch CoreClient::defaultLaunch()
{
    Launch launch;
    const QString installed = launch.environment.value(QStringLiteral("EMBLINK_STUDIO_CORE"));
    if (!installed.isEmpty()) {
        launch.program = installed;
        launch.arguments = {QStringLiteral("serve")};
        return launch;
    }
    // Development layout: the core's sources next to the application's (S0; packaging is §11 q4).
    launch.program = QStringLiteral(EMBLINK_STUDIO_PYTHON);
    launch.arguments = {QStringLiteral("-m"), QStringLiteral("emblink_studio_core"),
                        QStringLiteral("serve")};
    const QString src = QDir::toNativeSeparators(QStringLiteral(EMBLINK_STUDIO_CORE_SRC_DIR));
    const QString old = launch.environment.value(QStringLiteral("PYTHONPATH"));
    launch.environment.insert(QStringLiteral("PYTHONPATH"),
                              old.isEmpty() ? src : src + QDir::listSeparator() + old);
    return launch;
}

CoreClient::CoreClient(QObject *parent) : QObject(parent)
{
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &CoreClient::onStdout);
    connect(&m_process, &QProcess::readyReadStandardError, this, &CoreClient::onStderr);
    connect(&m_process, &QProcess::finished, this, &CoreClient::onFinished);
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            fail(tr("Could not start EmbLink Studio Core (%1): %2")
                     .arg(m_process.program(), m_process.errorString()));
    });
}

CoreClient::~CoreClient()
{
    if (m_process.state() != QProcess::NotRunning)
        stop(1000);
}

void CoreClient::start(const Launch &launch)
{
    m_ready = false;
    m_stopping = false;
    m_framer = Framer();
    m_pending.clear();
    m_process.setProcessEnvironment(launch.environment);
    m_process.start(launch.program, launch.arguments);

    const QJsonObject params{
        {QStringLiteral("protocol_version"), QString::fromLatin1(kProtocolVersion)},
        {QStringLiteral("client"),
         QJsonObject{{QStringLiteral("name"), QStringLiteral("emblink-studio")},
                     {QStringLiteral("version"), QCoreApplication::applicationVersion()}}},
        {QStringLiteral("capabilities"), QJsonObject()},
    };
    m_initId = request(
        QStringLiteral("initialize"), params,
        [this](const QJsonValue &result) {
            const QJsonObject info = result.toObject();
            const QString theirs = info.value(QStringLiteral("protocol_version")).toString();
            if (majorOf(theirs) != majorOf(QString::fromLatin1(kProtocolVersion))) {
                fail(tr("EmbLink Studio Core speaks protocol %1; this EmbLink Studio needs %2.x.")
                         .arg(theirs.isEmpty() ? tr("(none)") : theirs)
                         .arg(majorOf(QString::fromLatin1(kProtocolVersion))));
                // Not from inside the stdout handler: stop() waits for the process.
                QMetaObject::invokeMethod(this, [this] { stop(); }, Qt::QueuedConnection);
                return;
            }
            m_serverInfo = info;
            m_ready = true;
            notify(QStringLiteral("initialized"), QJsonObject());
            emit ready(info);
        },
        [this](int code, const QString &message, const QJsonValue &) {
            fail(tr("EmbLink Studio Core refused to initialize (%1): %2").arg(code).arg(message));
        });
}

void CoreClient::stop(int timeoutMs)
{
    if (m_process.state() == QProcess::NotRunning)
        return;
    m_stopping = true;
    // The core handles messages in order, so exit right behind shutdown gives exit code 0.
    request(QStringLiteral("shutdown"), QJsonValue(), {});
    notify(QStringLiteral("exit"));
    m_process.closeWriteChannel();
    if (!m_process.waitForFinished(timeoutMs)) {
        m_process.kill();
        m_process.waitForFinished(1000);
    }
}

int CoreClient::request(const QString &method, const QJsonValue &params, ResultHandler onResult,
                        ErrorHandler onError)
{
    const int id = m_nextId++;
    m_pending.insert(id, Pending{std::move(onResult), std::move(onError)});
    QJsonObject message{{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                        {QStringLiteral("id"), id},
                        {QStringLiteral("method"), method}};
    if (!params.isUndefined() && !params.isNull())
        message.insert(QStringLiteral("params"), params);
    send(message);
    return id;
}

void CoreClient::notify(const QString &method, const QJsonValue &params)
{
    QJsonObject message{{QStringLiteral("jsonrpc"), QStringLiteral("2.0")},
                        {QStringLiteral("method"), method}};
    if (!params.isUndefined() && !params.isNull())
        message.insert(QStringLiteral("params"), params);
    send(message);
}

void CoreClient::send(const QJsonObject &message)
{
    if (m_process.state() == QProcess::NotRunning && m_process.error() == QProcess::FailedToStart)
        return;
    m_process.write(Framer::encode(message));
}

void CoreClient::onStdout()
{
    m_framer.feed(m_process.readAllStandardOutput());
    while (auto message = m_framer.next())
        handle(*message);
    if (m_framer.failed()) {
        fail(tr("EmbLink Studio Core sent a malformed frame: %1").arg(m_framer.error()));
        m_process.kill();
    }
}

void CoreClient::onStderr()
{
    m_stderr.append(m_process.readAllStandardError());
    qsizetype nl;
    while ((nl = m_stderr.indexOf('\n')) >= 0) {
        emit logLine(QString::fromUtf8(m_stderr.left(nl)).trimmed());
        m_stderr.remove(0, nl + 1);
    }
}

void CoreClient::handle(const QJsonObject &message)
{
    if (message.contains(QStringLiteral("emblinkstudio.badFrame"))) {
        emit logLine(tr("dropped a frame that is not JSON: %1")
                         .arg(message.value(QStringLiteral("emblinkstudio.badFrame")).toString()));
        return;
    }
    const QJsonValue id = message.value(QStringLiteral("id"));
    if (!message.contains(QStringLiteral("id"))) {
        emit notificationReceived(message.value(QStringLiteral("method")).toString(),
                                  message.value(QStringLiteral("params")));
        return;
    }
    const auto it = m_pending.find(id.toInt(-1));
    if (it == m_pending.end()) {
        emit logLine(
            tr("a response with no matching request: id %1").arg(id.toVariant().toString()));
        return;
    }
    const Pending pending = it.value();
    m_pending.erase(it);
    if (message.contains(QStringLiteral("error"))) {
        const QJsonObject error = message.value(QStringLiteral("error")).toObject();
        if (pending.onError)
            pending.onError(error.value(QStringLiteral("code")).toInt(),
                            error.value(QStringLiteral("message")).toString(),
                            error.value(QStringLiteral("data")));
        else
            emit logLine(tr("error %1: %2")
                             .arg(error.value(QStringLiteral("code")).toInt())
                             .arg(error.value(QStringLiteral("message")).toString()));
    } else if (pending.onResult) {
        pending.onResult(message.value(QStringLiteral("result")));
    }
}

void CoreClient::onFinished(int exitCode, QProcess::ExitStatus status)
{
    if (!m_stderr.isEmpty()) {
        emit logLine(QString::fromUtf8(m_stderr).trimmed());
        m_stderr.clear();
    }
    const bool expected = m_stopping;
    if (!expected && !m_ready && status == QProcess::NormalExit && m_pending.contains(m_initId))
        fail(tr("EmbLink Studio Core exited (code %1) before it initialized.").arg(exitCode));
    m_ready = false;
    m_pending.clear();
    emit exited(status == QProcess::NormalExit ? exitCode : -1, expected);
}

void CoreClient::fail(const QString &message)
{
    m_ready = false;
    emit failed(message);
}

} // namespace emblinkstudio
