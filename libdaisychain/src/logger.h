// MIT License
// Copyright (c) 2025 Stephen J. Parker
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text.

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
