// # MIT License
// # Copyright (c) 2025 Stephen J. Parker
// # SPDX-License-Identifier: MIT
// # See LICENSE file for full license text.
//

#pragma once

#include "node.h"


namespace daisychain {
class FilterNode final : public Node
{
public:
    FilterNode();

    FilterNode (string filter, bool is_regex, bool negate);

    void Initialize (json&, bool) override;

    bool Execute (vector<string>& input, const string& sandbox, json& vars) override;

    json Serialize() override;

    void set_filter (const string& filter);

    string filter();

    void set_regex (bool is_regex);

    bool regex() const;

    void set_invert (bool invert);

    bool invert() const;

private:
    // not exposed
    using Node::set_batch_flag;
    using Node::set_outputfile;

    bool regex_;
    bool invert_;
    string filter_;
};
} // namespace daisychain
