// # MIT License
// # Copyright (c) 2025 Stephen J. Parker
// # SPDX-License-Identifier: MIT
// # See LICENSE file for full license text.
//

#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <filesystem>
#include <random>
#include "utils_win.h"
#ifndef _WIN32
#include <unistd.h>
#endif


#include <nlohmann/json.hpp>
using json = nlohmann::ordered_json;

namespace fs = std::filesystem;


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
std::string
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
std::string
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


#if defined(_WIN32)
#include <windows.h>
inline std::string
m_get_thread_name()
{
    PWSTR threadname = nullptr;
    HRESULT hr = GetThreadDescription (GetCurrentThread(), &threadname);
    if (FAILED (hr)) {
        return "noname";
    }
    auto name = wchar2string (threadname);
    LocalFree (threadname); // Free the allocated memory

    return name;
}
#elif defined(__linux__) || defined(__APPLE__)
inline std::string
m_get_thread_name()
{
    char name[16]; // Maximum 16 characters, as per pthread_setname_np limits
    int result = pthread_getname_np (pthread_self(), name, sizeof (name));
    if (result != 0) {
        return "noname";
    }
    return {name};
}
#endif


inline std::string
m_get_thread_id()
{
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return oss.str();
}


// This is for debugging nodes running in child processes.
inline void
m_debug_wait (bool dowait = false)
{
    if (dowait) {
#ifdef _WIN32
        std::cout << "attach to thread id " << m_get_thread_id() << std::endl;
#else
        std::cout << "attach to pid " << getpid() << std::endl;
#endif

        while (dowait)
            ; // set dowait=false in debugger to continue.

        //raise (SIGTRAP);
    }
} // m_debug_wait



// Function to get the common root folder between two file paths
inline fs::path
m_common_root (const fs::path &path1, const fs::path &path2) {
    auto it1 = path1.begin();
    auto it2 = path2.begin();

    fs::path common;

    while (it1 != path1.end() && it2 != path2.end() && *it1 == *it2) {
        common /= *it1;
        ++it1;
        ++it2;
    }

    return common;
}


// Function to find the minimum set of root folders that cover all paths
inline std::vector<fs::path>
m_minimum_root (const std::vector<fs::path> &paths) {
    if (paths.empty()) return {};

    if (paths.size() == 1) {
        if (fs::is_directory (paths[0])) {
            return {paths[0]};
        }
        return {paths[0].parent_path()};
    }

    std::vector<fs::path> rootFolders;
    std::vector<bool> used (paths.size(), false);

    // Loop through all paths to group them by common roots
    for (size_t i = 0; i < paths.size(); ++i) {
        if (used[i]) continue;  // Skip if already part of a group

        fs::path commonRootFolder = paths[i];
        used[i] = true;

        // Compare with other paths and group them based on common root
        for (size_t j = i + 1; j < paths.size(); ++j) {
            if (!used[j]) {
                fs::path newCommonRoot = m_common_root (commonRootFolder, paths[j]);
                if (!newCommonRoot.empty()) {
                    commonRootFolder = newCommonRoot;
                    used[j] = true;  // Mark path as processed
                }
            }
        }

        // Add the found common root folder to the result set
        rootFolders.push_back (commonRootFolder);
    }

    return rootFolders;
}