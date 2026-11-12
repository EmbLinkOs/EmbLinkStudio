// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    clocktreeview.cpp
 * @brief   Implementation of ClockTreeView: the graph from board.clocks, and its drawing.
 *
 * @details
 * build() turns the generator's resolved clock tree into nodes in columns and edges
 * between them; the canvas lays the columns out at a fixed pitch and draws each node
 * as a card (design tokens: radius.group, fill.group), the main path in the accent
 * colour, each factor in a pill on its edge. The canvas is as large as the tree and
 * scrolls inside the view.
 *
 * @see     clocktreeview.h
 * @author  Junior Deogracias
 * @date    2026-11-13 (created)
 */

#include "clocktreeview.h"
#include "format.h"
#include "theme.h"

#include <QEvent>
#include <QHelpEvent>
#include <QJsonArray>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>

namespace emblinkstudio
{

namespace
{

constexpr double kNodeW = 178;
constexpr double kNodeH = 74;
constexpr double kColumnPitch = 244;
constexpr double kRowPitch = 92;
constexpr double kMargin = 24;

QString factor(const QJsonValue &v)
{
    return QString::number(v.toDouble(), 'g', 10);
}

} // namespace

class ClockTreeView::Canvas : public QWidget
{
  public:
    explicit Canvas(ClockTreeView *view) : QWidget(view), m_view(view)
    {
        setMouseTracking(true);
    }

    QRectF rectOf(const Node &n) const
    {
        return QRectF(kMargin + n.column * kColumnPitch, m_top + n.row * kRowPitch, kNodeW, kNodeH);
    }

    void relayout()
    {
        double minRow = 0;
        double maxRow = 0;
        int columns = 1;
        for (const Node &n : m_view->m_nodes) {
            minRow = qMin(minRow, n.row);
            maxRow = qMax(maxRow, n.row);
            columns = qMax(columns, n.column + 1);
        }
        m_top = kMargin + 10 - minRow * kRowPitch;
        const double notesH = 22.0 * m_view->m_notes.size();
        setMinimumSize(int(kMargin * 2 + (columns - 1) * kColumnPitch + kNodeW),
                       int(m_top + maxRow * kRowPitch + kNodeH + 28 + notesH + kMargin));
        update();
    }

  protected:
    bool event(QEvent *e) override
    {
        if (e->type() == QEvent::ToolTip) {
            const auto *help = static_cast<QHelpEvent *>(e);
            for (const Node &n : m_view->m_nodes)
                if (rectOf(n).contains(help->pos()) && !n.tooltip.isEmpty()) {
                    QToolTip::showText(help->globalPos(), n.tooltip, this);
                    return true;
                }
            QToolTip::hideText();
            return true;
        }
        return QWidget::event(e);
    }

