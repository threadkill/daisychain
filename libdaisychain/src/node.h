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

#include <algorithm>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <list>
#include <string>
#include <sys/poll.h>
#include <utility>
#include <vector>
#include <wordexp.h>

#include "logger.h"
#include "utils.h"
#include <nlohmann/json.hpp>


namespace daisychain {
using namespace std;
using json = nlohmann::ordered_json;


enum DaisyNodeType : short {
    DC_INVALID = -1,
    DC_COMMANDLINE,
    DC_REMOTE,
    DC_FILTER,
    DC_CONCAT,
    DC_DISTRO,
    DC_FILELIST,
    DC_WATCH
};

static std::map<short, std::string> DaisyNodeNameByType = {
    {DC_COMMANDLINE,  "command"},
    {     DC_REMOTE,   "remote"},
    {     DC_FILTER,   "filter"},
    {     DC_CONCAT,   "concat"},
    {     DC_DISTRO,   "distro"},
    {   DC_FILELIST, "filelist"},
    {      DC_WATCH,    "watch"}
};

NLOHMANN_JSON_SERIALIZE_ENUM
(DaisyNodeType,
{
    {    DC_INVALID,    nullptr},
    {DC_COMMANDLINE,  "command"},
    {     DC_REMOTE,   "remote"},
    {     DC_FILTER,   "filter"},
    {     DC_CONCAT,   "concat"},
    {     DC_DISTRO,   "distro"},
    {   DC_FILELIST, "filelist"},
    {      DC_WATCH,    "watch"}
})


class Node
{
public:
    Node() :
        id_ (m_gen_uuid()),
        type_ (DC_INVALID),
        position_ (std::pair<float, float> (0.0, 0.0)),
        size_ (std::pair<int, int> (0, 0)),
        isroot_ (true),
        batch_ (false),
        test_ (false),
        eofs_ (0)
    {
        totalbytesread_ = 0;
        totalbyteswritten_ = 0;
    }


    virtual ~Node()
    {
        Node::Cleanup();
        LDEBUG << LOGNODE << "destroyed.";
    }


    virtual void
    Initialize (json& keydata, bool keep_uuid)
    {
        json::iterator jit = keydata.begin();
        const auto& uuid = jit.key();
        auto data = keydata[uuid];

        if (keep_uuid) {
            set_id (uuid);
        }

        if (data.count ("name") && !data["name"].get<string>().empty()) {
            set_name (data["name"]);
        }
        else {
            set_name (DaisyNodeNameByType[type_]);
        }

        if (data.count ("position")) {
            set_position (std::pair<float, float> (data["position"][0], data["position"][1]));
        }
        if (data.count ("size")) {
            set_size (std::pair<int, int> (data["size"][0], data["size"][1]));
        }
    }


    virtual json Serialize()
    {
        json json_ = {
            {id_,
             {{"type", type_},
              {"name", name_},
              {"position", position_}}}
        };

        return json_;
    }


    virtual bool Execute (const string& sandbox, json& env)
    {
        std::vector<string> inputs;
        int eof = 0;

        OpenInputs (sandbox);

        do {
            eof = ReadInputs (inputs);
        } while (batch_ && eof != -1);

        if (batch_) {
            concat_inputs (inputs);
        }

        return Execute (inputs, sandbox, env);
    } // Execute


    virtual bool Execute (vector<string>& inputs, const string& sandbox, json& vars) = 0;

    virtual void Stats()
    {
        LDEBUG << LOGNODE << "total bytes read: " << totalbytesread_;
        LDEBUG << LOGNODE << "total bytes written: " << totalbyteswritten_;
    } // Stats


    void AddInput (const string& fifo)
    {
        inputs_.push_back (fifo);
        isroot_ = false;
    } // AddInput


    void RemoveInput (const string& fifo)
    {
        inputs_.remove (fifo);

        if (inputs_.empty()) {
            isroot_ = true;
        }
    } // RemoveInput


    void AddOutput (const string& fifo) { outputs_.push_back (fifo); }

    void RemoveOutput (const string& fifo) { outputs_.remove (fifo); }

    void OpenInputs (const string& sandbox)
    {
        for (const auto& fifo : inputs_) {
            string filepath = sandbox;
            filepath.append ("/");
            filepath.append (fifo);
            int fd = open (filepath.c_str(), O_RDWR | O_NONBLOCK);

            if (fd == -1) {
                LERROR << LOGNODE << "Cannot open fifo for reading: " << filepath;
                continue;
            }

            fd_in_[fifo] = fd;
        }
    } // OpenInputs


    int ReadInputs (vector<string>& inputs)
    {
        struct pollfd pfds[fd_in_.size()];

        int i = 0;

        for (const auto& fd : fd_in_) {
            pfds[i].fd = fd.second;
            pfds[i].events = POLLIN;
            i++;
        }

        uint32_t BUFFSIZE = 8192;
        char cbuffer[BUFFSIZE + 1];
        string input;

        auto ret = poll (pfds, fd_in_.size(), 2);

        if (ret > 0) {
            i = 0;

            for (const auto& fd : fd_in_) {
                if (pfds[i].revents) {
                    ssize_t numbytes = 0;

                    do {
                        numbytes = read (fd.second, cbuffer, BUFFSIZE);
                        if (numbytes > 0) {
                            cbuffer[numbytes] = '\0';
                            input += cbuffer;
                            totalbytesread_ += numbytes;
                        }
                    } while (numbytes > 0 || (numbytes == -1 && errno == EINTR));
                }

                i++;
            }

            m_split_input (input, inputs);
        }

        auto count = std::count (inputs.begin(), inputs.end(), "EOF");

        if (count) {
            eofs_ += int (count);
            LDEBUG << LOGNODE << "EOF COUNT: " << eofs_;
        }

        return (eofs_ == fd_in_.size()) ? -1 : eofs_;
    } // ReadInputs


