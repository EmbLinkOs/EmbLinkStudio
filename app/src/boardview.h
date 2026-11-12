// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    boardview.h
 * @brief   BoardView: the open board's model, read-only (stage S1).
 *
 * @details
 * Shows what the generator resolved for the board (board.describe, clocks, memory,
 * pins): the chip, the board's console, LEDs and buttons, the clock tree's frequencies,
 * the memory regions, the devices and the pins in use. Each group is a rounded group
 * of rows, as System Settings lays them out (design brief section 4f). Every value is
 * the core's; the view only formats it for reading (180 MHz, 512 KB), in the
 * interface's language. The drawn Board, Pin map and Clock tree views of stage S2
 * replace it.
 *
 * @see     session.h; spec section 7 (Board view)
 * @author  Junior Deogracias
 * @date    2026-11-12 (created)
 */

#pragma once

#include "format.h"

#include <QHash>
#include <QJsonObject>
#include <QScrollArea>
#include <QStringList>

class QLabel;
class QVBoxLayout;

namespace emblinkstudio
{

class BoardView : public QScrollArea
{
    Q_OBJECT

  public:
    explicit BoardView(QWidget *parent = nullptr);

    // An empty describe shows the empty state: no board, or a board not generated yet.
    void setModel(const QJsonObject &status, const QJsonObject &describe, const QJsonObject &clocks,
                  const QJsonObject &memory, const QJsonObject &pins);

    // The groups shown, by title, for tests: "CHIP", "BOARD", "CLOCKS", ...
    QStringList groupTitles() const
    {
        return m_groupTitles;
    }
    // A row's value, for tests: value("CHIP", tr("Part")).
    QString value(const QString &group, const QString &label) const;

  protected:
    void changeEvent(QEvent *event) override;

  private:
    void render();
    void addGroup(const QString &title, const QList<QPair<QString, QString>> &rows);

    QWidget *m_content = nullptr;
    QVBoxLayout *m_layout = nullptr;
    QJsonObject m_status;
    QJsonObject m_describe;
    QJsonObject m_clocks;
    QJsonObject m_memory;
    QJsonObject m_pins;
    QStringList m_groupTitles;
    QHash<QString, QString> m_values;
};

} // namespace emblinkstudio
