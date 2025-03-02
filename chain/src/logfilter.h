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

//
// Created by Stephen Jeffrey Parker on 7/27/24.
//

#pragma once

#include <fcntl.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "logger.h"


class LogFilter: public el::LogDispatchCallback
{

public:
    ~LogFilter() override;

    void set_pattern (std::string&&);

    void set_logfile (const std::string&);

    void handle (const el::LogDispatchData*) override;

    void dispatch (el::base::type::string_t&&) noexcept;

private:
#ifdef _WIN32
    HANDLE fd{};
#else
    int fd = 0;
#endif
    std::string pattern_;
    std::string logfile_;
};
