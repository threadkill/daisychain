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

#include "graph.h"
#include <ftw.h>
#include <iomanip>
#include <sys/stat.h>


namespace daisychain {


Graph::Graph() : running_ (false), cleanup_ (true), test_ (false) { Initialize(); }


Graph::Graph (const string& filename) : filename_ (filename), running_ (false), cleanup_ (true), test_ (false)
{
    Initialize (filename);
}


Graph::~Graph()
{
    if (cleanup_) {
        Cleanup();
    }
    LDEBUG << "Graph destroyed: " << filename_;
}


bool
Graph::Initialize (const string& filename)
{
    filename_ = filename;

    input_.clear();
    environ_.clear();
    notes_.clear();
    test_ = false;
    nodes_.clear();
    edges_.clear();
    ordered_.clear();
    adjacencylist_.clear();
    process_group_ = 0;

    if (!filename_.empty()) {
        LINFO <<  "Initializing graph from file: " << filename_;

        std::ifstream filein (filename_);

        if (!filein.is_open()) {
            LERROR << "Failed to open graph file: " << filename_;

            return false;
        }

        json json_graph;

        filein >> json_graph;
        filein.close();
        Parse (json_graph);
    }

    return true;
} // Graph::Initialize


json
Graph::Serialize()
{
    json data;

    for (const auto& node : nodes_) {
        data["nodes"][node.first] = node.second->Serialize()[node.first];
    }

    for (const auto& edge : edges_) {
        data["connections"] += {edge.first, edge.second};
    }

    data["environment"] = environ_;

    data["notes"] = notes_;

    return data;
} // Graph::Serialize


bool
Graph::Parse (const json& json_graph)
{
    std::unordered_map<string, string> oldkeynew;

    if (json_graph.contains ("nodes")) {
        for (auto& [key, data] : json_graph["nodes"].items()) {
            string uuid = key;

            // avoid collisions between copied nodes (e.g. importing into opened graph).
            if (nodes_.count (key)) {
                uuid = m_gen_uuid();
                oldkeynew[key] = uuid;
            }

            json newdata = {
                {uuid, data}
            };
            auto node = CreateNode (newdata, true);
            AddNode (node);
        }
    }

    if (json_graph.contains ("connections")) {
        for (auto& connection : json_graph["connections"]) {
            string output = connection[0];
            string input = connection[1];

            if (oldkeynew.count (connection[0])) {
                output = oldkeynew.at (connection[0]);
            }

            if (oldkeynew.count (connection[1])) {
                input = oldkeynew.at (connection[1]);
            }

            Connect (output, input);
        }
    }

    if (json_graph.contains ("environment")) {
        for (auto& [key, data] : json_graph["environment"].items()) {
            if (!environ_.contains (key)) {
                environ_[key] = data;
            }
        }
    }

    if (json_graph.contains ("notes")) {
        for (auto& [key, data] : json_graph["notes"].items()) {
            if (!notes_.contains (key)) {
                notes_[key] = data;
            }
        }
    }

    return true;
} // Graph::Read


bool
Graph::Save (const string& filename)
{
    bool stat = true;
    std::ofstream fileout (filename);

    fileout << std::setw (4) << Serialize() << std::endl;

    if (fileout.bad()) {
        stat = false;
    }

    if (stat) {
        LINFO << "Saved graph to file: " << filename;
        filename_ = filename;
    }
    else {
        LERROR << "Save Failed: " << filename;
    }

    return stat;
} // Graph::Save


bool
Graph::PrepareFileSystem()
{
    bool status = true;
    struct stat ss{};

    if (sandbox_.empty()) {
        char temp[] = "/tmp/daisy-XXXXXX";
        sandbox_ = mkdtemp (temp);

        if (sandbox_.empty()) {
            LERROR << "Temp directory creation failed.";
            status = false;
        }
    }
    else if (stat (sandbox_.c_str(), &ss) != 0) {
        int ret = mkdir (sandbox_.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

        if (ret != 0) {
            LERROR << "Cannot create directory: " + sandbox_;
            status = false;
        }
    }

    if (status) {
        for (const auto& edge : edges_) {
            std::string filepath = sandbox_ + "/" + edge.first + "." + edge.second;

            if (stat (filepath.c_str(), &ss) != 0) {
                int ret = mkfifo (filepath.c_str(), S_IRUSR | S_IWUSR | S_IWGRP);

                if (ret != 0) {
                    LERROR << "Cannot create FIFO: " + filepath;
                    status = false;
                }
            }
        }
    }

    return status;
} // Graph::PrepareFileSystem


bool
Graph::Execute()
{
    return Execute (input_, environ_);
} // Graph::Execute


bool
Graph::Execute (const string& input)
{
    return Execute (input, environ_);
} // Graph::Execute


bool
Graph::Execute (const string& input, json& env)
{
    TIMED_FUNC (timerObj);
    setbuf (stdout, nullptr);

    if (!PrepareFileSystem()) {
        return false;
    }

    json merged_env = environ_;
    if (!env.empty()) {
        merged_env.merge_patch (env);
    }
    LDEBUG_IF (!merged_env.empty()) << "Environment variables:\n" << merged_env.dump (4) << "\n";

    vector<string> inputs;
    m_split_input (input, inputs);

    sort_();

    process_group_ = 0;
    running_ = true;

    pid_t group_pid = fork();

    if (group_pid == 0) {
        // Process leader for the group.
        LDEBUG << "Order of execution:";
        for (const auto& uuid : ordered_) {
            LDEBUG << uuid << " - " << nodes_[uuid]->name();
        }
        for (const auto& uuid : ordered_) {

            // Create child processes in a loop.
            pid_t child_pid = fork();

            switch (child_pid) {
            case -1: // fork() failed.
                LERROR << "Exit ... (" << child_pid << ")";
                ::_exit (child_pid);
            case 0: // the child of the fork()
            {
                // get parent's pid.
                auto ppid_ = getppid();
                // set process group ID to parent's pid.
                auto result = setpgid (0, ppid_);
                LERROR_IF (result != 0) << "Set Process Group ID failed. (" << result << ")";

                bool stat = false;

                if (nodes_[uuid]->is_root()) {
                    // root nodes receive initial input.
                    stat = nodes_[uuid]->Execute (inputs, sandbox_, merged_env);
                }
                else {
                    stat = nodes_[uuid]->Execute (sandbox_, merged_env);
                }

                LINFO_IF (stat) << "<" << nodes_[uuid]->name() << "> Finished.";
                LERROR_IF (!stat) << "<" << nodes_[uuid]->name() << "> Failed.";

                ::_exit (stat ? 0 : -1);
            }
            default: // parent of the fork();
                LDEBUG << "<" << nodes_[uuid]->name() << "> fork (pid:" << child_pid << ")";
                break;
            } // switch
        }

        wait_();
        ::_exit (0);
    }
    else if (group_pid > 0) {
        // set process group ID for parent process.
        auto result = setpgid (group_pid, group_pid);
        LERROR_IF (result != 0) << "Set Process Group ID failed. (" << result << ")";
        LDEBUG_IF (result == 0) << "Process Group ID:" << group_pid;
        process_group_ = group_pid;

        // CTRL-C
        sigint_handler = [&] (int signal) { Terminate(); };
        signal (SIGINT, signal_handler);
    }

    // Waiting on first fork.
    wait_ (group_pid);
    LINFO_IF (!test_) << "Graph execution finished.";
    LINFO_IF (test_) << "Graph test finished.";

    process_group_ = 0;
    running_ = false;

    return true;
} // Graph::Execute


bool
Graph::Execute (const string& input, const string& node_name)
{
    return false;
} // Graph::Execute


bool
Graph::Test()
{
    set_test_flag (true);
    auto stat = Execute();
    set_test_flag (false);

    return stat;
} // Graph::Execute


void
Graph::Terminate()
{
    if (!process_group_ && !running_)
        return;

    auto result = killpg (process_group_, SIGTERM);
    if (result == 0) {
        LWARN << " !!! Terminated !!! (" << process_group_ << ")";
        running_ = false;
        process_group_ = 0;
    }
    else {
        switch (errno) {
        case EINVAL:
            LERROR << "Terminate process group failed for ("
                   << process_group_
                   << "): Invalid signal";
            break;
        case EPERM:
            LERROR << "Terminate process group failed for ("
                   << process_group_
                   << "): Sending user is not the super user";
            break;
        case ESRCH:
            LERROR << "Terminate process group failed for ("
                   << process_group_
                   << "): No process can be found in the process group";
            break;
        }
    }
}


bool
Graph::Cleanup()
{
    if (sandbox_.empty()) {
        return true;
    }

    auto rmdirtree = [] (const char* path, const struct stat* buf, int type, struct FTW* ftwb) {
        int stat = std::remove (path);
        stat < 0 ? LERROR << "Could not remove: " << path : LDEBUG << "Removed: " << path;

        return stat < 0 ? -1 : 0;
    };

    int status = nftw (sandbox_.c_str(), rmdirtree, 10, FTW_DEPTH | FTW_MOUNT | FTW_PHYS);
    status == 0 ? LINFO << "Cleanup finished: " << sandbox_ : LERROR << "Cleanup failed: " << sandbox_;

    return status == 0;
}


void
Graph::Print()
{
    json data = Serialize();
    LINFO << "Printing graph ...\n" << data.dump (4);
} // Graph::Print


std::shared_ptr<Node>
Graph::CreateNode (json& keydata, bool keep_uuid)
{
    std::shared_ptr<Node> node;
    json::iterator jit = keydata.begin();

    auto type_ = jit.value()["type"].get<DaisyNodeType>();

    if (type_ == DaisyNodeType::DC_COMMANDLINE) {
        node = std::make_shared<CommandLineNode>();
        node->Initialize (keydata, keep_uuid);
    }
    else if (type_ == DaisyNodeType::DC_FILTER) {
        node = std::make_shared<FilterNode>();
        node->Initialize (keydata, keep_uuid);
    }
    else if (type_ == DaisyNodeType::DC_CONCAT) {
        node = std::make_shared<ConcatNode>();
        node->Initialize (keydata, keep_uuid);
    }
    else if (type_ == DaisyNodeType::DC_DISTRO) {
        node = std::make_shared<DistroNode>();
        node->Initialize (keydata, keep_uuid);
    }
    else if (type_ == DaisyNodeType::DC_FILELIST) {
        node = std::make_shared<FileListNode>();
        node->Initialize (keydata, keep_uuid);
    }
    else if (type_ == DaisyNodeType::DC_WATCH) {
        node = std::make_shared<WatchNode>();
        node->Initialize (keydata, keep_uuid);
    }
    else {
        LERROR << "Unknown type." << jit.value()["type"];
    }

    return node;
}


bool
Graph::AddNode (const std::shared_ptr<Node>& node)
{
    bool stat = true;

    if (node->id().empty()) {
        stat = false;
    }

    if (stat) {
        nodes_[node->id()] = node;
        adjacencylist_[node->id()];
        LDEBUG << "Added " << DaisyNodeNameByType[node->type()] << " node: " << node->name();
    }
    else {
        LERROR << "Node insertion failed for node: " << node->name();
    }

    return stat;
} // Graph::AddNode


bool
Graph::RemoveNode (const string& id)
{
    bool stat = true;

    try {
        for (auto& e: adjacencylist_) {
            e.second.erase (std::remove (e.second.begin(), e.second.end(), id), e.second.end());
        }

        adjacencylist_.erase (id);
        nodes_.erase (id);

        LDEBUG << "Num verts: " << nodes_.size();
    }
    catch (...) {
        stat = false;
    }

    return stat;
} // Graph::RemoveNode


bool
Graph::Connect (const string& parentid, const string& childid)
{
    Edge edge = {parentid, childid};

    edges_.push_back (edge);
    nodes_[edge.first]->AddOutput (edge.first + "." + edge.second);
    nodes_[edge.second]->AddInput (edge.first + "." + edge.second);
    adjacencylist_[parentid].push_back (childid);

    // cycle detection
    bool has_cycle = detect_cycle();

    if (has_cycle) {
        LERROR << "Cycle detected";
        LERROR << "Connection failed: " << edge.first << "." << edge.second;
        Disconnect (parentid, childid);
    }
    else {
        LDEBUG << "Connected: " << edge.first << "." << edge.second;
    }

    return (!has_cycle);
} // Graph::Connect


bool
Graph::Disconnect (const string& parentid, const string& childid)
{
    bool stat = true;

    try {
        Edge edge = {parentid, childid};

        edges_.remove (edge);

        if (nodes_.count (edge.first)) {
            nodes_[edge.first]->RemoveOutput (edge.first + "." + edge.second);
        }

        if (nodes_.count (edge.second)) {
            nodes_[edge.second]->RemoveInput (edge.first + "." + edge.second);
        }

        auto it = adjacencylist_.find (parentid);
        if (it != adjacencylist_.end()) {
            auto& nodes = it->second;
            nodes.erase (std::remove (nodes.begin(), nodes.end(), childid), nodes.end());
        }

        LDEBUG << "Disconnected: " << edge.first << "." << edge.second;
        LDEBUG << "Num edges: " << edges_.size();
    }
    catch (...) {
        stat = false;
    }

    return stat;
} // Graph::Disconnect


std::shared_ptr<Node>
Graph::get_node (const string& id)
{
    return nodes_[id];
}


void
Graph::set_filename (const string& filename)
{
    filename_ = filename;
    LDEBUG_IF (!filename.empty()) << "Filename set to: " << filename_;
} // Graph::set_filename


string
Graph::filename()
{
    return filename_;
} // Graph::filename


void
Graph::set_sandbox (const string& directory)
{
    sandbox_ = directory;
    LDEBUG << "Sandbox set to: " << sandbox_;
} // Graph::set_sandbox


string
Graph::sandbox()
{
    return sandbox_;
} // Graph::sandbox


void
Graph::set_input (const string& input)
{
    input_ = input;
    LINFO_IF (!input_.empty()) << "Input set.";
} // Graph::set_input


string&
Graph::input()
{
    return input_;
} // Graph::input


void
Graph::set_environ (json& env)
{
    environ_ = env;
} // Graph::set_environ


json
Graph::environ()
{
    return environ_;
} // Graph::environ


void
Graph::set_notes (json& notes)
{
    notes_ = notes;
} // Graph::set_notes


json
Graph::notes()
{
    return notes_;
} // Graph::notes


void
Graph::set_cleanup_flag (bool cleanup)
{
    cleanup_ = cleanup;
} // Graph::set_cleanup


bool
Graph::cleanup_flag() const
{
    return cleanup_;
} // Graph::cleanup


void
Graph::set_test_flag (bool test)
{
    test_ = test;
    for (const auto& node: nodes_) {
        node.second->set_test_flag (test);
    }
}


bool
Graph::test_flag() const
{
    return test_;
}


const map<string, std::shared_ptr<Node>>&
Graph::nodes()
{
    return nodes_;
} // Graph::nodes


const list<Edge>&
Graph::edges()
{
    return edges_;
} // Graph::edges


} // namespace daisychain
