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

#include "logger.h"
#include "utils.h"

#include <nlohmann/json.hpp>


namespace daisychain {
using namespace std;
using json = nlohmann::ordered_json;


class Graph;


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
    explicit Node (Graph*);
    virtual ~Node();

    virtual void Initialize (json&, bool);

    virtual json Serialize();

#ifdef _WIN32
    void Start (const string&, json&, const string&);

    void Start (vector<string>&, const string&, json&, const string&);

    void Join();

    virtual void Stop();
#endif

    virtual bool Execute (const string&, json&);

    virtual bool Execute (vector<string>&, const string&, json&) = 0;

    virtual void Stats();

    void AddInput (const string&);

    void RemoveInput (const string&);

    void AddOutput (const string& fifo) { outputs_.push_back (fifo); }

    void RemoveOutput (const string& fifo) { outputs_.remove (fifo); }

    void OpenInputs (const string&);

    void CloseInputs();

    void OpenOutputs (const string&);

    void CloseOutputs();

    void OpenWindowsPipes (const string&);

    void CloseWindowsPipes();

    int ReadInputs (std::vector<std::string>&);

    virtual void WriteOutputs (const std::string&);

    virtual void Cleanup();

    virtual void Reset();

    DaisyNodeType type() const { return type_; }

    void set_id() { id_ = m_gen_uuid(); }
    void set_id (const string& id) { id_ = id; }
    string id() { return id_; }

    void set_name (const string& name) { name_ = name; }
    string name() { return name_; }

    void set_position (std::pair<float, float> position) { position_ = position; }
    std::pair<float, float> position() const { return position_; }

    void set_size (std::pair<int, int> size) { size_ = size; }
    std::pair<int, int> size() const { return size_; }

    [[nodiscard]] bool is_root() const { return isroot_; }

    void set_batch_flag (const bool batch) { batch_ = batch; }
    [[nodiscard]] bool batch_flag() const { return batch_; }

    void set_test_flag (const bool test) { test_ = test; }
    [[nodiscard]] bool test_flag() const { return test_; }

    void set_outputfile (const string& output) { outputfile_ = output; }
    string outputfile() { return outputfile_; }

    int input_index (const string&) const;

    std::map<string, vector<unsigned int>> input_indices() const;

    string shell_expand (const string&) const;

#ifdef _WIN32
    static string get_pipename (const string& prefix, const string& uuidpair)
    {
        const string sandbox_ = std::filesystem::path (prefix).filename().string();
        return R"(\\.\pipe\)" + sandbox_ + "-" + uuidpair;
    }
#endif

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
    Graph* graph{};
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
    atomic<bool> terminate_;

#ifdef _WIN32
    struct PipeInfo {
        HANDLE handle{};
        OVERLAPPED overlapped{};
        HANDLE event{};
        std::string partialMessage;
    };

    std::vector<PipeInfo> pipeinfos_;
    std::vector<OVERLAPPED> overlapped_writes_;
    std::vector<HANDLE> read_events_;
    std::vector<HANDLE> write_events_;
    HANDLE terminate_event_{};

    map<const string, HANDLE> fd_in_;
    map<const string, HANDLE> fd_out_;
    std::thread thread_;
#else
    map<const string, int> fd_in_;
    map<const string, int> fd_out_;
#endif

    int eofs_;
    size_t totalbytesread_;
    size_t totalbyteswritten_;
};
} // namespace daisychain
