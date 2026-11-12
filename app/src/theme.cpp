// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    theme.cpp
 * @brief   Implementation of Theme: reading the tokens, the palette and the style sheets.
 *
 * @details
 * Tokens are looked up by walking the JSON object; a token is either a bare value or
 * an object with a "value" field. Font sizes in the tokens are pixels at the design's
 * scale and become point sizes for the screen's logical resolution. The style sheets
 * are written from the tokens with colours as #AARRGGBB, which Qt's style sheets read
 * for every colour, translucent ones included.
 *
 * @see     theme.h
 * @author  Junior Deogracias
 * @date    2026-11-12 (created)
 */

#include "theme.h"

#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocale>
#include <QRegularExpression>
#include <QScreen>

namespace emblinkstudio
{

namespace
{

Theme &installed()
{
    static Theme theme;
    return theme;
}

// The value at `path` below `node`; an object with a "value" field stands for that value.
QJsonValue walk(const QJsonObject &root, const QString &group, const QString &path)
{
    QJsonValue node = root.value(group);
    for (const QString &part : path.split(QLatin1Char('.'), Qt::SkipEmptyParts)) {
        if (!node.isObject())
            return {};
        node = node.toObject().value(part);
    }
    if (node.isObject() && node.toObject().contains(QStringLiteral("value")))
        return node.toObject().value(QStringLiteral("value"));
    return node;
}

void collectColors(const QJsonObject &node, const QString &prefix, QStringList &out)
{
    for (auto it = node.begin(); it != node.end(); ++it) {
        const QString path = prefix.isEmpty() ? it.key() : prefix + QLatin1Char('.') + it.key();
        if (it->isString() && it.key() != QStringLiteral("use")) {
            if (!Theme::parseColor(it->toString()).isValid())
                continue;
            out.append(path.endsWith(QStringLiteral(".value")) ? path.chopped(6) : path);
        } else if (it->isObject()) {
            collectColors(it->toObject(), path, out);
        }
    }
}

bool chineseInterface()
{
    const QLocale locale;
    return locale.language() == QLocale::Chinese;
}

QString css(const QColor &color)
{
    return color.name(color.alpha() == 255 ? QColor::HexRgb : QColor::HexArgb);
}

} // namespace

bool Theme::load(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("cannot open %1").arg(path);
        return false;
    }
    QJsonParseError parse;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parse);
    if (!doc.isObject()) {
        if (error)
            *error = QStringLiteral("%1: %2").arg(path, parse.errorString());
        return false;
    }
    m_root = doc.object();
    return true;
}

QColor Theme::color(const QString &path) const
{
    return parseColor(walk(m_root, QStringLiteral("color"), path).toString());
}

double Theme::number(const QString &path) const
{
    const int dot = path.indexOf(QLatin1Char('.'));
    if (dot < 0)
        return walk(m_root, path, QString()).toDouble();
    return walk(m_root, path.left(dot), path.mid(dot + 1)).toDouble();
}

QFont Theme::font(const QString &style) const
{
    const QJsonObject type = m_root.value(QStringLiteral("type")).toObject();
    const QJsonObject families = type.value(QStringLiteral("family")).toObject();
    const QJsonObject scale =
        type.value(QStringLiteral("scale")).toObject().value(style).toObject();

    auto familyList = [&](const QString &key) {
        const QJsonObject f = families.value(key).toObject();
        QStringList out{f.value(QStringLiteral("value")).toString()};
        for (const QJsonValue &v : f.value(QStringLiteral("fallback")).toArray())
            out.append(v.toString());
        return out;
    };
    const bool mono = scale.value(QStringLiteral("family")).toString() == QStringLiteral("mono");
    QStringList wanted = familyList(mono ? QStringLiteral("mono") : QStringLiteral("ui"));
    wanted.append(familyList(QStringLiteral("cjk")));
    // Only the families installed here, so Qt never searches for a missing one; the system
    // font stays the last resort. Inter and JetBrains Mono are bundled in a later change.
    const QStringList installed = QFontDatabase::families();
    QStringList list;
    for (const QString &family : std::as_const(wanted))
        if (installed.contains(family))
            list.append(family);

    double px = scale.value(QStringLiteral("size")).toDouble(13);
    QString tracking = scale.value(QStringLiteral("tracking")).toString();
    bool upper = scale.value(QStringLiteral("case")).toString() == QStringLiteral("upper");
    if (chineseInterface()) {
        px = qMax(px, 12.0);
        if (upper) {
            upper = false;
            tracking.clear();
        }
    }

    QFont font;
    if (!list.isEmpty())
        font.setFamilies(list);
    else if (mono)
        font.setStyleHint(QFont::Monospace);
    // The tokens are pixels at 72 logical dots per inch, macOS's scale.
    const QScreen *screen = QGuiApplication::primaryScreen();
    const double dpi = screen ? screen->logicalDotsPerInch() : 72.0;
    font.setPointSizeF(px * 72.0 / dpi);
    font.setWeight(QFont::Weight(scale.value(QStringLiteral("weight")).toInt(400)));
    if (tracking.endsWith(QStringLiteral("em")))
        font.setLetterSpacing(QFont::PercentageSpacing,
                              100.0 + tracking.chopped(2).toDouble() * 100.0);
    if (upper)
        font.setCapitalization(QFont::AllUppercase);
    return font;
}

