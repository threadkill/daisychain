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
#include <cstdio>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <string>
#include <stack>
#include <set>
#ifndef _WIN32
#include <sys/wait.h>
#endif

#include "logger.h"
#include "node.h"
#include "signalhandler.h"
#include "commandlinenode.h"
#include "concatnode.h"
#include "distronode.h"
#include "filelistnode.h"
#include "filternode.h"
#include "remotenode.h"
#include "watchnode.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif // if HAVE_CONFIG_H

#ifndef DAISYCHAIN_VERSION
#define DAISYCHAIN_VERSION "0.0.0"
#endif

using std::string;

namespace daisychain {
using Edge = pair<string, string>;


class Graph
{
public:
    Graph();
    explicit Graph (const string& filename);
    ~Graph();

    bool Initialize (const string& filename= "");

    json Serialize();

    bool Parse (const json& json_graph);

    bool Save (const string& filename="");

    bool PrepareFileSystem();

    bool Execute();

    bool Execute (const string& input);

    bool Execute (const string& input, json& env);

    bool Execute (const string& input, const string& node_name);

    bool Test();

    void Terminate();

    bool Cleanup();

    void Print();

    static std::shared_ptr<Node> CreateNode (json& data, bool keep_uuid);

    bool AddNode (const std::shared_ptr<Node>& node);

    bool RemoveNode (const string& id);

    bool Connect (const string& parent, const string& child);

    bool Disconnect (const string& parent, const string& child);

    std::shared_ptr<Node> get_node (const string& id);

    void set_filename (const string& filename);

    string filename();

    void set_sandbox (const string& directory);

    string sandbox();

    void set_environment (json& env);

    json environment();

    void set_notes (json& notes);

    json notes();

    void set_input (const string& input);

    string& input();

    void set_cleanup_flag (bool cleanup);

    bool cleanup_flag() const;

    void set_test_flag (bool test);

    bool test_flag() const;

    [[nodiscard]] string logfile() const { return sandbox_ + ".log"; }

    [[nodiscard]] bool running() const { return running_; }

    const map<string, std::shared_ptr<Node>>& nodes();

    const list<Edge>& edges();

private:
    string filename_;
    string sandbox_;
    string input_;
    json environment_;
    json notes_;
    bool running_;
    bool cleanup_;
    bool test_;

    map<string, std::shared_ptr<Node>> nodes_;
    list<Edge> edges_;
    std::unordered_map<string, vector<string>> adjacencylist_;
    vector<string> ordered_;

#ifndef _WIN32
    pid_t process_group_{};


    static inline void wait_ (pid_t pid = -1)
    {
        pid_t wpid;
        int status = 0;
        while ((wpid = waitpid (pid, &status, 0)) > 0);
        if (WIFSIGNALED (status)) {
            LWARN << "Process terminated via signal.";
        }
        else if (!WIFEXITED (status)) {
            LERROR << "Process failed (non-zero exit()). " << wpid;
        }
    }
#endif

    void sort_visitor_ (const string& v, std::set<string>& visited, std::stack<string>& stacked)
    {
        visited.insert (v);

        for (const auto& neighbor : adjacencylist_[v]) {
            if (visited.find (neighbor) == visited.end()) {
                sort_visitor_ (neighbor, visited, stacked);
            }
        }

        stacked.push (v);
    }


    void sort_()
    {
        std::set<string> visited;
        std::stack<string> stacked;

        for (const auto& neighbors: adjacencylist_) {
            if (visited.find (neighbors.first) == visited.end()) {
                sort_visitor_ (neighbors.first, visited, stacked);
            }
        }

        ordered_.clear();
        while (!stacked.empty()) {
            ordered_.push_back (stacked.top());
            stacked.pop();
        }
    }


    bool cycle_visitor_ (const string& v, std::set<string>& visited, std::set<string>& stacked) {
        // If the vertex is currently in the recursion stack, a cycle is found
        if (stacked.find (v) != stacked.end()) {
            return true;
        }

        // If the vertex is already visited and not in the current path, no cycle
        if (visited.find(v) != visited.end()) {
            return false;
        }

        // Mark the current vertex as visited and add it to the recursion stack
        visited.insert (v);
        stacked.insert (v);

        for (const auto& neighbor : adjacencylist_[v]) {
            if (cycle_visitor_ (neighbor, visited, stacked)) {
                return true;
            }
        }

        // Remove the vertex from the recursion stack before returning
        stacked.erase (v);

        return false;
    }


    bool detect_cycle()
    {
        std::set<string> visited;
        std::set<string> stacked;

        // Check for cycles starting from each vertex
        for (const auto& pair : adjacencylist_) {
            if (cycle_visitor_ (pair.first, visited, stacked)) {
                return true;
            }
        }

        return false;
    }
};


} // namespace daisychain
