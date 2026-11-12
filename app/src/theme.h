// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    theme.h
 * @brief   Theme: the design tokens, read from docs/design/tokens.json.
 *
 * @details
 * Every colour, size, radius, font and timing the frame uses comes from the tokens
 * (design brief section 4f); nothing else defines one. The tokens file is built into
 * the application as the resource :/design/tokens.json, so the file in docs/design is
 * the only source.
 *
 * A token is named by its path below its group: color("surface.ground"),
 * color("state.success.base"), number("radius.card"), font("title2"),
 * motion("move"). Colours are "#rrggbb" or "rgba(r,g,b,a)"; translucent colours are
 * layers drawn over the ground, and flattened() gives the opaque colour a layer makes
 * over a given surface, for the places Qt needs one (QPalette).
 *
 * install() sets the application's palette, font and style sheet; current() is the
 * installed theme. dockStyleSheet() styles Qt Advanced Docking System, which sets its
 * own style sheet on the dock manager and so must be given ours there.
 *
 * @see     docs/design/tokens.json, docs/design/brief.md section 4f
 * @author  Junior Deogracias
 * @date    2026-11-12 (created)
 */

#pragma once

#include <QColor>
#include <QEasingCurve>
#include <QFont>
#include <QJsonObject>
#include <QPalette>
#include <QString>
#include <QStringList>

class QApplication;

namespace emblinkstudio
{

class Theme
{
  public:
    // The tokens built into the application.
    static QString defaultPath()
    {
        return QStringLiteral(":/design/tokens.json");
    }

    // Reads a tokens file. On failure, returns false and says why in `error`.
    bool load(const QString &path, QString *error = nullptr);
    bool isLoaded() const
    {
        return !m_root.isEmpty();
    }

    // color("surface.ground"): a colour token below "color". Invalid if there is none.
    QColor color(const QString &path) const;
    // number("radius.card"), number("size.titlebar"), number("space.gutter").
    double number(const QString &path) const;
    // font("body"): a style of the type scale, in the interface family with its fallbacks
    // (the Chinese family among them). In a Chinese interface no text is set below 12 px,
    // and the caption loses its capitals and letter spacing (tokens: locale.chinese).
    QFont font(const QString &style) const;
    // motion("move"): its duration in milliseconds and its easing.
    int duration(const QString &motion) const;
    QEasingCurve easing(const QString &motion) const;

    QPalette palette() const;
    QString styleSheet() const;
    QString dockStyleSheet() const;

    // Every colour token, as a path, for checks and tests.
    QStringList colorPaths() const;

    // "#rrggbb", "#rrggbbaa" or "rgba(r,g,b,a)"; invalid otherwise.
    static QColor parseColor(const QString &value);
    // "linear", "ease-out" or "cubic-bezier(x1,y1,x2,y2)".
    static QEasingCurve parseEasing(const QString &value);
    // The opaque colour `layer` makes when drawn over `surface`.
    static QColor flattened(const QColor &layer, const QColor &surface);

    static void install(const Theme &theme, QApplication &app);
    static const Theme &current();

  private:
    QJsonObject m_root;
};

} // namespace emblinkstudio
