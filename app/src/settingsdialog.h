// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    settingsdialog.h
 * @brief   SettingsDialog: EmbLink Studio's settings, as macOS's System Settings lays them out.
 *
 * @details
 * A list of sections on the left and the chosen section's settings on the right, each
 * group of settings a rounded group of rows (design brief section 4f). Settings apply
 * as soon as they change; there is no OK or Apply. The first section, General, holds
 * the interface language; toolchains, probes, the core's log and shortcuts join it as
 * the stages that need them arrive (spec section 7, Settings).
 *
 * Opened with Settings (⌘, on macOS, Ctrl+, elsewhere), it is built directly in Qt,
 * without a mockup (owner, 2026-11-12).
 *
 * @see     language.h; docs/design/brief.md section 4f
 * @author  Junior Deogracias
 * @date    2026-11-12 (created)
 */

#pragma once

#include <QDialog>

class QComboBox;
class QLabel;
class QListWidget;

namespace emblinkstudio
{

class Language;

class SettingsDialog : public QDialog
{
    Q_OBJECT

  public:
    explicit SettingsDialog(Language *language, QWidget *parent = nullptr);

    QComboBox *languageBox() const
    {
        return m_languageBox;
    }

  protected:
    void changeEvent(QEvent *event) override;

  private:
    void retranslate();

    Language *m_language = nullptr;
    QListWidget *m_sections = nullptr;
    QLabel *m_title = nullptr;
    QLabel *m_languageLabel = nullptr;
    QLabel *m_languageHint = nullptr;
    QLabel *m_untranslated = nullptr;
    QComboBox *m_languageBox = nullptr;
};

} // namespace emblinkstudio
