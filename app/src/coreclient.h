// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    coreclient.h
 * @brief   CoreClient: the desktop's connection to EmbLink Studio Core.
 *
 * @details
 * Declares CoreClient, which runs EmbLink Studio Core as a child process and talks
 * JSON-RPC 2.0 to it over the core's standard input and output. Every message is framed
 * by Framer (Content-Length headers, as in LSP).
 *
 * CoreClient performs the initialize handshake, refuses a core whose protocol major
 * version differs from kProtocolVersion, matches each response to its request by id,
 * and reports notifications, the core's standard-error log and its exit as Qt signals.
 *
 * @see     Spec sections 4 (process model) and 5 (protocol); framing.h
 * @author  Junior Deogracias
 * @date    2026-11-11 (created)
 */

#pragma once

#include "framing.h"

#include <QHash>
#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStringList>
#include <functional>

namespace emblinkstudio
{

// The protocol this desktop speaks (spec section 5). A core of another major version is refused.
inline constexpr char kProtocolVersion[] = "0.1.0";

// Runs EmbLink Studio Core as a child process and talks JSON-RPC 2.0 to it over stdio.
class CoreClient : public QObject
{
    Q_OBJECT

  public:
    struct Launch {
        QString program;
        QStringList arguments;
        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    };

    // EMBLINK_STUDIO_CORE=<path to emblink-studio-core> if set; otherwise the core from this source
    // tree, run by the Python found at build time.
    static Launch defaultLaunch();

    using ResultHandler = std::function<void(const QJsonValue &result)>;
    using ErrorHandler =
        std::function<void(int code, const QString &message, const QJsonValue &data)>;

    explicit CoreClient(QObject *parent = nullptr);
    ~CoreClient() override;

    // Start the core and send initialize; ready() or failed() follows.
    void start(const Launch &launch);
    // shutdown, then exit; waits up to timeoutMs, then kills the core.
    void stop(int timeoutMs = 3000);

    bool isReady() const
    {
        return m_ready;
    }
    QJsonObject serverInfo() const
    {
        return m_serverInfo;
    }

    int request(const QString &method, const QJsonValue &params, ResultHandler onResult,
                ErrorHandler onError = {});
    void notify(const QString &method, const QJsonValue &params = QJsonValue());

  signals:
    void ready(const QJsonObject &initializeResult);
    void failed(const QString &message);
    void notificationReceived(const QString &method, const QJsonValue &params);
    void logLine(const QString &line);
    // expected is true when the core exited because stop() asked it to.
    void exited(int exitCode, bool expected);

  private:
    void send(const QJsonObject &message);
    void onStdout();
    void onStderr();
    void onFinished(int exitCode, QProcess::ExitStatus status);
    void handle(const QJsonObject &message);
    void fail(const QString &message);

    struct Pending {
        ResultHandler onResult;
        ErrorHandler onError;
    };

    QProcess m_process;
    Framer m_framer;
    QByteArray m_stderr;
    QHash<int, Pending> m_pending;
    int m_nextId = 1;
    int m_initId = 0;
    bool m_ready = false;
    bool m_stopping = false;
    QJsonObject m_serverInfo;
};

} // namespace emblinkstudio
