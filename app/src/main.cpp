// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Junior Deogracias
#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("EmbStudio"));
    QApplication::setOrganizationName(QStringLiteral("EmbLink"));
    QApplication::setApplicationVersion(QStringLiteral(EMBSTUDIO_VERSION));

    embstudio::MainWindow window;
    window.show();
    return app.exec();
}
