// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    boardview.cpp
 * @brief   Implementation of BoardView: the groups and rows, built from hw.json's sections.
 *
 * @details
 * render() rebuilds the content from the last model and runs again on a language
 * change. A value missing from the model shows as a dash rather than a guess. Technical
 * values (pins, addresses, names in the description) are set in the code font, which
 * is never translated.
 *
 * @see     boardview.h
 * @author  Junior Deogracias
 * @date    2026-11-12 (created)
 */

#include "boardview.h"
#include "theme.h"

#include <QEvent>
#include <QGridLayout>
#include <QJsonArray>
#include <QLabel>
#include <QLocale>
#include <QVBoxLayout>

namespace emblinkstudio
{

namespace
{

QString dash()
{
    return QStringLiteral("—");
}

QString text(const QJsonValue &v)
{
    if (v.isString())
        return v.toString();
    if (v.isDouble())
        return QLocale().toString(v.toDouble(), 'g', 12);
    if (v.isBool())
        return v.toBool() ? QStringLiteral("✓") : dash();
    return dash();
}

} // namespace

BoardView::BoardView(QWidget *parent) : QScrollArea(parent)
{
    setObjectName(QStringLiteral("viewContent"));
    setAttribute(Qt::WA_StyledBackground, true);
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);
    m_content = new QWidget(this);
    m_content->setObjectName(QStringLiteral("viewContent"));
    m_content->setAttribute(Qt::WA_StyledBackground, true);
    m_layout = new QVBoxLayout(m_content);
    m_layout->setContentsMargins(18, 16, 18, 20);
    m_layout->setSpacing(6);
    setWidget(m_content);
    render();
}

void BoardView::setModel(const QJsonObject &status, const QJsonObject &describe,
                         const QJsonObject &clocks, const QJsonObject &memory,
                         const QJsonObject &pins)
{
    m_status = status;
    m_describe = describe;
    m_clocks = clocks;
    m_memory = memory;
    m_pins = pins;
    render();
}

QString BoardView::value(const QString &group, const QString &label) const
{
    return m_values.value(group + QLatin1Char('/') + label);
}

void BoardView::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        render();
    QScrollArea::changeEvent(event);
}

void BoardView::addGroup(const QString &title, const QList<QPair<QString, QString>> &rows)
{
    if (rows.isEmpty())
        return;
    const Theme &theme = Theme::current();
    m_groupTitles.append(title);

    auto *caption = new QLabel(title, m_content);
    caption->setProperty("role", QStringLiteral("caption"));
    caption->setFont(theme.font(QStringLiteral("caption")));
    caption->setContentsMargins(4, 12, 0, 4);
    m_layout->addWidget(caption);

    auto *group = new QFrame(m_content);
    group->setProperty("role", QStringLiteral("group"));
    auto *column = new QVBoxLayout(group);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(0);
    for (int i = 0; i < rows.size(); ++i) {
        auto *row = new QWidget(group);
        row->setProperty("role", QStringLiteral("row"));
        row->setProperty("first", i == 0);
        row->setAttribute(Qt::WA_StyledBackground, true);
        auto *line = new QHBoxLayout(row);
        line->setContentsMargins(12, 8, 12, 8);
        auto *label = new QLabel(rows.at(i).first, row);
        label->setFont(theme.font(QStringLiteral("bodySmall")));
        auto *value = new QLabel(rows.at(i).second, row);
        value->setFont(theme.font(QStringLiteral("codeMeta")));
        value->setProperty("role", QStringLiteral("secondary"));
        value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        value->setWordWrap(true);
        line->addWidget(label);
        line->addStretch();
        line->addWidget(value, 1);
        column->addWidget(row);
        m_values.insert(title + QLatin1Char('/') + rows.at(i).first, rows.at(i).second);
    }
    m_layout->addWidget(group);
}

