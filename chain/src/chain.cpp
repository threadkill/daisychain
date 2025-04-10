// MIT License
// Copyright (c) 2025 Stephen J. Parker
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text.

#include "chain.h"


int
main (int argc, char* argv[])
{
    QLoggingCategory::setFilterRules ("kf.kio.widgets.kdirmodel=false"); // temp for kde bug

    configureLogger();
    LINFO << "DaisyChain " << DAISYCHAIN_VERSION;

    QApplication::setStyle (QStyleFactory::create ("fusion"));
    setNodeStyle();
    QApplication::setPalette (darkPalette());

    QApplication app (argc, argv);
    QApplication::setWindowIcon (QIcon (":daisy_alpha.png"));

    QCoreApplication::setOrganizationName ("threadkill");
    QCoreApplication::setApplicationName ("chain");
    QCoreApplication::setApplicationVersion (QT_VERSION_STR);
    QCoreApplication::setAttribute (Qt::AA_DontUseNativeMenuBar);

    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();

    QPixmap splashImage (":daisy_alpha_big.png");
    QSplashScreen splash (splashImage);
    splash.setWindowFlags (splash.windowFlags() | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    splash.show();

    ChainWindow chainWin;

    QTimer::singleShot (1500, [&] {
        chainWin.showMinimized();
        chainWin.showNormal();
        chainWin.resize (1280, 1024);
        auto centered = screenGeometry.center() - chainWin.frameGeometry().center();
        chainWin.move (centered);
        chainWin.raise();
        chainWin.activateWindow();
        splash.finish (&chainWin);
    });

    return QApplication::exec();
} // main
