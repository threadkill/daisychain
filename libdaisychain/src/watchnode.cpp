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

#include "watchnode.h"
#include <filesystem>
#include <cerrno>


namespace daisychain {
using namespace std;
using namespace filesystem;


WatchNode::WatchNode (Graph* parent) :
    Node (parent),
    passthru_ (false),
    recursive_ (true)
{
#ifdef _WIN32
    stopwatching_ = false;
#endif
    type_ = DaisyNodeType::DC_WATCH;
    batch_ = true;
    set_name (DaisyNodeNameByType[type_]);
}


WatchNode::~WatchNode() { WatchNode::Cleanup(); }


void
WatchNode::Initialize (json& keydata, bool keep_uuid)
{
    Node::Initialize (keydata, keep_uuid);

    json::iterator jit = keydata.begin();
    auto& uuid = jit.key();
    auto data = keydata[uuid];

    if (data.count ("passthru")) {
        set_passthru (data["passthru"]);
    }

    if (data.count ("recursive")) {
        set_recursive (data["recursive"]);
    }
} // WatchNode::Initialize


#ifndef _WIN32
bool
WatchNode::Execute (vector<string>& inputs, const string& sandbox, json& vars)
{
    bool stat = true;

    LINFO << "Executing " << (isroot_ ? "root: " : "node: ") << name_;

    if (!InitNotify()) {
        return false;
    }

    for (auto& input : inputs) {
        if (input != "EOF") {
            if (!Notify (sandbox, input)) {
                stat = false;
            }
        }
    }

    if (stat) {
        if (!test_) {
            if (!inputs.empty() && inputs[inputs.size() - 1] == "EOF") {
                inputs.pop_back();
            }

            Monitor (sandbox);
        }

        RemoveWatches();
        OpenOutputs (sandbox);
        WriteOutputs ("EOF");
        CloseOutputs();
        Reset();
    }

    return stat;
} // WatchNode::Execute
#endif


json
WatchNode::Serialize()
{
    auto json = Node::Serialize();
    json[id_]["passthru"] = passthru_;
    json[id_]["recursive"] = recursive_;

    return json;
} // WatchNode::Serialize


void
WatchNode::Cleanup()
{
    CloseOutputs();
    CloseInputs();
    RemoveWatches();
    RemoveMonitor();
} // WatchNode::Cleanup


#ifdef __APPLE__
#include <fcntl.h>
#include <sys/event.h>


bool
WatchNode::InitNotify()
{
    bool stat = true;

    notify_fd_ = kqueue();

    if (notify_fd_ == -1) {
        LERROR << "Failed to initialize kqueue instance.";
        stat = false;
    }
    else {
        LDEBUG << "kqueue initialized.";
    }

    return stat;
} // WatchNode::InitNotify


bool
WatchNode::Notify (const string& sandbox, const string& path)
{
    bool stat = true;

    std::vector<string> paths;
    paths.push_back (path);

    auto fs_path = filesystem::path (path);
    if (is_directory (fs_path)) {
        for (const auto& comp: recursive_directory_iterator (path)) {
            paths.push_back (comp.path());
        }
    }

    for (const auto& input : paths) {
        int watch_fd_ = open (input.c_str(), O_EVTONLY);
        if (watch_fd_ == -1) {
            stat = false;
            continue;
        }
        watch_fd_map_[watch_fd_] = input;
        struct kevent kev{};

        EV_SET (&kev,
                watch_fd_,
                EVFILT_VNODE,
                EV_ADD | EV_CLEAR | EV_ENABLE,
                NOTE_WRITE | NOTE_RENAME | NOTE_DELETE | NOTE_FUNLOCK,
                0,
                NULL);

        if (kevent (notify_fd_, &kev, 1, nullptr, 0, nullptr) == -1) {
            close (watch_fd_);
            stat = false;
            LDEBUG << "Watch failed on: " << input;
        }
        else if (passthru_ && is_regular_file (filesystem::path (input))) {
            OpenOutputs (sandbox);
            WriteOutputs (input);
            CloseOutputs();
        }
    }

    return stat;
} // WatchNode::Notify


void
WatchNode::Monitor (const string& sandbox)
{
    int numevents = -1;
    struct kevent event{};
    string previous;

    // must be stopped with a SIGINT (ctrl-c).
    while (true) {
        numevents = kevent (notify_fd_, nullptr, 0, &event, 1, nullptr);
        LDEBUG << "Num events: " << numevents;
        if (numevents == -1) {
            continue;
        }

        string path = watch_fd_map_[int(event.ident)];
        auto fs_path = filesystem::path (path);

        if (is_directory (fs_path)) {
            std::vector<string> newpaths;
            for (const auto& comp : recursive_directory_iterator (path)) {
                newpaths.push_back (comp.path());
            }
            if (newpaths.empty()) {
                continue;
            }
            for (auto& newpath : newpaths) {
                bool found = false;
                for (auto& fit : watch_fd_map_) {
                    if (fit.second == newpath) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    LDEBUG << "New file: " << newpath;
                    Notify (sandbox, newpath);
                    previous = newpath;
                }
            }
        }
        else {
            if (event.fflags & NOTE_WRITE) {
                LDEBUG << "Written: " << path;
                if (path != previous) {
                    OpenOutputs (sandbox);
                    WriteOutputs (path);
                    CloseOutputs();
                }
                else {
                    previous.clear();
                }
            }
            else if (event.fflags & NOTE_DELETE || event.fflags & NOTE_RENAME) {
                close (int(event.ident));
                watch_fd_map_.erase (int(event.ident));
                if (std::filesystem::exists (fs_path)) {
                    Notify (sandbox, path);
                    if (!passthru_) {
                        OpenOutputs (sandbox);
                        WriteOutputs (path);
                        CloseOutputs();
                    }
                }
                else {
                    LDEBUG << "File renamed or deleted: " << path;
                }
            }
            else if (event.fflags & NOTE_FUNLOCK) {
                LDEBUG << "File Closed: " << path;
                OpenOutputs (sandbox);
                WriteOutputs (path);
                CloseOutputs();
            }
        }
    }
} // WatchNode::Monitor


void
WatchNode::RemoveWatches()
{
    for (const auto& [wd, input] : watch_fd_map_) {
        close (wd);
    }
    watch_fd_map_.clear();
    LDEBUG << "Removed watched file descriptors.";
} // WatchNode::RemoveWatches


void
WatchNode::RemoveMonitor() const
{
} // WatchNode::RemoveMonitor

#endif


#ifdef __linux__
#include <climits>
#include <sys/inotify.h>
#include <unistd.h>


bool
WatchNode::InitNotify()
{
    bool stat = true;

    notify_fd_ = inotify_init1 (IN_CLOEXEC);

    if (notify_fd_ == -1) {
        LERROR << "Failed to initialize inotify instance.";
        stat = false;
    }
    else {
        LDEBUG << "inotify initialized.";
    }

    return stat;
} // WatchNode::InitNotify


bool
WatchNode::Notify (const string& sandbox, const string& path)
{
    bool stat = true;

    std::vector<string> paths;
    paths.push_back (path);

    // If path specified is a directory, then traverse and find all files and folders.
    auto fs_path = filesystem::path (path);
    if (is_directory(fs_path)) {
        for (const auto& comp: recursive_directory_iterator (path)) {
            paths.push_back (comp.path());
        }
    }

    for (const auto& input : paths) {
        fs_path = filesystem::path (input);

        // If paths.size() > 1 then we traversed a hierarchy. We only want to set watches on folders
        // when dealing with a traversal. Otherwise, if paths.size() == 1 then we're dealing with a
        // single file and want to set an explicit watch on it.
        if (is_directory (fs_path) || paths.size() == 1) {
            int watch_fd_ = inotify_add_watch (notify_fd_, input.c_str(), IN_ALL_EVENTS);

            if (watch_fd_ != -1) {
                watch_fd_map_[watch_fd_] = input;
                LDEBUG << "Added watch on: " << input;
            }
            else {
                stat = false;
                LERROR << "Watch failed on: " << input;

                // Lifted right from the man pages.
                switch (errno) {
                case EACCES:
                    LERROR << "Read access to the given file is not permitted.";
                    break;
                case EBADF:
                    LERROR << "The given file descriptor is not valid.";
                    break;
                case EEXIST:
                    LERROR << "Mask contains IN_MASK_CREATE and pathname refers "
                              "to a file already being watched by the same fd.";
                    break;
                case EFAULT:
                    LERROR << "Pathname points outside of the process's accessible address space.";
                    break;
                case EINVAL:
                    LERROR << "The given event mask contains no valid events; or mask contains "
                              "both IN_MASK_ADD and IN_MASK_CREATE; or fd is not an inotify file "
                              "descriptor.";
                    break;
                case ENAMETOOLONG:
                    LERROR << "Pathname is too long.";
                    break;
                case ENOENT:
                    LERROR << "A directory component in pathname does not exist or is a dangling "
                              "symbolic link.";
                    break;
                case ENOMEM:
                    LERROR << "Insufficient kernel memory was available.";
                    break;
                case ENOSPC:
                    LERROR << "The user limit on the total number of inotify watches was reached "
                              "or the kernel failed to allocate a needed resource.";
                    break;
                case ENOTDIR:
                    LERROR << "Mask contains IN_ONLYDIR and pathname is not a directory.";
                    break;
                }
            }
        }
        // All files found should pass through the graph initially.
        if (is_regular_file (fs_path)) {
            OpenOutputs (sandbox);
            WriteOutputs (input);
            CloseOutputs();
        }
    }

    return stat;
} // WatchNode::Notify


#define BUF_LEN (100 * (sizeof (struct inotify_event) + NAME_MAX + 1))

void
WatchNode::Monitor (const string& sandbox)
{
    struct pollfd pfds[1];
    pfds[0].fd = notify_fd_;
    pfds[0].events = POLLIN;

    string previous;    // Using this to keep track of successive events.

    // must be stopped with a SIGINT (ctrl-c).
    while (true) {
        auto ret = poll (pfds, 1, 2);

        if (ret <= 0 || !pfds[0].revents) {
            previous.clear();
            continue;
        }

        char buffer[BUF_LEN];
        ssize_t bytesread = read (notify_fd_, buffer, BUF_LEN);

        if (bytesread <= 0) {
            previous.clear();
            continue;
        }

        char* p;
        inotify_event* ev;

        for (p = buffer; p < buffer + bytesread;) {
            ev = (struct inotify_event*)p;
            string input;

            if (ev->mask & IN_CLOSE_WRITE) {
                input = ev->name;
                if (input.empty()) {
                    input = watch_fd_map_[ev->wd];
                }
                else {
                    input = watch_fd_map_[ev->wd] + "/" + ev->name;
                }
                LDEBUG << "File changed: " << input;
            }
            else if (ev->mask & IN_CREATE || ev->mask & IN_MOVED_TO) {
                if (ev->mask & IN_ISDIR) {
                    // A new directory exists; create a watch on it.
                    auto fullpath = watch_fd_map_[ev->wd] + "/" + ev->name;
                    Notify (sandbox, fullpath);
                }
                else {
                    input = watch_fd_map_[ev->wd] + "/" + ev->name;
                    LDEBUG << "New or renamed file: " << input;
                }
            }

            if (!input.empty()) {
                if (input != previous) {
                    previous = input;
                    OpenOutputs (sandbox);
                    WriteOutputs (input);
                    CloseOutputs();
                }
            }

            input.clear();

            p += sizeof (struct inotify_event) + ev->len;
        }
    }
} // WatchNode::Monitor


void
WatchNode::RemoveWatches()
{
    for (const auto& [wd, input] : watch_fd_map_) {
        inotify_rm_watch (notify_fd_, wd);
    }
    watch_fd_map_.clear();
    LDEBUG << "Removed watched paths.";
} // WatchNode::RemoveWatches


void
WatchNode::RemoveMonitor() const
{
    close (notify_fd_);
} // WatchNode::RemoveMonitor

#endif // ifdef __linux__


#ifdef _WIN32
bool
WatchNode::Execute (vector<string>& inputs, const string& sandbox, json& vars)
{
    bool stat = true;

    LINFO << "Executing " << (isroot_ ? "root: " : "node: ") << name_;

    if (!InitNotify()) {
        return false;
    }

    std::vector<path> allpaths (inputs.begin(), inputs.end());
    for (const auto& path: allpaths) {
        if (is_regular_file (path)) {
            watch_files_.emplace_back (path.string());
            if (passthru_) {
                OpenOutputs (sandbox);
                WriteOutputs (path.string());
                CloseOutputs();
            }
        }
    }

    auto common_folders = m_minimum_root (allpaths);
    for (auto& folder: common_folders) {
        if (!Notify (sandbox, folder.string())) {
            stat = false;
        }
    }

    if (!test_) {
        if (!inputs.empty() && inputs[inputs.size() - 1] == "EOF") {
            inputs.pop_back();
        }

        Monitor (sandbox);
    }

    RemoveWatches();
    OpenOutputs (sandbox);
    WriteOutputs ("EOF");
    CloseOutputs();
    Reset();

    return stat;
} // WatchNode::Execute


void
WatchNode::Stop()
{
    {
        std::lock_guard<std::mutex> lock (terminate_mutex_);
        stopwatching_ = true;
    }
    terminate_cv_.notify_all();
    terminate_.store (true);
    modified_cv_.notify_all();
} // WatchNode::Stop


bool
WatchNode::InitNotify()
{
    bool stat = true;

    iocp_ = CreateIoCompletionPort (INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (iocp_ == nullptr) {
        stat = false;
    }

    return stat;
} // WatchNode::InitNotify


bool
WatchNode::Notify (const string& sandbox, const string& path)
{
    bool stat = true;

    auto* dirinfo = new DirectoryInfo();
    dirinfo->directoryPath = path;

    dirinfo->hDir = CreateFile(
        path.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr
    );

    if (dirinfo->hDir == INVALID_HANDLE_VALUE) {
        LDEBUG << "Watch failed on: " << path;
        delete dirinfo;
        return false;
    }

    auto ioh = CreateIoCompletionPort (dirinfo->hDir, iocp_, reinterpret_cast<ULONG_PTR> (dirinfo), 0);
    if (ioh == nullptr) {
        CloseHandle (dirinfo->hDir);
        delete dirinfo;
        return false;
    }

    ZeroMemory (&dirinfo->overlapped, sizeof(OVERLAPPED));

    BOOL success = ReadDirectoryChangesW(
        dirinfo->hDir,
        dirinfo->buffer,
        BUFFER_SIZE,
        TRUE, // Monitor subdirectories
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
        FILE_NOTIFY_CHANGE_SIZE      | FILE_NOTIFY_CHANGE_LAST_WRITE,
        nullptr,
        &dirinfo->overlapped,
        nullptr);

    if (!success) {
        CloseHandle (dirinfo->hDir);
        delete dirinfo;
        return false;
    }

    watch_handle_map_[path] = dirinfo->hDir;
    dirinfos_.push_back (dirinfo);

    return stat;
} // WatchNode::Notify


DWORD WINAPI
WatchNode::MonitorThread()
{
    ULONG num_removed = 0;

    while (!terminate_.load()) {
        OVERLAPPED_ENTRY overlapped[MAXIMUM_WAIT_OBJECTS];
        BOOL success = GetQueuedCompletionStatusEx(
            iocp_,
            overlapped,
            MAXIMUM_WAIT_OBJECTS,
            &num_removed,
            500,
            FALSE);

        if (terminate_.load()) break;

        if (!success) {
            DWORD error = GetLastError();
            if (error == WAIT_TIMEOUT) {
                continue; // Timeout reached, check for termination
            }
            LERROR << "GetQueuedCompletionStatusEx failed with error: " << error;
            break;
        }

        std::vector<std::string> batch_modified_files;

        for (ULONG i = 0; i < num_removed; ++i) {
            LPOVERLAPPED lpOverlapped = overlapped[i].lpOverlapped;
            ULONG_PTR completionkey = overlapped[i].lpCompletionKey;
            DWORD bytes_transferred = overlapped[i].dwNumberOfBytesTransferred;

            if (lpOverlapped == nullptr && completionkey == 0) {
                LERROR << "lpOverlapped is nullptr.";
                break;
            }

            auto* dirinfo = reinterpret_cast<DirectoryInfo*>(completionkey);
            if (dirinfo == nullptr) {
                LERROR << "dirinfo is nullptr.";
                continue;
            }

            if (bytes_transferred == 0) {
                continue;
            }

            ZeroMemory (&dirinfo->overlapped, sizeof(OVERLAPPED));

            success = ReadDirectoryChangesW(
                dirinfo->hDir,
                dirinfo->buffer,
                BUFFER_SIZE,
                TRUE,
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                FILE_NOTIFY_CHANGE_SIZE      | FILE_NOTIFY_CHANGE_LAST_WRITE,
                nullptr,
                &dirinfo->overlapped,
                nullptr);

            if (!success) {
                LERROR << "ReadDirectoryChangesW failed with error: " << GetLastError();
                break;
            }

            FILE_NOTIFY_INFORMATION* notify;
            size_t offset = 0;

            do {
                notify = (FILE_NOTIFY_INFORMATION*)((BYTE*)dirinfo->buffer + offset);
                bool transmit = false;

                switch (notify->Action) {
                    case FILE_ACTION_ADDED:
                        LDEBUG << "File added: ";
                        transmit = true;
                        break;
                    case FILE_ACTION_REMOVED:
                        LDEBUG << "File removed: ";
                        break;
                    case FILE_ACTION_MODIFIED:
                        LDEBUG << "File modified: ";
                        transmit = true;
                        break;
                    case FILE_ACTION_RENAMED_OLD_NAME:
                        LDEBUG << "File renamed (old name): ";
                        break;
                    case FILE_ACTION_RENAMED_NEW_NAME:
                        LDEBUG << "File renamed (new name): ";
                        transmit = true;
                        break;
                    default:
                        LDEBUG << "Unknown action: ";
                        break;
                }

                if (transmit) {
                    std::wstring wfilename (notify->FileName, notify->FileNameLength / sizeof(WCHAR));
                    auto filename = wchar2string (wfilename.c_str());

                    // Time-based filtering of duplicates
                    auto now = std::chrono::steady_clock::now();
                    auto it = notifications_.find (filename);
                    if (it == notifications_.end() || (now - it->second) >= debounce_time) {
                        notifications_[filename] = now;
                        batch_modified_files.push_back (filename);
                    }
                }

                offset += notify->NextEntryOffset;
            } while (notify->NextEntryOffset != 0 && !terminate_.load());
        }

        if (!batch_modified_files.empty()) {
            std::lock_guard lock (modified_mutex_);
            for (const auto& filename : batch_modified_files) {
                modified_files_.push (filename);
            }
            modified_cv_.notify_one();
        }
    }

    return 0;
} // WatchNode::MonitorThread


void
WatchNode::Monitor (const string& sandbox)
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo (&sysinfo);
    int numthreads = sysinfo.dwNumberOfProcessors;

    std::vector<HANDLE> thread_handles;

    for (int i = 0; i < numthreads; ++i) {
        HANDLE hThread = CreateThread(
            nullptr,
            0,
            &WatchNode::ThreadProc,
            this,
            0,
            nullptr);

        if (hThread == nullptr) {
            std::cerr << "Failed to create worker thread." << std::endl;
            continue;
        }

        thread_handles.push_back (hThread);
    }

    while (!stopwatching_ && !terminate_.load()) {
        std::unique_lock lock (modified_mutex_);

        modified_cv_.wait (lock, [this] {
            return !modified_files_.empty() || stopwatching_;
        });

        if (stopwatching_ || terminate_.load()) {
            lock.unlock();
            WriteOutputs ("EOF");
            lock.lock();
            break;
        }

        if (!modified_files_.empty()) {
            auto filename = modified_files_.front();
            modified_files_.pop();

            lock.unlock();
            WriteOutputs (filename);
            lock.lock();
        }
    }

    for (int i = 0; i < numthreads; ++i) {
        PostQueuedCompletionStatus (iocp_, 0, 0, nullptr);
    }

    WaitForMultipleObjects(
        static_cast<DWORD> (thread_handles.size()), thread_handles.data(), TRUE, INFINITE);

    for (HANDLE hThread : thread_handles) {
        CloseHandle (hThread);
    }

    RemoveWatches();

    for (auto dirinfo : dirinfos_) {
        if (dirinfo->hDir != nullptr && dirinfo->hDir != INVALID_HANDLE_VALUE) {
            CloseHandle (dirinfo->hDir);
        }
        dirinfo->hDir = nullptr;
        delete dirinfo;
    }

    dirinfos_.clear();
} // WatchNode::Monitor


void
WatchNode::RemoveWatches()
{
    if (iocp_ != nullptr && iocp_ != INVALID_HANDLE_VALUE) {
        CloseHandle (iocp_);
    }
    iocp_ = nullptr;

    LDEBUG << "Removed watched file descriptors.";
} // WatchNode::RemoveWatches


void
WatchNode::RemoveMonitor() const
{
} // WatchNode::RemoveMonitor
#endif
}
