// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
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
