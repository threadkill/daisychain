// # MIT License
// # Copyright (c) 2025 Stephen J. Parker
// # SPDX-License-Identifier: MIT
// # See LICENSE file for full license text.
//

//
// Created by Stephen Jeffrey Parker on 6/29/22.
//

#pragma once

#include <functional>
#include <algorithm>
#include <cstdio>


static std::function<void (int)> sigint_handler;
static std::function<void (int)> sigterm_handler;


inline void
signal_handler (int signal)
{
    switch (signal) {
    case SIGINT:
        sigint_handler (signal);
        break;
    case SIGTERM:
        sigterm_handler (signal);
        break;
    default:
        break;
    }
}


