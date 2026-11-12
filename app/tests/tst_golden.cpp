// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    tst_golden.cpp
 * @brief   Stage S2's golden test: every value the views show is the generator's.
 *
 * @details
 * "For both boards, a golden test finds every value in the views equal to hw.json and
 * to the generator's board.md and build_report.md" (spec section 8, S2).
 *
 * For the RTOS's nucleo_f446re and pico2_w, the test opens the board through the real
 * core, as the application does, renders the Board, Clock tree, Memory and Devices
 * views, and compares what each shows with the board's hw.json and with the tables
 * and lines of its board.md and build_report.md, which the generator writes from the
 * same model. EMBLINK_STUDIO_TEST_RTOS names the checkout; CI checks the RTOS out at a
 * pinned commit, and the test is skipped where it is not set. Runs in English.
 *
 * @see     boardview.h, clocktreeview.h, memoryview.h, devicesview.h
 * @author  Junior Deogracias
 * @date    2026-11-13 (created)
 */

#include "boardview.h"
#include "clocktreeview.h"
#include "coreclient.h"
#include "devicesview.h"
#include "format.h"
#include "memoryview.h"
#include "session.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocale>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace emblinkstudio;

namespace
{

constexpr int kTimeoutMs = 20000;

QString rtos()
{
    return qEnvironmentVariable("EMBLINK_STUDIO_TEST_RTOS");
}

QString read(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
}

// The rows of the Markdown table under `heading`, without its header, backticks removed.
QList<QStringList> table(const QString &markdown, const QString &heading)
{
    QList<QStringList> rows;
    const QStringList lines = markdown.split(QLatin1Char('\n'));
    int i = int(lines.indexOf(heading));
    if (i < 0)
        return rows;
    while (++i < lines.size() && !lines.at(i).startsWith(QLatin1Char('|')))
        ;
    i += 2; // the header and its separator
    for (; i < lines.size() && lines.at(i).startsWith(QLatin1Char('|')); ++i) {
        QStringList cells = lines.at(i).split(QLatin1Char('|'));
        cells.removeFirst();
        cells.removeLast();
        for (QString &c : cells)
            c = c.trimmed().remove(QLatin1Char('`'));
        rows.append(cells);
    }
    return rows;
}

// "512K", "16M", "4032K" as board.md writes sizes.
double bytes(const QString &size)
{
    if (size.endsWith(QLatin1Char('K')))
        return size.chopped(1).toDouble() * 1024;
    if (size.endsWith(QLatin1Char('M')))
        return size.chopped(1).toDouble() * 1024 * 1024;
    return size.toDouble();
}

QJsonObject at(const QJsonObject &o, const QString &path)
{
    QJsonObject node = o;
    for (const QString &part : path.split(QLatin1Char('.')))
        node = node.value(part).toObject();
    return node;
}

} // namespace

class TestGolden : public QObject
{
    Q_OBJECT

  private:
    QTemporaryDir m_config;
    CoreClient *m_core = nullptr;
    Session *m_session = nullptr;
    QJsonObject m_hw;
    QString m_boardMd;
    QString m_report;

    void open(const QString &board)
    {
        const QString dir = rtos() + QStringLiteral("/boards/") + board;
        m_hw = QJsonDocument::fromJson(read(dir + QStringLiteral("/generated/hw.json")).toUtf8())
                   .object();
        m_boardMd = read(dir + QStringLiteral("/generated/board.md"));
        m_report = read(dir + QStringLiteral("/generated/build_report.md"));
        QVERIFY(!m_hw.isEmpty() && !m_boardMd.isEmpty() && !m_report.isEmpty());
        QSignalSpy model(m_session, &Session::modelChanged);
        m_session->open(dir);
        QVERIFY(model.wait(kTimeoutMs));
        QVERIFY(!m_session->describe().isEmpty());
    }

  private slots:
    void initTestCase()
    {
        if (rtos().isEmpty())
            QSKIP("EMBLINK_STUDIO_TEST_RTOS is not set");
        QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
        QVERIFY(m_config.isValid());
        m_core = new CoreClient(this);
        CoreClient::Launch launch = CoreClient::defaultLaunch();
        launch.environment.insert(QStringLiteral("EMBLINK_STUDIO_CONFIG_DIR"), m_config.path());
        QSignalSpy ready(m_core, &CoreClient::ready);
        m_core->start(launch);
        QVERIFY(ready.wait(kTimeoutMs));
        m_session = new Session(m_core, this);
    }

    void cleanupTestCase()
    {
        if (m_core)
            m_core->stop();
    }

