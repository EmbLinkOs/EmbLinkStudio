// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    mainwindow.h
 * @brief   MainWindow: the main window of EmbLink Studio, in the frame of the design brief.
 *
 * @details
 * The window is the frame every screen shares (design brief section 4d): a stage bar
 * at the top, merged with the title bar on macOS, holding the project and the four
 * stages (Configure, Code, Debug, Verify) as one segmented control, with ⌘1 to ⌘4
 * (Ctrl elsewhere); and below it the Workbench, where every view is a card. Each stage
 * keeps its own layout.
 *
 * The views so far show whether EmbLink Studio Core is connected, with a restart when
 * the core fails or stops unexpectedly, and the core's log. The window owns the
 * CoreClient for the life of the application.
 *
 * Every visible text is set in retranslate(), which runs again when the interface
 * language changes, so the window follows a change made in Settings at once.
 *
 * @see     Spec sections 4 and 7; design brief sections 4d, 4f, 4g;
 *          coreclient.h, workbench.h, segmentedcontrol.h, language.h
 * @author  Junior Deogracias
 * @date    2026-11-11 (created)
 */

#pragma once

#include "coreclient.h"

#include <QList>
#include <QMainWindow>
#include <functional>

class QAction;
class QLabel;
class QMenu;
class QPlainTextEdit;
class QPushButton;
class QToolBar;

namespace emblinkstudio
{

class Language;
class SegmentedControl;
class Workbench;

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(Language *language, QWidget *parent = nullptr);

    // The stages, in the order of the switch: "configure", "code", "debug", "verify".
    static QStringList stageIds();
    void setStage(const QString &id);
    QString stage() const;

  protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

  private:
    enum class CoreState {
        Starting,
        Connected,
        Stopped
    };

    void buildStageBar();
    void buildMenus();
    void retranslate();
    void startCore();
    void showAbout();
    void showSettings();
    void setStatus(std::function<QString()> text, CoreState state);
    void fillLayoutsMenu();
    void saveLayoutAs();

    Language *m_language = nullptr;
    CoreClient m_core;
    Workbench *m_workbench = nullptr;

    QToolBar *m_stageBar = nullptr;
    QLabel *m_project = nullptr;
    SegmentedControl *m_stages = nullptr;
    QLabel *m_corePill = nullptr;

    QMenu *m_fileMenu = nullptr;
    QMenu *m_viewMenu = nullptr;
    QMenu *m_helpMenu = nullptr;
    QMenu *m_layouts = nullptr;
    QAction *m_settingsAction = nullptr;
    QAction *m_quitAction = nullptr;
    QAction *m_saveLayoutAction = nullptr;
    QAction *m_resetLayoutAction = nullptr;
    QAction *m_aboutAction = nullptr;
    QList<QAction *> m_stageActions;

    QLabel *m_status = nullptr;
    QPushButton *m_restart = nullptr;
    QPlainTextEdit *m_log = nullptr;
    std::function<QString()> m_statusText;
    CoreState m_coreState = CoreState::Starting;
};

} // namespace emblinkstudio