    void paintEvent(QPaintEvent *) override
    {
        const Theme &theme = Theme::current();
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QColor accent = theme.color(QStringLiteral("accent.base"));
        const QColor line = theme.color(QStringLiteral("line.strong"));

        QHash<QString, Node> byId;
        for (const Node &n : m_view->m_nodes)
            byId.insert(n.id, n);

        // Edges first, under the cards: elbows from the right of one card to the left of the next.
        QFont labelFont = theme.font(QStringLiteral("codeMeta"));
        for (const Edge &e : m_view->m_edges) {
            if (!byId.contains(e.from) || !byId.contains(e.to))
                continue;
            const QRectF a = rectOf(byId.value(e.from));
            const QRectF b = rectOf(byId.value(e.to));
            QPainterPath path;
            QPointF labelAt;
            if (qFuzzyCompare(a.left(), b.left())) { // same column: straight down
                path.moveTo(a.center().x(), a.bottom());
                path.lineTo(b.center().x(), b.top());
                labelAt = QPointF(a.center().x() + 26, (a.bottom() + b.top()) / 2);
            } else {
                const double midX = (a.right() + b.left()) / 2;
                path.moveTo(a.right(), a.center().y());
                path.lineTo(midX, a.center().y());
                path.lineTo(midX, b.center().y());
                path.lineTo(b.left(), b.center().y());
                labelAt = QPointF(
                    qFuzzyCompare(a.center().y(), b.center().y()) ? midX : (midX + b.left()) / 2,
                    b.center().y());
            }
            const bool main = e.to == QStringLiteral("sys") || e.to == QStringLiteral("vco") ||
                              e.to == QStringLiteral("pll_in");
            p.setPen(QPen(main ? accent : line, 1.6));
            p.setBrush(Qt::NoBrush);
            p.drawPath(path);
            if (!e.label.isEmpty()) {
                p.setFont(labelFont);
                const QFontMetricsF fm(labelFont);
                const QRectF pill(labelAt.x() - fm.horizontalAdvance(e.label) / 2 - 7,
                                  labelAt.y() - 10, fm.horizontalAdvance(e.label) + 14, 20);
                QPainterPath bg;
                bg.addRoundedRect(pill, 10, 10);
                p.fillPath(bg, theme.color(QStringLiteral("surface.card")));
                p.setPen(QPen(main ? accent : line, 1));
                p.drawPath(bg);
                p.setPen(theme.color(QStringLiteral("accent.text")));
                p.drawText(pill, Qt::AlignCenter, e.label);
            }
        }

        // Cards.
        const QFont titleFont = theme.font(QStringLiteral("caption"));
        const QFont valueFont = theme.font(QStringLiteral("title2"));
        const QFont detailFont = theme.font(QStringLiteral("codeMeta"));
        const double radius = theme.number(QStringLiteral("radius.group"));
        for (const Node &n : m_view->m_nodes) {
            const QRectF r = rectOf(n);
            QPainterPath card;
            card.addRoundedRect(r, radius, radius);
            p.fillPath(card, Theme::flattened(theme.color(QStringLiteral("fill.group")),
                                              theme.color(QStringLiteral("surface.card"))));
            const bool sys = n.id == QStringLiteral("sys");
            p.setPen(
                QPen(sys ? accent : theme.color(QStringLiteral("line.control")), sys ? 1.5 : 1));
            p.drawPath(card);

            p.setFont(titleFont);
            p.setPen(theme.color(QStringLiteral("text.tertiary")));
            p.drawText(r.adjusted(12, 9, -12, 0), Qt::AlignLeft | Qt::AlignTop, n.title);
            p.setFont(valueFont);
            p.setPen(theme.color(QStringLiteral("text.strong")));
            p.drawText(r.adjusted(12, 24, -12, 0), Qt::AlignLeft | Qt::AlignTop, n.value);
            p.setFont(detailFont);
            p.setPen(theme.color(QStringLiteral("text.quiet")));
            p.drawText(
                r.adjusted(12, 0, -12, -10), Qt::AlignLeft | Qt::AlignBottom,
                QFontMetricsF(detailFont).elidedText(n.detail, Qt::ElideRight, r.width() - 24));

            if (n.load >= 0) { // how close the bus runs to its limit
                const QRectF track(r.left() + 12, r.bottom() - 5, r.width() - 24, 2.5);
                p.fillRect(track, theme.color(QStringLiteral("fill.control")));
                QRectF fill = track;
                fill.setWidth(track.width() * qMin(1.0, n.load));
                p.fillRect(fill, theme.color(n.load > 1.0 ? QStringLiteral("state.danger.base")
                                                          : QStringLiteral("state.success.base")));
            }
        }

        // The generator's notes, under the tree.
        double y = 0;
        for (const Node &n : m_view->m_nodes)
            y = qMax(y, rectOf(n).bottom());
        p.setFont(theme.font(QStringLiteral("small")));
        p.setPen(theme.color(QStringLiteral("text.secondary")));
        y += 28;
        for (const QString &note : std::as_const(m_view->m_notes)) {
            p.drawText(QPointF(kMargin, y), note);
            y += 22;
        }
    }

