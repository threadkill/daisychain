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

#ifdef _WIN32
#include <cstdlib>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <tlhelp32.h>
#include <windows.h>


// Helper function to generate a random string of alphanumeric characters
inline std::string
random_string (size_t length)
{
    const std::string characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::random_device rd;         // Obtain a random number from hardware
    std::mt19937 generator (rd()); // Seed the random number generator
    std::uniform_int_distribution<> dist (0, static_cast<int> (characters.size() - 1));

    std::string randomString;
    for (size_t i = 0; i < length; ++i) {
        randomString += characters[dist (generator)];
    }
    return randomString;
}

// Function to emulate mkdtemp
inline std::string
mkdtemp_ (const std::string& templateStr)
{
    const std::string mask = "XXXXXX";

    // Find the position of the mask (XXXXXX) in the template
    const size_t pos = templateStr.find (mask);
    if (pos == std::string::npos) {
        std::cerr << "Template does not contain the required mask 'XXXXXX'." << std::endl;
        return "";
    }

    // Try creating the directory with different random strings until it succeeds
    for (int attempt = 0; attempt < 10; ++attempt) {
        constexpr size_t maskLength = 6;
        std::string randomStr = random_string (maskLength);

        // Replace the 'XXXXXX' mask with the random string
        std::string tempdir = templateStr;
        std::string temppath (std::getenv ("TEMP"));
        temppath += "\\";
        tempdir.replace (pos, maskLength, randomStr);
        temppath += tempdir;

        if (CreateDirectoryA (temppath.c_str(), nullptr)) {
            return temppath;
        }

        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            continue;
        }

        break;
    }

    std::cerr << "Failed to create a temp directory after multiple attempts." << std::endl;
    return "";
}


inline void
delete_directory_contents (const std::string& directory)
{
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile ((directory + "\\*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open directory: " << directory << std::endl;
        return;
    }

    do {
        std::string fileOrDir = findFileData.cFileName;

        // Skip the "." and ".." directories
        if (fileOrDir == "." || fileOrDir == "..") {
            continue;
        }

        std::string fullPath = directory + "\\" + fileOrDir;

        // Check if it's a directory
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // Recursively delete the directory contents
            delete_directory_contents (fullPath);

            if (!RemoveDirectory (fullPath.c_str())) {
                std::cerr << "Failed to remove directory: " << fullPath
                          << " Error: " << GetLastError() << std::endl;
            }
        }
        else if (!DeleteFile (fullPath.c_str())) {
            std::cerr << "Failed to delete file: " << fullPath << " Error: " << GetLastError()
                      << std::endl;
        }
    } while (FindNextFile (hFind, &findFileData) != 0);

    FindClose (hFind);
}

inline bool
delete_directory_recursive (const std::string& directory)
{
    bool stat = false;
    // First, delete the contents of the directory
    delete_directory_contents (directory);

    // Finally, delete the root directory itself
    if (!RemoveDirectory (directory.c_str())) {
        std::cerr << "Failed to remove directory: " << directory << " Error: " << GetLastError() << std::endl;
    }
    else {
        std::cout << "Directory " << directory << " deleted successfully." << std::endl;
        stat = true;
    }

    return stat;
}


inline std::string
wchar2string (const wchar_t* wcharStr)
{
    if (!wcharStr) {
        return {};
    }

    int size_needed = WideCharToMultiByte (CP_UTF8, 0, wcharStr, -1, nullptr, 0, nullptr, nullptr);
    std::string result (size_needed - 1, 0);
    WideCharToMultiByte (CP_UTF8, 0, wcharStr, -1, &result[0], size_needed, nullptr, nullptr);

    return result;
}


// Function to retrieve child process PIDs given a process handle
inline std::vector<DWORD>
get_child_processes (HANDLE processHandle)
{
    std::vector<DWORD> childPIDs;

    DWORD parentPID = GetProcessId (processHandle);
    if (parentPID == 0) {
        throw std::runtime_error ("Failed to get process ID from handle.");
    }

    // Take a snapshot of all running processes
    HANDLE hSnapshot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        throw std::runtime_error ("Failed to create process snapshot.");
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof (PROCESSENTRY32);

    // Iterate through all processes and find children
    if (Process32First (hSnapshot, &pe)) {
        do {
            if (pe.th32ParentProcessID == parentPID) {
                childPIDs.push_back (pe.th32ProcessID);
            }
        } while (Process32Next (hSnapshot, &pe));
    }

    CloseHandle (hSnapshot);
    return childPIDs;
};


inline bool
terminate_pid (DWORD pid)
{
    HANDLE hProcess = OpenProcess (PROCESS_TERMINATE, FALSE, pid);
    if (!hProcess) {
        std::cerr << "Failed to open process " << pid << ". Error: " << GetLastError() << std::endl;
        return false;
    }

    BOOL result = TerminateProcess (hProcess, 1);
    CloseHandle (hProcess);

    if (!result) {
        std::cerr << "Failed to terminate process " << pid << ". Error: " << GetLastError() << std::endl;
        return false;
    }

    std::cout << "Successfully terminated process " << pid << std::endl;
    return true;
};

#endif
