// # MIT License
// # Copyright (c) 2025 Stephen J. Parker
// # SPDX-License-Identifier: MIT
// # See LICENSE file for full license text.
//

#pragma once

#include <QtWidgets/QPlainTextEdit>

#include "chainmodel.h"


class CommandModel : public ChainModel
{
    Q_OBJECT

public:
    CommandModel();

    ~CommandModel() override = default;

    QJsonObject save() const override;

    void load (QJsonObject const &) override;

    QWidget* embeddedWidget() override; // embeddedWidget

public:
    static QString Name() { return {"Shell Command"}; } // Name

    QString name() const override { return CommandModel::Name(); } // name

    bool resizable() const override { return true; }

private:
    QPlainTextEdit* _textEdit;
};