    void CloseInputs()
    {
        for (const auto& fd : fd_in_) {
            int stat = close (fd.second);

            if (stat == -1) {
                LERROR << LOGNODE << "Cannot close input file descriptor: " << fd.first;
                continue;
            }
        }
    } // CloseInputs


    void OpenOutputs (const string& sandbox)
    {
        for (const auto& fifo : outputs_) {
            string filepath = sandbox;
            filepath.append ("/");
            filepath.append (fifo);

            int fd = open (filepath.c_str(), O_WRONLY);

            if (fd == -1) {
                LERROR << LOGNODE << "Cannot open output for writing: " << filepath;
                continue;
            }

            fd_out_[fifo] = fd;
        }
    } // OpenOutputs


    virtual void WriteOutputs (const string& output)
    {
        string token = output + '\n';

        struct pollfd pfds[fd_out_.size()];

        int i = 0;

        for (const auto& fd : fd_out_) {
            pfds[i].fd = fd.second;
            pfds[i].events = POLLOUT;
            i++;
        }

        size_t byteswritten = 0;
        auto totalbytes = token.size() * fd_out_.size();
        int ret = 0;

        do {
            ret = poll (pfds, fd_out_.size(), 2);

            if (ret > 0) {
                i = 0;

                for (const auto& fd : fd_out_) {
                    if (pfds[i].revents) {
                        size_t numbytes = 0;
                        size_t tokensize = 0;

                        do {
                            tokensize = token.size();
                            numbytes = write (fd.second, token.c_str(), token.size());

                            if (numbytes == -1) {
                                LERROR << LOGNODE << "Cannot write to file descriptor: " << fd.first;
                                continue;
                            }
                            else if (numbytes == tokensize) {
                                pfds[i].events = 0;
                            }
                            else if (numbytes) {
                                token = output.substr (numbytes) + '\n';
                            }

                            byteswritten += numbytes;
                            totalbyteswritten_ += numbytes;
                        } while (numbytes < tokensize || (numbytes == -1 && errno == EINTR));
                    }

                    i++;
                }
            }
        } while (byteswritten < totalbytes);
    } // WriteOutputs


    void CloseOutputs()
    {
        for (const auto& fd : fd_out_) {
            int stat = close (fd.second);

            if (stat == -1) {
                LERROR << LOGNODE << "Cannot close output file descriptor: " << fd.first;
                continue;
            }
        }
    } // CloseOutputs


    virtual void Cleanup()
    {
        CloseOutputs();
        CloseInputs();
    } // Cleanup


    DaisyNodeType type() { return type_; }

    void set_id() { id_ = m_gen_uuid(); }
    void set_id (const string& id) { id_ = id; }
    string id() { return id_; }

    void set_name (const string& name) { name_ = name; }
    string name() { return name_; }

    void set_position (std::pair<float, float> position) { position_ = position; }
    std::pair<float, float> position() { return position_; }

    void set_size (std::pair<int, int> size) { size_ = size; }
    std::pair<int, int> size() { return size_; }

    [[nodiscard]] bool is_root() const { return isroot_; }

    void set_batch_flag (bool batch) { batch_ = batch; }
    [[nodiscard]] bool batch_flag() const { return batch_; }

    void set_test_flag (bool test) { test_ = test; }
    [[nodiscard]] bool test_flag() const { return test_; }

    void set_outputfile (const string& output) { outputfile_ = output; }
    string outputfile() { return outputfile_; }

    int input_index (const string& id)
    {
        int idx = 0;

        for (const auto& input : inputs_) {
            if (input == id) {
                break;
            }
            idx++;
        }

        return idx;
    } // input_index


    std::map<string, vector<unsigned int>>
    input_indices()
    {
        int idx = 0;
        std::map<string, vector<unsigned int>> indices;

        for (const auto& input : inputs_) {
            if (!indices.count (input)) {
                indices[input];
            }
            indices[input].push_back (idx);
            idx++;
        }

        return indices;
    }


    string shell_expand (const string& input)
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


    static void concat_inputs (vector<string>& inputs)
    {
        // drop EOF and concatenate inputs into a newline-separated string.
        string input = m_join_if (inputs, "\n", [] (const std::string& s) { return (s != "EOF"); });

        // still using the vector, but now there's only a single element with combined string.
        inputs.clear();
        if (!input.empty()) {
            inputs.emplace_back (input);
        }
        inputs.emplace_back ("EOF");
    } // concat_inputs

protected:
    string id_;
    string name_;
    std::pair<float, float> position_;
    std::pair<int, int> size_;
    DaisyNodeType type_;
    bool batch_;
    bool test_;
    string outputfile_;

    bool isroot_;
    list<string> inputs_;
    list<string> outputs_;
    map<const string, int> fd_in_;
    map<const string, int> fd_out_;
    int eofs_;
    size_t totalbytesread_;
    size_t totalbyteswritten_;
};
} // namespace daisychain
