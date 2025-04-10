// # MIT License
// # Copyright (c) 2025 Stephen J. Parker
// # SPDX-License-Identifier: MIT
// # See LICENSE file for full license text.
//

#pragma once

#include "node.h"


namespace daisychain {
class ConcatNode final : public Node
{
public:
    ConcatNode ();

    bool Execute (vector<string>& input, const string& sandbox, json& vars) override;
};
} // namespace daisychain
