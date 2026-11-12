// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    devicesview.h
 * @brief   DevicesView: the board's devices in initialization order (spec section 7).
 *
 * @details
 * A list of the devices the generator resolved (board.describe: devices), in the order
 * the kernel initializes them (SPEC-016 §4): each device's name, class, compatible
 * string (with its peripheral when it is not the device's own name, as board.md writes
 * it), level and the devices it depends on. Choosing one shows its binding fields
 * beside the list: addresses in hexadecimal, frequencies in Hz units, DMA channels and
 * pins as the generator gives them. rows() gives the list's cells, for the golden
 * tests of stage S2.
 *
 * @see     format.h; board.md's "Devices"
 * @author  Junior Deogracias
 * @date    2026-11-13 (created)
 */

#pragma once

#include "format.h"

#include <QJsonArray>
#include <QList>
#include <QStringList>
#include <QWidget>

class QLabel;
class QTreeWidget;

namespace emblinkstudio
{

class DevicesView : public QWidget
{
    Q_OBJECT

  public:
    explicit DevicesView(QWidget *parent = nullptr);

    void setDevices(const QJsonArray &devices);

    // Each device's cells: name, class, compatible, level, depends on.
    QList<QStringList> rows() const;
    // The chosen device's fields, as shown: (field, value).
    QList<QStringList> fields() const;
    void select(int row);

  protected:
    void changeEvent(QEvent *event) override;

  private:
    void render();
    void showFields();

    QJsonArray m_devices;
    QTreeWidget *m_list = nullptr;
    QTreeWidget *m_fields = nullptr;
    QLabel *m_title = nullptr;
};

} // namespace emblinkstudio