  private:
    ClockTreeView *m_view;
    double m_top = kMargin;
};

ClockTreeView::ClockTreeView(QWidget *parent) : QScrollArea(parent)
{
    setObjectName(QStringLiteral("viewContent"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFrameShape(QFrame::NoFrame);
    m_canvas = new Canvas(this);
    m_canvas->setObjectName(QStringLiteral("viewContent"));
    m_canvas->setAttribute(Qt::WA_StyledBackground, true);
    setWidget(m_canvas);
    setWidgetResizable(true);
}

ClockTreeView::Node ClockTreeView::node(const QString &id) const
{
    for (const Node &n : m_nodes)
        if (n.id == id)
            return n;
    return {};
}

void ClockTreeView::setClocks(const QJsonObject &clocks)
{
    m_clocks = clocks;
    build();
}

void ClockTreeView::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        build();
    QScrollArea::changeEvent(event);
}

void ClockTreeView::build()
{
    m_nodes.clear();
    m_edges.clear();
    m_notes.clear();
    const QJsonObject r = m_clocks.value(QStringLiteral("resolved")).toObject();
    const QJsonObject limits = m_clocks.value(QStringLiteral("limits")).toObject();
    if (r.isEmpty()) {
        m_notes.append(tr("Open a generated board to see its clock tree."));
        m_canvas->relayout();
        return;
    }

    // The source oscillator, with the SoC's words for it.
    const QString sourceName = r.value(QStringLiteral("source")).toString();
    QString sourceDoc;
    for (const QJsonValue &v : limits.value(QStringLiteral("sources")).toArray())
        if (v.toObject().value(QStringLiteral("name")).toString() == sourceName)
            sourceDoc = v.toObject().value(QStringLiteral("doc")).toString();
    Node source{QStringLiteral("source"), tr("Source"),
                formatHz(r.value(QStringLiteral("source_hz")).toDouble()),
                sourceName + (r.value(QStringLiteral("bypass")).toBool()
                                  ? QStringLiteral(" · ") + tr("bypass")
                                  : QString()),
                sourceDoc};
    m_nodes.append(source);

    int column = 1;
    QString last = QStringLiteral("source");
    QString sysLabel;
    const QJsonObject pll = r.value(QStringLiteral("pll")).toObject();
    if (!pll.isEmpty()) {
        const QString pllName = pll.value(QStringLiteral("name")).toString();
        m_nodes.append({QStringLiteral("pll_in"),
                        tr("PLL input"),
                        formatHz(pll.value(QStringLiteral("vco_in_hz")).toDouble()),
                        pllName,
                        {},
                        -1,
                        column,
                        0});
        m_edges.append({QStringLiteral("source"), QStringLiteral("pll_in"),
                        QStringLiteral("÷ ") + factor(pll.value(QStringLiteral("m")))});
        ++column;
        m_nodes.append({QStringLiteral("vco"),
                        tr("VCO"),
                        formatHz(pll.value(QStringLiteral("vco_hz")).toDouble()),
                        pllName,
                        {},
                        -1,
                        column,
                        0});
        m_edges.append({QStringLiteral("pll_in"), QStringLiteral("vco"),
                        QStringLiteral("× ") + factor(pll.value(QStringLiteral("n")))});
        if (pll.contains(QStringLiteral("q_hz"))) {
            m_nodes.append({QStringLiteral("pll_q"),
                            tr("PLL Q output"),
                            formatHz(pll.value(QStringLiteral("q_hz")).toDouble()),
                            pllName,
                            {},
                            -1,
                            column,
                            1});
            m_edges.append({QStringLiteral("vco"), QStringLiteral("pll_q"),
                            QStringLiteral("÷ ") + factor(pll.value(QStringLiteral("q")))});
        }
        sysLabel = pll.contains(QStringLiteral("postdiv1"))
                       ? QStringLiteral("÷ %1 ÷ %2")
                             .arg(factor(pll.value(QStringLiteral("postdiv1"))),
                                  factor(pll.value(QStringLiteral("postdiv2"))))
                       : QStringLiteral("÷ ") + factor(pll.value(QStringLiteral("p")));
        last = QStringLiteral("vco");
        ++column;
    }

    const double cpuHz = r.value(QStringLiteral("cpu_hz")).toDouble();
    const double hclkHz = r.value(QStringLiteral("hclk_hz")).toDouble();
    m_nodes.append({QStringLiteral("sys"),
                    tr("System clock"),
                    formatHz(cpuHz),
                    qFuzzyCompare(cpuHz, hclkHz) || hclkHz == 0
                        ? tr("CPU and AHB")
                        : tr("CPU · AHB %1").arg(formatHz(hclkHz)),
                    {},
                    -1,
                    column,
                    0});
    m_edges.append({last, QStringLiteral("sys"), sysLabel});
    ++column;

    // The buses, with the SoC's limit where it gives one.
    const QJsonObject busHz = r.value(QStringLiteral("bus_hz")).toObject();
    QList<QJsonObject> buses;
    for (const QJsonValue &v : limits.value(QStringLiteral("buses")).toArray())
        buses.append(v.toObject());
    if (buses.isEmpty())
        for (auto it = busHz.begin(); it != busHz.end(); ++it)
            buses.append(QJsonObject{{QStringLiteral("name"), it.key()}});
    const double first = -(buses.size() - 1) / 2.0;
    for (int i = 0; i < buses.size(); ++i) {
        const QJsonObject bus = buses.at(i);
        const QString name = bus.value(QStringLiteral("name")).toString();
        const double hz = busHz.value(name).toDouble();
        const double max = bus.value(QStringLiteral("max_hz")).toDouble();
        const QString from = bus.value(QStringLiteral("from")).toString();
        QString divider;
        if (!from.isEmpty() && from != QStringLiteral("hclk") &&
            r.contains(from + QStringLiteral("_div")))
            divider = QStringLiteral("÷ ") + factor(r.value(from + QStringLiteral("_div")));
        else if (r.value(QStringLiteral("ahb_div")).toDouble() > 1)
            divider = QStringLiteral("÷ ") + factor(r.value(QStringLiteral("ahb_div")));
        m_nodes.append({QStringLiteral("bus:") + name,
                        tr("Bus %1").arg(name.toUpper()),
                        formatHz(hz),
                        max > 0 ? tr("limit %1").arg(formatHz(max)) : QString(),
                        {},
                        max > 0 ? hz / max : -1,
                        column,
                        first + i});
        m_edges.append({QStringLiteral("sys"), QStringLiteral("bus:") + name, divider});
    }

    if (r.contains(QStringLiteral("flash_wait_states")))
        m_notes.append(
            tr("Flash wait states: %1.").arg(factor(r.value(QStringLiteral("flash_wait_states")))));
    const QJsonObject scale = r.value(QStringLiteral("voltage_scale")).toObject();
    if (!scale.isEmpty())
        m_notes.append(
            tr("Regulator: %1 (VOS %2), over-drive %3.")
                .arg(scale.value(QStringLiteral("name")).toString(),
                     factor(scale.value(QStringLiteral("vos"))),
                     scale.value(QStringLiteral("overdrive")).toBool() ? tr("on") : tr("off")));
    const QJsonObject timer = m_clocks.value(QStringLiteral("timer")).toObject();
    if (!timer.isEmpty())
        m_notes.append(tr("Kernel tick timer: %1 at %2.")
                           .arg(timer.value(QStringLiteral("source")).toString(),
                                formatHz(timer.value(QStringLiteral("hz")).toDouble())));
    m_canvas->relayout();
}

} // namespace emblinkstudio
