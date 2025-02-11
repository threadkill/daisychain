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
        std::string stem = fs::path(input).stem().string();
        set_variable ("STEM", stem);
        std::string extension = fs::path(input).extension().string();
        set_variable ("EXT", extension);
    }

    if (!outputfile_.empty()) {
        if (outputfile_.find ("STDOUT") != std::string::npos) {
            use_std_out = true;
        }
        else {
            output = shell_expand (outputfile_);
            set_variable ("OUTPUT", output);
        }
    }

    if (test_) {
        LTEST << LOGNODE << "\n" << shell_expand (command_);
        OpenOutputs (sandbox);
        WriteOutputs (output);
        CloseOutputs();
        return true;
    }

    // Log the command line for debugging
    LDEBUG << LOGNODE << "Executing command line: " << shell_expand (command_);

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
                WriteOutputs (token);
            }
        } else {
            WriteOutputs (output);
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
    std::string pipename = R"(\\.\pipe\)" + name_ + "_" + id_;
    std::string cmd = "cmd.exe /C \"call " + command + "\"";

    STARTUPINFOEXA si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    ZeroMemory (&si, sizeof (si));
    ZeroMemory (&pi, sizeof (pi));

    sa.nLength = sizeof (SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE read_handle = CreateNamedPipeA (pipename.c_str(),
                                           PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
                                           PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                                           1,    // single instance
                                           8192, // out buffer size
                                           8192, // in buffer size
                                           0,    // default timeout
                                           &sa);

    if (read_handle == INVALID_HANDLE_VALUE) {
        LERROR << LOGNODE << "Failed to create named pipe for STDOUT. " << GetLastError();
        return false;
    }

    HANDLE write_handle = CreateFileA (pipename.c_str(),
                                       GENERIC_WRITE,
                                       0,
                                       &sa,
                                       OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                       nullptr);

    if (write_handle == INVALID_HANDLE_VALUE) {
        LERROR << LOGNODE << "Failed to open named pipe client handle for STDOUT. " << GetLastError();
        CloseHandle (read_handle);
        return false;
    }

    SIZE_T attr_list_size = 0;
    InitializeProcThreadAttributeList (nullptr, 1, 0, &attr_list_size);

    si.lpAttributeList = static_cast<LPPROC_THREAD_ATTRIBUTE_LIST> (HeapAlloc (GetProcessHeap(), 0, attr_list_size));
    if (!si.lpAttributeList) {
        CloseHandle (read_handle);
        CloseHandle (write_handle);
        return false;
    }

    if (!InitializeProcThreadAttributeList (si.lpAttributeList, 1, 0, &attr_list_size)) {
        HeapFree (GetProcessHeap(), 0, si.lpAttributeList);
        CloseHandle (read_handle);
        CloseHandle (write_handle);
        return false;
    }

    HANDLE inherited_handles[] = {write_handle};
    if (!UpdateProcThreadAttribute (si.lpAttributeList,
                                    0,
                                    PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                    inherited_handles,
                                    sizeof (inherited_handles),
                                    nullptr,
                                    nullptr)) {
        DeleteProcThreadAttributeList (si.lpAttributeList);
        HeapFree (GetProcessHeap(), 0, si.lpAttributeList);
        CloseHandle (read_handle);
        CloseHandle (write_handle);
        return false;
    }

    si.StartupInfo.cb = sizeof (STARTUPINFOEXA);
    si.StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
    si.StartupInfo.hStdOutput = write_handle;
    si.StartupInfo.hStdError = write_handle;
    si.StartupInfo.hStdInput = nullptr;

    auto env = get_environment();

    // only explicitly named handles are inherited.
    BOOL result = CreateProcessA (nullptr,
                                  &cmd[0], // command line
                                  nullptr,
                                  nullptr,
                                  TRUE, // bInheritHandles
                                  EXTENDED_STARTUPINFO_PRESENT | CREATE_NO_WINDOW,
                                  env.data(),
                                  nullptr,
                                  &si.StartupInfo,
                                  &pi);

    CloseHandle (write_handle);
    DeleteProcThreadAttributeList (si.lpAttributeList);
    HeapFree (GetProcessHeap(), 0, si.lpAttributeList);

    if (!result) {
        LERROR << LOGNODE << "CreateProcess failed with error " << GetLastError();
        CloseHandle (read_handle);
        return false;
    }

    HANDLE read_event = CreateEvent (nullptr, TRUE, FALSE, nullptr);
    if (!read_event) {
        LERROR << LOGNODE << "CreateEvent for read failed: " << GetLastError();
        CloseHandle (pi.hProcess);
        CloseHandle (pi.hThread);
        CloseHandle (read_handle);
        return false;
    }

    const DWORD buffersize = 8192;
    char buffer[buffersize];
    DWORD bytesread = 0;
    HANDLE read_wait_handles[2] = {read_event, terminate_event_};
    bool finished = false;

    while (!finished) {
        OVERLAPPED ol;
        ZeroMemory (&ol, sizeof (ol));
        ol.hEvent = read_event;

        if (!ReadFile (read_handle, buffer, buffersize - 1, &bytesread, &ol)) {
            DWORD read_error = GetLastError();
            if (read_error == ERROR_IO_PENDING) {
                DWORD read_wait_result = WaitForMultipleObjects (2, read_wait_handles, FALSE, INFINITE);

                switch (read_wait_result) {
                    case WAIT_OBJECT_0:
                        if (!GetOverlappedResult (read_handle, &ol, &bytesread, FALSE)) {
                            DWORD error = GetLastError();
                            if (error != ERROR_BROKEN_PIPE) {
                                LERROR << LOGNODE << "GetOverlappedResult failed with error " << error;
                            }
                            finished = true;
                        }
                        else {
                            if (bytesread == 0) {
                                finished = true;
                            }
                            else {
                                buffer[bytesread] = '\0';
                                output += buffer;
                            }
                        }
                        break;
                    case WAIT_OBJECT_0 + 1: // terminate event
                        finished = true;
                        break;
                    default:
                        LERROR << LOGNODE << "WaitForMultipleObjects(read) failed with error " << GetLastError();
                        finished = true;
                        break;
                }
            }
            else if (read_error == ERROR_BROKEN_PIPE) { // normal termination. not an error.
                finished = true;
            }
            else {
                LERROR << LOGNODE << "ReadFile failed with error " << read_error;
                finished = true;
            }
        }
        else {
            if (bytesread == 0) {
                finished = true;
            }
            else {
                buffer[bytesread] = '\0';
                output += buffer;
            }
        }

        ResetEvent (read_event);
    }

    CloseHandle (read_event);
    HANDLE process_wait_handles[2] = {pi.hProcess, terminate_event_};

    DWORD process_wait_result = WaitForMultipleObjects (2, process_wait_handles, FALSE, INFINITE);

    switch (process_wait_result) {
        case WAIT_OBJECT_0:     // normal event
            break;
        case WAIT_OBJECT_0 + 1: // terminate event
            TerminateProcess (pi.hProcess, 1);
            WaitForSingleObject (pi.hProcess, INFINITE);
            break;
        default:
            LERROR << LOGNODE << "WaitForMultipleObjects(proc) failed with error " << GetLastError();
            break;
    }

    DWORD exitcode;
    if (!GetExitCodeProcess (pi.hProcess, &exitcode)) {
        LERROR << LOGNODE << "GetExitCodeProcess failed with error " << GetLastError();
        exitcode = 1;
    }

    CloseHandle (read_handle);
    CloseHandle (pi.hProcess);
    CloseHandle (pi.hThread);

    return (exitcode == 0);
}


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
CommandLineNode::shell_expand (const string& input)
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