    void views_data()
    {
        QTest::addColumn<QString>("board");
        QTest::newRow("nucleo_f446re") << QStringLiteral("st/nucleo_f446re");
        QTest::newRow("pico2_w") << QStringLiteral("rpi/pico2_w");
    }

    void views()
    {
        QFETCH(QString, board);
        open(board);
        const QJsonObject clock = at(m_hw, QStringLiteral("derived.clock"));

        // -- Board: board.md's first table and its pins -----------------------------------
        BoardView boardView;
        boardView.setModel(m_session->status(), m_session->describe(), m_session->clocks(),
                           m_session->memory(), m_session->pins());
        QHash<QString, QString> fact; // board.md's first table: | Field | Value |
        for (const QString &line : m_boardMd.split(QLatin1Char('\n')))
            if (line.startsWith(QStringLiteral("| ")) && line.count(QLatin1Char('|')) == 3) {
                const QStringList cells = line.split(QLatin1Char('|'));
                fact.insert(cells.at(1).trimmed(), cells.at(2).trimmed().remove(QLatin1Char('`')));
            }
        const QString soc = fact.value(QStringLiteral("SoC")); // "STM32F446RE (lqfp64)"
        QCOMPARE(boardView.value(QStringLiteral("Chip"), QStringLiteral("Part")),
                 soc.section(QLatin1Char(' '), 0, 0));
        QCOMPARE(boardView.value(QStringLiteral("Chip"), QStringLiteral("Package")),
                 soc.section(QLatin1Char('('), 1).chopped(1).toUpper());
        QVERIFY(
            fact.value(QStringLiteral("CPU clock"))
                .startsWith(boardView.value(QStringLiteral("Chip"), QStringLiteral("CPU clock"))));
        QCOMPARE(boardView.value(QStringLiteral("Board"), QStringLiteral("Console")),
                 fact.value(QStringLiteral("Console")));
        for (const QStringList &pin : table(m_boardMd, QStringLiteral("## Pins"))) {
            // | PA2 | pins.usart2.tx | af7 (usart2_tx) |, | PA5 | leds.user | output |
            QCOMPARE(boardView.value(QStringLiteral("Pins in use"), pin.at(0)),
                     pin.at(2) + QStringLiteral(" · ") + pin.at(1));
        }

        // -- Clock tree: build_report.md's clock line, bus table and notes ----------------
        ClockTreeView clocks;
        clocks.setClocks(m_session->clocks());
        QCOMPARE(clocks.node(QStringLiteral("sys")).value,
                 formatHz(clock.value(QStringLiteral("cpu_hz")).toDouble()));
        QCOMPARE(clocks.node(QStringLiteral("source")).value,
                 formatHz(clock.value(QStringLiteral("source_hz")).toDouble()));
        // "hse 8 MHz / M 8 = 1 MHz VCO input, x N 360 = 360 MHz, / P 2 = 180 MHz, Q 7 (51.4286
        // MHz)"
        static const QRegularExpression line(QStringLiteral(
            R"((\w+) (\S+ \w?Hz) / M (\d+) = (\S+ \w?Hz) VCO input, x N (\d+) = (\S+ \w?Hz), / P (\d+) = (\S+ \w?Hz)(?:, Q (\d+) \((\S+ \w?Hz)\))?)"));
        const QRegularExpressionMatch m = line.match(m_report);
        QVERIFY2(m.hasMatch(), "build_report.md has no clock line");
        QCOMPARE(clocks.node(QStringLiteral("source")).detail.section(QLatin1Char(' '), 0, 0),
                 m.captured(1));
        QCOMPARE(clocks.node(QStringLiteral("source")).value, m.captured(2));
        QCOMPARE(clocks.node(QStringLiteral("pll_in")).value, m.captured(4));
        QCOMPARE(clocks.node(QStringLiteral("vco")).value, m.captured(6));
        QCOMPARE(clocks.node(QStringLiteral("sys")).value, m.captured(8));
        QHash<QString, QString> label;
        for (const ClockTreeView::Edge &e : clocks.edges())
            label.insert(e.to, e.label);
        QCOMPARE(label.value(QStringLiteral("pll_in")), QStringLiteral("÷ ") + m.captured(3));
        QCOMPARE(label.value(QStringLiteral("vco")), QStringLiteral("× ") + m.captured(5));
        // P, or the RP2350's two post-dividers whose product is P.
        static const QRegularExpression divs(QStringLiteral(R"(\d+)"));
        int p = 1;
        for (const QRegularExpressionMatch &d :
             divs.globalMatch(label.value(QStringLiteral("sys"))))
            p *= d.captured(0).toInt();
        QCOMPARE(p, m.captured(7).toInt());
        if (!m.captured(9).isEmpty()) {
            QCOMPARE(label.value(QStringLiteral("pll_q")), QStringLiteral("÷ ") + m.captured(9));
            QCOMPARE(clocks.node(QStringLiteral("pll_q")).value, m.captured(10));
        }
        const QList<QStringList> buses = table(m_report, QStringLiteral("## Clock tree"));
        QVERIFY(!buses.isEmpty());
        for (const QStringList &bus : buses) {
            const ClockTreeView::Node n = clocks.node(QStringLiteral("bus:") + bus.at(0));
            QCOMPARE(n.value, bus.at(1));
            QCOMPARE(n.detail, QStringLiteral("limit ") + bus.at(2));
        }
        for (const QString &note :
             {QStringLiteral("Flash wait states"), QStringLiteral("Regulator")}) {
            const int i = int(m_report.indexOf(note + QStringLiteral(":")));
            if (i < 0)
                continue;
            const QString wanted = m_report.mid(i, m_report.indexOf(QLatin1Char('\n'), i) - i);
            QVERIFY2(clocks.notes().contains(wanted), qPrintable(wanted));
        }

        // -- Memory: board.md's memory map and partitions, build_report.md's image layout --
        MemoryView memory;
        memory.setMemory(m_session->memory());
        const QJsonArray regions =
            at(m_hw, QStringLiteral("soc.memory")).value(QStringLiteral("regions")).toArray();
        const QList<QStringList> map = table(m_boardMd, QStringLiteral("## Memory map"));
        const QList<QStringList> shown = memory.rows(MemoryView::Group::Regions);
        QCOMPARE(shown.size(), map.size());
        QCOMPARE(shown.size(), regions.size());
        for (int i = 0; i < map.size(); ++i) {
            QCOMPARE(shown.at(i).at(0), map.at(i).at(0));
            QCOMPARE(shown.at(i).at(1), map.at(i).at(1));
            QCOMPARE(bytes(map.at(i).at(2)),
                     regions.at(i).toObject().value(QStringLiteral("size")).toDouble());
            QCOMPARE(shown.at(i).at(2), formatBytes(bytes(map.at(i).at(2))));
            QCOMPARE(shown.at(i).at(3), map.at(i).at(3));
        }
        const QList<QStringList> parts = table(m_boardMd, QStringLiteral("## Flash partitions"));
        const QList<QStringList> shownParts = memory.rows(MemoryView::Group::Partitions);
        QCOMPARE(shownParts.size(), parts.size());
        for (int i = 0; i < parts.size(); ++i) {
            QCOMPARE(shownParts.at(i).at(0).section(QLatin1Char(' '), 0, 0), parts.at(i).at(0));
            QCOMPARE(shownParts.at(i).at(1), parts.at(i).at(1));
            QCOMPARE(shownParts.at(i).at(2), formatBytes(bytes(parts.at(i).at(2))));
            QCOMPARE(shownParts.at(i).at(3), parts.at(i).at(3));
        }
        const QList<QStringList> image = table(m_report, QStringLiteral("## Image layout"));
        const QList<QStringList> shownImage = memory.rows(MemoryView::Group::Image);
        QCOMPARE(shownImage.size(), image.size());
        for (int i = 0; i < image.size(); ++i) {
            QCOMPARE(shownImage.at(i).at(0), image.at(i).at(0));
            QCOMPARE(shownImage.at(i).at(1), image.at(i).at(1));
            QCOMPARE(shownImage.at(i).at(2), formatBytes(bytes(image.at(i).at(2))));
        }

        // -- Devices: board.md's devices, in initialization order ---------------------------
        DevicesView devices;
        devices.setDevices(m_session->describe().value(QStringLiteral("devices")).toArray());
        const QList<QStringList> listed = table(m_boardMd, QStringLiteral("## Devices"));
        const QList<QStringList> rows = devices.rows();
        QCOMPARE(rows.size(), listed.size());
        for (int i = 0; i < listed.size(); ++i) {
            // board.md: Device | Compatible | Class | Level | Depends on
            QCOMPARE(rows.at(i).at(0), listed.at(i).at(0));
            QCOMPARE(rows.at(i).at(1), listed.at(i).at(2));
            QCOMPARE(rows.at(i).at(2), listed.at(i).at(1));
            QCOMPARE(rows.at(i).at(3), listed.at(i).at(3));
            QCOMPARE(QString(rows.at(i).at(4)).replace(QStringLiteral("—"), QStringLiteral("-")),
                     listed.at(i).at(4));
        }
    }
};

QTEST_MAIN(TestGolden)
#include "tst_golden.moc"
