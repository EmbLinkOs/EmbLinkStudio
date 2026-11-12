// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    session.h
 * @brief   Session: the board the desktop has open, as the core knows it (stage S1).
 *
 * @details
 * The desktop keeps no knowledge of its own (spec section 4): the Session asks the
 * core and holds the answers for the views. Opening a board fetches its status and,
 * when it is generated, its model (board.describe, pins, clocks, memory), and validates
 * it when its checkout is trusted. validate() and generate() run the checkout's
 * generator; when the core answers untrusted, the Session emits trustNeeded() and the
 * window asks the person, who can trust and run the action in one step with
 * trustAndRun(). After a generate, the model and the problems are fetched again.
 *
 * When the core restarts, reopen() opens the same board again, so a crash loses
 * nothing (spec section 4, process model).
 *
 * @see     coreclient.h; protocol/project.*.schema.json, protocol/board.*.schema.json
 * @author  Junior Deogracias
 * @date    2026-11-12 (created)
 */

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <functional>

namespace emblinkstudio
{

class CoreClient;

class Session : public QObject
{
    Q_OBJECT

  public:
    // The core's error codes the Session acts on (spec section 5, Errors).
    enum ErrorCode {
        NoProject = 1002,
        Untrusted = 1003
    };
    enum class Action {
        Validate,
        Generate
    };
    Q_ENUM(Action)

    explicit Session(CoreClient *core, QObject *parent = nullptr);

    void open(const QString &path);
    // Opens the last board again, after the core restarted; nothing if none was open.
    void reopen();
    void close();
    void validate();
    void generate();
    // Trusts the board's checkout, then runs `action`.
    void trustAndRun(Action action);

    bool isOpen() const
    {
        return !m_status.isEmpty();
    }
    QString path() const
    {
        return m_path;
    }
    bool isBusy() const
    {
        return m_busy > 0;
    }
    // project.status: kind, path, id, name, rtos, trusted, generated, board (summary).
    QJsonObject status() const
    {
        return m_status;
    }
    QJsonObject describe() const
    {
        return m_describe;
    }
    QJsonObject pins() const
    {
        return m_pins;
    }
    QJsonObject clocks() const
    {
        return m_clocks;
    }
    QJsonObject memory() const
    {
        return m_memory;
    }
    QJsonArray diagnostics() const
    {
        return m_diagnostics;
    }
    // Whether the generator has given its verdict since the board was opened.
    bool hasVerdict() const
    {
        return m_hasVerdict;
    }
    bool isValid() const
    {
        return m_valid;
    }

  signals:
    void opened();
    void closed();
    void statusChanged();
    void modelChanged();
    void diagnosticsChanged();
    void busyChanged(bool busy);
    // The generator needs the checkout trusted before `action` (spec section 9).
    void trustNeeded(const QString &checkout, emblinkstudio::Session::Action action);
    // A message for the person: the core refused or a tool failed.
    void failed(const QString &message);

  private:
    void run(Action action);
    void fetchStatus();
    void fetchModel();
    void setVerdict(const QJsonObject &result);
    void call(const QString &method, const QJsonValue &params,
              std::function<void(const QJsonValue &)> onResult);
    void clear();

    CoreClient *m_core = nullptr;
    QString m_path;
    QJsonObject m_status;
    QJsonObject m_describe;
    QJsonObject m_pins;
    QJsonObject m_clocks;
    QJsonObject m_memory;
    QJsonArray m_diagnostics;
    bool m_hasVerdict = false;
    bool m_valid = false;
    int m_busy = 0;
    // Bumped on every open and close, so an answer about a board no longer open is dropped.
    int m_generation = 0;
};

} // namespace emblinkstudio
