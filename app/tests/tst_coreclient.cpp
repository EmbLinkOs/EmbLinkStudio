// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    tst_coreclient.cpp
 * @brief   Qt Test suite for CoreClient, the desktop's link to the core.
 *
 * @details
 * Runs CoreClient against the real core (S0's acceptance test: initialize succeeds
 * and shutdown/exit ends with code 0) and against fake cores that misbehave
 * (app/tests/fake_core.py): a wrong protocol major version, a crash after
 * initialize, an exit before initialize, and a program that does not exist.
 *
 * @see     coreclient.h; fake_core.py
 * @author  Junior Deogracias
 * @date    2026-11-11 (created)
 */

#include "coreclient.h"

#include <QSignalSpy>
#include <QTest>

using emblinkstudio::CoreClient;

namespace
{

CoreClient::Launch fakeCore(const QString &behaviour)
{
    CoreClient::Launch launch;
    launch.program = QStringLiteral(EMBLINK_STUDIO_PYTHON);
    launch.arguments = {QStringLiteral(EMBLINK_STUDIO_FAKE_CORE), behaviour};
    return launch;
}

constexpr int kTimeout = 15000;

// True once the spy has caught a signal. A signal emitted inside start() counts: on Windows a
// program that cannot be found fails synchronously, before any wait() could see it.
bool arrived(QSignalSpy &spy)
{
    return !spy.isEmpty() || spy.wait(kTimeout);
}

} // namespace

class TestCoreClient : public QObject
{
    Q_OBJECT

  private slots:
    void realCoreInitializesAndStopsCleanly()
    {
        CoreClient core;
        QSignalSpy ready(&core, &CoreClient::ready);
        QSignalSpy failed(&core, &CoreClient::failed);
        QSignalSpy exited(&core, &CoreClient::exited);
        core.start(CoreClient::defaultLaunch());
        QVERIFY2(arrived(ready), qPrintable(failed.isEmpty() ? QStringLiteral("timeout")
                                                             : failed.first().first().toString()));
        QVERIFY(core.isReady());
        const QJsonObject info = ready.first().first().toJsonObject();
        QCOMPARE(info.value("server").toObject().value("name").toString(), "emblink-studio-core");
        QCOMPARE(info.value("protocol_version").toString(),
                 QString(emblinkstudio::kProtocolVersion));

        core.stop();
        QTRY_COMPARE_WITH_TIMEOUT(exited.size(), 1, kTimeout);
        QCOMPARE(exited.first().at(0).toInt(), 0); // shutdown then exit: code 0
        QCOMPARE(exited.first().at(1).toBool(), true);
        QVERIFY(failed.isEmpty());
    }

    void unknownMethodIsAnError()
    {
        CoreClient core;
        QSignalSpy ready(&core, &CoreClient::ready);
        core.start(CoreClient::defaultLaunch());
        QVERIFY(arrived(ready));
        int code = 0;
        core.request("no.such", QJsonValue(), {},
                     [&](int c, const QString &, const QJsonValue &) { code = c; });
        QTRY_COMPARE_WITH_TIMEOUT(code, -32601, kTimeout);
        core.stop();
    }

    void wrongMajorVersionIsRefused()
    {
        CoreClient core;
        QSignalSpy ready(&core, &CoreClient::ready);
        QSignalSpy failed(&core, &CoreClient::failed);
        QSignalSpy exited(&core, &CoreClient::exited);
        core.start(fakeCore("wrong-major"));
        QVERIFY(arrived(failed));
        QVERIFY(failed.first().first().toString().contains("9.0.0"));
        QVERIFY(ready.isEmpty());
        QVERIFY(!core.isReady());
        QTRY_COMPARE_WITH_TIMEOUT(exited.size(), 1, kTimeout); // and the core is stopped
    }

    void crashIsReportedAsUnexpected()
    {
        CoreClient core;
        QSignalSpy ready(&core, &CoreClient::ready);
        QSignalSpy exited(&core, &CoreClient::exited);
        QSignalSpy log(&core, &CoreClient::logLine);
        core.start(fakeCore("crash"));
        QVERIFY(arrived(ready));
        QTRY_COMPARE_WITH_TIMEOUT(exited.size(), 1, kTimeout);
        QCOMPARE(exited.first().at(0).toInt(), 3);
        QCOMPARE(exited.first().at(1).toBool(), false);
        QVERIFY(!core.isReady());
        bool sawStderr = false;
        for (const auto &line : log)
            sawStderr |= line.first().toString().contains("crashing on purpose");
        QVERIFY(sawStderr);
    }

    void exitBeforeInitializeFails()
    {
        CoreClient core;
        QSignalSpy failed(&core, &CoreClient::failed);
        core.start(fakeCore("silent-exit"));
        QVERIFY(arrived(failed));
        QVERIFY(failed.first().first().toString().contains("before it initialized"));
    }

    void missingProgramFails()
    {
        CoreClient core;
        QSignalSpy failed(&core, &CoreClient::failed);
        CoreClient::Launch launch;
        launch.program = QStringLiteral("emblink-studio-core-that-does-not-exist");
        core.start(launch);
        QVERIFY(arrived(failed));
        QVERIFY(failed.first().first().toString().contains("Could not start"));
    }
};

QTEST_GUILESS_MAIN(TestCoreClient)
#include "tst_coreclient.moc"
