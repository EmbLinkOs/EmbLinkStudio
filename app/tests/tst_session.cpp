// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    tst_session.cpp
 * @brief   Qt Test suite for Session and its views, against the real core (stage S1).
 *
 * @details
 * Runs EmbLink Studio Core as the application does, on a temporary EmbLinkRTOS
 * checkout whose generator is the core's test stand-in (core/tests/fake_hwgen.py), with
 * the core's settings in a temporary directory. Checks that a board opens; that the
 * generator waits for trust, and that trusting runs it; that generating brings the
 * model, which the Board view shows; that validation's errors reach the Problems view
 * with their place; that a board opens again after the core restarts; and that a
 * refusal reaches the person as a message.
 *
 * @see     session.h, boardview.h, problemsview.h
 * @author  Junior Deogracias
 * @date    2026-11-12 (created)
 */

#include "boardview.h"
#include "coreclient.h"
#include "problemsview.h"
#include "session.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QLocale>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeWidget>

using emblinkstudio::BoardView;
using emblinkstudio::CoreClient;
using emblinkstudio::ProblemsView;
using emblinkstudio::Session;

namespace
{

constexpr int kTimeoutMs = 20000;

const char kBoard[] = "schema_version: 0\n"
                      "kind: board\n"
                      "name: demo\n"
                      "vendor: acme\n"
                      "clocks:\n"
                      "  cpu_hz: 48000000\n";

bool write(const QString &path, const QByteArray &text)
{
    QDir().mkpath(QFileInfo(path).path());
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(text) == text.size();
}

// A checkout with the fake generator and one board, acme/demo, not generated yet.
struct Checkout {
    QTemporaryDir dir;
    QString root() const
    {
        return dir.filePath(QStringLiteral("rtos"));
    }
    QString board() const
    {
        return root() + QStringLiteral("/boards/acme/demo");
    }
    bool make(const QByteArray &boardYaml = kBoard)
    {
        QFile fake(QStringLiteral(EMBLINK_STUDIO_FAKE_HWGEN));
        return dir.isValid() && fake.open(QIODevice::ReadOnly) &&
               write(root() + QStringLiteral("/hw/gen/emb-hwgen.py"), fake.readAll()) &&
               write(root() + QStringLiteral("/hw/gen/emb_hwgen/__init__.py"),
                     "VERSION = \"0.1.0\"\nSCHEMA_VERSION = 0\n") &&
               write(board() + QStringLiteral("/board.yaml"), boardYaml);
    }
};

CoreClient::Launch launch(const QTemporaryDir &config)
{
    CoreClient::Launch l = CoreClient::defaultLaunch();
    l.environment.insert(QStringLiteral("EMBLINK_STUDIO_CONFIG_DIR"), config.path());
    return l;
}

bool startCore(CoreClient &core, const QTemporaryDir &config)
{
    QSignalSpy ready(&core, &CoreClient::ready);
    core.start(launch(config));
    return ready.wait(kTimeoutMs);
}

} // namespace

