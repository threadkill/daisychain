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

#include <easylogging++.h>

#define LINFO LOG (INFO)
#define LINFO_IF(condition) LOG_IF (condition, INFO)
#define LWARN LOG (WARNING)
#define LWARN_IF(condition) LOG_IF (condition, WARNING)
#define LERROR LOG (ERROR)
#define LERROR_IF(condition) LOG_IF (condition, ERROR)
#define LDEBUG LOG (DEBUG)
#define LDEBUG_IF(condition) LOG_IF (condition, DEBUG)
#define LOGNODE ("<" + (!this->name_.empty() ? this->name_ : DaisyNodeNameByType[this->type_]) + "> ")
#define LTEST CLOG (INFO, "test")


void configureLogger (el::Level level = el::Level::Error);


inline void
configureLogger (const std::string& level)
{
    if (level == "info") {
        configureLogger (el::Level::Info);
    }
    else if (level == "warn") {
        configureLogger (el::Level::Warning);
    }
    else if (level == "error") {
        configureLogger (el::Level::Error);
    }
    else if (level == "debug") {
        configureLogger (el::Level::Debug);
    }
    else if (level == "off") {
        configureLogger (el::Level::Unknown);
    }
    else {
        LERROR << "Invalid. Log levels: info, warning, error, debug, off";
    }
}
