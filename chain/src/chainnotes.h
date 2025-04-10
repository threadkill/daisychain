// MIT License
// Copyright (c) 2025 Stephen J. Parker
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text.

#pragma once
#include <QPlainTextEdit>
#include <nlohmann/json.hpp>


using json = nlohmann::ordered_json;


class ChainNotes : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit ChainNotes (QWidget* parent = Q_NULLPTR);
    ~ChainNotes() override = default;

    json serialize();

private:
    void setupUI();
};


