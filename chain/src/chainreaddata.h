// MIT License
// Copyright (c) 2025 Stephen J. Parker
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text.

#pragma once

#include <QtNodes/NodeDelegateModel>

using QtNodes::NodeData;
using QtNodes::NodeDataType;


class ChainReadData : public NodeData
{
public:
    ChainReadData() = default;

    NodeDataType type() const override { return NodeDataType {"text", "In"}; } // type
};
