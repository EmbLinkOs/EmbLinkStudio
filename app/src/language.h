// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    language.h
 * @brief   Language: the interface language, chosen in Settings and applied at once.
 *
 * @details
 * EmbLink Studio speaks English (the source language), French and Simplified Chinese
 * (design brief section 4f). Every visible string goes through tr(); the translations
 * are Qt Linguist files in app/i18n, compiled into the application under :/i18n.
 *
 * set() swaps the installed translator and the default locale without a restart:
 * Qt then sends every widget a LanguageChange event, and each window retranslates its
 * own texts. Numbers and dates follow the language through QLocale; hex values, option
 * keys, pin and register names are never translated.
 *
 * The choice is kept in the user's settings under general/language. With no choice
 * yet, the system's language is used when it is one of the three, English otherwise.
 *
 * @see     docs/design/tokens.json ("locale"), app/i18n
 * @author  Junior Deogracias
 * @date    2026-11-12 (created)
 */

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class QLocale;
class QTranslator;

namespace emblinkstudio
{

class Language : public QObject
{
    Q_OBJECT

  public:
    explicit Language(QObject *parent = nullptr);
    ~Language() override;

    // "en", "fr", "zh_CN": the languages the interface ships in, English first.
    static QStringList codes();
    // The language's own name, as the language menu shows it: "Français", "简体中文".
    static QString nativeName(const QString &code);
    // The supported language closest to `locale`; "en" when none is close.
    static QString bestFor(const QLocale &locale);

    QString current() const
    {
        return m_current;
    }

    // Applies `code` at once; false, and nothing changed, if it is not supported or its
    // translation cannot be loaded.
    bool set(const QString &code);
    // Applies the saved choice, or the system's language when nothing is saved.
    void restore();
    // Saves the current choice in the user's settings.
    void save() const;

  signals:
    void changed(const QString &code);

  private:
    QString m_current;
    QTranslator *m_app = nullptr;
    QTranslator *m_qt = nullptr;
};

} // namespace emblinkstudio
