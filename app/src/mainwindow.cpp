// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    mainwindow.cpp
 * @brief   Implementation of MainWindow: the stage bar, menus, Settings, About, the core.
 *
 * @details
 * Builds the stage bar (the project, the stage switch, the core's state), two
 * Workbench views, "Core" (the status and the restart button) and "Core log", and the
 * File, View and Help menus. The View menu holds the four stages with their shortcuts,
 * shows or hides each view, opens and saves named layouts, and resets the layout.
 * The layout and the stage are saved to the user's settings when the window closes and
 * restored when it opens (per project in .emblink-studio/ once projects exist, spec
 * section 6). The About dialog shows the third-party notices bundled as the
 * :/NOTICES.md resource. The core is started when the window is built and stopped when
 * it closes.
 *
 * Texts that depend on the core's state are kept as functions, so a language change
 * rewrites them in the new language rather than leaving the old one on screen.
 *
 * @see     mainwindow.h
 * @author  Junior Deogracias
 * @date    2026-11-11 (created)
 */

#include "boardview.h"
#include "language.h"
#include "mainwindow.h"
#include "problemsview.h"
#include "segmentedcontrol.h"
#include "session.h"
#include "settingsdialog.h"
#include "theme.h"
#include "workbench.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QTextBrowser>
#include <QToolBar>
#include <QUrl>
#include <QVBoxLayout>
#include <utility>

namespace emblinkstudio
{

namespace
{

QString dot(const QColor &color)
{
    return QStringLiteral("<span style=\"color:%1\">●</span>").arg(color.name());
}

} // namespace

QStringList MainWindow::stageIds()
{
    return {QStringLiteral("configure"), QStringLiteral("code"), QStringLiteral("debug"),
            QStringLiteral("verify")};
}

MainWindow::MainWindow(Language *language, QWidget *parent)
    : QMainWindow(parent), m_language(language)
{
    resize(1280, 800);
    setUnifiedTitleAndToolBarOnMac(true);

    m_workbench = new Workbench(this);
    const Theme &theme = Theme::current();
    const int gutter = int(theme.number(QStringLiteral("space.gutter")));
    m_workbench->manager()->setStyleSheet(theme.dockStyleSheet());
    m_workbench->manager()->setContentsMargins(gutter, gutter, gutter, gutter);

    auto *coreView = new QWidget;
    // A view's own background is the card's (theme: QWidget#viewContent).
    coreView->setObjectName(QStringLiteral("viewContent"));
    coreView->setAttribute(Qt::WA_StyledBackground, true);
    auto *layout = new QVBoxLayout(coreView);
    layout->setContentsMargins(16, 14, 16, 14);
    m_status = new QLabel(coreView);
    m_status->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    m_status->setWordWrap(true);
    m_restart = new QPushButton(coreView);
    m_restart->setProperty("role", QStringLiteral("primary"));
    m_restart->setVisible(false);
    layout->addWidget(m_status);
    layout->addWidget(m_restart, 0, Qt::AlignLeft);
    layout->addStretch();

    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(5000);
    m_log->setFont(theme.font(QStringLiteral("code")));

    m_session = new Session(&m_core, this);
    m_boardView = new BoardView;
    m_problems = new ProblemsView;

    // The ids name the views in saved layouts: keep them stable.
    m_workbench->addView(QStringLiteral("board"), QString(), m_boardView,
                         ads::CenterDockWidgetArea);
    ads::CDockWidget *problems = m_workbench->addView(QStringLiteral("problems"), QString(),
                                                      m_problems, ads::BottomDockWidgetArea);
    m_workbench->addView(QStringLiteral("coreLog"), QString(), m_log, ads::BottomDockWidgetArea,
                         problems);
    m_workbench->addView(QStringLiteral("core"), QString(), coreView, ads::BottomDockWidgetArea,
                         problems);
    problems->setAsCurrentTab();
    m_workbench->captureDefaultLayout();

    buildStageBar();
    buildMenus();

    connect(m_workbench, &Workbench::stageChanged, this,
            [this](const QString &id) { m_stages->setCurrentIndex(int(stageIds().indexOf(id))); });
    {
        QSettings settings;
        settings.beginGroup(QStringLiteral("layout"));
        m_workbench->restore(settings);
    }
    if (m_workbench->stage().isEmpty())
        m_workbench->setStage(stageIds().first());

    connect(m_session, &Session::statusChanged, this, &MainWindow::retranslate);
    connect(m_session, &Session::modelChanged, this, &MainWindow::showModel);
    connect(m_session, &Session::diagnosticsChanged, this, [this] {
        m_problems->setDiagnostics(m_session->diagnostics(), m_session->isOpen(),
                                   m_session->hasVerdict(), m_session->isValid());
    });
    connect(m_session, &Session::busyChanged, m_problems, &ProblemsView::setBusy);
    connect(m_session, &Session::opened, this, [this] {
        rememberBoard(m_session->status().value(QStringLiteral("path")).toString());
    });
    connect(m_session, &Session::trustNeeded, this,
            [this](const QString &checkout, Session::Action action) {
                askTrust(checkout, int(action));
            });
    connect(m_session, &Session::failed, this, [this](const QString &message) {
        QMessageBox::warning(this, tr("EmbLink Studio"), message);
    });
    connect(m_problems, &ProblemsView::validateRequested, m_session, &Session::validate);
    connect(m_problems, &ProblemsView::openRequested, this, [](const QString &file, int) {
        // No editor yet (stage S8): the person's own editor opens the file.
        QDesktopServices::openUrl(QUrl::fromLocalFile(file));
    });

    connect(m_restart, &QPushButton::clicked, this, &MainWindow::startCore);
    connect(&m_core, &CoreClient::logLine, m_log, &QPlainTextEdit::appendPlainText);
    connect(&m_core, &CoreClient::ready, this, [this](const QJsonObject &info) {
        const QJsonObject server = info.value(QStringLiteral("server")).toObject();
        const QString name = server.value(QStringLiteral("name")).toString();
        const QString version = server.value(QStringLiteral("version")).toString();
        const QString protocol = info.value(QStringLiteral("protocol_version")).toString();
        setStatus(
            [=] { return tr("Connected to %1 %2, protocol %3.").arg(name, version, protocol); },
            CoreState::Connected);
        // After a restart, the board that was open opens again; at start, the one asked for.
        if (!m_pendingOpen.isEmpty())
            m_session->open(std::exchange(m_pendingOpen, QString()));
        else
            m_session->reopen();
    });
    connect(&m_core, &CoreClient::failed, this, [this](const QString &message) {
        setStatus([=] { return message; }, CoreState::Stopped);
    });
    connect(&m_core, &CoreClient::exited, this, [this](int code, bool expected) {
        if (!expected)
            setStatus(
                [=] {
                    return tr("EmbLink Studio Core stopped unexpectedly (exit code %1). Views "
                              "are read-only until it restarts.")
                        .arg(code);
                },
                CoreState::Stopped);
    });

    retranslate();
    startCore();
}

void MainWindow::buildStageBar()
{
    m_stageBar = new QToolBar(this);
    m_stageBar->setObjectName(QStringLiteral("stageBar"));
    m_stageBar->setMovable(false);
    m_stageBar->setFloatable(false);
    m_stageBar->toggleViewAction()->setVisible(false);
    addToolBar(Qt::TopToolBarArea, m_stageBar);

    // The window's own controls sit to the left on macOS; keep clear of the edge everywhere.
    auto *lead = new QWidget(m_stageBar);
    lead->setFixedWidth(8);
    m_stageBar->addWidget(lead);
    m_project = new QLabel(m_stageBar);
    m_project->setObjectName(QStringLiteral("projectName"));
    QFont projectFont = Theme::current().font(QStringLiteral("body"));
    projectFont.setWeight(QFont::DemiBold);
    m_project->setFont(projectFont);
    m_stageBar->addWidget(m_project);

    m_stages = new SegmentedControl(m_stageBar);
    for (int i = 0; i < int(stageIds().size()); ++i)
        m_stages->addSegment(QString());
    m_stageBar->addWidget(m_stages);
    connect(m_stages, &SegmentedControl::currentChanged, this,
            [this](int index) { m_workbench->setStage(stageIds().value(index)); });

    auto *spacer = new QWidget(m_stageBar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_stageBar->addWidget(spacer);

    m_corePill = new QLabel(m_stageBar);
    m_corePill->setObjectName(QStringLiteral("corePill"));
    m_corePill->setTextFormat(Qt::RichText);
    m_stageBar->addWidget(m_corePill);
    auto *trail = new QWidget(m_stageBar);
    trail->setFixedWidth(8);
    m_stageBar->addWidget(trail);
}

void MainWindow::buildMenus()
{
    m_fileMenu = menuBar()->addMenu(QString());
    m_openAction = m_fileMenu->addAction(QString(), this, &MainWindow::chooseBoard);
    m_openAction->setShortcut(QKeySequence::Open);
    m_recentMenu = m_fileMenu->addMenu(QString());
    connect(m_recentMenu, &QMenu::aboutToShow, this, &MainWindow::fillRecentMenu);
    m_closeAction = m_fileMenu->addAction(QString(), m_session, &Session::close);
    m_fileMenu->addSeparator();
    m_settingsAction = m_fileMenu->addAction(QString(), this, &MainWindow::showSettings);
    m_settingsAction->setShortcut(QKeySequence::Preferences);
    m_settingsAction->setMenuRole(QAction::PreferencesRole);
    m_fileMenu->addSeparator();
    m_quitAction = m_fileMenu->addAction(QString(), QKeySequence::Quit, this, &QWidget::close);
    m_quitAction->setMenuRole(QAction::QuitRole);

    m_viewMenu = menuBar()->addMenu(QString());
    for (int i = 0; i < int(stageIds().size()); ++i) {
        // Ctrl is ⌘ on macOS: ⌘1 to ⌘4 (design brief section 4g).
        QAction *a =
            m_viewMenu->addAction(QString(), this, [this, i] { m_stages->setCurrentIndex(i); });
        a->setShortcut(QKeySequence(Qt::CTRL | Qt::Key(Qt::Key_1 + i)));
        m_stageActions.append(a);
    }
    m_viewMenu->addSeparator();
    for (ads::CDockWidget *v : m_workbench->views())
        m_viewMenu->addAction(v->toggleViewAction());
    m_viewMenu->addSeparator();
    m_layouts = m_viewMenu->addMenu(QString());
    connect(m_layouts, &QMenu::aboutToShow, this, &MainWindow::fillLayoutsMenu);
    m_saveLayoutAction = m_viewMenu->addAction(QString(), this, &MainWindow::saveLayoutAs);
    m_resetLayoutAction = m_viewMenu->addAction(QString(), m_workbench, &Workbench::resetLayout);

    m_boardMenu = menuBar()->addMenu(QString());
    m_validateAction = m_boardMenu->addAction(QString(), m_session, &Session::validate);
    m_generateAction = m_boardMenu->addAction(QString(), m_session, &Session::generate);
    // ⌥⌘G (design brief section 4g); Ctrl+Alt+G elsewhere.
    m_generateAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_G));

    m_helpMenu = menuBar()->addMenu(QString());
    m_aboutAction = m_helpMenu->addAction(QString(), this, &MainWindow::showAbout);
    m_aboutAction->setMenuRole(QAction::AboutRole);
}

void MainWindow::retranslate()
{
    setWindowTitle(tr("EmbLink Studio"));
    const QJsonObject status = m_session->status();
    m_project->setText(m_session->isOpen() ? status.value(QStringLiteral("id")).toString()
                                           : tr("No board open"));
    m_project->setToolTip(status.value(QStringLiteral("path")).toString());
    const QStringList stages{tr("Configure"), tr("Code"), tr("Debug"), tr("Verify")};
    for (int i = 0; i < int(stages.size()); ++i) {
        m_stages->setSegmentText(i, stages.at(i));
        m_stageActions.at(i)->setText(stages.at(i));
    }
    m_stages->setAccessibleName(tr("Stage"));

    const QList<ads::CDockWidget *> views = m_workbench->views();
    views.at(0)->setWindowTitle(tr("Board"));
    views.at(1)->setWindowTitle(tr("Problems"));
    views.at(2)->setWindowTitle(tr("Core log"));
    views.at(3)->setWindowTitle(tr("Core"));
    for (ads::CDockWidget *v : views)
        v->toggleViewAction()->setText(v->windowTitle());
    m_restart->setText(tr("&Restart the core"));

    m_fileMenu->setTitle(tr("&File"));
    m_openAction->setText(tr("&Open Board..."));
    m_recentMenu->setTitle(tr("Open &Recent"));
    m_closeAction->setText(tr("&Close Board"));
    m_closeAction->setEnabled(m_session->isOpen());
    m_boardMenu->setTitle(tr("&Board"));
    m_validateAction->setText(tr("&Validate"));
    m_generateAction->setText(tr("&Generate"));
    m_validateAction->setEnabled(m_session->isOpen());
    m_generateAction->setEnabled(m_session->isOpen());
    m_settingsAction->setText(tr("&Settings..."));
    m_quitAction->setText(tr("&Quit"));
    m_viewMenu->setTitle(tr("&View"));
    m_layouts->setTitle(tr("&Layouts"));
    m_saveLayoutAction->setText(tr("&Save layout as..."));
    m_resetLayoutAction->setText(tr("&Reset layout"));
    m_helpMenu->setTitle(tr("&Help"));
    m_aboutAction->setText(tr("&About EmbLink Studio"));

    setStatus(m_statusText, m_coreState);
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        retranslate();
    QMainWindow::changeEvent(event);
}

void MainWindow::setStage(const QString &id)
{
    const int index = int(stageIds().indexOf(id));
    if (index >= 0)
        m_stages->setCurrentIndex(index);
}

QString MainWindow::stage() const
{
    return m_workbench->stage();
}

void MainWindow::openBoard(const QString &path)
{
    if (m_core.isReady())
        m_session->open(path);
    else
        m_pendingOpen = path;
}

void MainWindow::chooseBoard()
{
    QSettings settings;
    const QStringList recent = settings.value(QStringLiteral("recentBoards")).toStringList();
    const QString start = recent.isEmpty() ? QDir::homePath() : recent.first();
    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("Open a board of an EmbLinkRTOS checkout, such as boards/st/nucleo_f446re"),
        start);
    if (!dir.isEmpty())
        openBoard(dir);
}

