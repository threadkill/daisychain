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
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QWidget>
#include <nlohmann/json.hpp>


using json = nlohmann::ordered_json;


class EnvironWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EnvironWidget (QWidget* parent = Q_NULLPTR);
    ~EnvironWidget() override = default;

public Q_SLOTS:

    void populateUI (const json&);

Q_SIGNALS:

    void environWidgetChanged (json&);

private:
    QTableWidget* table_;
    void setupUI();

    json serialize();

    void addRow (const QString& key, const QString& value);

private Q_SLOTS:

    void updateTable();
};
