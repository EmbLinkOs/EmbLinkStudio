// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    memoryview.cpp
 * @brief   Implementation of MemoryView: the groups, and the partition strip.
 *
 * @details
 * Each row is a name on the left and its cells on the right in the code font. The
 * partition strip gives each partition a share of its width equal to its share of the
 * partitions' total, a reserved one striped; the shares are drawn, not shown as
 * numbers.
 *
 * @see     memoryview.h
 * @author  Junior Deogracias
 * @date    2026-11-13 (created)
 */

#include "format.h"
#include "memoryview.h"
#include "theme.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>

namespace emblinkstudio
{

namespace
{

// The partitions, side by side, each as wide as its share of the whole.
class PartitionStrip : public QWidget
{
  public:
    struct Part {
        QString name;
        double size;
        bool reserved;
    };

    PartitionStrip(QList<Part> parts, QWidget *parent) : QWidget(parent), m_parts(std::move(parts))
    {
        setMinimumHeight(40);
    }

  protected:
    void paintEvent(QPaintEvent *) override
    {
        const Theme &theme = Theme::current();
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        double total = 0;
        for (const Part &part : m_parts)
            total += part.size;
        if (total <= 0)
            return;
        const QColor colors[] = {theme.color(QStringLiteral("accent.base")),
                                 theme.color(QStringLiteral("peripheral.i2c")),
                                 theme.color(QStringLiteral("peripheral.gpioOut")),
                                 theme.color(QStringLiteral("peripheral.spi"))};
        const QRectF area = QRectF(rect()).adjusted(4, 6, -4, -6);
        double x = area.left();
        p.setFont(theme.font(QStringLiteral("codeMeta")));
        for (int i = 0; i < m_parts.size(); ++i) {
            const double w = area.width() * m_parts.at(i).size / total;
            const QRectF r(x, area.top(), qMax(2.0, w - 2), area.height());
            QPainterPath path;
            path.addRoundedRect(r, 6, 6);
            QColor fill = colors[i % 4];
            fill.setAlphaF(0.22);
            p.fillPath(path, fill);
            if (m_parts.at(i).reserved) {
                p.save();
                p.setClipPath(path);
                p.setPen(QPen(theme.color(QStringLiteral("line.strong")), 1));
                for (double s = r.left() - r.height(); s < r.right(); s += 6)
                    p.drawLine(QPointF(s, r.bottom()), QPointF(s + r.height(), r.top()));
                p.restore();
            }
            p.setPen(QPen(colors[i % 4], 1));
            p.drawPath(path);
            p.setPen(theme.color(QStringLiteral("text.primary")));
            p.drawText(r.adjusted(8, 0, -8, 0), Qt::AlignVCenter | Qt::AlignLeft,
                       QFontMetricsF(p.font()).elidedText(m_parts.at(i).name, Qt::ElideRight,
                                                          r.width() - 16));
            x += w;
        }
    }

  private:
    QList<Part> m_parts;
};

QString joined(const QJsonValue &v, const QString &sep = QStringLiteral(", "))
{
    QStringList out;
    for (const QJsonValue &item : v.toArray())
        out.append(item.isString() ? item.toString() : QString::number(item.toDouble(), 'g', 12));
    return out.join(sep);
}

} // namespace

MemoryView::MemoryView(QWidget *parent) : QScrollArea(parent)
{
    setObjectName(QStringLiteral("viewContent"));
    setAttribute(Qt::WA_StyledBackground, true);
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);
    m_content = new QWidget(this);
    m_content->setObjectName(QStringLiteral("viewContent"));
    m_content->setAttribute(Qt::WA_StyledBackground, true);
    m_layout = new QVBoxLayout(m_content);
    m_layout->setContentsMargins(18, 10, 18, 20);
    m_layout->setSpacing(6);
    setWidget(m_content);
    render();
}

void MemoryView::setMemory(const QJsonObject &memory)
{
    m_memory = memory;
    render();
}

void MemoryView::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        render();
    QScrollArea::changeEvent(event);
}