void MainWindow::rememberBoard(const QString &path)
{
    if (path.isEmpty())
        return;
    QSettings settings;
    QStringList recent = settings.value(QStringLiteral("recentBoards")).toStringList();
    recent.removeAll(path);
    recent.prepend(path);
    while (recent.size() > 8)
        recent.removeLast();
    settings.setValue(QStringLiteral("recentBoards"), recent);
}

void MainWindow::fillRecentMenu()
{
    m_recentMenu->clear();
    const QStringList recent = QSettings().value(QStringLiteral("recentBoards")).toStringList();
    if (recent.isEmpty()) {
        m_recentMenu->addAction(tr("No recent boards"))->setEnabled(false);
        return;
    }
    for (const QString &path : recent)
        m_recentMenu->addAction(QDir::toNativeSeparators(path), this,
                                [this, path] { openBoard(path); });
}

void MainWindow::askTrust(const QString &checkout, int action)
{
    QMessageBox box(QMessageBox::Question, tr("Trust this checkout?"),
                    tr("Validating and generating run the generator of the EmbLinkRTOS checkout "
                       "at %1, which is code from that checkout.")
                        .arg(QDir::toNativeSeparators(checkout)),
                    QMessageBox::NoButton, this);
    box.setInformativeText(tr("Trust it only if you know where it comes from. EmbLink Studio "
                              "remembers your answer for this checkout."));
    QPushButton *trust = box.addButton(tr("Trust and Continue"), QMessageBox::AcceptRole);
    box.addButton(QMessageBox::Cancel);
    box.setDefaultButton(trust);
    box.exec();
    if (box.clickedButton() == trust)
        m_session->trustAndRun(Session::Action(action));
}

