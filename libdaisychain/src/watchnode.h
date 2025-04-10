// MIT License
// Copyright (c) 2025 Stephen J. Parker
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text.

#pragma once

#include <queue>
#include <set>
#include "node.h"
#include "utils.h"


namespace daisychain {
class WatchNode final : public Node
{
public:
    WatchNode();

    ~WatchNode() override;

    void Initialize (json&, bool) override;

    bool Execute (vector<string>& input, const string& sandbox, json& vars) override;

#ifdef _WIN32
    void Stop() override;
#endif

    json Serialize() override;

    void Cleanup() override;

    void Reset() override
    {
#ifdef _WIN32
        only_files_ = false;
        only_dirs_ = false;
        watch_files_.clear();       // explicitly watched files
        watch_dirs_.clear();        // explicitly watched files
        dirinfos_.clear();
        terminate_.store (false);

        while (!modified_files_.empty()) {
            modified_files_.pop();
        }
#endif
        watch_fd_map_.clear();
        notifications_.clear();
        Node::Reset();
    }

    bool passthru() const { return passthru_; }

    void set_passthru (bool passthru) { passthru_ = passthru; }

    bool recursive() const { return recursive_; }

    void set_recursive (bool recursive) { recursive_ = recursive; }

private:
    bool InitNotify();

    bool Notify (const string& sandbox, const string& path);

    void Monitor (const string& sandbox);

    void RemoveWatches();

    void RemoveMonitor() const;

    bool passthru_;
    bool recursive_;
    int notify_fd_ = 0;
    map<int, string> watch_fd_map_;
    std::unordered_map<string, std::chrono::steady_clock::time_point> notifications_;
    std::chrono::milliseconds debounce_time = std::chrono::milliseconds (2000);

#ifdef _WIN32
    #define BUFFER_SIZE (1024 * 64)
    struct DirectoryInfo {
        std::string directoryPath;
        HANDLE hDir;
        OVERLAPPED overlapped;
        BYTE buffer[BUFFER_SIZE];
    };

    HANDLE iocp_{}; // IO Completion Port HANDLE
    bool only_files_{};
    bool only_dirs_{};
    std::set<string> watch_files_;
    std::set<string> watch_dirs_;
    std::queue<string> modified_files_;
    std::vector<DirectoryInfo*> dirinfos_;
#endif
};
} // namespace daisychain
