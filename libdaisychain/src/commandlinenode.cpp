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
    TIMED_SCOPE (timerObj, LOGNODE);

    // root nodes call this method directly. non-root nodes get here via `Node::Execute (sandbox,
    // env) after processing their inputs.
    LINFO << "Executing " << (isroot_ ? "root: " : "node: ") << name_;

    bool stat = true;

#ifndef _WIN32
    // prepare the shell environment
    for (auto& [key, value] : env.items()) {
        if (setenv (key.c_str(), shell_expand (value.get<string>()).c_str(), true) < 0)
            return false;
    }
#else
    // prepare the shell environment
    for (auto& [key, value] : env.items()) {
        set_variable (key, value.get<string>());
    }
#endif

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


#ifdef _WIN32
#include <windows.h>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem> // For fs::path
namespace fs = std::filesystem;

bool
CommandLineNode::run_command (const std::string& input, const std::string& sandbox)
{
    // Set up variables
    bool stat = false;
    bool use_std_out = false;
    std::string output = input;

    // Replace newline delimiters with spaces before shell expansion
    bool singlefile = true;
    std::string input_ = input;
    for (char& ch : input_) {
        if (ch == '\n') {
            singlefile = false;
            ch = ' ';
        }
    }

    set_variable ("SANDBOX", sandbox);
    set_variable ("INPUT", input_);

    if (singlefile) {
        std::string basename = fs::path(input).filename().string();
        set_variable ("BASENAME", basename);
    }

    if (!outputfile_.empty()) {
        if (outputfile_.find("STDOUT") != std::string::npos) {
            use_std_out = true;
        }
        else {
            output = shell_expand (outputfile_);
            set_variable ("OUTPUT", output);
        }
    }

    if (test_) {
        LTEST << LOGNODE << "\n" << shell_expand (command_);
        OpenOutputs(sandbox);
        WriteOutputs(output);
        CloseOutputs();
        return true;
    }

    // Log the command line for debugging
    LDEBUG << LOGNODE << "Executing command line: " << shell_expand ("echo \"" + command_ + "\"");

    std::string std_out;
    stat = run_cmdexe (command_, std_out);

    // Log the output from the child process
    if (!std_out.empty()) {
        LDEBUG << LOGNODE << "Child process output:\n" << std_out;
    }

    if (!stat) {
        LERROR << LOGNODE << "run_command failed.";
        LERROR << LOGNODE << "\n" << shell_expand (command_);
    } else {
        LDEBUG << LOGNODE << "run_command succeeded.";

        if (use_std_out) {
            std::vector<std::string> tokens;
            set_variable ("STDOUT", std_out);
            output = shell_expand (outputfile_);
            m_split_input (output, tokens);

            for (const auto& token : tokens) {
                WriteOutputs(token);
            }
        } else {
            WriteOutputs(output);
        }
    }

    return stat;
} // CommandLineNode::run_command

#else