class TestSession : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase()
    {
        QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    }

    void formatting()
    {
        QCOMPARE(emblinkstudio::formatHz(180000000), QStringLiteral("180 MHz"));
        QCOMPARE(emblinkstudio::formatHz(32768), QStringLiteral("32.768 kHz"));
        QCOMPARE(emblinkstudio::formatBytes(524288), QStringLiteral("512 KB"));
        QCOMPARE(emblinkstudio::formatBytes(16384), QStringLiteral("16 KB"));
    }

    void generatingWaitsForTrustThenShowsTheModel()
    {
        Checkout checkout;
        QVERIFY(checkout.make());
        QTemporaryDir config;
        CoreClient core;
        QVERIFY(startCore(core, config));
        Session session(&core);

        QSignalSpy opened(&session, &Session::opened);
        session.open(checkout.board());
        QVERIFY(opened.wait(kTimeoutMs));
        QCOMPARE(session.status().value(QStringLiteral("id")).toString(),
                 QStringLiteral("acme/demo"));
        QVERIFY(!session.status().value(QStringLiteral("generated")).toBool());
        QVERIFY(!session.status().value(QStringLiteral("trusted")).toBool());
        QVERIFY(session.describe().isEmpty());

        QSignalSpy trust(&session, &Session::trustNeeded);
        session.generate();
        QVERIFY(trust.wait(kTimeoutMs));
        QCOMPARE(trust.first().at(1).value<Session::Action>(), Session::Action::Generate);
        QVERIFY(trust.first().at(0).toString().endsWith(QStringLiteral("rtos")));

        QSignalSpy model(&session, &Session::modelChanged);
        session.trustAndRun(Session::Action::Generate);
        QVERIFY(model.wait(kTimeoutMs));
        QVERIFY(session.status().value(QStringLiteral("trusted")).toBool());
        QCOMPARE(session.describe()
                     .value(QStringLiteral("board"))
                     .toObject()
                     .value(QStringLiteral("name"))
                     .toString(),
                 QStringLiteral("demo"));
        QCOMPARE(session.clocks()
                     .value(QStringLiteral("resolved"))
                     .toObject()
                     .value(QStringLiteral("cpu_hz"))
                     .toDouble(),
                 48000000.0);
        QVERIFY(session.hasVerdict());
        QVERIFY(session.isValid());

        BoardView view;
        view.setModel(session.status(), session.describe(), session.clocks(), session.memory(),
                      session.pins());
        QVERIFY(view.groupTitles().contains(QStringLiteral("Chip")));
        QCOMPARE(view.value(QStringLiteral("Chip"), QStringLiteral("Part")),
                 QStringLiteral("ACME1"));
        QCOMPARE(view.value(QStringLiteral("Clocks"), QStringLiteral("CPU")),
                 QStringLiteral("48 MHz"));
        QCOMPARE(view.value(QStringLiteral("Pins in use"), QStringLiteral("PA0")),
                 QStringLiteral("uart0_tx · pins.uart0.tx"));
        core.stop();
    }

    void errorsReachTheProblemsViewWithTheirPlace()
    {
        Checkout checkout;
        QVERIFY(checkout.make(QByteArray(kBoard) +
                              "pins: {}  # error: pins.uart0.tx: PA9 cannot carry uart0_tx\n"));
        QTemporaryDir config;
        CoreClient core;
        QVERIFY(startCore(core, config));
        Session session(&core);
        QSignalSpy opened(&session, &Session::opened);
        session.open(checkout.board());
        QVERIFY(opened.wait(kTimeoutMs));

        QSignalSpy diagnostics(&session, &Session::diagnosticsChanged);
        diagnostics.clear();
        session.trustAndRun(Session::Action::Validate);
        QVERIFY(diagnostics.wait(kTimeoutMs));
        QVERIFY(session.hasVerdict());
        QVERIFY(!session.isValid());
        QCOMPARE(session.diagnostics().size(), 1);
        const QJsonObject d = session.diagnostics().first().toObject();
        QCOMPARE(d.value(QStringLiteral("line")).toInt(), 7);
        QCOMPARE(d.value(QStringLiteral("path")).toString(), QStringLiteral("pins.uart0.tx"));

        ProblemsView view;
        QSignalSpy open(&view, &ProblemsView::openRequested);
        view.setDiagnostics(session.diagnostics(), true, session.hasVerdict(), session.isValid());
        QCOMPARE(view.rowCount(), 1);
        QVERIFY2(view.summary().startsWith(QStringLiteral("1 error")), qPrintable(view.summary()));
        QCOMPARE(view.list()->topLevelItem(0)->text(1),
                 QStringLiteral("board.yaml:7 · pins.uart0.tx"));
        emit view.list()->itemActivated(view.list()->topLevelItem(0), 0);
        QCOMPARE(open.count(), 1);
        QVERIFY(open.first().at(0).toString().endsWith(QStringLiteral("board.yaml")));
        QCOMPARE(open.first().at(1).toInt(), 7);
        core.stop();
    }

    void theBoardOpensAgainAfterTheCoreRestarts()
    {
        Checkout checkout;
        QVERIFY(checkout.make());
        QTemporaryDir config;
        CoreClient core;
        QVERIFY(startCore(core, config));
        Session session(&core);
        QSignalSpy opened(&session, &Session::opened);
        session.open(checkout.board());
        QVERIFY(opened.wait(kTimeoutMs));

        core.stop();
        QVERIFY(startCore(core, config));
        session.reopen();
        QVERIFY(opened.wait(kTimeoutMs));
        QCOMPARE(opened.count(), 2);
        QCOMPARE(session.status().value(QStringLiteral("id")).toString(),
                 QStringLiteral("acme/demo"));
        core.stop();
    }

    void aRefusalReachesThePerson()
    {
        QTemporaryDir config;
        QTemporaryDir nowhere;
        CoreClient core;
        QVERIFY(startCore(core, config));
        Session session(&core);
        QSignalSpy failed(&session, &Session::failed);
        session.open(nowhere.path());
        QVERIFY(failed.wait(kTimeoutMs));
        QVERIFY2(failed.first().first().toString().contains(QStringLiteral("board.yaml")),
                 qPrintable(failed.first().first().toString()));
        QVERIFY(!session.isOpen());

        QSignalSpy closed(&session, &Session::closed);
        session.close();
        QCOMPARE(closed.count(), 0); // nothing was open
        core.stop();
    }
};

QTEST_MAIN(TestSession)
#include "tst_session.moc"
