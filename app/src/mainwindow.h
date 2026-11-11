// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    mainwindow.h
 * @brief   MainWindow: the main window of EmbLink Studio (stage S0).
 *
 * @details
 * Shows whether EmbLink Studio Core is connected, offers a restart when the core
 * fails or stops unexpectedly, and shows the core's log in a dock. The window owns
 * the CoreClient for the life of the application.
 *
 * @see     Spec section 4; coreclient.h
 * @author  Junior Deogracias
 * @date    2026-11-11 (created)
 */

#pragma once

#include "coreclient.h"

#include <QMainWindow>

class QLabel;
class QPlainTextEdit;
class QPushButton;

namespace emblinkstudio
{

// S0's window: the core's state, its log, and a restart if it stops (spec section 4).
class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = nullptr);

  protected:
    void closeEvent(QCloseEvent *event) override;

  private:
    void startCore();
    void showAbout();
    void setStatus(const QString &text, bool canRestart);

    CoreClient m_core;
    QLabel *m_status = nullptr;
    QPushButton *m_restart = nullptr;
    QPlainTextEdit *m_log = nullptr;
};

} // namespace emblinkstudio
