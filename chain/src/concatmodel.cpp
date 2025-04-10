// # MIT License
// # Copyright (c) 2025 Stephen J. Parker
// # SPDX-License-Identifier: MIT
// # See LICENSE file for full license text.
//

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
