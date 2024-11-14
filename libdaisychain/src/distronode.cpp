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


DistroNode::DistroNode()
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
        while (eofs_ <= fd_in_.size() && !terminate_.load()) {
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
            if (ReadInputs (inputs) == -1) {
                eofs_ = fd_in_.size();
                break;
            }
        }

        CloseInputs();
    }

    OpenOutputs (sandbox);
    WriteOutputs ("EOF");
    CloseOutputs();
    Stats();
    Reset();

    return true;
} // DistroNode::Execute


void
DistroNode::OpenNextOutput (const string& sandbox)
{
#ifndef _WIN32
    if (output_it_ == outputs_.end()) {
        output_it_ = outputs_.begin();
    }

    auto fifo = *output_it_;
    string filepath = sandbox;

    filepath.append ("/");
    filepath.append (fifo);

    int fd = open (filepath.c_str(), O_WRONLY);

    if (fd == -1) {
        LERROR << LOGNODE << "Cannot open output for writing: " << filepath;
    }

    fd_out_[fifo] = fd;
#endif
} // DistroNode::OpenNextOutput


void
DistroNode::CloseNextOutput()
{
#ifndef _WIN32
    int stat = close (fd_out_[*output_it_]);

    if (stat == -1) {
        LERROR << LOGNODE << "Cannot close output file descriptor: " << *output_it_;
    }

    ++output_it_;
#endif
} // DistroNode::CloseNextOutput


#ifdef _WIN32
void
DistroNode::WriteNextOutput (const std::string& output) {
    std::string token = output + '\n';

    if (output_it_ == outputs_.end()) {
        output_it_ = outputs_.begin();
    }

    DWORD wrote = 0;
    size_t written = 0;
    HANDLE handle = fd_out_[*output_it_]; // Define handle outside of the loop for later use

    while (written < token.size() && !terminate_.load()) {
        handle = fd_out_[*output_it_];  // Update handle for the current output
        OVERLAPPED& overlapped_write = write_events_[std::distance(outputs_.begin(), output_it_)].overlapped;

        BOOL fSuccess = WriteFile(
            handle,
            token.data() + written,
            static_cast<DWORD>(token.size() - written),
            &wrote,
            &overlapped_write
        );

        if (!fSuccess) {
            DWORD error = GetLastError();
            if (error == ERROR_IO_PENDING) {
                // Wait for async write completion or termination event
                HANDLE events[] = { overlapped_write.hEvent, terminate_event_ };
                DWORD wait_result = WaitForMultipleObjects (2, events, FALSE, INFINITE);

                if (wait_result == WAIT_OBJECT_0 + 1) {  // Termination event signaled
                    LDEBUG << LOGNODE << "Termination event signaled, exiting WriteNextOutput.";
                    return;
                }

                BOOL overlapped_result = GetOverlappedResult (handle, &overlapped_write, &wrote, TRUE);
                if (!overlapped_result) {
                    DWORD overlapped_error = GetLastError();
                    LERROR << LOGNODE << "GetOverlappedResult failed with error: " << overlapped_error;
                    return;
                }
            } else if (error == ERROR_BROKEN_PIPE) {
                LERROR << LOGNODE << "Broken pipe for handle: " << handle;
                ++output_it_;
                if (output_it_ == outputs_.end()) {
                    output_it_ = outputs_.begin();
                }
                continue;
            } else {
                LERROR << LOGNODE << "WriteFile failed with error: " << error;
                return;
            }
        }

        // Update the written count and reset the event after a successful write
        written += wrote;
        totalbyteswritten_ += wrote;
        ResetEvent (overlapped_write.hEvent);

        // Advance iterator only after a successful write to distribute strings
        ++output_it_;
        if (output_it_ == outputs_.end()) {
            output_it_ = outputs_.begin();
        }
    }

    FlushFileBuffers(handle);
}


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

            if (numbytes) {
                token = output.substr (numbytes) + '\n';
            }
        } while (numbytes < tokensize && !terminate_.load());

        if (numbytes == tokensize) {
            return;
        }
    }
} // DistroNode::WriteAnyOutput

#else

void
DistroNode::WriteNextOutput (const string& output)
{
    string token = output + '\n';

    struct pollfd pfds[1];

    pfds[0].fd = fd_out_[*output_it_];
    pfds[0].events = POLLOUT;

    int ret;

    do {
        ret = (poll (pfds, 1, 2) && pfds[0].revents);

        if (ret > 0) {
            size_t numbytes;
            int tokensize;

            do {
                tokensize = int (token.size());
                numbytes = write (pfds[0].fd, token.c_str(), token.size());

                if (numbytes == -1) {
                    LDEBUG << LOGNODE << "Cannot write to file descriptor: " << *output_it_;
                    continue;
                }
                else if (numbytes == tokensize) {
                    pfds[0].events = 0;
                }
                else if (numbytes) {
                    token = output.substr (numbytes) + '\n';
                    LDEBUG << LOGNODE << "SPLIT token.";
                }
                else {
                    LDEBUG << LOGNODE << "ZERO bytes.";
                }

                totalbyteswritten_ += int (numbytes);
            } while (numbytes < tokensize);

            break;
        }
    } while (true);
} // DistroNode::WriteNextOutput


void
DistroNode::WriteAnyOutput (const string& output)
{
    string token = output + '\n';

    struct pollfd pfds[fd_out_.size()];

    int i = 0;

    for (const auto& fd : fd_out_) {
        pfds[i].fd = fd.second;
        pfds[i].events = POLLOUT;
        i++;
    }

    size_t numbytes;

    if (poll (pfds, fd_out_.size(), 2) > 0) {
        i = 0;

        for (const auto& fd : fd_out_) {
            if (pfds[i].revents) {
                do {
                    numbytes = write (fd.second, token.c_str(), token.size());

                    if (numbytes == -1) {
                        LERROR << LOGNODE << "Cannot write to file descriptor: " << fd.first;
                    }
                    else if (numbytes) {
                        token = output.substr (numbytes) + '\n';
                    }
                } while (numbytes != token.size());

                return;
            }

            i++;
        }
    }
} // DistroNode::WriteAnyOutput
#endif
} // namespace daisychain
