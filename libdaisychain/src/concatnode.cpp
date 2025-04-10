// # MIT License
// # Copyright (c) 2025 Stephen J. Parker
// # SPDX-License-Identifier: MIT
// # See LICENSE file for full license text.
//

#include "concatnode.h"
#include <regex>


namespace daisychain {
using namespace std;


ConcatNode::ConcatNode()
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
        while (eofs_ <= fd_in_.size() && !terminate_.load()) {
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
            ReadInputs (inputs);
        }
        CloseInputs();
    }

    OpenOutputs (sandbox);
    WriteOutputs ("EOF");
    CloseOutputs();
    Stats();
    Reset();

    return true;
} // ConcatNode::Execute


} // namespace daisychain
