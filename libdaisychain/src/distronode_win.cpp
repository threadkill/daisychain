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

#include "distronode.h"


namespace daisychain {
using namespace std;


DistroNode::DistroNode() : Node()
{
    type_ = DaisyNodeType::DC_DISTRO;
    set_name (DaisyNodeNameByType[type_]);
}


bool
DistroNode::Execute (vector<string>& inputs, const string& sandbox, json& vars)
{
    LINFO << "Executing " << (isroot_ ? "root: " : "node: ") << name_;

    if (outputs_.empty()) {
        return true;
    }

    output_it_ = outputs_.begin();

    if (isroot_) {
        for (auto& input : inputs) {
            OpenNextOutput (sandbox);
            WriteNextOutput (input);
            CloseNextOutput();
        }
    }
    else {
        while (eofs_ <= fd_in_.size()) {
            for (auto& input : inputs) {
                if (input != "EOF") {
                    OpenNextOutput (sandbox);
                    WriteNextOutput (input);
                    CloseNextOutput();
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
} // DistroNode::Execute


void
DistroNode::OpenNextOutput (const string& sandbox)
{
    if (output_it_ == outputs_.end()) {
        output_it_ = outputs_.begin();
    }

    auto fifo = *output_it_;
    string sandbox_ = std::filesystem::path(sandbox).filename().string();
    string pipename = R"(\\.\pipe\)" + sandbox_ + "-" + fifo;

    HANDLE handle = CreateFile(
        pipename.c_str(), // Pipe name
        GENERIC_WRITE,    // Read and write access
        0,                // No sharing
        nullptr,          // Default security attributes
        OPEN_EXISTING,    // Opens existing pipe
        0,                // Default attributes
        nullptr           // No template file
    );

    // Check if the pipe was successfully opened
    if (handle == INVALID_HANDLE_VALUE) {
        LERROR << LOGNODE << "Cannot open output for writing: " << pipename;
        LERROR << LOGNODE << GetLastError();
        return;
    }

    auto connected = ConnectNamedPipe (handle, nullptr);

    if (!connected) {
        LERROR << LOGNODE << "Failed to connect to pipe: " << pipename;
        LERROR << LOGNODE << GetLastError();
        CloseHandle (handle);
        return;
    }

    fd_in_[fifo] = handle;
} // DistroNode::OpenNextOutput


void
DistroNode::WriteNextOutput (const string& output)
{
    string token = output + '\n';
    DWORD byteswritten = 0;
    BOOL ret = false;
    OVERLAPPED overlapped = { 0 };
    overlapped.hEvent = CreateEvent (nullptr, TRUE, FALSE, nullptr);
    auto handle = fd_out_[*output_it_];
    DWORD numbytes = 0;
    auto tokensize = static_cast<DWORD>(token.size());

    do {
        ret = WriteFile (handle, token.c_str(), tokensize, &numbytes, nullptr);

        if (!ret) {
            LERROR << LOGNODE << "Cannot write to file descriptor: " << *output_it_;
            continue;
        }
        if (numbytes) {
            token = output.substr (numbytes) + '\n';
        }

        byteswritten += numbytes;
        totalbyteswritten_ += numbytes;
    } while (numbytes < tokensize || (!ret && GetLastError() == ERROR_IO_PENDING));
} // DistroNode::WriteNextOutput


void
DistroNode::WriteAnyOutput (const string& output)
{
    string token = output + '\n';
    BOOL ret = false;
    OVERLAPPED overlapped = { 0 };
    overlapped.hEvent = CreateEvent (nullptr, TRUE, FALSE, nullptr);

    for (const auto& [path, handle] : fd_out_) {
        DWORD numbytes = 0;
        auto tokensize = static_cast<DWORD>(token.size());
        do {
            ret = WriteFile (handle, token.c_str(), tokensize, &numbytes, &overlapped);

            if (!ret && GetLastError() == ERROR_IO_PENDING) {
                break;
            }
            else if (numbytes) {
                token = output.substr (numbytes) + '\n';
            }
        } while (numbytes < tokensize);

        if (numbytes == tokensize) {
            return;
        }
    }
} // DistroNode::WriteAnyOutput


void
DistroNode::CloseNextOutput()
{
    if (const auto stat = CloseHandle (fd_out_[*output_it_]); !stat) {
        LERROR << LOGNODE << "Cannot close output file descriptor: " << *output_it_;
    }
    output_it_++;
} // DistroNode::CloseNextOutput


} // namespace daisychain
