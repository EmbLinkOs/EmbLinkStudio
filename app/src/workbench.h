// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    workbench.h
 * @brief   Workbench: the dockable layout of the main window.
 *
 * @details
 * Every view is a dock of Qt Advanced Docking System (spec section 11, question 1):
 * it can be moved, split, tabbed, floated onto another monitor, closed and reopened.
 * No view is fixed in place (spec section 7, "The layout is the person's").
 *
 * The Workbench places the views, remembers the default layout so it can be reset,
 * saves and restores the current layout, and keeps layouts saved by name. It knows
 * nothing about what the views show, so it is tested on its own.
 *
 * @see     Spec sections 7 and 11; mainwindow.h
 * @author  Junior Deogracias
 * @date    2026-11-11 (created)
 */

#pragma once

#include <DockManager.h>
#include <DockWidget.h>
#include <QList>
#include <QObject>
#include <QStringList>

class QMainWindow;
class QSettings;

namespace emblinkstudio
{

class Workbench : public QObject
{
    Q_OBJECT

  public:
    // Becomes the window's central widget.
    explicit Workbench(QMainWindow *window);

    // Adds a view in `area`, or as a tab beside `tabWith` when one is given. `id` names the
    // view in saved layouts, so it must stay the same from one release to the next.
    ads::CDockWidget *addView(const QString &id, const QString &title, QWidget *content,
                              ads::DockWidgetArea area, ads::CDockWidget *tabWith = nullptr);

    // Call once every view is placed: the layout at that moment is what Reset restores.
    void captureDefaultLayout();
    void resetLayout();

    QByteArray saveLayout() const;
    // False, and the layout unchanged, if the state is from another layout version.
    bool restoreLayout(const QByteArray &state);

    void saveNamedLayout(const QString &name);
    bool openNamedLayout(const QString &name);
    void removeNamedLayout(const QString &name);
    QStringList namedLayouts() const;

    // The current layout and every named layout, under `settings`' current group.
    void save(QSettings &settings) const;
    void restore(QSettings &settings);

    QList<ads::CDockWidget *> views() const
    {
        return m_views;
    }
    ads::CDockManager *manager() const
    {
        return m_manager;
    }

    // Bumped when a change to the views would make old saved layouts misleading.
    static constexpr int kLayoutVersion = 1;

  signals:
    void namedLayoutsChanged();

  private:
    ads::CDockManager *m_manager = nullptr;
    QList<ads::CDockWidget *> m_views;
    QByteArray m_default;
};

} // namespace emblinkstudio
