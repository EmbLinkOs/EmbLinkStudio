// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    language.cpp
 * @brief   Implementation of Language: loading, swapping and saving the translation.
 *
 * @details
 * Two translators are installed for a language other than English: EmbLink Studio's
 * own (:/i18n/emblinkstudio_<code>.qm) and Qt's, for the texts of Qt's standard
 * dialogs and buttons, when Qt's translations are installed beside Qt. Only the first
 * is required; without Qt's, standard buttons stay in English.
 *
 * @see     language.h
 * @author  Junior Deogracias
 * @date    2026-11-12 (created)
 */

#include "language.h"

#include <QCoreApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

namespace emblinkstudio
{

namespace
{

const QString kSettingsKey = QStringLiteral("general/language");

QLocale localeFor(const QString &code)
{
    if (code == QStringLiteral("zh_CN"))
        return QLocale(QLocale::Chinese, QLocale::SimplifiedHanScript, QLocale::China);
    if (code == QStringLiteral("fr"))
        return QLocale(QLocale::French, QLocale::France);
    return QLocale(QLocale::English, QLocale::UnitedStates);
}

} // namespace

Language::Language(QObject *parent) : QObject(parent), m_current(QStringLiteral("en"))
{
}

Language::~Language()
{
    if (m_app)
        QCoreApplication::removeTranslator(m_app);
    if (m_qt)
        QCoreApplication::removeTranslator(m_qt);
}

QStringList Language::codes()
{
    return {QStringLiteral("en"), QStringLiteral("fr"), QStringLiteral("zh_CN")};
}

QString Language::nativeName(const QString &code)
{
    if (code == QStringLiteral("fr"))
        return QStringLiteral("Français");
    if (code == QStringLiteral("zh_CN"))
        return QStringLiteral("简体中文");
    return QStringLiteral("English");
}

QString Language::bestFor(const QLocale &locale)
{
    if (locale.language() == QLocale::French)
        return QStringLiteral("fr");
    // Simplified Chinese is the only Chinese shipped; a Traditional script falls back to English.
    if (locale.language() == QLocale::Chinese && locale.script() != QLocale::TraditionalHanScript)
        return QStringLiteral("zh_CN");
    return QStringLiteral("en");
}

bool Language::set(const QString &code)
{
    if (!codes().contains(code))
        return false;

    QTranslator *app = nullptr;
    QTranslator *qt = nullptr;
    if (code != QStringLiteral("en")) {
        app = new QTranslator(this);
        if (!app->load(QStringLiteral(":/i18n/emblinkstudio_%1.qm").arg(code))) {
            delete app;
            return false;
        }
        qt = new QTranslator(this);
        if (!qt->load(localeFor(code), QStringLiteral("qtbase"), QStringLiteral("_"),
                      QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
            delete qt;
            qt = nullptr;
        }
    }

    if (m_app) {
        QCoreApplication::removeTranslator(m_app);
        delete m_app;
    }
    if (m_qt) {
        QCoreApplication::removeTranslator(m_qt);
        delete m_qt;
    }
    m_app = app;
    m_qt = qt;
    QLocale::setDefault(localeFor(code));
    if (m_qt)
        QCoreApplication::installTranslator(m_qt);
    if (m_app)
        QCoreApplication::installTranslator(m_app);

    const bool changedCode = code != m_current;
    m_current = code;
    if (changedCode)
        emit changed(code);
    return true;
}

void Language::restore()
{
    QSettings settings;
    const QString saved = settings.value(kSettingsKey).toString();
    if (!saved.isEmpty() && set(saved))
        return;
    set(bestFor(QLocale::system()));
}

void Language::save() const
{
    QSettings settings;
    settings.setValue(kSettingsKey, m_current);
}

} // namespace emblinkstudio
