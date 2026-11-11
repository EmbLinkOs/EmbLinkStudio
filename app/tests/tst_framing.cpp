// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    tst_framing.cpp
 * @brief   Qt Test suite for Framer, the Content-Length framing.
 *
 * @details
 * Covers a round trip with non-ASCII text, Content-Length counting bytes rather
 * than characters, delivery one byte at a time, extra and lower-case headers, a
 * missing Content-Length (fatal) and a body that is not JSON (skipped).
 *
 * @see     framing.h
 * @author  Junior Deogracias
 * @date    2026-11-11 (created)
 */

#include "framing.h"

#include <QTest>

using emblinkstudio::Framer;

class TestFraming : public QObject
{
    Q_OBJECT

  private slots:
    void roundTrip()
    {
        const QJsonObject message{{"jsonrpc", "2.0"}, {"method", "log"}, {"text", "µs, °C"}};
        Framer framer;
        framer.feed(Framer::encode(message));
        const auto out = framer.next();
        QVERIFY(out.has_value());
        QCOMPARE(*out, message);
        QVERIFY(!framer.next().has_value());
    }

    void lengthCountsBytes()
    {
        const QByteArray frame = Framer::encode(QJsonObject{{"t", "é"}});
        const qsizetype split = frame.indexOf("\r\n\r\n");
        QCOMPARE(frame.left(split),
                 "Content-Length: " + QByteArray::number(frame.size() - split - 4));
    }

    void byteByByte()
    {
        const QByteArray frames =
            Framer::encode(QJsonObject{{"n", 1}}) + Framer::encode(QJsonObject{{"n", 2}});
        Framer framer;
        QList<int> seen;
        for (char c : frames) {
            framer.feed(QByteArray(1, c));
            while (auto m = framer.next())
                seen.append(m->value("n").toInt());
        }
        QCOMPARE(seen, (QList<int>{1, 2}));
        QVERIFY(!framer.failed());
    }

    void extraHeaders()
    {
        Framer framer;
        framer.feed("Content-Type: application/json\r\ncontent-length: 7\r\n\r\n{\"a\":1}");
        const auto out = framer.next();
        QVERIFY(out.has_value());
        QCOMPARE(out->value("a").toInt(), 1);
    }

    void missingLengthFails()
    {
        Framer framer;
        framer.feed("X-Other: 1\r\n\r\n{}");
        QVERIFY(!framer.next().has_value());
        QVERIFY(framer.failed());
    }

    void badBodyIsSkippedNotFatal()
    {
        Framer framer;
        framer.feed("Content-Length: 3\r\n\r\nnot" + Framer::encode(QJsonObject{{"ok", true}}));
        const auto bad = framer.next();
        QVERIFY(bad.has_value());
        QVERIFY(bad->contains("emblinkstudio.badFrame"));
        const auto good = framer.next();
        QVERIFY(good.has_value());
        QCOMPARE(good->value("ok").toBool(), true);
    }
};

QTEST_GUILESS_MAIN(TestFraming)
#include "tst_framing.moc"