void MainWindow::showModel()
{
    m_boardView->setModel(m_session->status(), m_session->describe(), m_session->clocks(),
                          m_session->memory(), m_session->pins());
}

void MainWindow::startCore()
{
    setStatus([] { return tr("Starting EmbLink Studio Core..."); }, CoreState::Starting);
    m_core.start(CoreClient::defaultLaunch());
}

void MainWindow::setStatus(std::function<QString()> text, CoreState state)
{
    m_statusText = std::move(text);
    m_coreState = state;
    m_status->setText(m_statusText ? m_statusText() : QString());
    m_restart->setVisible(state == CoreState::Stopped);

    const Theme &theme = Theme::current();
    switch (state) {
    case CoreState::Starting:
        m_corePill->setText(dot(theme.color(QStringLiteral("state.warning.base"))) +
                            QLatin1Char(' ') + tr("Core starting").toHtmlEscaped());
        break;
    case CoreState::Connected:
        m_corePill->setText(dot(theme.color(QStringLiteral("state.success.base"))) +
                            QLatin1Char(' ') + tr("Core connected").toHtmlEscaped());
        break;
    case CoreState::Stopped:
        m_corePill->setText(dot(theme.color(QStringLiteral("state.danger.base"))) +
                            QLatin1Char(' ') + tr("Core stopped").toHtmlEscaped());
        break;
    }
}

