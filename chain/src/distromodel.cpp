// # MIT License
// # Copyright (c) 2025 Stephen J. Parker
// # SPDX-License-Identifier: MIT
// # See LICENSE file for full license text.
//

#include "distromodel.h"


DistroModel::DistroModel() : ChainModel()
{
}


QWidget*
DistroModel::embeddedWidget()
{
    if (!_widget) {
        _widget = ChainModel::embeddedWidget();
        _widget->setObjectName ("distromodel");

        _batchChk->setEnabled (false);
        _batchChk->setVisible (false);
        _outputEdit->setEnabled (false);
        _outputEdit->setVisible (false);
        _outputLabel->setVisible (false);
    }

    return _widget;
}
