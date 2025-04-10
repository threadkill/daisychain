// MIT License
// Copyright (c) 2025 Stephen J. Parker
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text.

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
