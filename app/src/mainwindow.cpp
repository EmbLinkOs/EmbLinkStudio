// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
#include "mainwindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFile>
#include <QLabel>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace embstudio
{

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(tr("EmbStudio"));
    resize(960, 600);

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    m_status = new QLabel(central);
    m_status->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    m_status->setWordWrap(true);
    m_restart = new QPushButton(tr("&Restart the core"), central);
    m_restart->setVisible(false);
    layout->addWidget(m_status);
    layout->addWidget(m_restart, 0, Qt::AlignLeft);
    layout->addStretch();
    setCentralWidget(central);

    // QDockWidget until the docking framework is decided (spec section 11, question 1).
    auto *dock = new QDockWidget(tr("Core log"), this);
    dock->setObjectName(QStringLiteral("coreLog"));
    m_log = new QPlainTextEdit(dock);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(5000);
    dock->setWidget(m_log);
    addDockWidget(Qt::BottomDockWidgetArea, dock);

    QMenu *file = menuBar()->addMenu(tr("&File"));
    file->addAction(tr("&Quit"), QKeySequence::Quit, this, &QWidget::close);
    QMenu *view = menuBar()->addMenu(tr("&View"));
    view->addAction(dock->toggleViewAction());
    QMenu *help = menuBar()->addMenu(tr("&Help"));
    help->addAction(tr("&About EmbStudio"), this, &MainWindow::showAbout);

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
            setStatus(tr("EmbStudio Core stopped unexpectedly (exit code %1). Views are "
                         "read-only until it restarts.")
                          .arg(code),
                      true);
    });

    startCore();
}

void MainWindow::startCore()
{
    setStatus(tr("Starting EmbStudio Core..."), false);
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
    dialog.setWindowTitle(tr("About EmbStudio"));
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(tr("<b>EmbStudio</b> %1<br>The IDE made for EmbLinkRTOS. "
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_core.stop();
    event->accept();
}

} // namespace embstudio
