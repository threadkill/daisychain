// # MIT License
// # Copyright (c) 2025 Stephen J. Parker
// # SPDX-License-Identifier: MIT
// # See LICENSE file for full license text.
//

#pragma once

#include "chainmodel.h"


class ConcatModel : public ChainModel
{
    Q_OBJECT

public:
    ConcatModel();

    ~ConcatModel() override = default;

    QWidget* embeddedWidget() override; // embeddedWidget

    static QString Name() { return {"Concat"}; } // Name

    QString name() const override { return ConcatModel::Name(); } // name

    unsigned int nPorts (PortType portType) const override;
};
