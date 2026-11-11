// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    framing.h
 * @brief   Framer: Content-Length framing of JSON-RPC messages, desktop side.
 *
 * @details
 * Each message is a compact JSON object preceded by a "Content-Length: <bytes>"
 * header and a blank line, the framing LSP uses. Framer::encode() builds one frame;
 * a Framer instance decodes a stream incrementally: feed() it bytes as they arrive
 * and call next() until it returns nothing.
 *
 * The core's side of the same format is core/src/emblink_studio_core/framing.py.
 *
 * @see     Spec section 4 (transport)
 * @author  Junior Deogracias
 * @date    2026-11-11 (created)
 */

#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <optional>

namespace emblinkstudio
{

// Content-Length framing of JSON-RPC messages, as in LSP (spec section 4).
class Framer
{
  public:
    static QByteArray encode(const QJsonObject &message);

    // Append bytes read from the core.
    void feed(const QByteArray &bytes);

    // The next whole message, or nothing until more bytes arrive. After a framing error the
    // stream cannot be trusted: failed() is true and next() returns nothing from then on.
    std::optional<QJsonObject> next();

    bool failed() const
    {
        return !m_error.isEmpty();
    }
    QString error() const
    {
        return m_error;
    }

  private:
    QByteArray m_buffer;
    QString m_error;
};

} // namespace emblinkstudio
