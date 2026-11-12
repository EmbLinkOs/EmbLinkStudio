// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    format.h
 * @brief   How the views write the core's numbers: frequencies, sizes and addresses.
 *
 * @details
 * The views compute nothing (spec section 4); they only write the generator's values
 * for reading. Frequencies and sizes follow the interface's language (180 MHz, 1,5 MB
 * in French); addresses are hexadecimal and the same in every language (tokens:
 * locale.numbers).
 *
 * @see     boardview.h, clocktreeview.h, memoryview.h, devicesview.h
 * @author  Junior Deogracias
 * @date    2026-11-13 (created)
 */

#pragma once

#include <QJsonValue>
#include <QString>

namespace emblinkstudio
{

// "180 MHz", "8 MHz", "32.768 kHz".
QString formatHz(double hz);
// "512 KB", "16 KB", "1.5 MB", "512 B" (1 KB is 1,024 bytes, as the generator counts).
QString formatBytes(double bytes);
// "0x08000000": eight hexadecimal digits, more when the value needs them.
QString formatHex(double value);
// A field of the generator's model as the views write it: base addresses in hexadecimal,
// *_hz as a frequency, objects as "key value" pairs, lists joined with commas.
QString formatField(const QString &key, const QJsonValue &value);

} // namespace emblinkstudio
