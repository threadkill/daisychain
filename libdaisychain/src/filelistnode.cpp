// # MIT License
// # Copyright (c) 2025 Stephen J. Parker
// # SPDX-License-Identifier: MIT
// # See LICENSE file for full license text.
//

#include "filelistnode.h"
#include <string>


namespace daisychain {
using namespace std;


FileListNode::FileListNode()
{
    type_ = DaisyNodeType::DC_FILELIST;
    set_name (DaisyNodeNameByType[type_]);
}


bool
FileListNode::Execute (vector<string>& inputs, const string& sandbox, json& vars)
{
    LINFO << "Executing " << (isroot_ ? "root: " : "node: ") << name_;

    if (isroot_) {
        for (auto& input : inputs) {
            std::ifstream infile (input);
            std::string line;

            while (std::getline (infile, line)) {
                OpenOutputs (sandbox);
                WriteOutputs (line);
                CloseOutputs();
            }
        }
    }
    else {
        while (eofs_ <= fd_in_.size() && !terminate_.load()) {
            for (auto& input : inputs) {
                if (input != "EOF") {
                    std::ifstream infile (input);
                    std::string line;

                    while (std::getline (infile, line)) {
                        OpenOutputs (sandbox);
                        WriteOutputs (line);
                        CloseOutputs();
                    }
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

    // all processing is done for this node. Send EOF downstream.
    OpenOutputs (sandbox);
    WriteOutputs ("EOF");
    CloseOutputs();
    Stats();
    Reset();

    return true;
} // FileListNode::Execute


} // namespace daisychain
