// # MIT License
// # Copyright (c) 2025 Stephen J. Parker
// # SPDX-License-Identifier: MIT
// # See LICENSE file for full license text.
//

#pragma once

#include "node.h"


namespace daisychain {
class FileListNode final : public Node
{
public:
    FileListNode();

    bool Execute (vector<string>& input, const string& sandbox, json& vars) override;

private:
    // not exposed
    using Node::set_batch_flag;
};
} // namespace daisychain
