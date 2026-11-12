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
 * The views so far: Board, the open board's model; Clock tree, Memory and Devices,
 * drawn from the generator's model (stage S2); Problems, the generator's
 * diagnostics, each linked to its file; Core, whether EmbLink Studio Core is connected,
 * with a restart when it fails or stops; and the core's log. The window owns the
 * CoreClient and the Session (the open board) for the life of the application. Open
 * Board (⌘O) opens a board of an EmbLinkRTOS checkout; the Board menu validates and
 * generates it, asking first to trust the checkout when the core needs it.
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

class BoardView;
class ClockTreeView;
class DevicesView;
class Language;
class MemoryView;
class ProblemsView;
class SegmentedControl;
class Session;
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
    // Opens a board directory of an EmbLinkRTOS checkout, once the core is ready.
    void openBoard(const QString &path);
    // Brings the view with this id to the front: "board", "clocks", "memory", "devices", ...
    void showView(const QString &id);
    Session *session() const
    {
        return m_session;
    }

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
    void chooseBoard();
    void fillRecentMenu();
    void rememberBoard(const QString &path);
    void askTrust(const QString &checkout, int action);
    void showModel();

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
    QMenu *m_boardMenu = nullptr;
    QMenu *m_recentMenu = nullptr;
    QMenu *m_layouts = nullptr;
    QAction *m_openAction = nullptr;
    QAction *m_closeAction = nullptr;
    QAction *m_validateAction = nullptr;
    QAction *m_generateAction = nullptr;
    QAction *m_settingsAction = nullptr;
    QAction *m_quitAction = nullptr;
    QAction *m_saveLayoutAction = nullptr;
    QAction *m_resetLayoutAction = nullptr;
    QAction *m_aboutAction = nullptr;
    QList<QAction *> m_stageActions;

    QLabel *m_status = nullptr;
    QPushButton *m_restart = nullptr;
    QPlainTextEdit *m_log = nullptr;
    Session *m_session = nullptr;
    BoardView *m_boardView = nullptr;
    ClockTreeView *m_clockView = nullptr;
    MemoryView *m_memoryView = nullptr;
    DevicesView *m_devicesView = nullptr;
    ProblemsView *m_problems = nullptr;
    QString m_pendingOpen;
    std::function<QString()> m_statusText;
    CoreState m_coreState = CoreState::Starting;
};

} // namespace emblinkstudio
