// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    tst_language.cpp
 * @brief   Qt Test suite for Language, the interface language.
 *
 * @details
 * Checks the three languages and their own names; the choice of a language for the
 * system's locale; that French and Simplified Chinese translate the interface and set
 * the locale numbers follow, and that English removes the translation; that a language
 * not shipped is refused without changing anything; that open widgets are told to
 * retranslate; and that the choice survives a restart. Settings go to a temporary
 * directory, never to the user's.
 *
 * @see     language.h, app/i18n
 * @author  Junior Deogracias
 * @date    2026-11-12 (created)
 */

#include "language.h"

#include <QCoreApplication>
#include <QEvent>
#include <QLocale>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QWidget>

using emblinkstudio::Language;

namespace
{

QString stage(const char *text)
{
    return QCoreApplication::translate("emblinkstudio::MainWindow", text);
}

// Counts the LanguageChange events a widget receives.
class Listener : public QWidget
{
  public:
    int changes = 0;

  protected:
    void changeEvent(QEvent *event) override
    {
        if (event->type() == QEvent::LanguageChange)
            ++changes;
        QWidget::changeEvent(event);
    }
};

} // namespace

class TestLanguage : public QObject
{
    Q_OBJECT

  private:
    QTemporaryDir m_dir;

  private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_dir.path());
        QCoreApplication::setOrganizationName(QStringLiteral("EmbLinkTest"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_language"));
    }

    void threeLanguagesInTheirOwnNames()
    {
        QCOMPARE(Language::codes(), (QStringList{QStringLiteral("en"), QStringLiteral("fr"),
                                                 QStringLiteral("zh_CN")}));
        QCOMPARE(Language::nativeName(QStringLiteral("fr")), QStringLiteral("Français"));
        QCOMPARE(Language::nativeName(QStringLiteral("zh_CN")), QStringLiteral("简体中文"));
    }

    void bestForTheSystem()
    {
        QCOMPARE(Language::bestFor(QLocale(QLocale::French, QLocale::Canada)),
                 QStringLiteral("fr"));
        QCOMPARE(Language::bestFor(
                     QLocale(QLocale::Chinese, QLocale::SimplifiedHanScript, QLocale::China)),
                 QStringLiteral("zh_CN"));
        QCOMPARE(Language::bestFor(
                     QLocale(QLocale::Chinese, QLocale::TraditionalHanScript, QLocale::Taiwan)),
                 QStringLiteral("en"));
        QCOMPARE(Language::bestFor(QLocale(QLocale::German)), QStringLiteral("en"));
    }

    void frenchAndChineseTranslateTheInterface()
    {
        Language language;
        QVERIFY(language.set(QStringLiteral("fr")));
        QCOMPARE(stage("Configure"), QStringLiteral("Configurer"));
        QCOMPARE(stage("Verify"), QStringLiteral("Vérification"));
        QCOMPARE(QCoreApplication::translate("emblinkstudio::SettingsDialog", "General"),
                 QStringLiteral("Général"));
        QCOMPARE(QLocale().toString(6.8, 'f', 1), QStringLiteral("6,8"));

        QVERIFY(language.set(QStringLiteral("zh_CN")));
        QCOMPARE(stage("Configure"), QStringLiteral("配置"));
        QCOMPARE(stage("Debug"), QStringLiteral("调试"));
        QCOMPARE(QLocale().language(), QLocale::Chinese);

        QVERIFY(language.set(QStringLiteral("en")));
        QCOMPARE(stage("Configure"), QStringLiteral("Configure"));
        QCOMPARE(QLocale().toString(6.8, 'f', 1), QStringLiteral("6.8"));
    }

    void languageNotShippedIsRefused()
    {
        Language language;
        QVERIFY(language.set(QStringLiteral("fr")));
        QSignalSpy changed(&language, &Language::changed);
        QVERIFY(!language.set(QStringLiteral("de")));
        QCOMPARE(language.current(), QStringLiteral("fr"));
        QCOMPARE(stage("Configure"), QStringLiteral("Configurer"));
        QCOMPARE(changed.count(), 0);
        QVERIFY(language.set(QStringLiteral("en")));
    }

    void openWidgetsAreToldToRetranslate()
    {
        Language language;
        Listener widget;
        QSignalSpy changed(&language, &Language::changed);
        QVERIFY(language.set(QStringLiteral("fr")));
        QCoreApplication::processEvents();
        QVERIFY(widget.changes >= 1);
        QCOMPARE(changed.count(), 1);
        QVERIFY(language.set(QStringLiteral("en")));
    }

    void choiceSurvivesARestart()
    {
        {
            Language language;
            QVERIFY(language.set(QStringLiteral("zh_CN")));
            language.save();
        }
        Language again;
        again.restore();
        QCOMPARE(again.current(), QStringLiteral("zh_CN"));
        QCOMPARE(stage("Code"), QStringLiteral("代码"));
        QVERIFY(again.set(QStringLiteral("en")));
    }
};

QTEST_MAIN(TestLanguage)
#include "tst_language.moc"
