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

#include "node.h"


namespace daisychain {
class WatchNode : public Node
{
public:
    WatchNode();

    ~WatchNode() override;

    void Initialize (json&, bool) override;

    bool Execute (vector<string>& input, const string& sandbox, json& vars) override;

    json Serialize() override;

    void Cleanup() override;

    bool passthru() const { return passthru_; }

    void set_passthru (bool passthru) { passthru_ = passthru; }

private:
    bool InitNotify();

    bool Notify (const string& sandbox, const string& path);

    void Monitor (const string& sandbox);

    void RemoveWatches();

    void RemoveMonitor() const;

    map<int, string> watch_fd_map_;

    int notify_fd_ = 0;

    bool passthru_;
};
} // namespace daisychain
