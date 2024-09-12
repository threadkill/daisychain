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

#pragma once
#include <QFontDatabase>


namespace chain {
static QFont&
chainfont()
{
    static int id_regular = QFontDatabase::addApplicationFont (":/MesloLGS NF Regular.ttf");
    static int id_bold = QFontDatabase::addApplicationFont (":/MesloLGS NF Bold.ttf");
    static int id_italic = QFontDatabase::addApplicationFont (":/MesloLGS NF Italic.ttf");
    static int id_bitalic = QFontDatabase::addApplicationFont (":/MesloLGS NF Bold Italic.ttf");

    static QString family = QFontDatabase::applicationFontFamilies (id_regular).at (0);
    static QFont monospace (family);
    monospace.setPixelSize (11);

    return monospace;
} // chainfont
} // namespace chain
