// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    workbench.cpp
 * @brief   Implementation of Workbench: placing views, layouts saved and restored.
 *
 * @details
 * Layouts are Qt Advanced Docking System's own state, tagged with kLayoutVersion.
 * Named layouts are its perspectives. In QSettings the current layout is the value
 * "current" and the named ones the array "Perspectives", both under the caller's group.
 *
 * @see     workbench.h
 * @author  Junior Deogracias
 * @date    2026-11-11 (created)
 */

#include "workbench.h"

#include <DockAreaWidget.h>
#include <QMainWindow>
#include <QSettings>

namespace emblinkstudio
{

namespace
{

// Config flags are global to the process, so they are set once, before the first manager.
void configureDocking()
{
    static bool done = false;
    if (done)
        return;
    done = true;
    using M = ads::CDockManager;
    // Splitters resize live; every area has its close, undock and tabs-menu buttons.
    M::setConfigFlags(M::DefaultOpaqueConfig);
    M::setConfigFlag(M::FocusHighlighting, true);
}

} // namespace

Workbench::Workbench(QMainWindow *window) : QObject(window)
{
    configureDocking();
    m_manager = new ads::CDockManager(window);
}

ads::CDockWidget *Workbench::addView(const QString &id, const QString &title, QWidget *content,
                                     ads::DockWidgetArea area, ads::CDockWidget *tabWith)
{
    auto *view = new ads::CDockWidget(m_manager, title);
    view->setObjectName(id);
    view->setWidget(content);
    // Every view can be moved, floated and closed: none is fixed in place.
    view->setFeatures(ads::CDockWidget::DefaultDockWidgetFeatures);
    if (tabWith && tabWith->dockAreaWidget())
        m_manager->addDockWidgetTabToArea(view, tabWith->dockAreaWidget());
    else
        m_manager->addDockWidget(area, view);
    m_views.append(view);
    return view;
}

void Workbench::captureDefaultLayout()
{
    m_default = saveLayout();
}

void Workbench::resetLayout()
{
    if (m_default.isEmpty())
        return;
    for (ads::CDockWidget *view : std::as_const(m_views))
        view->toggleView(true);
    restoreLayout(m_default);
}

QByteArray Workbench::saveLayout() const
{
    return m_manager->saveState(kLayoutVersion);
}

bool Workbench::restoreLayout(const QByteArray &state)
{
    return !state.isEmpty() && m_manager->restoreState(state, kLayoutVersion);
}

void Workbench::saveNamedLayout(const QString &name)
{
    m_manager->addPerspective(name);
    emit namedLayoutsChanged();
}

bool Workbench::openNamedLayout(const QString &name)
{
    if (!m_manager->perspectiveNames().contains(name))
        return false;
    m_manager->openPerspective(name);
    return true;
}

void Workbench::removeNamedLayout(const QString &name)
{
    m_manager->removePerspective(name);
    emit namedLayoutsChanged();
}

QStringList Workbench::namedLayouts() const
{
    return m_manager->perspectiveNames();
}

void Workbench::save(QSettings &settings) const
{
    settings.setValue(QStringLiteral("current"), saveLayout());
    m_manager->savePerspectives(settings);
}

void Workbench::restore(QSettings &settings)
{
    m_manager->loadPerspectives(settings);
    restoreLayout(settings.value(QStringLiteral("current")).toByteArray());
    emit namedLayoutsChanged();
}

} // namespace emblinkstudio
