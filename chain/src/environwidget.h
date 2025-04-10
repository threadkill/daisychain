// MIT License
// Copyright (c) 2025 Stephen J. Parker
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text.

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
