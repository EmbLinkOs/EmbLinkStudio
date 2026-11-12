// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    settingsdialog.cpp
 * @brief   Implementation of SettingsDialog: the General section and the language.
 *
 * @details
 * The language box lists each language in its own name, so a person can always find
 * theirs. Choosing one applies it and saves it at once; the dialog then retranslates
 * itself through the LanguageChange event like every other window.
 *
 * @see     settingsdialog.h
 * @author  Junior Deogracias
 * @date    2026-11-12 (created)
 */

#include "language.h"
#include "settingsdialog.h"
#include "theme.h"

#include <QComboBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

namespace emblinkstudio
{

SettingsDialog::SettingsDialog(Language *language, QWidget *parent)
    : QDialog(parent), m_language(language)
{
    setObjectName(QStringLiteral("settings"));
    resize(720, 460);
    const Theme &theme = Theme::current();
    const int gutter = int(theme.number(QStringLiteral("space.gutter")));

    auto *outer = new QHBoxLayout(this);
    outer->setContentsMargins(gutter, gutter, gutter, gutter);
    outer->setSpacing(gutter);

    // Sections, in a card on the left.
    auto *sectionsCard = new QFrame(this);
    sectionsCard->setProperty("role", QStringLiteral("card"));
    sectionsCard->setFixedWidth(200);
    auto *sectionsLayout = new QVBoxLayout(sectionsCard);
    sectionsLayout->setContentsMargins(8, 10, 8, 10);
    m_sections = new QListWidget(sectionsCard);
    m_sections->setObjectName(QStringLiteral("settingsSections"));
    m_sections->addItem(QString());
    m_sections->setCurrentRow(0);
    sectionsLayout->addWidget(m_sections);
    outer->addWidget(sectionsCard);

    // The chosen section, in the work card.
    auto *page = new QFrame(this);
    page->setProperty("role", QStringLiteral("card"));
    auto *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(26, 22, 26, 22);
    pageLayout->setSpacing(10);
    m_title = new QLabel(page);
    m_title->setFont(theme.font(QStringLiteral("title1")));
    pageLayout->addWidget(m_title);
    pageLayout->addSpacing(8);

    auto *group = new QFrame(page);
    group->setProperty("role", QStringLiteral("group"));
    auto *row = new QHBoxLayout(group);
    row->setContentsMargins(12, 8, 12, 8);
    auto *labels = new QVBoxLayout;
    labels->setSpacing(1);
    m_languageLabel = new QLabel(group);
    m_languageLabel->setFont(theme.font(QStringLiteral("bodySmall")));
    m_languageHint = new QLabel(group);
    m_languageHint->setProperty("role", QStringLiteral("quiet"));
    m_languageHint->setFont(theme.font(QStringLiteral("meta")));
    labels->addWidget(m_languageLabel);
    labels->addWidget(m_languageHint);
    row->addLayout(labels, 1);
    m_languageBox = new QComboBox(group);
    for (const QString &code : Language::codes())
        m_languageBox->addItem(Language::nativeName(code), code);
    m_languageBox->setMinimumWidth(170);
    m_languageLabel->setBuddy(m_languageBox);
    row->addWidget(m_languageBox);
    pageLayout->addWidget(group);

    m_untranslated = new QLabel(page);
    m_untranslated->setProperty("role", QStringLiteral("quiet"));
    m_untranslated->setFont(theme.font(QStringLiteral("small")));
    m_untranslated->setWordWrap(true);
    pageLayout->addWidget(m_untranslated);
    pageLayout->addStretch();
    outer->addWidget(page, 1);

    const int index = m_languageBox->findData(m_language->current());
    m_languageBox->setCurrentIndex(index < 0 ? 0 : index);
    connect(m_languageBox, &QComboBox::currentIndexChanged, this, [this](int i) {
        if (m_language->set(m_languageBox->itemData(i).toString()))
            m_language->save();
    });

    retranslate();
}

void SettingsDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        retranslate();
    QDialog::changeEvent(event);
}

void SettingsDialog::retranslate()
{
    setWindowTitle(tr("Settings"));
    m_sections->item(0)->setText(tr("General"));
    m_title->setText(tr("General"));
    m_languageLabel->setText(tr("&Language"));
    m_languageHint->setText(tr("Applies at once, without a restart."));
    m_untranslated->setText(tr("Option keys, pin and register names, file names and code stay as "
                               "they are in every language."));
}

} // namespace emblinkstudio
