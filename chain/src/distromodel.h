// # MIT License
// # Copyright (c) 2025 Stephen J. Parker
// # SPDX-License-Identifier: MIT
// # See LICENSE file for full license text.
//

#pragma once

#include "chainmodel.h"


class DistroModel : public ChainModel
{
    Q_OBJECT

public:
    DistroModel();

    ~DistroModel() override = default;

    QWidget* embeddedWidget() override; // embeddedWidget

    static QString Name() { return {"Distro"}; } // Name

    QString name() const override { return DistroModel::Name(); } // name
};
