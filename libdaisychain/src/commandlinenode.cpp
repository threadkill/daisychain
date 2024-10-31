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

#include "commandlinenode.h"
#include <cstdlib>
#include <utility>


namespace daisychain {
using namespace std;


CommandLineNode::CommandLineNode()
{
    type_ = DaisyNodeType::DC_COMMANDLINE;
    set_name (DaisyNodeNameByType[type_]);
}


CommandLineNode::CommandLineNode (string cmd) :
    command_ (std::move (cmd))
{
    type_ = DaisyNodeType::DC_COMMANDLINE;
    set_name (DaisyNodeNameByType[type_]);
    set_command (command_);
}


void
CommandLineNode::Initialize (json& keydata, bool keep_uuid)
{
    Node::Initialize (keydata, keep_uuid);

    json::iterator jit = keydata.begin();
    const auto& uuid = jit.key();
    auto data = keydata[uuid];

    set_command (data["command"]);
    set_outputfile (data.count ("outputfile") ? data["outputfile"] : "");
    set_batch_flag (data.count ("batch") != 0 && data["batch"].get<bool>());
}


bool
CommandLineNode::Execute (vector<string>& inputs, const string& sandbox, json& env)
{
    // root nodes call this method directly. non-root nodes get here via `Node::Execute (sandbox,
    // env) after processing their inputs.
    LINFO << "Executing " << (isroot_ ? "root: " : "node: ") << LOGNODE ;

    bool stat = true;

    // prepare the shell environment
    for (auto& [key, value] : env.items()) {
#ifdef _WIN32
        if (!SetEnvironmentVariable (key.c_str(), shell_expand (value.get<string>()).c_str()))
#else
        if (setenv (key.c_str(), shell_expand (value.get<string>()).c_str(), true) < 0)
#endif
            return false;
    }

    // root nodes are passed a single string of all inputs and these
    // inputs may need to be tokenized if batch == false.
    if (isroot_) {
        if (batch_) {
            concat_inputs (inputs);
        }

        for (auto& input : inputs) {
            if (input != "EOF" && !terminate_.load()) {
                stat = run_command (input, sandbox);

                if (!stat)
                    break;
            }
        }
    }
    else {
        // tokenized processing which continues until EOF.
        while (eofs_ <= fd_in_.size() && !terminate_.load()) {
            for (auto& input : inputs) {
                if (input != "EOF") {
                    // performs open/write/close on outputs.
                    stat = run_command (input, sandbox);

                    if (!stat)
                        break;
                }
            }

            if (!stat)
                break;

            inputs.clear();

            if (eofs_ == fd_in_.size() || terminate_.load()) {
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

    return stat;
} // CommandLineNode::Execute


json
CommandLineNode::Serialize()
{
    auto json_ = Node::Serialize();
    json_[id_]["command"] = command_;
    json_[id_]["batch"] = batch_;
    json_[id_]["outputfile"] = outputfile_;

    if (size_ != std::pair<int, int>(0,0)) {json_[id_]["size"] = size_;}

    return json_;
} // CommandLineNode::Serialize


void
CommandLineNode::set_command (const string& cmd)
{
    command_ = cmd;
} // CommandLineNode::set_command


string
CommandLineNode::command()
{
    return command_;
} // CommandLineNode::command


bool
CommandLineNode::run_command (const string& input, const string& sandbox)
{
    bool stat = false;
    bool use_std_out = false;
    string std_out;
    vector<string> tokens;

    auto output = input;

#ifdef _WIN32
    // replacing newline delimiters with spaces before shell expansion.
    auto input_ = input;
    for (char& ch : input_) {
        if (ch == '\n') {
            ch = ' ';
        }
    }

    if (!SetEnvironmentVariable ("INPUT", shell_expand (input_).c_str()))
#else
    if (setenv ("INPUT", shell_expand (input).c_str(), true) < 0)
#endif
        return false;

    if (!outputfile_.empty()) {
        if (outputfile_.find ("STDOUT") != string::npos) {
            use_std_out = true;
        }
        else {
            output = shell_expand (outputfile_);
#ifdef _WIN32
            SetEnvironmentVariable ("OUTPUT", output.c_str());
#else
            setenv ("OUTPUT", output.c_str(), true);
#endif
        }
    }

    if (test_) {
        LTEST << LOGNODE << "\n" << shell_expand (command_);
        OpenOutputs (sandbox);
        WriteOutputs (output);
        CloseOutputs();

        return true;
    }

#ifdef _WIN32
#ifdef _DEBUG
    _putenv (R"(COMSPEC=C:\Windows\System32\cmd.exe)");
    _putenv (R"(PATH=%PATH%;C:\Windows\System32)");
#endif
    string cmd = command_ + " 2>&1";
    FILE* fp = _popen (cmd.c_str(), "r");
#else
    // setting IFS explicitly to newline-only facilitates handling paths with spaces.
    // redirecting stderr to stdout for log capture.
    string cmd = "IFS=\"\n\";" + command_ + " 2>&1";
    FILE* fp = popen (cmd.c_str(), "r");
#endif

    if (fp == nullptr) {
        stat = false;
    }

    char pbuff[8192] = {};

    while (!feof (fp)) {
        if (fgets (pbuff, 8192, fp) != nullptr) {
            std_out += pbuff;
        }
    }

#ifdef _WIN32
    if (_pclose (fp) == 0) {
#else
    if (pclose (fp) == 0) {
#endif
        stat = true;
    }

    if (!stat) {
        LERROR << LOGNODE << "run_command failed.";
        LERROR << LOGNODE << "\n" << shell_expand (command_);
        if (!std_out.empty()) {
            LERROR << LOGNODE << '\n' << std_out;
        }
    }
    else {
        LDEBUG << LOGNODE << "run_command succeeded.";
        if (!std_out.empty()) {
            LDEBUG << LOGNODE << '\n' << std_out;
        }

        OpenOutputs (sandbox);

        // capture program output and use for output var.
        if (use_std_out) {
#ifdef _WIN32
            SetEnvironmentVariable ("STDOUT", std_out.c_str());
#else
            setenv ("STDOUT", std_out.c_str(), true);
#endif
            output = shell_expand (outputfile_);
            m_split_input (output, tokens);

            for (auto const& token : tokens) {
                WriteOutputs (token);
            }
        }
        else {
            WriteOutputs (output);
        }
        CloseOutputs();
    }

    return stat;
} // CommandLineNode::run_command
} // namespace daisychain
