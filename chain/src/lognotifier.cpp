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

#include "lognotifier.h"

#include <QThread>
#include <QTimer>
#ifndef _WIN32
#include <unistd.h>
#else
#include <filesystem>
#endif


#ifdef __linux__
#include <climits>
#include <sys/inotify.h>


#define BUF_LEN (10 * (sizeof (struct inotify_event) + NAME_MAX + 1))

LogNotifier::LogNotifier() : QObject(), keepalive (true), dev_fd (-1)
{
    dev_fd = inotify_init();

    if (dev_fd == -1) {
        keepalive = false;
    }
}


void
LogNotifier::setLogFile (const std::string& filename)
{
    if (watch_fds.count (filename)) {
        return;
    }

    auto watch_fd = inotify_add_watch (dev_fd, filename.c_str(), IN_MODIFY);

    if (watch_fd == -1) {
        keepalive = false;
    }
    else {
        watch_fds[filename] = watch_fd;
        watch_files[watch_fd] = filename;
    }
}


void
LogNotifier::monitor()
{
    char* p;
    char buffer[BUF_LEN];
    ssize_t bytesread = 0;
    inotify_event* event;

    while (keepalive) {
        bytesread = read (dev_fd, buffer, BUF_LEN);

        for (p = buffer; p < buffer + bytesread;) {
            event = (struct inotify_event*)p;

            if (event->mask & IN_MODIFY) {
                Q_EMIT (fileChanged(watch_files[event->wd]));
            }

            p += sizeof (struct inotify_event) + event->len;
        }
    }
} // LogNotifier::monitor


#endif // ifdef __linux__

#ifdef __APPLE__
#include <fcntl.h>
#include <sys/event.h>


LogNotifier::LogNotifier() : QObject(), keepalive (true), dev_fd (-1)
{
    dev_fd = kqueue();

    if (dev_fd == -1) {
        keepalive = false;
    }
}


void
LogNotifier::setLogFile (const std::string& filename)
{
    if (watch_fds.count (filename)) {
        return;
    }

    auto watch_fd = ::open (filename.c_str(), O_EVTONLY);

    if (watch_fd == -1) {
        keepalive = false;
    }
    else {
        struct kevent modified{};

        EV_SET (&modified,
            watch_fd,
            EVFILT_VNODE,
            EV_ADD | EV_CLEAR | EV_ENABLE,
            NOTE_WRITE,
            0,
            NULL);

        if (kevent (dev_fd, &modified, 1, nullptr, 0, nullptr) == -1) {
            keepalive = false;
        }
        else {
            watch_fds[filename] = watch_fd;
            watch_files[watch_fd] = filename;
        }
    }
}


void
LogNotifier::monitor()
{
    int numevents = -1;
    struct kevent event{};

    while (keepalive) {
        numevents = kevent (dev_fd, nullptr, 0, &event, 1, nullptr);

        if (numevents != -1) {
            Q_EMIT (fileChanged(watch_files[int(event.ident)]));
        }
    }
} // LogNotifier::monitor


#endif // ifdef __APPLE__

#ifdef _WIN32

LogNotifier::LogNotifier() : QObject(), keepalive (true)
{
}


void
LogNotifier::setLogFile (const std::string& filename)
{
    if (std::find (watch_files.begin(), watch_files.end(), filename) == watch_files.end()) {
        watch_files.insert (filename);
    }

    if (log_dir) {
        return;
    }

    std::filesystem::path path_ (filename);
    auto directory = path_.parent_path().string();

    log_dir = CreateFileA(
        directory.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr);

    if (log_dir == INVALID_HANDLE_VALUE) {
        return;
    }
}


void
LogNotifier::monitor()
{
    char buffer[1024];
    DWORD bytesReturned;

    while (keepalive)
    {
        // Monitor the directory for changes
        if (ReadDirectoryChangesW(
                log_dir,                      // Directory handle
                &buffer,                         // Output buffer
                sizeof(buffer),                  // Size of the buffer
                FALSE,                           // Do not monitor subdirectories
                FILE_NOTIFY_CHANGE_LAST_WRITE,   // Watch for file changes
                &bytesReturned,                  // Bytes returned
                nullptr,                         // NULL for synchronous operation
                nullptr) == 0)                   // NULL for no completion routine
        {
            return;
        }

        auto* fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
        std::wstring changedFile(fni->FileName, fni->FileNameLength / sizeof(WCHAR));
        std::string changedFileName (changedFile.begin(), changedFile.end());

        // Check if the change was on the specific file we are watching
        if (watch_files.contains (changedFileName)) {
            Q_EMIT (fileChanged (changedFileName));
        }

        QThread::msleep (100);
    }

} // LogNotifier::monitor
#endif


LogNotifier::~LogNotifier()
{
#ifdef _WIN32
    CloseHandle (log_dir);
#else
    for (const auto& watch_fd : watch_fds) {
        ::close (watch_fd.second);
    }
    close (dev_fd);
#endif
}


