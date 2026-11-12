// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
//
// This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
// Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.

/**
 * @file    main.cpp
 * @brief   Entry point of the EmbLink Studio desktop application.
 *
 * @details
 * Sets the application name, organization and version (EMBLINK_STUDIO_VERSION,
 * defined by CMake from the project version), applies the interface language, then
 * installs the theme from the built-in design tokens and shows MainWindow, which
 * starts EmbLink Studio Core. The language comes first, so the theme's fonts follow
 * its rules (no Chinese text below 12 px).
 *
 * Options, for development and for the pictures in progress-screenshots/:
 *   --language <en|fr|zh_CN>  use this language for this run, without saving it
 *   --stage <configure|code|debug|verify>  open on this stage
 *   --open <board>            open this board of an EmbLinkRTOS checkout
 *   --view <id>               bring a view to the front: board, clocks, memory, devices
 *   --screenshot <file>       save a picture of the window to <file>, then quit
 *
 * @see     mainwindow.h, theme.h, language.h
 * @author  Junior Deogracias
 * @date    2026-11-11 (created)
 */

#include "language.h"
#include "mainwindow.h"
#include "theme.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("EmbLink Studio"));
    QApplication::setOrganizationName(QStringLiteral("EmbLink"));
    QApplication::setApplicationVersion(QStringLiteral(EMBLINK_STUDIO_VERSION));

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption languageOption(
        QStringLiteral("language"), QStringLiteral("Use <code> for this run: en, fr or zh_CN."),
        QStringLiteral("code"));
    const QCommandLineOption stageOption(
        QStringLiteral("stage"), QStringLiteral("Open on <stage>: configure, code, debug, verify."),
        QStringLiteral("stage"));
    const QCommandLineOption screenshotOption(
        QStringLiteral("screenshot"),
        QStringLiteral("Save a picture of the window to <file>, then quit."),
        QStringLiteral("file"));
    const QCommandLineOption openOption(
        QStringLiteral("open"),
        QStringLiteral("Open <board>, a board directory of an EmbLinkRTOS checkout."),
        QStringLiteral("board"));
    const QCommandLineOption viewOption(
        QStringLiteral("view"),
        QStringLiteral("Bring the view <id> to the front: board, clocks, memory, devices."),
        QStringLiteral("id"));
    parser.addOptions({languageOption, stageOption, openOption, viewOption, screenshotOption});
    parser.process(app);

    emblinkstudio::Language language(&app);
    language.restore();
    if (parser.isSet(languageOption) && !language.set(parser.value(languageOption)))
        qWarning("Unknown language %s", qPrintable(parser.value(languageOption)));

    emblinkstudio::Theme theme;
    QString error;
    if (!theme.load(emblinkstudio::Theme::defaultPath(), &error))
        qFatal("The design tokens are missing: %s", qPrintable(error));
    emblinkstudio::Theme::install(theme, app);
    // A Chinese interface sets no text below 12 px: the fonts follow the language.
    QObject::connect(&language, &emblinkstudio::Language::changed, &app, [&app] {
        app.setFont(emblinkstudio::Theme::current().font(QStringLiteral("body")));
    });

    emblinkstudio::MainWindow window(&language);
    if (parser.isSet(stageOption))
        window.setStage(parser.value(stageOption));
    if (parser.isSet(openOption))
        window.openBoard(parser.value(openOption));
    if (parser.isSet(viewOption))
        window.showView(parser.value(viewOption));
    window.show();

    if (parser.isSet(screenshotOption)) {
        const QString file = parser.value(screenshotOption);
        // Long enough for the core to open and validate a board, and the stage switch to settle.
        QTimer::singleShot(3000, &window, [&window, &app, file] {
            if (!window.grab().save(file))
                qWarning("Cannot save %s", qPrintable(file));
            app.exit(0);
        });
    }
    return app.exec();
}
