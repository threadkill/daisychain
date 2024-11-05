
#pragma once

#ifdef _WIN32
#include <windows.h>
#include <string>
#include <iostream>
#include <random>
#include <cstdlib>


// Helper function to generate a random string of alphanumeric characters
inline std::string
generateRandomString (size_t length) {
    const std::string characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::random_device rd;  // Obtain a random number from hardware
    std::mt19937 generator(rd());  // Seed the random number generator
    std::uniform_int_distribution<> dist(0, static_cast<int> (characters.size() - 1));

    std::string randomString;
    for (size_t i = 0; i < length; ++i) {
        randomString += characters[dist(generator)];
    }
    return randomString;
}

// Function to emulate mkdtemp
inline std::string
mkdtemp_ (const std::string& templateStr) {
    const std::string mask = "XXXXXX";

    // Find the position of the mask (XXXXXX) in the template
    const size_t pos = templateStr.find (mask);
    if (pos == std::string::npos) {
        std::cerr << "Template does not contain the required mask 'XXXXXX'." << std::endl;
        return "";
    }

    bool success = false;

    // Try creating the directory with different random strings until it succeeds
    for (int attempt = 0; attempt < 100; ++attempt) {
        constexpr size_t maskLength = 6;
        // Limit attempts to avoid infinite loop
        // Generate a new random string
        std::string randomStr = generateRandomString (maskLength);

        // Replace the 'XXXXXX' mask with the random string
        std::string tempdir = templateStr;
        std::string temppath (std::getenv ("TEMP"));
        temppath += "\\";
        tempdir.replace (pos, maskLength, randomStr);
        temppath += tempdir;

        // Try to create the directory
        if (CreateDirectoryA (temppath.c_str(), nullptr)) {
            // Directory creation succeeded
            success = true;
            return temppath;
        } else if (GetLastError() == ERROR_ALREADY_EXISTS) {
            // Directory already exists, retry with a different random string
            continue;
        } else {
            // Some other error occurred
            std::cerr << "Failed to create directory: " << GetLastError() << std::endl;
            return "";
        }
    }

    // If we reach here, we failed to create a unique directory
    std::cerr << "Failed to create a unique directory after multiple attempts." << std::endl;
    return "";
}


inline void
DeleteDirectoryContents (const std::string& directory) {
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
            DeleteDirectoryContents(fullPath);

            // Now delete the empty directory
            if (!RemoveDirectory (fullPath.c_str())) {
                std::cerr << "Failed to remove directory: " << fullPath << " Error: " << GetLastError() << std::endl;
            }
        } else {
            // It's a file, so delete it
            if (!DeleteFile (fullPath.c_str())) {
                std::cerr << "Failed to delete file: " << fullPath << " Error: " << GetLastError() << std::endl;
            }
        }
    } while (FindNextFile (hFind, &findFileData) != 0);

    FindClose(hFind);
}

inline bool
DeleteDirectoryRecursively (const std::string& directory) {
    bool stat = false;
    // First, delete the contents of the directory
    DeleteDirectoryContents (directory);

    // Finally, delete the root directory itself
    if (!RemoveDirectory (directory.c_str())) {
        std::cerr << "Failed to remove directory: " << directory << " Error: " << GetLastError() << std::endl;
    } else {
        std::cout << "Directory " << directory << " deleted successfully." << std::endl;
        stat = true;
    }

    return stat;
}


inline std::string
wchar2string (const wchar_t* wcharStr) {
    if (!wcharStr) {
        return {};
    }

    int size_needed = WideCharToMultiByte (CP_UTF8, 0, wcharStr, -1, nullptr, 0, nullptr, nullptr);
    std::string result (size_needed - 1, 0);
    WideCharToMultiByte (CP_UTF8, 0, wcharStr, -1, &result[0], size_needed, nullptr, nullptr);

    return result;
}
#endif