// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
// The desktop's link to the core, against the real core (S0's acceptance: initialize succeeds)
// and against fake cores that misbehave.
#include "coreclient.h"

#include <QSignalSpy>
#include <QTest>

using embstudio::CoreClient;

namespace
{

CoreClient::Launch fakeCore(const QString &behaviour)
{
    CoreClient::Launch launch;
    launch.program = QStringLiteral(EMBSTUDIO_PYTHON);
    launch.arguments = {QStringLiteral(EMBSTUDIO_FAKE_CORE), behaviour};
    return launch;
}

constexpr int kTimeout = 15000;

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
        QVERIFY2(ready.wait(kTimeout),
                 qPrintable(failed.isEmpty() ? QStringLiteral("timeout")
                                             : failed.first().first().toString()));
        QVERIFY(core.isReady());
        const QJsonObject info = ready.first().first().toJsonObject();
        QCOMPARE(info.value("server").toObject().value("name").toString(), "embstudio-core");
        QCOMPARE(info.value("protocol_version").toString(), QString(embstudio::kProtocolVersion));

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
        QVERIFY(ready.wait(kTimeout));
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
        QVERIFY(failed.wait(kTimeout));
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
        QVERIFY(ready.wait(kTimeout));
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
        QVERIFY(failed.wait(kTimeout));
        QVERIFY(failed.first().first().toString().contains("before it initialized"));
    }

    void missingProgramFails()
    {
        CoreClient core;
        QSignalSpy failed(&core, &CoreClient::failed);
        CoreClient::Launch launch;
        launch.program = QStringLiteral("embstudio-core-that-does-not-exist");
        core.start(launch);
        QVERIFY(failed.wait(kTimeout));
        QVERIFY(failed.first().first().toString().contains("Could not start"));
    }
};

QTEST_GUILESS_MAIN(TestCoreClient)
#include "tst_coreclient.moc"
