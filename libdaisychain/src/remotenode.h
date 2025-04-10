// MIT License
// Copyright (c) 2025 Stephen J. Parker
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text.

#pragma once

#include "node.h"


namespace daisychain {
class RemoteNode : public Node
{
public:
    RemoteNode();

    bool Execute (const string& sandbox);

    json Serialize() override;

    void set_host_id (const string& host_id);

    string host_id();

private:
    string host_id_;
};
} // namespace daisychain
