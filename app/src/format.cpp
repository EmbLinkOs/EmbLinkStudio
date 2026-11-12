// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    format.cpp
 * @brief   Implementation of the views' number formats.
 *
 * @details
 * Uses the default QLocale, which Language sets with the interface's language, so a
 * change of language rewrites every value the next time a view renders.
 *
 * @see     format.h
 * @author  Junior Deogracias
 * @date    2026-11-13 (created)
 */

#include "format.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QLocale>

namespace emblinkstudio
{

namespace
{

// The interface's locale, for its decimal mark, without group separators: 1500 MHz, as
// datasheets and the generator write technical values.
QLocale technical()
{
    QLocale locale;
    locale.setNumberOptions(QLocale::OmitGroupSeparator);
    return locale;
}

} // namespace

QString formatHz(double hz)
{
    const QLocale locale = technical();
    if (hz >= 1e6)
        return locale.toString(hz / 1e6, 'g', 6) + QStringLiteral(" MHz");
    if (hz >= 1e3)
        return locale.toString(hz / 1e3, 'g', 6) + QStringLiteral(" kHz");
    return locale.toString(hz, 'g', 6) + QStringLiteral(" Hz");
}

QString formatBytes(double bytes)
{
    const QLocale locale = technical();
    if (bytes >= 1024.0 * 1024.0)
        return locale.toString(bytes / (1024.0 * 1024.0), 'g', 4) + QStringLiteral(" MB");
    if (bytes >= 1024.0)
        return locale.toString(bytes / 1024.0, 'g', 4) + QStringLiteral(" KB");
    return locale.toString(bytes, 'g', 6) + QStringLiteral(" B");
}

QString formatHex(double value)
{
    return QStringLiteral("0x") +
           QStringLiteral("%1").arg(qulonglong(value), 8, 16, QLatin1Char('0')).toUpper();
}

QString formatField(const QString &key, const QJsonValue &value)
{
    if (value.isObject()) {
        QStringList parts;
        const QJsonObject o = value.toObject();
        for (auto it = o.begin(); it != o.end(); ++it)
            parts.append(it.key() + QLatin1Char(' ') + formatField(it.key(), it.value()));
        return parts.join(QStringLiteral(", "));
    }
    if (value.isArray()) {
        QStringList parts;
        for (const QJsonValue &v : value.toArray())
            parts.append(formatField(key, v));
        return parts.join(QStringLiteral(", "));
    }
    if (value.isBool())
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    if (value.isDouble()) {
        if (key == QStringLiteral("base") || key.endsWith(QStringLiteral("_base")))
            return formatHex(value.toDouble());
        if (key.endsWith(QStringLiteral("_hz")) || key == QStringLiteral("hz"))
            return formatHz(value.toDouble());
        return QString::number(value.toDouble(), 'g', 12);
    }
    if (value.isNull())
        return QStringLiteral("—");
    return value.toString();
}

} // namespace emblinkstudio
