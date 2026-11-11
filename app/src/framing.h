// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <optional>

namespace embstudio
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

} // namespace embstudio