int Theme::duration(const QString &motion) const
{
    return m_root.value(QStringLiteral("motion"))
        .toObject()
        .value(motion)
        .toObject()
        .value(QStringLiteral("duration"))
        .toInt(200);
}

QEasingCurve Theme::easing(const QString &motion) const
{
    return parseEasing(m_root.value(QStringLiteral("motion"))
                           .toObject()
                           .value(motion)
                           .toObject()
                           .value(QStringLiteral("easing"))
                           .toString());
}

QStringList Theme::colorPaths() const
{
    QStringList out;
    collectColors(m_root.value(QStringLiteral("color")).toObject(), QString(), out);
    return out;
}

QColor Theme::parseColor(const QString &value)
{
    const QString v = value.trimmed();
    if (v.startsWith(QLatin1Char('#'))) {
        if (v.size() == 9) { // #rrggbbaa, as CSS writes it
            QColor c(v.left(7));
            c.setAlpha(v.mid(7).toInt(nullptr, 16));
            return c;
        }
        return QColor(v);
    }
    static const QRegularExpression rgba(
        QStringLiteral(R"(^rgba\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*([0-9.]+)\s*\)$)"));
    const QRegularExpressionMatch m = rgba.match(v);
    if (!m.hasMatch())
        return {};
    QColor c(m.captured(1).toInt(), m.captured(2).toInt(), m.captured(3).toInt());
    c.setAlphaF(qBound(0.0, m.captured(4).toDouble(), 1.0));
    return c;
}

QEasingCurve Theme::parseEasing(const QString &value)
{
    const QString v = value.trimmed();
    if (v == QStringLiteral("linear"))
        return QEasingCurve(QEasingCurve::Linear);
    if (v == QStringLiteral("ease-out"))
        return QEasingCurve(QEasingCurve::OutCubic);
    static const QRegularExpression bezier(QStringLiteral(
        R"(^cubic-bezier\(\s*([0-9.]+)\s*,\s*([0-9.-]+)\s*,\s*([0-9.]+)\s*,\s*([0-9.-]+)\s*\)$)"));
    const QRegularExpressionMatch m = bezier.match(v);
    if (!m.hasMatch())
        return QEasingCurve(QEasingCurve::OutCubic);
    QEasingCurve curve(QEasingCurve::BezierSpline);
    curve.addCubicBezierSegment(QPointF(m.captured(1).toDouble(), m.captured(2).toDouble()),
                                QPointF(m.captured(3).toDouble(), m.captured(4).toDouble()),
                                QPointF(1, 1));
    return curve;
}

QColor Theme::flattened(const QColor &layer, const QColor &surface)
{
    const double a = layer.alphaF();
    return QColor::fromRgbF(layer.redF() * a + surface.redF() * (1 - a),
                            layer.greenF() * a + surface.greenF() * (1 - a),
                            layer.blueF() * a + surface.blueF() * (1 - a));
}

