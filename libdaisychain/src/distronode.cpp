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
    string filepath = sandbox;

    filepath.append ("/");
    filepath.append (fifo);

    int fd = open (filepath.c_str(), O_WRONLY);

    if (fd == -1) {
        LERROR << LOGNODE << "Cannot open output for writing: " << filepath;
    }

    fd_out_[fifo] = fd;
} // DistroNode::OpenNextOutput


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


void
DistroNode::CloseNextOutput()
{
    int stat = close (fd_out_[*output_it_]);

    if (stat == -1) {
        LERROR << LOGNODE << "Cannot close output file descriptor: " << *output_it_;
    }

    output_it_++;
} // DistroNode::CloseNextOutput


} // namespace daisychain
