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

#include "node.h"


namespace daisychain {
class FilterNode : public Node
{
public:
    FilterNode (Graph*);
    FilterNode (Graph*, string filter, bool is_regex, bool negate);

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
