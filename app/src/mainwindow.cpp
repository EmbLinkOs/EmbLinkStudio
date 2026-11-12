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

#include "language.h"
#include "mainwindow.h"
#include "segmentedcontrol.h"
#include "settingsdialog.h"
#include "theme.h"
#include "workbench.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QTextBrowser>
#include <QToolBar>
#include <QVBoxLayout>

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

    // The ids name the views in saved layouts: keep them stable.
    m_workbench->addView(QStringLiteral("core"), QString(), coreView, ads::CenterDockWidgetArea);
    m_workbench->addView(QStringLiteral("coreLog"), QString(), m_log, ads::BottomDockWidgetArea);
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

    m_helpMenu = menuBar()->addMenu(QString());
    m_aboutAction = m_helpMenu->addAction(QString(), this, &MainWindow::showAbout);
    m_aboutAction->setMenuRole(QAction::AboutRole);
}

void MainWindow::retranslate()
{
    setWindowTitle(tr("EmbLink Studio"));
    m_project->setText(tr("No project open"));
    const QStringList stages{tr("Configure"), tr("Code"), tr("Debug"), tr("Verify")};
    for (int i = 0; i < int(stages.size()); ++i) {
        m_stages->setSegmentText(i, stages.at(i));
        m_stageActions.at(i)->setText(stages.at(i));
    }
    m_stages->setAccessibleName(tr("Stage"));

    const QList<ads::CDockWidget *> views = m_workbench->views();
    views.at(0)->setWindowTitle(tr("Core"));
    views.at(1)->setWindowTitle(tr("Core log"));
    for (ads::CDockWidget *v : views)
        v->toggleViewAction()->setText(v->windowTitle());
    m_restart->setText(tr("&Restart the core"));

    m_fileMenu->setTitle(tr("&File"));
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
