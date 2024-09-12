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

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <random>

#include <unistd.h>
#include <csignal>
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;


inline void
m_trim_if (std::string& s, const std::string& chars) {
    auto is_trim_char = [&](char c) {
        return chars.find(c) != std::string::npos;
    };

    auto start = std::find_if_not (s.begin(), s.end(), is_trim_char);
    auto end   = std::find_if_not (s.rbegin(), s.rend(), is_trim_char).base();

    if (start < end) {
        s.erase (end, s.end());
        s.erase (s.begin(), start);
    } else {
        s.clear();
    }
}


inline void
m_split (const std::string& s, const std::string& delims, std::vector<std::string>& tokens)
{
    tokens.clear();
    size_t start = 0;
    size_t end = s.find_first_of (delims);

    while (end != std::string::npos) {
        if (end > start) {
            tokens.push_back (s.substr (start, end - start));
        }
        start = end + 1;
        end = s.find_first_of (delims, start);
    }

    if (start < s.length()) {
        tokens.push_back (s.substr(start));
    }
}


template <typename C>
inline std::string
m_join (const C& container, const std::string& delimiter)
{
    std::ostringstream result;
    auto it = container.begin();
    auto end = container.end();

    if (it != end) {
        result << *it;
        ++it;
    }

    for (; it != end; ++it) {
        result << delimiter << *it;
    }

    return result.str();
}


template <typename C, typename F>
inline std::string
m_join_if (C& container, const std::string& separator, F comp)
{
    std::ostringstream oss;
    bool first = true;

    for (auto& elem : container) {
        if (comp (elem)) {
            if (!first) {
                oss << separator;
            }
            oss << elem;
            first = false;
        }
    }

    return oss.str();
}


inline void
m_split_input (const std::string& input,
    std::vector<std::string>& inputs,
    const std::string& delimiters = "\t\n")
{
    std::vector<std::string> tokens;
    //std::string trimmed = input;
    //m_trim_if (trimmed, "\t ");
    m_split (input, delimiters, tokens);
    inputs.insert (inputs.begin(), tokens.begin(), tokens.end());
    //inputs.erase (std::remove (inputs.begin(), inputs.end(), ""), inputs.end());
} // m_split_input


// This is for debugging nodes running in child processes.
inline void
m_debug_wait (bool dowait = false)
{
#ifndef NDEBUG
    if (dowait) {
        std::cout << "attach to pid " << getpid() << std::endl;

        while (dowait)
            ; // set dowait=false in debugger to continue.

        //raise (SIGTRAP);
    }
#endif // ifndef NDEBUG
} // m_debug_wait


inline int
m_is_numeric (const std::string& input)
{
    if (!input.empty()) {
        if (input.find_first_not_of ("-0123456789.") == std::string::npos) {
            if (input.find_first_of ('.') == std::string::npos) {
                return 2; // real
            }
            else {
                return 1; // integer
            }
        }
    }

    return 0;
} // m_is_numeric


inline json
m_parse_envars (const std::vector<std::string>& inputs)
{
    json env = {};

    // loop over strings in the form of "<key>=<value>" and create JSON object.
    for (const auto& keyval : inputs) {
        std::vector<std::string> pair;
        m_split_input (keyval, pair, "=");
        env[pair[0]] = pair[1];
    }

    return env;
} // m_parse_envars


inline std::string
m_gen_uuid()
{
    std::random_device rd;
    std::mt19937 gen (rd());
    std::uniform_int_distribution<int> dis (0, 15);
    std::uniform_int_distribution<int> dis2 (8, 11);

    std::stringstream ss;

    // Generate 8-4-4-4-12 structure UUID
    for (int i = 0; i < 8; i++)
        ss << std::hex << dis (gen);

    ss << "-";

    for (int i = 0; i < 4; i++)
        ss << std::hex << dis (gen);

    ss << "-4"; // UUID version 4

    for (int i = 0; i < 3; i++)
        ss << std::hex << dis (gen);

    // Ensures the first digit is 8, 9, A, or B
    ss << "-" << std::hex << dis2 (gen);

    for (int i = 0; i < 3; i++)
        ss << std::hex << dis (gen);

    ss << "-";

    for (int i = 0; i < 12; i++)
        ss << std::hex << dis (gen);

    return ss.str();
}
