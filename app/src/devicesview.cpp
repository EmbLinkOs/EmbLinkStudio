// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    devicesview.cpp
 * @brief   Implementation of DevicesView: the list, and the chosen device's fields.
 *
 * @details
 * The list keeps the generator's order, which is the initialization order; it is not
 * sortable, so that order is never lost. The level is written in capitals, EARLY or
 * KERNEL, as the kernel's names for the levels are. A device with no dependency shows a
 * dash.
 *
 * @see     devicesview.h
 * @author  Junior Deogracias
 * @date    2026-11-13 (created)
 */

#include "devicesview.h"
#include "format.h"
#include "theme.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonObject>
#include <QLabel>
#include <QSplitter>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace emblinkstudio
{

DevicesView::DevicesView(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("viewContent"));
    setAttribute(Qt::WA_StyledBackground, true);
    const Theme &theme = Theme::current();

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    auto *split = new QSplitter(Qt::Horizontal, this);
    layout->addWidget(split);

    m_list = new QTreeWidget(split);
    m_list->setObjectName(QStringLiteral("problems")); // the same quiet list style
    m_list->setColumnCount(5);
    m_list->setRootIsDecorated(false);
    m_list->setUniformRowHeights(true);
    m_list->setSortingEnabled(false);
    m_list->setFont(theme.font(QStringLiteral("bodySmall")));
    m_list->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_list->header()->setStretchLastSection(true);

    auto *side = new QWidget(split);
    auto *sideLayout = new QVBoxLayout(side);
    sideLayout->setContentsMargins(12, 4, 4, 4);
    m_title = new QLabel(side);
    m_title->setFont(theme.font(QStringLiteral("title3")));
    sideLayout->addWidget(m_title);
    m_fields = new QTreeWidget(side);
    m_fields->setObjectName(QStringLiteral("problems"));
    m_fields->setColumnCount(2);
    m_fields->setRootIsDecorated(false);
    m_fields->setFont(theme.font(QStringLiteral("codeMeta")));
    m_fields->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_fields->header()->setStretchLastSection(true);
    sideLayout->addWidget(m_fields, 1);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 2);

    connect(m_list, &QTreeWidget::currentItemChanged, this, [this] { showFields(); });
    render();
}

void DevicesView::setDevices(const QJsonArray &devices)
{
    m_devices = devices;
    render();
}

QList<QStringList> DevicesView::rows() const
{
    QList<QStringList> out;
    for (int i = 0; i < m_list->topLevelItemCount(); ++i) {
        QStringList cells;
        for (int c = 0; c < m_list->columnCount(); ++c)
            cells.append(m_list->topLevelItem(i)->text(c));
        out.append(cells);
    }
    return out;
}

QList<QStringList> DevicesView::fields() const
{
    QList<QStringList> out;
    for (int i = 0; i < m_fields->topLevelItemCount(); ++i)
        out.append({m_fields->topLevelItem(i)->text(0), m_fields->topLevelItem(i)->text(1)});
    return out;
}

void DevicesView::select(int row)
{
    if (row >= 0 && row < m_list->topLevelItemCount())
        m_list->setCurrentItem(m_list->topLevelItem(row));
}

void DevicesView::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        render();
    QWidget::changeEvent(event);
}

void DevicesView::render()
{
    const int current = m_list->indexOfTopLevelItem(m_list->currentItem());
    m_list->setHeaderLabels(
        {tr("Device"), tr("Class"), tr("Compatible"), tr("Level"), tr("Depends on")});
    m_fields->setHeaderLabels({tr("Field"), tr("Value")});
    m_list->clear();
    const Theme &theme = Theme::current();
    for (const QJsonValue &v : std::as_const(m_devices)) {
        const QJsonObject d = v.toObject();
        const QString name = d.value(QStringLiteral("name")).toString();
        const QString peripheral = d.value(QStringLiteral("peripheral")).toString();
        QString compatible = d.value(QStringLiteral("compatible")).toString();
        if (!peripheral.isEmpty() && peripheral != name)
            compatible += QStringLiteral(" (%1)").arg(peripheral);
        QStringList deps;
        for (const QJsonValue &dep : d.value(QStringLiteral("deps")).toArray())
            deps.append(dep.toString());
        auto *item = new QTreeWidgetItem(
            {name, d.value(QStringLiteral("class")).toString(), compatible,
             d.value(QStringLiteral("level")).toString().toUpper(),
             deps.isEmpty() ? QStringLiteral("—") : deps.join(QStringLiteral(", "))});
        item->setFont(2, theme.font(QStringLiteral("codeMeta")));
        item->setForeground(2, theme.color(QStringLiteral("text.secondary")));
        m_list->addTopLevelItem(item);
    }
    if (m_list->topLevelItemCount() > 0)
        m_list->setCurrentItem(
            m_list->topLevelItem(qBound(0, current, m_list->topLevelItemCount() - 1)));
    showFields();
}

void DevicesView::showFields()
{
    m_fields->clear();
    const int row = m_list->indexOfTopLevelItem(m_list->currentItem());
    if (row < 0 || row >= m_devices.size()) {
        m_title->setText(m_devices.isEmpty() ? tr("Open a generated board to see its devices.")
                                             : QString());
        return;
    }
    const QJsonObject d = m_devices.at(row).toObject();
    m_title->setText(d.value(QStringLiteral("name")).toString());
    const QJsonObject fields = d.value(QStringLiteral("fields")).toObject();
    for (auto it = fields.begin(); it != fields.end(); ++it)
        m_fields->addTopLevelItem(
            new QTreeWidgetItem({it.key(), formatField(it.key(), it.value())}));
}

} // namespace emblinkstudio
