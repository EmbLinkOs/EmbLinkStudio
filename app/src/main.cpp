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
 * defined by CMake from the project version), then shows MainWindow, which starts
 * EmbLink Studio Core.
 *
 * @see     mainwindow.h
 * @author  Junior Deogracias
 * @date    2026-11-11 (created)
 */

#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("EmbLink Studio"));
    QApplication::setOrganizationName(QStringLiteral("EmbLink"));
    QApplication::setApplicationVersion(QStringLiteral(EMBLINK_STUDIO_VERSION));

    emblinkstudio::MainWindow window;
    window.show();
    return app.exec();
}
