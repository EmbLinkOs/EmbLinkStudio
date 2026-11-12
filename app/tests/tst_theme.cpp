// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    tst_theme.cpp
 * @brief   Qt Test suite for Theme, the design tokens in Qt.
 *
 * @details
 * Checks that the tokens built into the application are the file in docs/design; that
 * every colour token parses, in each of the forms the file uses; that the palette, the
 * fonts, the motions and the style sheets are taken from the tokens; and that a Chinese
 * interface sets no text below 12 px and drops the caption's capitals.
 *
 * @see     theme.h, docs/design/tokens.json
 * @author  Junior Deogracias
 * @date    2026-11-12 (created)
 */

#include "theme.h"

#include <QFile>
#include <QFontDatabase>
#include <QLocale>
#include <QRegularExpression>
#include <QTest>

using emblinkstudio::Theme;

class TestTheme : public QObject
{
    Q_OBJECT

  private:
    Theme m_theme;

  private slots:
    void initTestCase()
    {
        QString error;
        QVERIFY2(m_theme.load(Theme::defaultPath(), &error), qPrintable(error));
    }

    void builtInTokensAreTheFileInTheRepository()
    {
        QFile builtIn(Theme::defaultPath());
        QFile source(QStringLiteral(EMBLINK_STUDIO_TOKENS));
        QVERIFY(builtIn.open(QIODevice::ReadOnly));
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(builtIn.readAll(), source.readAll());
    }

    void everyColourTokenParses()
    {
        const QStringList paths = m_theme.colorPaths();
        QVERIFY2(paths.size() >= 50, qPrintable(QString::number(paths.size())));
        for (const QString &path : paths)
            QVERIFY2(m_theme.color(path).isValid(), qPrintable(path));
        QVERIFY(!m_theme.color(QStringLiteral("surface.nothing")).isValid());
    }

    void colourForms()
    {
        QCOMPARE(Theme::parseColor(QStringLiteral("#111214")), QColor(0x11, 0x12, 0x14));
        const QColor hairline = Theme::parseColor(QStringLiteral("rgba(255,255,255,.05)"));
        QCOMPARE(hairline.red(), 255);
        QCOMPARE(hairline.alpha(), 13);
        QCOMPARE(Theme::parseColor(QStringLiteral("#64d2ff80")).alpha(), 128);
        QVERIFY(!Theme::parseColor(QStringLiteral("cyan-ish")).isValid());
    }

    void flattenedIsWhatALayerMakes()
    {
        const QColor half = Theme::flattened(QColor(255, 255, 255, 128), Qt::black);
        QCOMPARE(half.alpha(), 255);
        QVERIFY(qAbs(half.red() - 128) <= 1);
    }

    void paletteComesFromTheTokens()
    {
        const QPalette p = m_theme.palette();
        QCOMPARE(p.color(QPalette::Window), m_theme.color(QStringLiteral("surface.ground")));
        QCOMPARE(p.color(QPalette::Highlight), m_theme.color(QStringLiteral("accent.base")));
        QCOMPARE(p.color(QPalette::HighlightedText),
                 m_theme.color(QStringLiteral("text.onAccent")));
        QCOMPARE(p.color(QPalette::Base).alpha(), 255); // flattened, never translucent
    }

    void numbersAndMotions()
    {
        QCOMPARE(m_theme.number(QStringLiteral("radius.card")), 12.0);
        QCOMPARE(m_theme.number(QStringLiteral("space.gutter")), 6.0);
        QCOMPARE(m_theme.number(QStringLiteral("size.titlebar")), 50.0);
        QCOMPARE(m_theme.duration(QStringLiteral("move")), 350);
        QCOMPARE(m_theme.easing(QStringLiteral("move")).type(), QEasingCurve::BezierSpline);
        // cubic-bezier(.2,.8,.2,1) is ahead of linear halfway.
        QVERIFY(m_theme.easing(QStringLiteral("move")).valueForProgress(0.5) > 0.6);
        QCOMPARE(Theme::parseEasing(QStringLiteral("linear")).type(), QEasingCurve::Linear);
    }

    void fontsFollowTheScale()
    {
        QLocale::setDefault(QLocale(QLocale::English));
        const QFont caption = m_theme.font(QStringLiteral("caption"));
        QCOMPARE(caption.capitalization(), QFont::AllUppercase);
        QCOMPARE(caption.weight(), QFont::DemiBold);
        // Only installed families are named: never one Qt would have to search for. (Where
        // no font is installed at all, as on Windows' offscreen platform, there is none to name.)
        const QStringList installed = QFontDatabase::families();
        if (!installed.isEmpty())
            for (const QString &family : caption.families())
                QVERIFY2(installed.contains(family), qPrintable(family));
        QVERIFY(caption.pointSizeF() < m_theme.font(QStringLiteral("small")).pointSizeF());
    }

    void chineseSetsNoTextBelowTwelvePixels()
    {
        QLocale::setDefault(
            QLocale(QLocale::Chinese, QLocale::SimplifiedHanScript, QLocale::China));
        const QFont caption = m_theme.font(QStringLiteral("caption"));
        QCOMPARE(caption.capitalization(), QFont::MixedCase);
        QCOMPARE(caption.pointSizeF(), m_theme.font(QStringLiteral("small")).pointSizeF());
        QLocale::setDefault(QLocale(QLocale::English));
    }

    void styleSheetsAreComplete()
    {
        static const QRegularExpression leftover(QStringLiteral("%\\d"));
        const QString app = m_theme.styleSheet();
        const QString dock = m_theme.dockStyleSheet();
        QVERIFY(app.contains(QStringLiteral("#111214")));
        QVERIFY(dock.contains(QStringLiteral("ads--CDockAreaWidget")));
        QVERIFY2(!app.contains(leftover), qPrintable(app));
        QVERIFY2(!dock.contains(leftover), qPrintable(dock));
    }
};

QTEST_MAIN(TestTheme)
#include "tst_theme.moc"
