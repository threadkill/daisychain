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

#include <unordered_map>
#include <QObject>


class LogNotifier final : public QObject
{
    Q_OBJECT

public:
    LogNotifier();
    ~LogNotifier() override;

    void setLogFile (const std::string&);

public Q_SLOTS:

    void monitor();

Q_SIGNALS:

    void fileChanged (const std::string&);

private:
    int dev_fd;
    bool keepalive;
    std::unordered_map<std::string, int> watch_fds;
    std::unordered_map<int, std::string> watch_files;
};
