// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    problemsview.cpp
 * @brief   Implementation of ProblemsView: the summary, the list, opening a problem's file.
 *
 * @details
 * Each row is a dot in the state's colour (danger for an error, warning for a
 * warning), the message, and the place as "board.yaml:25 · pins.usart2.tx", the full
 * path in its tooltip. Errors come before warnings; within each, the generator's order
 * is kept. Counts use Qt's plural forms, so each language says them its own way.
 *
 * @see     problemsview.h
 * @author  Junior Deogracias
 * @date    2026-11-12 (created)
 */

#include "problemsview.h"
#include "theme.h"

#include <QEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonObject>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace emblinkstudio
{

namespace
{

constexpr int kFileRole = Qt::UserRole;
constexpr int kLineRole = Qt::UserRole + 1;

QIcon dot(const QColor &color)
{
    QPixmap pixmap(14, 14);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    p.drawEllipse(QRectF(3.5, 3.5, 7, 7));
    return QIcon(pixmap);
}

} // namespace

ProblemsView::ProblemsView(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("viewContent"));
    setAttribute(Qt::WA_StyledBackground, true);
    const Theme &theme = Theme::current();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 10, 12, 8);
    layout->setSpacing(8);
    auto *header = new QHBoxLayout;
    m_summary = new QLabel(this);
    m_summary->setFont(theme.font(QStringLiteral("bodySmall")));
    m_summary->setWordWrap(true);
    m_validate = new QPushButton(this);
    header->addWidget(m_summary, 1);
    header->addWidget(m_validate);
    layout->addLayout(header);

    m_list = new QTreeWidget(this);
    m_list->setObjectName(QStringLiteral("problems"));
    m_list->setColumnCount(2);
    m_list->setRootIsDecorated(false);
    m_list->setUniformRowHeights(true);
    m_list->setAlternatingRowColors(false);
    m_list->header()->setStretchLastSection(false);
    m_list->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_list->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_list->setFont(theme.font(QStringLiteral("bodySmall")));
    layout->addWidget(m_list, 1);
    layout->addStretch(); // keeps the summary at the top while the list is hidden

    connect(m_validate, &QPushButton::clicked, this, &ProblemsView::validateRequested);
    connect(m_list, &QTreeWidget::itemActivated, this,
            [this](QTreeWidgetItem *item) { activate(item); });
    render();
}

void ProblemsView::setDiagnostics(const QJsonArray &diagnostics, bool open, bool hasVerdict,
                                  bool valid)
{
    m_diagnostics = diagnostics;
    m_open = open;
    m_hasVerdict = hasVerdict;
    m_valid = valid;
    render();
}

void ProblemsView::setBusy(bool busy)
{
    m_busy = busy;
    m_validate->setEnabled(m_open && !busy);
}

QString ProblemsView::summary() const
{
    return m_summary->text();
}

int ProblemsView::rowCount() const
{
    return m_list->topLevelItemCount();
}

void ProblemsView::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        render();
    QWidget::changeEvent(event);
}

void ProblemsView::activate(QTreeWidgetItem *item)
{
    const QString file = item->data(0, kFileRole).toString();
    if (!file.isEmpty())
        emit openRequested(file, item->data(0, kLineRole).toInt());
}

void ProblemsView::render()
{
    const Theme &theme = Theme::current();
    m_validate->setText(tr("Validate"));
    m_validate->setEnabled(m_open && !m_busy);
    m_list->setHeaderLabels({tr("Problem"), tr("Where")});
    m_list->clear();

    int errors = 0;
    int warnings = 0;
    QList<QTreeWidgetItem *> errorRows;
    QList<QTreeWidgetItem *> warningRows;
    for (const QJsonValue &v : std::as_const(m_diagnostics)) {
        const QJsonObject d = v.toObject();
        const bool error = d.value(QStringLiteral("severity")).toString() ==
                           QStringLiteral("error");
        (error ? errors : warnings)++;
        const QString file = d.value(QStringLiteral("file")).toString();
        const int line = d.value(QStringLiteral("line")).toInt();
        QString where;
        if (!file.isEmpty())
            where = QFileInfo(file).fileName() +
                    (line > 0 ? QStringLiteral(":%1").arg(line) : QString());
        const QString path = d.value(QStringLiteral("path")).toString();
        if (!path.isEmpty())
            where += (where.isEmpty() ? QString() : QStringLiteral(" · ")) + path;
        auto *item = new QTreeWidgetItem({d.value(QStringLiteral("message")).toString(), where});
        item->setIcon(0, dot(theme.color(error ? QStringLiteral("state.danger.base")
                                               : QStringLiteral("state.warning.base"))));
        item->setData(0, kFileRole, file);
        item->setData(0, kLineRole, line);
        item->setToolTip(0, d.value(QStringLiteral("message")).toString());
        item->setToolTip(1, file.isEmpty() ? where : file + QStringLiteral(":%1").arg(line));
        item->setFont(1, theme.font(QStringLiteral("codeMeta")));
        item->setForeground(1, theme.color(QStringLiteral("text.tertiary")));
        (error ? errorRows : warningRows).append(item);
    }
    m_list->addTopLevelItems(errorRows + warningRows);
    m_list->setVisible(!m_diagnostics.isEmpty());

    if (!m_open)
        m_summary->setText(tr("Open a board to see what the generator says about it."));
    else if (!m_hasVerdict)
        m_summary->setText(tr("Not validated yet."));
    else if (errors == 0 && warnings == 0)
        m_summary->setText(tr("No problems: the generator accepts this board."));
    else if (errors == 0)
        m_summary->setText(tr("Valid, with %n warning(s).", nullptr, warnings));
    else
        m_summary->setText(
            tr("%n error(s)", nullptr, errors) +
            (warnings ? QStringLiteral(", ") + tr("%n warning(s)", nullptr, warnings) : QString()) +
            QStringLiteral(". ") + tr("The generator refuses this board as it is."));
}

} // namespace emblinkstudio
