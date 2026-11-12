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

void Workbench::setStage(const QString &stage)
{
    if (stage == m_stage)
        return;
    if (!m_stage.isEmpty())
        m_stageLayouts.insert(m_stage, saveLayout());
    m_stage = stage;
    const auto saved = m_stageLayouts.constFind(stage);
    if (saved != m_stageLayouts.cend())
        restoreLayout(saved.value());
    emit stageChanged(stage);
}

void Workbench::save(QSettings &settings) const
{
    settings.setValue(QStringLiteral("current"), saveLayout());
    m_manager->savePerspectives(settings);
    settings.setValue(QStringLiteral("stage"), m_stage);
    settings.beginGroup(QStringLiteral("stages"));
    settings.remove(QString());
    for (auto it = m_stageLayouts.cbegin(); it != m_stageLayouts.cend(); ++it)
        if (it.key() != m_stage)
            settings.setValue(it.key(), it.value());
    if (!m_stage.isEmpty())
        settings.setValue(m_stage, saveLayout());
    settings.endGroup();
}

void Workbench::restore(QSettings &settings)
{
    m_manager->loadPerspectives(settings);
    restoreLayout(settings.value(QStringLiteral("current")).toByteArray());
    settings.beginGroup(QStringLiteral("stages"));
    m_stageLayouts.clear();
    for (const QString &key : settings.childKeys())
        m_stageLayouts.insert(key, settings.value(key).toByteArray());
    settings.endGroup();
    const QString stage = settings.value(QStringLiteral("stage")).toString();
    if (!stage.isEmpty() && stage != m_stage) {
        m_stage = stage;
        emit stageChanged(stage);
    }
    emit namedLayoutsChanged();
}

} // namespace emblinkstudio
