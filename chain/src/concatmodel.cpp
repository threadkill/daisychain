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

#include "concatmodel.h"


ConcatModel::ConcatModel() : ChainModel()
{
}


QWidget*
ConcatModel::embeddedWidget()
{
    if (!_widget) {
        _widget = ChainModel::embeddedWidget();
        _widget->setObjectName ("concatmodel");

        _batchChk->setEnabled (false);
        _batchChk->setVisible (false);
        _outputEdit->setEnabled (false);
        _outputEdit->setVisible (false);
        _outputLabel->setVisible (false);
    }

    return _widget;
}


unsigned int
ConcatModel::nPorts (PortType portType) const
{
    unsigned int result = 1;

    switch (portType) {
    case PortType::In:
        result = 4;
        break;

    case PortType::Out:
        result = 1;

    default:
        break;
    } // switch

    return result;
} // ConcatModel::nPorts