QPalette Theme::palette() const
{
    const QColor ground = color(QStringLiteral("surface.ground"));
    const QColor card = color(QStringLiteral("surface.card"));
    auto over = [&](const char *path, const QColor &surface) {
        return flattened(color(QString::fromLatin1(path)), surface);
    };

    QPalette p;
    p.setColor(QPalette::Window, ground);
    p.setColor(QPalette::WindowText, color(QStringLiteral("text.primary")));
    p.setColor(QPalette::Base, over("fill.well", card));
    p.setColor(QPalette::AlternateBase, card);
    p.setColor(QPalette::Text, color(QStringLiteral("text.primary")));
    p.setColor(QPalette::PlaceholderText, color(QStringLiteral("text.quiet")));
    p.setColor(QPalette::Button, over("fill.control", card));
    p.setColor(QPalette::ButtonText, color(QStringLiteral("text.primary")));
    p.setColor(QPalette::BrightText, color(QStringLiteral("text.strong")));
    p.setColor(QPalette::Highlight, color(QStringLiteral("accent.base")));
    p.setColor(QPalette::HighlightedText, color(QStringLiteral("text.onAccent")));
    p.setColor(QPalette::Link, color(QStringLiteral("accent.link")));
    p.setColor(QPalette::LinkVisited, color(QStringLiteral("accent.link")));
    p.setColor(QPalette::ToolTipBase, over("surface.floating", ground));
    p.setColor(QPalette::ToolTipText, color(QStringLiteral("text.primary")));
    p.setColor(QPalette::Light, over("line.strong", card));
    p.setColor(QPalette::Midlight, over("line.control", card));
    p.setColor(QPalette::Mid, over("line.card", card));
    p.setColor(QPalette::Dark, ground);
    p.setColor(QPalette::Shadow, Qt::black);
    for (QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
        p.setColor(QPalette::Disabled, role, color(QStringLiteral("text.disabled")));
    p.setColor(QPalette::Disabled, QPalette::Highlight, over("fill.selected", card));
    return p;
}

QString Theme::styleSheet() const
{
    auto c = [&](const char *path) { return css(color(QString::fromLatin1(path))); };
    auto n = [&](const char *path) { return QString::number(number(QString::fromLatin1(path))); };
    const QColor card = color(QStringLiteral("surface.card"));
    auto over = [&](const char *path, const QColor &surface) {
        return css(flattened(color(QString::fromLatin1(path)), surface));
    };

    QString s;
    s += QStringLiteral("QMainWindow, QDialog { background: %1; }\n").arg(c("surface.ground"));
    s += QStringLiteral("QToolBar#stageBar { background: %1; border: 0; border-bottom: 1px solid "
                        "%2; spacing: 12px; padding: 7px 12px; }\n")
             .arg(c("surface.titlebar"), c("line.hairline"));
    s += QStringLiteral("QLabel#projectName { font-weight: 600; }\n");
    s += QStringLiteral(
             "QLabel#corePill { background: %1; border: 1px solid %2; border-radius: 14px; "
             "padding: 0 12px; min-height: 26px; color: %3; }\n")
             .arg(c("fill.hover"), c("line.control"), c("text.secondary"));
    s += QStringLiteral("QLabel[role=\"caption\"] { color: %1; }\n").arg(c("text.tertiary"));
    s += QStringLiteral("QLabel[role=\"secondary\"] { color: %1; }\n").arg(c("text.secondary"));
    s += QStringLiteral("QLabel[role=\"quiet\"] { color: %1; }\n").arg(c("text.quiet"));
    s += QStringLiteral("QFrame[role=\"card\"] { background: %1; border: 1px solid %2; "
                        "border-radius: %3px; }\n")
             .arg(c("surface.card"), c("line.card"), n("radius.card"));
    s += QStringLiteral("QFrame[role=\"group\"] { background: %1; border: 0; border-radius: %2px; "
                        "}\n")
             .arg(over("fill.group", card), n("radius.group"));
    s += QStringLiteral("QMenu { background: %1; border: 1px solid %2; border-radius: %3px; "
                        "padding: 6px; }\n"
                        "QMenu::item { padding: 5px 14px; border-radius: 6px; }\n"
                        "QMenu::item:selected { background: %4; }\n"
                        "QMenu::separator { height: 1px; background: %2; margin: 5px 8px; }\n")
             .arg(over("surface.floating", color(QStringLiteral("surface.ground"))),
                  c("line.strong"), n("radius.group"), over("accent.tint", card));
    s += QStringLiteral("QToolTip { background: %1; color: %2; border: 1px solid %3; "
                        "border-radius: 8px; padding: 5px 8px; }\n")
             .arg(over("surface.floating", color(QStringLiteral("surface.ground"))),
                  c("text.primary"), c("line.strong"));
    s += QStringLiteral("QPushButton { background: %1; border: 0; border-radius: %2px; "
                        "padding: 0 14px; min-height: %3px; }\n"
                        "QPushButton:hover { background: %4; }\n"
                        "QPushButton:disabled { color: %5; }\n"
                        "QPushButton[role=\"primary\"] { background: %6; color: %7; "
                        "font-weight: 700; }\n")
             .arg(c("fill.control"), n("radius.button"), n("size.button"), c("line.strong"),
                  c("text.disabled"), c("accent.base"), c("text.onAccent"));
    s += QStringLiteral("QLineEdit, QPlainTextEdit, QTextBrowser, QComboBox { background: %1; "
                        "border: 1px solid %2; border-radius: %3px; padding: 4px 8px; "
                        "selection-background-color: %4; }\n"
                        "QComboBox { min-height: %5px; padding: 0 10px; }\n"
                        "QComboBox::drop-down { border: 0; width: 20px; }\n"
                        "QComboBox QAbstractItemView { background: %6; border: 1px solid %7; "
                        "border-radius: 8px; padding: 4px; selection-background-color: %4; }\n")
             .arg(over("fill.well", card), c("line.control"), n("radius.control"),
                  over("accent.tint", card), n("size.control"),
                  over("surface.floating", color(QStringLiteral("surface.ground"))),
                  c("line.strong"));
    s += QStringLiteral("QListWidget#settingsSections { background: transparent; border: 0; "
                        "outline: 0; }\n"
                        "QListWidget#settingsSections::item { padding: 7px 10px; "
                        "border-radius: 8px; }\n"
                        "QListWidget#settingsSections::item:selected { background: %1; "
                        "color: %2; }\n")
             .arg(c("fill.selected"), c("text.strong"));
    s += QStringLiteral(
             "QScrollBar:vertical { width: 10px; background: transparent; margin: 2px; }\n"
             "QScrollBar:horizontal { height: 10px; background: transparent; "
             "margin: 2px; }\n"
             "QScrollBar::handle { background: %1; border-radius: 3px; min-height: 24px; "
             "min-width: 24px; }\n"
             "QScrollBar::handle:hover { background: %2; }\n"
             "QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }\n"
             "QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }\n")
             .arg(c("line.strong"), c("text.disabled"));
    return s;
}

QString Theme::dockStyleSheet() const
{
    auto c = [&](const char *path) { return css(color(QString::fromLatin1(path))); };
    auto n = [&](const char *path) { return QString::number(number(QString::fromLatin1(path))); };

    QString s;
    s += QStringLiteral("ads--CDockContainerWidget { background: %1; }\n").arg(c("surface.ground"));
    s += QStringLiteral("ads--CDockSplitter::handle { background: %1; }\n"
                        "ads--CDockSplitter::handle:horizontal { width: %2px; }\n"
                        "ads--CDockSplitter::handle:vertical { height: %2px; }\n")
             .arg(c("surface.ground"), n("space.gutter"));
    s += QStringLiteral("ads--CDockAreaWidget { background: %1; border: 1px solid %2; "
                        "border-radius: %3px; }\n")
             .arg(c("surface.card"), c("line.card"), n("radius.card"));
    s += QStringLiteral("ads--CDockAreaTitleBar { background: transparent; border: 0; "
                        "border-bottom: 1px solid %1; min-height: 34px; padding: 0 6px; }\n")
             .arg(c("line.hairline"));
    s += QStringLiteral("ads--CDockWidgetTab { background: transparent; border: 0; "
                        "border-radius: 6px; margin: 5px 1px; padding: 0 9px; }\n"
                        "ads--CDockWidgetTab:hover { background: %1; }\n"
                        "ads--CDockWidgetTab[activeTab=\"true\"] { background: %2; }\n"
                        "ads--CDockWidgetTab QLabel { color: %3; }\n"
                        "ads--CDockWidgetTab[activeTab=\"true\"] QLabel { color: %4; }\n")
             .arg(c("fill.hover"), c("fill.selected"), c("text.secondary"), c("text.strong"));
    s += QStringLiteral("ads--CDockWidget { background: %1; border: 0; }\n"
                        "QWidget#viewContent { background: %1; }\n"
                        "ads--CDockWidget > QScrollArea { background: transparent; border: 0; }\n")
             .arg(c("surface.card"));
    s += QStringLiteral("ads--CDockWidgetTab QAbstractButton { background: transparent; border: 0; "
                        "border-radius: 4px; min-height: 0; padding: 0; margin: 0 0 0 4px; }\n"
                        "ads--CDockWidgetTab QAbstractButton:hover { background: %1; }\n"
                        "ads--CTitleBarButton::menu-indicator { image: none; width: 0; }\n")
             .arg(c("line.strong"));
    s += QStringLiteral("ads--CTitleBarButton { background: transparent; border: 0; "
                        "border-radius: 6px; padding: 2px; }\n"
                        "ads--CTitleBarButton:hover { background: %1; }\n")
             .arg(c("fill.hover"));
    s += QStringLiteral("ads--CDockAreaWidget QPlainTextEdit, ads--CDockAreaWidget QTextBrowser "
                        "{ background: %1; border: 0; border-radius: 0; color: %2; }\n")
             .arg(c("surface.card"), c("text.primary"));
    return s;
}

void Theme::install(const Theme &theme, QApplication &app)
{
    installed() = theme;
    app.setPalette(theme.palette());
    app.setFont(theme.font(QStringLiteral("body")));
    app.setStyleSheet(theme.styleSheet());
}

const Theme &Theme::current()
{
    Theme &theme = installed();
    if (!theme.isLoaded())
        theme.load(defaultPath());
    return theme;
}

} // namespace emblinkstudio
