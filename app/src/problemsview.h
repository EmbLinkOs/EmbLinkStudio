// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    problemsview.h
 * @brief   ProblemsView: every diagnostic, each linked to its file and line (spec section 7).
 *
 * @details
 * Lists the generator's diagnostics for the open board, errors first, each with its
 * message and its place: the file, the line and the node's path in the YAML. A
 * double-click (or Return) asks to open the file at that line. The summary above the
 * list says whether the board has been validated and what the generator found; the
 * Validate button asks the generator again.
 *
 * @see     session.h; protocol/project.validate.result.schema.json
 * @author  Junior Deogracias
 * @date    2026-11-12 (created)
 */

#pragma once

#include <QJsonArray>
#include <QWidget>

class QLabel;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

namespace emblinkstudio
{

class ProblemsView : public QWidget
{
    Q_OBJECT

  public:
    explicit ProblemsView(QWidget *parent = nullptr);

    void setDiagnostics(const QJsonArray &diagnostics, bool open, bool hasVerdict, bool valid);
    void setBusy(bool busy);

    QString summary() const;
    int rowCount() const;
    QTreeWidget *list() const
    {
        return m_list;
    }

  signals:
    void validateRequested();
    void openRequested(const QString &file, int line);

  protected:
    void changeEvent(QEvent *event) override;

  private:
    void render();
    void activate(QTreeWidgetItem *item);

    QLabel *m_summary = nullptr;
    QPushButton *m_validate = nullptr;
    QTreeWidget *m_list = nullptr;
    QJsonArray m_diagnostics;
    bool m_open = false;
    bool m_hasVerdict = false;
    bool m_valid = false;
    bool m_busy = false;
};

} // namespace emblinkstudio
