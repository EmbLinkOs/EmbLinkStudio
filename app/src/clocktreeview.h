// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    clocktreeview.h
 * @brief   ClockTreeView: the clock tree the generator resolved, drawn (spec section 7).
 *
 * @details
 * Draws the path from the source oscillator to the buses, left to right: the source,
 * then the PLL when there is one (its input after the M divider, the VCO after the N
 * multiplier, and its Q output), the system clock after the P divider (or the
 * RP2350's two post-dividers), and each bus with its divider and its limit, a bar
 * showing how close the bus runs to it. Boards without a PLL go from the source
 * straight to the system clock. Below, the generator's notes: flash wait states, the
 * regulator, the kernel's tick timer.
 *
 * Every frequency, factor and limit is the generator's (board.clocks: resolved,
 * limits, timer); nothing is computed here. nodes(), edges() and notes() give what is
 * drawn, for the golden tests of stage S2.
 *
 * @see     format.h; build_report.md's "Clock tree" in each board's generated/
 * @author  Junior Deogracias
 * @date    2026-11-13 (created)
 */

#pragma once

#include <QJsonObject>
#include <QList>
#include <QPointF>
#include <QScrollArea>
#include <QStringList>

namespace emblinkstudio
{

class ClockTreeView : public QScrollArea
{
    Q_OBJECT

  public:
    struct Node {
        QString id; // "source", "pll_in", "vco", "pll_q", "sys", "bus:<name>"
        QString title;
        QString value;
        QString detail;
        QString tooltip;
        double load = -1; // the frequency over its limit, when there is one
        int column = 0;
        double row = 0;
    };
    struct Edge {
        QString from;
        QString to;
        QString label; // "÷ 8", "× 360"
    };

    explicit ClockTreeView(QWidget *parent = nullptr);

    void setClocks(const QJsonObject &clocks);

    QList<Node> nodes() const
    {
        return m_nodes;
    }
    QList<Edge> edges() const
    {
        return m_edges;
    }
    QStringList notes() const
    {
        return m_notes;
    }
    // The node with `id`, or an empty one.
    Node node(const QString &id) const;

  protected:
    void changeEvent(QEvent *event) override;

  private:
    class Canvas;
    void build();

    Canvas *m_canvas = nullptr;
    QJsonObject m_clocks;
    QList<Node> m_nodes;
    QList<Edge> m_edges;
    QStringList m_notes;
};

} // namespace emblinkstudio