void MemoryView::addGroup(const QString &title, Group group, const QList<QStringList> &rows,
                          QWidget *above)
{
    m_rows.insert(int(group), rows);
    if (rows.isEmpty())
        return;
    const Theme &theme = Theme::current();
    auto *caption = new QLabel(title, m_content);
    caption->setProperty("role", QStringLiteral("caption"));
    caption->setFont(theme.font(QStringLiteral("caption")));
    caption->setContentsMargins(4, 12, 0, 4);
    m_layout->addWidget(caption);
    if (above)
        m_layout->addWidget(above);

    auto *frame = new QFrame(m_content);
    frame->setProperty("role", QStringLiteral("group"));
    auto *column = new QVBoxLayout(frame);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(0);
    for (int i = 0; i < rows.size(); ++i) {
        const QStringList &cells = rows.at(i);
        auto *row = new QWidget(frame);
        row->setProperty("role", QStringLiteral("row"));
        row->setProperty("first", i == 0);
        row->setAttribute(Qt::WA_StyledBackground, true);
        auto *line = new QHBoxLayout(row);
        line->setContentsMargins(12, 8, 12, 8);
        line->setSpacing(18);
        auto *name = new QLabel(cells.value(0), row);
        name->setFont(theme.font(QStringLiteral("bodySmall")));
        line->addWidget(name, 1);
        for (int c = 1; c < cells.size(); ++c) {
            auto *cell = new QLabel(cells.at(c), row);
            cell->setFont(theme.font(QStringLiteral("codeMeta")));
            cell->setProperty("role",
                              c == 1 ? QStringLiteral("secondary") : QStringLiteral("quiet"));
            cell->setTextInteractionFlags(Qt::TextSelectableByMouse);
            cell->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            cell->setMinimumWidth(c == cells.size() - 1 ? 150 : 90);
            line->addWidget(cell);
        }
        column->addWidget(row);
    }
    m_layout->addWidget(frame);
}

void MemoryView::render()
{
    while (QLayoutItem *item = m_layout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    m_rows.clear();
    if (m_memory.isEmpty()) {
        auto *empty = new QLabel(tr("Open a generated board to see its memory."), m_content);
        empty->setProperty("role", QStringLiteral("secondary"));
        m_layout->addWidget(empty);
        m_layout->addStretch();
        return;
    }

    QList<QStringList> regions;
    for (const QJsonValue &v : m_memory.value(QStringLiteral("regions")).toArray()) {
        const QJsonObject region = v.toObject();
        regions.append({region.value(QStringLiteral("name")).toString(),
                        formatHex(region.value(QStringLiteral("base")).toDouble()),
                        formatBytes(region.value(QStringLiteral("size")).toDouble()),
                        joined(region.value(QStringLiteral("attrs")))});
    }
    addGroup(tr("Memory map"), Group::Regions, regions);

    QList<QStringList> partitions;
    QList<PartitionStrip::Part> parts;
    for (const QJsonValue &v : m_memory.value(QStringLiteral("partitions")).toArray()) {
        const QJsonObject part = v.toObject();
        QString units;
        if (part.contains(QStringLiteral("sectors")))
            units = tr("sectors %1").arg(joined(part.value(QStringLiteral("sectors"))));
        else if (part.value(QStringLiteral("pages")).toArray().size() == 2)
            units = tr("pages %1 to %2")
                        .arg(part.value(QStringLiteral("pages")).toArray().at(0).toInt())
                        .arg(part.value(QStringLiteral("pages")).toArray().at(1).toInt());
        if (units.isEmpty())
            units = QStringLiteral("—"); // no erase units given, as board.md writes it
        const QString name = part.value(QStringLiteral("name")).toString();
        const bool reserved = part.value(QStringLiteral("reserved")).toBool();
        partitions.append({reserved ? tr("%1 (reserved)").arg(name) : name,
                           formatHex(part.value(QStringLiteral("base")).toDouble()),
                           formatBytes(part.value(QStringLiteral("size")).toDouble()), units});
        parts.append({name, part.value(QStringLiteral("size")).toDouble(), reserved});
    }
    addGroup(tr("Flash partitions"), Group::Partitions, partitions,
             parts.isEmpty() ? nullptr : new PartitionStrip(parts, m_content));

    QList<QStringList> image;
    const QJsonObject linker = m_memory.value(QStringLiteral("linker")).toObject();
    const QJsonObject flash = linker.value(QStringLiteral("flash")).toObject();
    if (!flash.isEmpty())
        image.append({QStringLiteral("FLASH"),
                      formatHex(flash.value(QStringLiteral("origin")).toDouble()),
                      formatBytes(flash.value(QStringLiteral("length")).toDouble())});
    const QJsonObject ram = linker.value(QStringLiteral("ram")).toObject();
    if (!ram.isEmpty()) {
        const QString spans = joined(ram.value(QStringLiteral("regions")), QStringLiteral(" + "));
        image.append(
            {spans.isEmpty() ? QStringLiteral("RAM") : QStringLiteral("RAM (%1)").arg(spans),
             formatHex(ram.value(QStringLiteral("origin")).toDouble()),
             formatBytes(ram.value(QStringLiteral("length")).toDouble())});
    }
    addGroup(tr("Image layout"), Group::Image, image);
    m_layout->addStretch();
}

} // namespace emblinkstudio
