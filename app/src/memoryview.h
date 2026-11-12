// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    memoryview.h
 * @brief   MemoryView: the chip's memory, the flash partitions and the image layout (spec section
 * 7).
 *
 * @details
 * Three groups, from board.memory: the chip's regions (name, base address, size and
 * attributes); the flash partitions, drawn as a strip in proportion to their sizes,
 * each with its erase units (sectors, or a range of pages); and the image layout the
 * linker uses (FLASH and RAM, with the regions RAM spans). Every value is the
 * generator's, written as board.md and build_report.md write it. rows() gives each
 * group's rows, for the golden tests of stage S2.
 *
 * @see     format.h; board.md's "Memory map" and "Flash partitions", build_report.md's
 *          "Image layout"
 * @author  Junior Deogracias
 * @date    2026-11-13 (created)
 */

#pragma once

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QScrollArea>
#include <QStringList>

class QVBoxLayout;

namespace emblinkstudio
{

class MemoryView : public QScrollArea
{
    Q_OBJECT

  public:
    enum class Group {
        Regions,
        Partitions,
        Image
    };

    explicit MemoryView(QWidget *parent = nullptr);

    void setMemory(const QJsonObject &memory);

    // Each row's cells, as shown: Regions (name, base, size, attributes), Partitions (name,
    // base, size, erase units), Image (memory, origin, length).
    QList<QStringList> rows(Group group) const
    {
        return m_rows.value(int(group));
    }

  protected:
    void changeEvent(QEvent *event) override;

  private:
    void render();
    void addGroup(const QString &title, Group group, const QList<QStringList> &rows,
                  QWidget *above = nullptr);

    QWidget *m_content = nullptr;
    QVBoxLayout *m_layout = nullptr;
    QJsonObject m_memory;
    QHash<int, QList<QStringList>> m_rows;
};

} // namespace emblinkstudio
