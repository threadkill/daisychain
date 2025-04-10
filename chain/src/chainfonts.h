// MIT License
// Copyright (c) 2025 Stephen J. Parker
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text.

#pragma once
#include <QFontDatabase>


namespace chain {
inline QFont&
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
