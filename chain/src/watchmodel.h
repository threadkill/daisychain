// # MIT License
// # Copyright (c) 2025 Stephen J. Parker
// # SPDX-License-Identifier: MIT
// # See LICENSE file for full license text.
//

#pragma once

#include "chainmodel.h"


class WatchModel : public ChainModel
{
    Q_OBJECT

public:
    WatchModel();

    ~WatchModel() override = default;

    QJsonObject save() const override;

    void load (QJsonObject const &) override;

    QWidget* embeddedWidget() override; // embeddedWidget

    static QString Name() { return {"Watch"}; } // Name

    QString name() const override { return WatchModel::Name(); } // name

private:
    QCheckBox* _passthruChk;
    QCheckBox* _recursiveChk;
};
