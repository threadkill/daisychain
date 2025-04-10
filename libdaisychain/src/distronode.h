// MIT License
// Copyright (c) 2025 Stephen J. Parker
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text.

#pragma once

#include "node.h"


namespace daisychain {
class DistroNode final : public Node
{
public:
    DistroNode();

    bool Execute (vector<string>& input, const string& sandbox, json& vars) override;

    void OpenNextOutput (const string& sandbox);

    void CloseNextOutput();

    void WriteNextOutput (const string& output);

    void WriteAnyOutput (const string& output);

private:
    list<string>::iterator output_it_;
};
} // namespace daisychain
