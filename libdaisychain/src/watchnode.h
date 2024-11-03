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

#include <queue>
#include "node.h"
#include "utils.h"


namespace daisychain {
class WatchNode : public Node
{
public:
    WatchNode (Graph*);

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
#ifndef _WIN32
        fd_in_.clear();
        fd_out_.clear();
#else
        stopwatching_ = false;
        watch_handle_map_.clear();  // only directories can be watched on windows
        watch_files_.clear();       // explicitly watched files
        dirinfos_.clear();
        terminate_.store (false);

        while (!modified_files_.empty()) {
            modified_files_.pop();
        }
#endif
        eofs_ = 0;
        totalbytesread_ = 0;
        totalbyteswritten_ = 0;
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

    map<int, string> watch_fd_map_;

    int notify_fd_ = 0;

    bool passthru_;

    bool recursive_;

#ifdef _WIN32
    DWORD WINAPI MonitorThread();

    #define BUFFER_SIZE (1024 * 64)
    struct DirectoryInfo {
        std::string directoryPath;
        HANDLE hDir;
        OVERLAPPED overlapped;
        BYTE buffer[BUFFER_SIZE];
    };

    static DWORD WINAPI ThreadProc (LPVOID lpParameter) {
        auto* worker = static_cast<WatchNode*>(lpParameter);
        worker->MonitorThread();
        return 0;
    }

    HANDLE iocp_{};                         // IO Completion Port HANDLE
    map<string, HANDLE> watch_handle_map_;  // only directories can be watched on windows
    std::vector<fs::path> watch_files_;     // explicitly watched files
    std::mutex modified_mutex_;
    std::mutex terminate_mutex_;
    std::condition_variable modified_cv_;
    std::condition_variable terminate_cv_;
    std::queue<string> modified_files_;
    std::vector<DirectoryInfo*> dirinfos_;
    std::unordered_map<string, std::chrono::steady_clock::time_point> notifications_;
    std::chrono::milliseconds debounce_time = std::chrono::milliseconds (2000);
    bool stopwatching_;

#endif
};
} // namespace daisychain
