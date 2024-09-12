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

#include "concatnode.h"
#include <regex>


namespace daisychain {
using namespace std;


ConcatNode::ConcatNode() : Node()
{
    type_ = DaisyNodeType::DC_CONCAT;
    set_name (DaisyNodeNameByType[type_]);
}


bool
ConcatNode::Execute (vector<string>& inputs, const string& sandbox, json& vars)
{
    LINFO << "Executing " << (isroot_ ? "root: " : "node: ") << name_;

    if (isroot_) {
        for (auto& input : inputs) {
            OpenOutputs (sandbox);
            WriteOutputs (input);
            CloseOutputs();
        }
    }
    else {
        while (eofs_ <= fd_in_.size()) {
            for (auto& input : inputs) {
                if (input != "EOF") {
                    OpenOutputs (sandbox);
                    WriteOutputs (input);
                    CloseOutputs();
                }
            }

            inputs.clear();

            if (eofs_ == fd_in_.size()) {
                break;
            }
            else {
                ReadInputs (inputs);
            }
        }

        CloseInputs();
    }

    OpenOutputs (sandbox);
    WriteOutputs ("EOF");
    CloseOutputs();
    Stats();

    return true;
} // ConcatNode::Execute


} // namespace daisychain
