// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
#include "framing.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QList>

namespace embstudio
{

QByteArray Framer::encode(const QJsonObject &message)
{
    const QByteArray body = QJsonDocument(message).toJson(QJsonDocument::Compact);
    return "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body;
}

void Framer::feed(const QByteArray &bytes)
{
    if (!failed())
        m_buffer.append(bytes);
}

std::optional<QJsonObject> Framer::next()
{
    if (failed())
        return std::nullopt;
    const qsizetype end = m_buffer.indexOf("\r\n\r\n");
    if (end < 0)
        return std::nullopt;

    qint64 length = -1;
    const QList<QByteArray> lines = m_buffer.left(end).split('\n');
    for (QByteArray line : lines) {
        line = line.trimmed();
        const qsizetype colon = line.indexOf(':');
        if (colon < 0) {
            m_error = QStringLiteral("malformed header line: %1").arg(QString::fromLatin1(line));
            return std::nullopt;
        }
        if (line.left(colon).trimmed().toLower() == "content-length") {
            bool ok = false;
            length = line.mid(colon + 1).trimmed().toLongLong(&ok);
            if (!ok || length < 0) {
                m_error = QStringLiteral("bad Content-Length");
                return std::nullopt;
            }
        }
    }
    if (length < 0) {
        m_error = QStringLiteral("missing Content-Length header");
        return std::nullopt;
    }
    const qsizetype bodyStart = end + 4;
    if (m_buffer.size() - bodyStart < length)
        return std::nullopt;

    const QByteArray body = m_buffer.mid(bodyStart, length);
    m_buffer.remove(0, bodyStart + length);

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        // The frame's length was right, so the stream is still in step: report and skip it.
        return QJsonObject{{QStringLiteral("embstudio.badFrame"), parseError.errorString()}};
    }
    return doc.object();
}

} // namespace embstudio
