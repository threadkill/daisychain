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

#include "watchmodel.h"


WatchModel::WatchModel() :
    ChainModel(),
    _passthru{nullptr}
{
}


QWidget*
WatchModel::embeddedWidget()
{
    if (!_widget) {
        _widget = ChainModel::embeddedWidget();
        _widget->setObjectName ("watchmodel");

        _batchChk->setEnabled (false);
        _batchChk->setVisible (false);
        _outputEdit->setEnabled (false);
        _outputEdit->setVisible (false);
        _outputLabel->setVisible (false);

        _passthru = new QCheckBox ("passthru");
        _passthru->setParent (_widget);
        _passthru->setChecked (false);
        _passthru->setObjectName ("_passthruChk");

        _layout->addWidget (_passthru);

        connect (_passthru, &QCheckBox::stateChanged, this, &WatchModel::onChanged);
    }

    return _widget;
}
