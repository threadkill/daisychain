// # MIT License
// # Copyright (c) 2025 Stephen J. Parker
// # SPDX-License-Identifier: MIT
// # See LICENSE file for full license text.
//

#pragma once

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>

#include "chainmodel.h"


class FilterModel : public ChainModel
{
    Q_OBJECT

public:
    FilterModel();

    ~FilterModel() override = default;

    QJsonObject save() const override;

    void load (QJsonObject const &) override;

    QWidget* embeddedWidget() override; // embeddedWidget

    static QString Name() { return { "Filter" }; } // Name

    [[nodiscard]] QString name() const override { return FilterModel::Name(); } // name

    bool resizable() const override { return true; }

private:
    QLineEdit* _lineEdit;
    QCheckBox* _regexChk;
    QCheckBox* _invertChk;
};