void BoardView::render()
{
    while (QLayoutItem *item = m_layout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    m_groupTitles.clear();
    m_values.clear();
    const Theme &theme = Theme::current();

    const QJsonObject board = m_describe.value(QStringLiteral("board")).toObject();
    const QJsonObject soc = m_describe.value(QStringLiteral("soc")).toObject();
    if (m_describe.isEmpty()) {
        auto *empty = new QLabel(m_content);
        empty->setProperty("role", QStringLiteral("secondary"));
        empty->setWordWrap(true);
        empty->setText(m_status.isEmpty()
                           ? tr("Open a board of an EmbLinkRTOS checkout to see its model.")
                           : tr("%1 has no generated model yet. Generate it to see its chip, "
                                "clocks, memory and pins.")
                                 .arg(m_status.value(QStringLiteral("id")).toString()));
        m_layout->addWidget(empty);
        m_layout->addStretch();
        return;
    }

    const QJsonObject cpu = soc.value(QStringLiteral("cpu")).toObject();
    auto *title = new QLabel(text(board.value(QStringLiteral("name"))), m_content);
    title->setFont(theme.font(QStringLiteral("title2")));
    m_layout->addWidget(title);
    QStringList facts{text(soc.value(QStringLiteral("part"))),
                      text(cpu.value(QStringLiteral("core"))),
                      text(soc.value(QStringLiteral("package"))).toUpper()};
    auto *sub = new QLabel(facts.join(QStringLiteral(" · ")), m_content);
    sub->setProperty("role", QStringLiteral("secondary"));
    sub->setFont(theme.font(QStringLiteral("small")));
    m_layout->addWidget(sub);
    if (!board.value(QStringLiteral("doc")).toString().isEmpty()) {
        auto *doc = new QLabel(board.value(QStringLiteral("doc")).toString(), m_content);
        doc->setWordWrap(true);
        doc->setProperty("role", QStringLiteral("quiet"));
        doc->setFont(theme.font(QStringLiteral("small")));
        doc->setContentsMargins(0, 6, 0, 0);
        m_layout->addWidget(doc);
    }

    const QJsonObject resolved = m_clocks.value(QStringLiteral("resolved")).toObject();
    const double cpuHz = resolved.value(QStringLiteral("cpu_hz")).toDouble();
    QStringList debug;
    const QJsonObject debugCaps = soc.value(QStringLiteral("debug")).toObject();
    for (auto it = debugCaps.begin(); it != debugCaps.end(); ++it)
        if (it->toBool())
            debug.append(it.key().toUpper());
    addGroup(tr("Chip"),
             {{tr("Part"), text(soc.value(QStringLiteral("part")))},
              {tr("Core"), text(cpu.value(QStringLiteral("core")))},
              {tr("FPU"), text(cpu.value(QStringLiteral("fpu")))},
              {tr("Package"), text(soc.value(QStringLiteral("package"))).toUpper()},
              {tr("CPU clock"), cpuHz > 0 ? formatHz(cpuHz) : dash()},
              {tr("Debug"), debug.isEmpty() ? dash() : debug.join(QStringLiteral(", "))}});

    QList<QPair<QString, QString>> boardRows{
        {tr("Revision"), text(board.value(QStringLiteral("revision")))},
        {tr("Description"),
         board.value(QStringLiteral("curated")).toBool() ? tr("Curated") : tr("Not curated yet")}};
    const QJsonObject console = board.value(QStringLiteral("console")).toObject();
    if (!console.isEmpty())
        boardRows.append(
            {tr("Console"),
             tr("%1 at %2 baud")
                 .arg(console.value(QStringLiteral("device")).toString(),
                      QString::number(console.value(QStringLiteral("baud")).toInt()))});
    for (const QJsonValue &led : board.value(QStringLiteral("leds")).toArray())
        boardRows.append({tr("LED %1").arg(led.toObject().value(QStringLiteral("name")).toString()),
                          led.toObject().value(QStringLiteral("gpio")).toString()});
    for (const QJsonValue &button : board.value(QStringLiteral("buttons")).toArray())
        boardRows.append(
            {tr("Button %1").arg(button.toObject().value(QStringLiteral("name")).toString()),
             button.toObject().value(QStringLiteral("gpio")).toString()});
    const QJsonObject probe = board.value(QStringLiteral("debug_probe")).toObject();
    if (!probe.isEmpty()) {
        QStringList ways;
        for (auto it = probe.begin(); it != probe.end(); ++it)
            ways.append(it.key() + QStringLiteral(" ") + formatField(it.key(), it.value()));
        boardRows.append({tr("Debug probe"), ways.join(QStringLiteral("\n"))});
    }
    addGroup(tr("Board"), boardRows);

    QList<QPair<QString, QString>> clockRows;
    if (resolved.contains(QStringLiteral("source")))
        clockRows.append(
            {tr("Source"), resolved.value(QStringLiteral("source")).toString() +
                               QStringLiteral(" · ") +
                               formatHz(resolved.value(QStringLiteral("source_hz")).toDouble())});
    const QJsonObject pll = resolved.value(QStringLiteral("pll")).toObject();
    if (pll.contains(QStringLiteral("vco_hz")))
        clockRows.append(
            {tr("PLL output (VCO)"), formatHz(pll.value(QStringLiteral("vco_hz")).toDouble())});
    if (cpuHz > 0)
        clockRows.append({tr("CPU"), formatHz(cpuHz)});
    const QJsonObject buses = resolved.value(QStringLiteral("bus_hz")).toObject();
    for (auto it = buses.begin(); it != buses.end(); ++it)
        clockRows.append({tr("Bus %1").arg(it.key().toUpper()), formatHz(it->toDouble())});
    if (resolved.contains(QStringLiteral("flash_wait_states")))
        clockRows.append(
            {tr("Flash wait states"), text(resolved.value(QStringLiteral("flash_wait_states")))});
    addGroup(tr("Clocks"), clockRows);

    QList<QPair<QString, QString>> memoryRows;
    for (const QJsonValue &v : m_memory.value(QStringLiteral("regions")).toArray()) {
        const QJsonObject region = v.toObject();
        memoryRows.append({region.value(QStringLiteral("name")).toString(),
                           tr("%1 at %2")
                               .arg(formatBytes(region.value(QStringLiteral("size")).toDouble()),
                                    formatHex(region.value(QStringLiteral("base")).toDouble()))});
    }
    for (const QJsonValue &v : m_memory.value(QStringLiteral("partitions")).toArray()) {
        const QJsonObject part = v.toObject();
        memoryRows.append({tr("Partition %1").arg(part.value(QStringLiteral("name")).toString()),
                           tr("%1 at %2")
                               .arg(formatBytes(part.value(QStringLiteral("size")).toDouble()),
                                    formatHex(part.value(QStringLiteral("base")).toDouble()))});
    }
    addGroup(tr("Memory"), memoryRows);

    QList<QPair<QString, QString>> pinRows;
    for (const QJsonValue &v : m_pins.value(QStringLiteral("assigned")).toArray()) {
        const QJsonObject pin = v.toObject();
        // The mode as the generator writes it (board.md): af7 (usart2_tx), fixed (...), output.
        const QString mode = pin.value(QStringLiteral("mode")).toString();
        const QString signal = pin.value(QStringLiteral("signal")).toString();
        QString how = mode;
        if (mode == QStringLiteral("af"))
            how = QStringLiteral("af%1 (%2)")
                      .arg(pin.value(QStringLiteral("af")).toInt())
                      .arg(signal);
        else if (mode == QStringLiteral("fixed"))
            how = QStringLiteral("fixed (%1)").arg(signal);
        pinRows.append(
            {pin.value(QStringLiteral("pin")).toString(),
             how + QStringLiteral(" · ") + pin.value(QStringLiteral("owner")).toString()});
    }
    addGroup(tr("Pins in use"), pinRows);

    QList<QPair<QString, QString>> deviceRows;
    for (const QJsonValue &v : m_describe.value(QStringLiteral("devices")).toArray()) {
        const QJsonObject device = v.toObject();
        deviceRows.append({device.value(QStringLiteral("name")).toString(),
                           device.value(QStringLiteral("class")).toString()});
    }
    addGroup(tr("Devices"), deviceRows);
    m_layout->addStretch();
}

} // namespace emblinkstudio
