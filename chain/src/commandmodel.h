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

#pragma once

#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>

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
    QTextEdit* _textEdit;
};
