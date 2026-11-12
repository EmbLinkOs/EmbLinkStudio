// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    session.cpp
 * @brief   Implementation of Session: the requests behind opening, validating and generating.
 *
 * @details
 * Every request carries the session's generation; an answer that arrives after the
 * board was closed or another opened is ignored. The busy count rises with each request
 * and falls with its answer, so the window can show that the core is working. Errors
 * other than untrusted become failed(), with the core's message, which is written for
 * the person.
 *
 * @see     session.h
 * @author  Junior Deogracias
 * @date    2026-11-12 (created)
 */

#include "coreclient.h"
#include "session.h"

namespace emblinkstudio
{

Session::Session(CoreClient *core, QObject *parent) : QObject(parent), m_core(core)
{
}

void Session::call(const QString &method, const QJsonValue &params,
                   std::function<void(const QJsonValue &)> onResult)
{
    const int generation = m_generation;
    if (m_busy++ == 0)
        emit busyChanged(true);
    auto done = [this] {
        if (--m_busy == 0)
            emit busyChanged(false);
    };
    m_core->request(
        method, params,
        [this, generation, onResult, done](const QJsonValue &result) {
            done();
            if (generation == m_generation)
                onResult(result);
        },
        [this, generation, done](int code, const QString &message, const QJsonValue &data) {
            done();
            if (generation != m_generation)
                return;
            Q_UNUSED(code);
            Q_UNUSED(data);
            emit failed(message);
        });
}

void Session::clear()
{
    m_status = {};
    m_describe = {};
    m_pins = {};
    m_clocks = {};
    m_memory = {};
    m_diagnostics = {};
    m_hasVerdict = false;
    m_valid = false;
}

void Session::open(const QString &path)
{
    ++m_generation;
    const bool wasOpen = isOpen();
    clear();
    if (wasOpen)
        emit closed();
    m_path = path;
    call(QStringLiteral("project.open"), QJsonObject{{QStringLiteral("path"), path}},
         [this](const QJsonValue &result) {
             m_status = result.toObject();
             emit opened();
             emit statusChanged();
             emit diagnosticsChanged();
             fetchModel();
             // A trusted checkout is validated at once, so the problems are there on opening.
             if (m_status.value(QStringLiteral("trusted")).toBool())
                 run(Action::Validate);
         });
}

void Session::reopen()
{
    if (!m_path.isEmpty())
        open(m_path);
}

void Session::close()
{
    ++m_generation;
    const bool wasOpen = isOpen();
    clear();
    m_path.clear();
    m_core->request(QStringLiteral("project.close"), QJsonValue(), [](const QJsonValue &) {});
    if (wasOpen) {
        emit closed();
        emit statusChanged();
        emit modelChanged();
        emit diagnosticsChanged();
    }
}

void Session::fetchStatus()
{
    call(QStringLiteral("project.status"), QJsonValue(), [this](const QJsonValue &result) {
        m_status = result.toObject();
        emit statusChanged();
    });
}

void Session::fetchModel()
{
    if (!m_status.value(QStringLiteral("generated")).toBool()) {
        m_describe = m_pins = m_clocks = m_memory = {};
        emit modelChanged();
        return;
    }
    // Four answers; the views are told once, when the last arrives.
    auto remaining = std::make_shared<int>(4);
    auto arrived = [this, remaining](QJsonObject &slot) {
        return [this, remaining, &slot](const QJsonValue &result) {
            slot = result.toObject();
            if (--*remaining == 0)
                emit modelChanged();
        };
    };
    call(QStringLiteral("board.describe"), QJsonValue(), arrived(m_describe));
    call(QStringLiteral("board.pins"), QJsonValue(), arrived(m_pins));
    call(QStringLiteral("board.clocks"), QJsonValue(), arrived(m_clocks));
    call(QStringLiteral("board.memory"), QJsonValue(), arrived(m_memory));
}

void Session::setVerdict(const QJsonObject &result)
{
    m_diagnostics = result.value(QStringLiteral("diagnostics")).toArray();
    m_hasVerdict = true;
    m_valid = result.contains(QStringLiteral("valid"))
                  ? result.value(QStringLiteral("valid")).toBool()
                  : result.value(QStringLiteral("errors")).toInt() == 0;
    emit diagnosticsChanged();
}

void Session::validate()
{
    run(Action::Validate);
}

void Session::generate()
{
    run(Action::Generate);
}

void Session::run(Action action)
{
    if (!isOpen())
        return;
    const QString method = action == Action::Validate ? QStringLiteral("project.validate")
                                                      : QStringLiteral("generate.run");
    const int generation = m_generation;
    if (m_busy++ == 0)
        emit busyChanged(true);
    auto done = [this] {
        if (--m_busy == 0)
            emit busyChanged(false);
    };
    m_core->request(
        method, QJsonValue(),
        [this, generation, action, done](const QJsonValue &result) {
            done();
            if (generation != m_generation)
                return;
            setVerdict(result.toObject());
            if (action == Action::Generate &&
                result.toObject().value(QStringLiteral("written")).toBool()) {
                fetchStatus();
                m_status.insert(QStringLiteral("generated"), true);
                fetchModel();
            }
        },
        [this, generation, action, done](int code, const QString &message, const QJsonValue &data) {
            done();
            if (generation != m_generation)
                return;
            if (code == Untrusted) {
                emit trustNeeded(data.toObject().value(QStringLiteral("path")).toString(), action);
                return;
            }
            emit failed(message);
        });
}

void Session::trustAndRun(Action action)
{
    call(QStringLiteral("project.trust"), QJsonValue(), [this, action](const QJsonValue &result) {
        m_status = result.toObject();
        emit statusChanged();
        run(action);
    });
}

} // namespace emblinkstudio
