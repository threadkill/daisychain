// DaisyChain - a node-based dependency graph for file processing.
// Copyright (C) 2015  Stephen J. Parker
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>

#include "chain.h"
#include "chainstyles.h"


int
main (int argc, char* argv[])
{
    QLoggingCategory::setFilterRules ("kf.kio.widgets.kdirmodel=false"); // temp for kde bug

    configureLogger();
    LINFO << "DaisyChain " << DAISYCHAIN_VERSION;

    //QApplication::setAttribute (Qt::AA_DisableWindowContextHelpButton);
    //QApplication::setAttribute (Qt::AA_DisableHighDpiScaling);

    QApplication::setStyle (QStyleFactory::create ("fusion"));
    setNodeStyle();
    QApplication::setPalette (darkPalette());

    QApplication app (argc, argv);

    QApplication::setWindowIcon (QIcon (":daisy_alpha.png"));

    QCoreApplication::setOrganizationName ("threadkill");
    QCoreApplication::setApplicationName ("chain");
    QCoreApplication::setApplicationVersion (QT_VERSION_STR);
    QCoreApplication::setAttribute (Qt::AA_DontUseNativeMenuBar);

    // QCoreApplication::setAttribute (Qt::AA_Use96Dpi);

    ChainWindow chainWin;

    chainWin.show();
    chainWin.resize (1280, 1024);
    chainWin.move (QGuiApplication::screens().at (0)->geometry().center()
                   - chainWin.frameGeometry().center());

    return app.exec();
} // main
