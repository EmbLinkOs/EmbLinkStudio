// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    segmentedcontrol.cpp
 * @brief   Implementation of SegmentedControl: layout, painting, the slide, input.
 *
 * @details
 * Segments are as wide as their text plus the token padding, laid out from the left
 * inside a 3 px track. The chosen segment's rectangle is animated between the old and
 * the new place; while it is not moving, it follows the layout, so a font or text
 * change never leaves it behind.
 *
 * @see     segmentedcontrol.h
 * @author  Junior Deogracias
 * @date    2026-11-12 (created)
 */

#include "segmentedcontrol.h"
#include "theme.h"

#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QVariantAnimation>
#include <QtMath>

namespace emblinkstudio
{

namespace
{

constexpr double kTrackPad = 3.0;    // the well around the segments
constexpr double kSegmentPad = 13.0; // either side of a segment's text
constexpr int kSegmentHeight = 26;   // in the title bar (tokens: size.control)

QFont segmentFont()
{
    QFont font = Theme::current().font(QStringLiteral("body"));
    font.setWeight(QFont::Medium);
    return font;
}

} // namespace

SegmentedControl::SegmentedControl(QWidget *parent) : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setFont(segmentFont());
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_slide = new QVariantAnimation(this);
    m_slide->setDuration(Theme::current().duration(QStringLiteral("move")));
    m_slide->setEasingCurve(Theme::current().easing(QStringLiteral("move")));
    connect(m_slide, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_knob = value.toRectF();
        update();
    });
}

int SegmentedControl::addSegment(const QString &text)
{
    m_texts.append(text);
    updateGeometry();
    update();
    if (m_current < 0)
        setCurrentIndex(0);
    return int(m_texts.size()) - 1;
}

void SegmentedControl::setSegmentText(int index, const QString &text)
{
    if (index < 0 || index >= count())
        return;
    m_texts[index] = text;
    updateGeometry();
    update();
}

QString SegmentedControl::segmentText(int index) const
{
    return index >= 0 && index < count() ? m_texts.at(index) : QString();
}

void SegmentedControl::setCurrentIndex(int index)
{
    if (index < 0 || index >= count() || index == m_current)
        return;
    const QRectF from = m_current >= 0 ? segmentRect(m_current) : segmentRect(index);
    m_current = index;
    m_slide->stop();
    if (isVisible()) {
        m_slide->setStartValue(m_knob.isNull() ? from : m_knob);
        m_slide->setEndValue(segmentRect(index));
        m_slide->start();
    } else {
        m_knob = segmentRect(index);
    }
    update();
    emit currentChanged(index);
}

double SegmentedControl::segmentWidth(int index) const
{
    return fontMetrics().horizontalAdvance(m_texts.at(index)) + 2 * kSegmentPad;
}

QRectF SegmentedControl::segmentRect(int index) const
{
    double x = kTrackPad;
    for (int i = 0; i < index; ++i)
        x += segmentWidth(i);
    return QRectF(x, kTrackPad, index < count() ? segmentWidth(index) : 0,
                  height() - 2 * kTrackPad);
}

QSize SegmentedControl::sizeHint() const
{
    double w = 2 * kTrackPad;
    for (int i = 0; i < count(); ++i)
        w += segmentWidth(i);
    return QSize(qCeil(w), kSegmentHeight + int(2 * kTrackPad));
}

QSize SegmentedControl::minimumSizeHint() const
{
    return sizeHint();
}

int SegmentedControl::segmentAt(const QPointF &pos) const
{
    for (int i = 0; i < count(); ++i)
        if (segmentRect(i).contains(pos))
            return i;
    return -1;
}

void SegmentedControl::paintEvent(QPaintEvent *)
{
    const Theme &theme = Theme::current();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const double trackRadius = theme.number(QStringLiteral("radius.track"));
    const double segRadius = theme.number(QStringLiteral("radius.control"));
    QPainterPath track;
    track.addRoundedRect(QRectF(rect()), trackRadius, trackRadius);
    p.fillPath(track, theme.color(QStringLiteral("fill.well")));

    if (m_current >= 0) {
        if (m_slide->state() != QAbstractAnimation::Running)
            m_knob = segmentRect(m_current);
        // The knob shadow (tokens: elevation.knob), then the chosen segment.
        QPainterPath shadow;
        shadow.addRoundedRect(m_knob.translated(0, 1), segRadius, segRadius);
        p.fillPath(shadow, QColor(0, 0, 0, 102));
        QPainterPath knob;
        knob.addRoundedRect(m_knob, segRadius, segRadius);
        p.fillPath(knob, theme.color(QStringLiteral("fill.segment")));
    }

    for (int i = 0; i < count(); ++i) {
        const QRectF r = segmentRect(i);
        QColor ink = theme.color(QStringLiteral("text.secondary"));
        if (i == m_current)
            ink = theme.color(QStringLiteral("text.strong"));
        else if (i == m_hover)
            ink = theme.color(QStringLiteral("text.primary"));
        p.setPen(ink);
        p.drawText(r, Qt::AlignCenter, m_texts.at(i));
    }

    if (hasFocus() && m_current >= 0) {
        QPen ring(theme.color(QStringLiteral("accent.base")), 2);
        p.setPen(ring);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(segmentRect(m_current).adjusted(1, 1, -1, -1), segRadius, segRadius);
    }
}

void SegmentedControl::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        setCurrentIndex(segmentAt(event->position()));
}

void SegmentedControl::mouseMoveEvent(QMouseEvent *event)
{
    const int hover = segmentAt(event->position());
    if (hover != m_hover) {
        m_hover = hover;
        update();
    }
}

void SegmentedControl::leaveEvent(QEvent *)
{
    m_hover = -1;
    update();
}

void SegmentedControl::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Left:
    case Qt::Key_Up:
        setCurrentIndex(m_current - 1);
        break;
    case Qt::Key_Right:
    case Qt::Key_Down:
        setCurrentIndex(m_current + 1);
        break;
    case Qt::Key_Home:
        setCurrentIndex(0);
        break;
    case Qt::Key_End:
        setCurrentIndex(count() - 1);
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

void SegmentedControl::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::FontChange)
        updateGeometry();
    else if (event->type() == QEvent::LanguageChange)
        setFont(segmentFont()); // a Chinese interface sets no text below 12 px
    QWidget::changeEvent(event);
}

} // namespace emblinkstudio
