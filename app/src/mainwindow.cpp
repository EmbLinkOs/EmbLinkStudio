// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    mainwindow.cpp
 * @brief   Implementation of MainWindow: layout, menus, About dialog, core lifecycle.
 *
 * @details
 * Builds two Workbench views, "Core" (the status label and the restart button) and
 * "Core log", and the File, View and Help menus. The View menu shows or hides each
 * view, opens and saves named layouts, and resets the layout. The layout is saved to
 * the user's settings when the window closes and restored when it opens (per project
 * in .emblink-studio/ once projects exist, spec section 6). The About dialog shows the third-party
 * notices bundled as the
 * :/NOTICES.md resource. The core is started when the window is built and stopped
 * when it closes.
 *
 * @see     mainwindow.h
 * @author  Junior Deogracias
 * @date    2026-11-11 (created)
 */

#include "mainwindow.h"
#include "workbench.h"

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
#include <QVBoxLayout>

namespace emblinkstudio
{

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(tr("EmbLink Studio"));
    resize(960, 600);

    m_workbench = new Workbench(this);

    auto *coreView = new QWidget;
    auto *layout = new QVBoxLayout(coreView);
    m_status = new QLabel(coreView);
    m_status->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    m_status->setWordWrap(true);
    m_restart = new QPushButton(tr("&Restart the core"), coreView);
    m_restart->setVisible(false);
    layout->addWidget(m_status);
    layout->addWidget(m_restart, 0, Qt::AlignLeft);
    layout->addStretch();

    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(5000);

    // The ids name the views in saved layouts: keep them stable.
    m_workbench->addView(QStringLiteral("core"), tr("Core"), coreView, ads::CenterDockWidgetArea);
    m_workbench->addView(QStringLiteral("coreLog"), tr("Core log"), m_log,
                         ads::BottomDockWidgetArea);
    m_workbench->captureDefaultLayout();
    {
        QSettings settings;
        settings.beginGroup(QStringLiteral("layout"));
        m_workbench->restore(settings);
    }

    QMenu *file = menuBar()->addMenu(tr("&File"));
    file->addAction(tr("&Quit"), QKeySequence::Quit, this, &QWidget::close);
    QMenu *view = menuBar()->addMenu(tr("&View"));
    for (ads::CDockWidget *v : m_workbench->views())
        view->addAction(v->toggleViewAction());
    view->addSeparator();
    m_layouts = view->addMenu(tr("&Layouts"));
    connect(m_layouts, &QMenu::aboutToShow, this, &MainWindow::fillLayoutsMenu);
    view->addAction(tr("&Save layout as..."), this, &MainWindow::saveLayoutAs);
    view->addAction(tr("&Reset layout"), m_workbench, &Workbench::resetLayout);
    QMenu *help = menuBar()->addMenu(tr("&Help"));
    help->addAction(tr("&About EmbLink Studio"), this, &MainWindow::showAbout);

    connect(m_restart, &QPushButton::clicked, this, &MainWindow::startCore);
    connect(&m_core, &CoreClient::logLine, m_log, &QPlainTextEdit::appendPlainText);
    connect(&m_core, &CoreClient::ready, this, [this](const QJsonObject &info) {
        const QJsonObject server = info.value(QStringLiteral("server")).toObject();
        setStatus(tr("Connected to %1 %2, protocol %3.")
                      .arg(server.value(QStringLiteral("name")).toString(),
                           server.value(QStringLiteral("version")).toString(),
                           info.value(QStringLiteral("protocol_version")).toString()),
                  false);
    });
    connect(&m_core, &CoreClient::failed, this,
            [this](const QString &message) { setStatus(message, true); });
    connect(&m_core, &CoreClient::exited, this, [this](int code, bool expected) {
        if (!expected)
            setStatus(tr("EmbLink Studio Core stopped unexpectedly (exit code %1). Views are "
                         "read-only until it restarts.")
                          .arg(code),
                      true);
    });

    startCore();
}

void MainWindow::startCore()
{
    setStatus(tr("Starting EmbLink Studio Core..."), false);
    m_core.start(CoreClient::defaultLaunch());
}

void MainWindow::setStatus(const QString &text, bool canRestart)
{
    m_status->setText(text);
    m_restart->setVisible(canRestart);
}

void MainWindow::showAbout()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("About EmbLink Studio"));
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(tr("<b>EmbLink Studio</b> %1<br>The IDE made for EmbLinkRTOS. "
                                    "Licensed under the Apache License 2.0.")
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
