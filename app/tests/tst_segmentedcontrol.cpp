// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    tst_segmentedcontrol.cpp
 * @brief   Qt Test suite for SegmentedControl, the stage switch.
 *
 * @details
 * Checks that the first segment is chosen when segments are added; that a click
 * chooses the segment under it; that the keyboard moves the choice and stops at the
 * ends; that an index out of range is ignored; and that a longer text, as a
 * translation brings, widens the control instead of being cut.
 *
 * @see     segmentedcontrol.h
 * @author  Junior Deogracias
 * @date    2026-11-12 (created)
 */

#include "segmentedcontrol.h"

#include <QSignalSpy>
#include <QTest>

using emblinkstudio::SegmentedControl;

namespace
{

void fill(SegmentedControl &c)
{
    for (const char *text : {"Configure", "Code", "Debug", "Verify"})
        c.addSegment(QString::fromLatin1(text));
}

} // namespace

class TestSegmentedControl : public QObject
{
    Q_OBJECT

  private slots:
    void firstSegmentIsChosen()
    {
        SegmentedControl c;
        QCOMPARE(c.currentIndex(), -1);
        fill(c);
        QCOMPARE(c.count(), 4);
        QCOMPARE(c.currentIndex(), 0);
    }

    void clickChoosesTheSegment()
    {
        SegmentedControl c;
        fill(c);
        c.resize(c.sizeHint());
        c.show();
        QVERIFY(QTest::qWaitForWindowExposed(&c));
        QSignalSpy changed(&c, &SegmentedControl::currentChanged);
        QTest::mouseClick(&c, Qt::LeftButton, {}, c.segmentRect(2).center().toPoint());
        QCOMPARE(c.currentIndex(), 2);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(changed.first().first().toInt(), 2);
        QTest::mouseClick(&c, Qt::LeftButton, {}, c.segmentRect(2).center().toPoint());
        QCOMPARE(changed.count(), 1); // the same segment again changes nothing
    }

    void keyboardMovesTheChoice()
    {
        SegmentedControl c;
        fill(c);
        QTest::keyClick(&c, Qt::Key_Right);
        QCOMPARE(c.currentIndex(), 1);
        QTest::keyClick(&c, Qt::Key_End);
        QCOMPARE(c.currentIndex(), 3);
        QTest::keyClick(&c, Qt::Key_Right);
        QCOMPARE(c.currentIndex(), 3);
        QTest::keyClick(&c, Qt::Key_Home);
        QCOMPARE(c.currentIndex(), 0);
        QTest::keyClick(&c, Qt::Key_Left);
        QCOMPARE(c.currentIndex(), 0);
    }

    void outOfRangeIsIgnored()
    {
        SegmentedControl c;
        fill(c);
        c.setCurrentIndex(9);
        c.setCurrentIndex(-1);
        QCOMPARE(c.currentIndex(), 0);
        QCOMPARE(c.segmentText(9), QString());
    }

    void longerTextWidensTheControl()
    {
        SegmentedControl c;
        fill(c);
        const int before = c.sizeHint().width();
        c.setSegmentText(0, QStringLiteral("Configurer"));
        c.setSegmentText(3, QStringLiteral("Vérification"));
        QVERIFY(c.sizeHint().width() > before);
        QCOMPARE(c.segmentText(3), QStringLiteral("Vérification"));
        QVERIFY(c.segmentRect(3).right() <= c.sizeHint().width());
    }
};

QTEST_MAIN(TestSegmentedControl)
#include "tst_segmentedcontrol.moc"
