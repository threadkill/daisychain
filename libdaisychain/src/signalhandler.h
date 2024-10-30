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