bool
CommandLineNode::run_command (const string& input, const string& sandbox)
{
    //setbuf (stdout, nullptr);
    bool stat = false;
    bool use_std_out = false;
    string std_out;
    vector<string> tokens;

    auto output = input;

    if (setenv ("INPUT", shell_expand (input).c_str(), true) < 0)
        return false;

    if (!outputfile_.empty()) {
        if (outputfile_.find ("STDOUT") != string::npos) {
            use_std_out = true;
        }
        else {
            output = shell_expand (outputfile_);
            setenv ("OUTPUT", output.c_str(), true);
        }
    }

    if (test_) {
        LTEST << LOGNODE << "\n" << shell_expand (command_);
        OpenOutputs (sandbox);
        WriteOutputs (output);
        CloseOutputs();

        return true;
    }

    // setting IFS explicitly to newline-only facilitates handling paths with spaces.
    // redirecting stderr to stdout for log capture.
    string cmd = "IFS=\"\n\";" + command_ + " 2>&1";
    FILE* fp = popen (cmd.c_str(), "r");

    if (fp == nullptr) {
        stat = false;
    }

    char pbuff[8192] = {};

    while (!feof (fp) && !terminate_.load()) {
        if (fgets (pbuff, 8192, fp) != nullptr) {
            std_out += pbuff;
        }
    }

    if (pclose (fp) == 0) {
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
            setenv ("STDOUT", std_out.c_str(), true);
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
#endif

#ifdef _WIN32

bool
CommandLineNode::run_cmdexe (const std::string& command, std::string& output)
{
    std::string cmdLine = "cmd.exe /C \"call " + command + "\"";

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;

    ZeroMemory (&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory (&pi, sizeof(pi));

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    // Create pipes for STDOUT and STDERR
    HANDLE hStdOutRead = nullptr;
    HANDLE hStdOutWrite = nullptr;

    if (!CreatePipe (&hStdOutRead, &hStdOutWrite, &sa, 0)) {
        LERROR << LOGNODE << "Failed to create pipe for STDOUT. " << GetLastError();
        return false;
    }

    // Ensure the read handle is not inherited
    SetHandleInformation (hStdOutRead, HANDLE_FLAG_INHERIT, 0);

    // Redirect both STDOUT and STDERR to the same pipe
    si.hStdOutput = hStdOutWrite;
    si.hStdError = hStdOutWrite;
    si.hStdInput = nullptr; // No need to send any input

    auto env = get_environment();

    // Create the process
    BOOL result = CreateProcessA(
        nullptr,
        &cmdLine[0],      // Command line
        nullptr,          // Process security attributes
        nullptr,          // Primary thread security attributes
        true,             // Handles are inherited
        CREATE_NO_WINDOW, // Creation flags (hide the window)
        env.data(),       // Use parent's environment
        nullptr,          // Use parent's current directory
        &si,
        &pi
    );

    CloseHandle (hStdOutWrite);

    if (!result) {
        LERROR << LOGNODE << "CreateProcess failed with error " << GetLastError();
        CloseHandle (hStdOutRead);
        return false;
    }

    const DWORD bufferSize = 8192;
    char buffer[bufferSize];
    DWORD bytesRead;

    // Read until there is no more data
    while (true) {
        BOOL success = ReadFile (hStdOutRead, buffer, bufferSize - 1, &bytesRead, nullptr);
        if (!success) {
            DWORD error = GetLastError();
            if (error == ERROR_BROKEN_PIPE) {
                // Child process has closed the pipe (normal termination)
                break;
            } else {
                LERROR << LOGNODE << "ReadFile failed with error " << error;
                break;
            }
        }
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            output += buffer;
        }
    }

    WaitForSingleObject (pi.hProcess, INFINITE);

    DWORD exitCode;
    if (!GetExitCodeProcess (pi.hProcess, &exitCode)) {
        LERROR << LOGNODE << "GetExitCodeProcess failed with error " << GetLastError();
        exitCode = 1;
    }

    CloseHandle (hStdOutRead);
    CloseHandle (pi.hProcess);
    CloseHandle (pi.hThread);

    return (exitCode == 0);
} // CommandLineNode::run_cmdexe


string
CommandLineNode::shell_expand (const string& input)
{
    // Perform variable expansion for use outside of CreateProcess
    std::string command = "echo " + input;
    string output;
    if (!run_cmdexe (command, output))
        return "";

    // Remove any trailing newline characters that echo adds
    if (!output.empty() && output.back() == '\n') {
        output.pop_back();
        if (!output.empty() && output.back() == '\r') {
            output.pop_back();
        }
    }

    return output;
} // CommandLineNode::shell_expand

#else
string
CommandLineNode::shell_expand (const string& input) const
{
    wordexp_t p;
    char** w;
    bool stat = false;

    setenv ("IFS", "", true);
    int ret = wordexp (("\"" + input + "\"").c_str(), &p, WRDE_SHOWERR);
    if (ret == WRDE_SYNTAX) {
        LERROR << LOGNODE << "Shell syntax error.";
    }
    else if (ret == WRDE_BADCHAR) {
        LERROR << LOGNODE
               << "Words argument contains unquoted characters: '\\n', ‘|’, ‘&’, ‘;’, ‘<’, "
                  "‘>’, ‘(’, ‘)’, ‘{’, ‘}’.";
    }
    else {
        stat = true;
    }

    if (!stat) {
        return "";
    }

    w = p.we_wordv;
    const string& output = w[0];

    wordfree (&p);

    return output;
} // parse_outputfile
#endif
} // namespace daisychain
