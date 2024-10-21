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

#include "logfilter.h"


LogFilter::~LogFilter()
{
#ifdef _WIN32
    CloseHandle (fd);
#else
    ::close (fd);
#endif
}


void
LogFilter::set_pattern (std::string&& pattern)
{
    pattern_ = pattern;
}


#ifdef _WIN32
void
LogFilter::set_logfile (const std::string& filename)
{
    logfile_ = filename;
    fd = CreateFileA(
        filename.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (fd == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Unable to open the file for writing." << std::endl;
        return;
    }

    set_pattern ("(" + logfile_ + ")");
}


void
LogFilter::dispatch (el::base::type::string_t&& logline) noexcept
{
    if (logline.starts_with (pattern_)) {
        DWORD bytesWritten;

        BOOL result = WriteFile(
            fd,
            logline.c_str(),
            static_cast<DWORD>(strlen (logline.c_str())),
            &bytesWritten,
            nullptr);

        if (!result) {
            std::cerr << "Error: Unable to write to the file." << std::endl;
        }

        FlushFileBuffers (fd);
    }
}

#else

void
LogFilter::set_logfile (const std::string& filename)
{
    logfile_ = filename;
    if ((fd = ::open (logfile_.c_str(), O_WRONLY)) == -1) {
        std::cout << "Could not open logfile for reading/writing: " << logfile_ << std::endl;
    }
    else {
        set_pattern ("(" + logfile_ + ")");
    }
}


void
LogFilter::dispatch (el::base::type::string_t&& logline) noexcept
{
    if (logline.starts_with (pattern_)) {
        ::write (fd, logline.c_str(), logline.size());
        // ideally, we would have used a Q_EMIT here to notify the file change but that causes
        // issues on macos when fork() is involved.
    }
}
#endif


void
LogFilter::handle (const el::LogDispatchData* data)
{
    dispatch (data->logMessage()->logger()->logBuilder()->build (
        data->logMessage(),
        data->dispatchAction() == el::base::DispatchAction::NormalLog)
    );
}