void MainWindow::showSettings()
{
    SettingsDialog dialog(m_language, this);
    dialog.exec();
}

void MainWindow::showAbout()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("About EmbLink Studio"));
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(tr("<b>EmbLink Studio</b> %1<br>The development system for "
                                    "EmbLinkRTOS. Licensed under the Apache License 2.0.")
                                     .arg(QApplication::applicationVersion()),
                                 &dialog));
    auto *notices = new QTextBrowser(&dialog);
    QFile file(QStringLiteral(":/NOTICES.md"));
    notices->setMarkdown(file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll())
                                                        : tr("The notices list is missing."));
    layout->addWidget(notices);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    dialog.resize(640, 480);
    dialog.exec();
}

void MainWindow::fillLayoutsMenu()
{
    m_layouts->clear();
    const QStringList names = m_workbench->namedLayouts();
    if (names.isEmpty()) {
        m_layouts->addAction(tr("No saved layouts"))->setEnabled(false);
        return;
    }
    for (const QString &name : names)
        m_layouts->addAction(name, this, [this, name] { m_workbench->openNamedLayout(name); });
    m_layouts->addSeparator();
    QMenu *remove = m_layouts->addMenu(tr("&Delete"));
    for (const QString &name : names)
        remove->addAction(name, this, [this, name] { m_workbench->removeNamedLayout(name); });
}

void MainWindow::saveLayoutAs()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Save layout"),
                                               tr("Name of this layout, such as Debug:"),
                                               QLineEdit::Normal, QString(), &ok)
                             .trimmed();
    if (ok && !name.isEmpty())
        m_workbench->saveNamedLayout(name);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("layout"));
    m_workbench->save(settings);
    settings.endGroup();
    m_core.stop();
    event->accept();
}

} // namespace emblinkstudio
