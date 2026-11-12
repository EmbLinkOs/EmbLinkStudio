// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    segmentedcontrol.h
 * @brief   SegmentedControl: a row of choices, one of them chosen, as macOS draws it.
 *
 * @details
 * The stage switch in the title bar (Configure, Code, Debug, Verify) and every
 * segmented choice in the frame. It is drawn from the theme's tokens: a well-coloured
 * track, the chosen segment raised with the knob shadow, and the chosen segment slides
 * to its new place with the "move" motion (design brief section 4f).
 *
 * The control takes keyboard focus; Left and Right (or Up and Down) choose the
 * neighbouring segment, Home and End the first and the last. Segment texts can be
 * changed in place, so the control is retranslated without being rebuilt.
 *
 * @see     theme.h; docs/design/tokens.json
 * @author  Junior Deogracias
 * @date    2026-11-12 (created)
 */

#pragma once

#include <QRectF>
#include <QStringList>
#include <QWidget>

class QVariantAnimation;

namespace emblinkstudio
{

class SegmentedControl : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentChanged)

  public:
    explicit SegmentedControl(QWidget *parent = nullptr);

    int addSegment(const QString &text);
    void setSegmentText(int index, const QString &text);
    QString segmentText(int index) const;
    int count() const
    {
        return int(m_texts.size());
    }

    int currentIndex() const
    {
        return m_current;
    }
    // Chooses `index` and emits currentChanged; out-of-range indexes are ignored.
    void setCurrentIndex(int index);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    // The rectangle segment `index` occupies, in widget coordinates.
    QRectF segmentRect(int index) const;

  signals:
    void currentChanged(int index);

  protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;

  private:
    int segmentAt(const QPointF &pos) const;
    double segmentWidth(int index) const;

    QStringList m_texts;
    int m_current = -1;
    int m_hover = -1;
    QRectF m_knob;
    QVariantAnimation *m_slide = nullptr;
};

} // namespace emblinkstudio
