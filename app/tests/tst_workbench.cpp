// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    tst_workbench.cpp
 * @brief   Qt Test suite for Workbench, the dockable layout.
 *
 * @details
 * Checks the layout rule of spec section 7: every view can be moved, floated and closed;
 * a layout saved is the layout restored, in a new window as after a restart; named
 * layouts survive a restart through QSettings; Reset brings back the default; and a
 * layout from another version is refused rather than half applied; and each stage keeps
 * its own layout, across a restart too.
 *
 * @see     workbench.h
 * @author  Junior Deogracias
 * @date    2026-11-11 (created)
 */

#include "workbench.h"

#include <DockAreaWidget.h>
#include <QLabel>
#include <QMainWindow>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <memory>

using emblinkstudio::Workbench;

namespace
{

// A window with three views: "editor" in the centre, "log" at the bottom and "console"
// as a tab beside the log: the default layout.
struct Window {
    QMainWindow window;
    Workbench *workbench = nullptr;
    ads::CDockWidget *editor = nullptr;
    ads::CDockWidget *log = nullptr;
    ads::CDockWidget *console = nullptr;

    Window()
    {
        workbench = new Workbench(&window);
        editor = workbench->addView(QStringLiteral("editor"), QStringLiteral("Editor"),
                                    new QLabel(QStringLiteral("editor")),
                                    ads::CenterDockWidgetArea);
        log = workbench->addView(QStringLiteral("log"), QStringLiteral("Log"),
                                 new QLabel(QStringLiteral("log")), ads::BottomDockWidgetArea);
        console = workbench->addView(QStringLiteral("console"), QStringLiteral("Console"),
                                     new QLabel(QStringLiteral("console")),
                                     ads::BottomDockWidgetArea, log);
        workbench->captureDefaultLayout();
        window.resize(1000, 700);
        window.show();
    }

    // Moves the console out of the log's area, to the right of the editor.
    void moveConsoleRight()
    {
        workbench->manager()->addDockWidget(ads::RightDockWidgetArea, console,
                                            editor->dockAreaWidget());
    }

    bool consoleTabbedWithLog() const
    {
        return console->dockAreaWidget() == log->dockAreaWidget();
    }
};

} // namespace

class TestWorkbench : public QObject
{
    Q_OBJECT

  private slots:
    void noViewIsFixed()
    {
        Window w;
        const auto needed = ads::CDockWidget::DockWidgetMovable |
                            ads::CDockWidget::DockWidgetFloatable |
                            ads::CDockWidget::DockWidgetClosable;
        QCOMPARE(w.workbench->views().size(), 3);
        for (ads::CDockWidget *view : w.workbench->views())
            QVERIFY2((view->features() & needed) == needed, qPrintable(view->objectName()));
    }

    void defaultLayoutTabsTheConsoleWithTheLog()
    {
        Window w;
        QVERIFY(w.consoleTabbedWithLog());
        QVERIFY(w.editor->dockAreaWidget() != w.log->dockAreaWidget());
    }

    void savedLayoutIsRestoredInANewWindow()
    {
        QByteArray state;
        {
            Window first;
            first.moveConsoleRight();
            QVERIFY(!first.consoleTabbedWithLog());
            state = first.workbench->saveLayout();
        }
        Window second;
        QVERIFY(second.consoleTabbedWithLog());
        QVERIFY(second.workbench->restoreLayout(state));
        QVERIFY(!second.consoleTabbedWithLog());
        QVERIFY(second.console->dockAreaWidget() != second.editor->dockAreaWidget());
    }

    void closedViewStaysClosedAfterRestore()
    {
        QByteArray state;
        {
            Window first;
            first.log->toggleView(false);
            state = first.workbench->saveLayout();
        }
        Window second;
        QVERIFY(second.workbench->restoreLayout(state));
        QVERIFY(second.log->isClosed());
        QVERIFY(!second.console->isClosed());
    }

    void resetBringsBackTheDefault()
    {
        Window w;
        w.moveConsoleRight();
        w.log->toggleView(false);
        w.workbench->resetLayout();
        QVERIFY(w.consoleTabbedWithLog());
        QVERIFY(!w.log->isClosed());
    }

    void namedLayoutsSurviveARestart()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ini = dir.filePath(QStringLiteral("layout.ini"));
        {
            Window first;
            first.workbench->saveNamedLayout(QStringLiteral("Configure"));
            first.moveConsoleRight();
            first.workbench->saveNamedLayout(QStringLiteral("Debug"));
            QSettings settings(ini, QSettings::IniFormat);
            first.workbench->save(settings);
        }
        Window second;
        QSettings settings(ini, QSettings::IniFormat);
        second.workbench->restore(settings);
        QCOMPARE(second.workbench->namedLayouts(),
                 (QStringList{QStringLiteral("Configure"), QStringLiteral("Debug")}));
        QVERIFY(!second.consoleTabbedWithLog()); // the current layout came back too
        QVERIFY(second.workbench->openNamedLayout(QStringLiteral("Configure")));
        QVERIFY(second.consoleTabbedWithLog());
        QVERIFY(second.workbench->openNamedLayout(QStringLiteral("Debug")));
        QVERIFY(!second.consoleTabbedWithLog());
        QVERIFY(!second.workbench->openNamedLayout(QStringLiteral("No such layout")));
    }

    void layoutFromAnotherVersionIsRefused()
    {
        Window first;
        first.moveConsoleRight();
        const QByteArray state =
            first.workbench->manager()->saveState(Workbench::kLayoutVersion + 1);
        Window second;
        QVERIFY(!second.workbench->restoreLayout(state));
        QVERIFY(second.consoleTabbedWithLog());
        QVERIFY(!second.workbench->restoreLayout(QByteArray()));
    }

    void eachStageKeepsItsLayout()
    {
        Window w;
        QSignalSpy changed(w.workbench, &Workbench::stageChanged);
        w.workbench->setStage(QStringLiteral("configure"));
        w.moveConsoleRight();
        // A stage entered for the first time keeps the layout on screen.
        w.workbench->setStage(QStringLiteral("code"));
        QVERIFY(!w.consoleTabbedWithLog());
        w.workbench->resetLayout();
        QVERIFY(w.consoleTabbedWithLog());
        w.workbench->setStage(QStringLiteral("configure"));
        QVERIFY(!w.consoleTabbedWithLog());
        w.workbench->setStage(QStringLiteral("code"));
        QVERIFY(w.consoleTabbedWithLog());
        w.workbench->setStage(QStringLiteral("code")); // no change, no signal
        QCOMPARE(changed.count(), 4);
        QCOMPARE(w.workbench->stage(), QStringLiteral("code"));
    }

    void stageLayoutsSurviveARestart()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString ini = dir.filePath(QStringLiteral("layout.ini"));
        {
            Window first;
            first.workbench->setStage(QStringLiteral("configure"));
            first.moveConsoleRight();
            first.workbench->setStage(QStringLiteral("code"));
            first.workbench->resetLayout();
            QSettings settings(ini, QSettings::IniFormat);
            first.workbench->save(settings);
        }
        Window second;
        QSettings settings(ini, QSettings::IniFormat);
        second.workbench->restore(settings);
        QCOMPARE(second.workbench->stage(), QStringLiteral("code"));
        QVERIFY(second.consoleTabbedWithLog());
        second.workbench->setStage(QStringLiteral("configure"));
        QVERIFY(!second.consoleTabbedWithLog());
    }
};

QTEST_MAIN(TestWorkbench)
#include "tst_workbench.moc"
