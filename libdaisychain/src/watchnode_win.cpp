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


namespace daisychain {
using namespace std;
using namespace filesystem;


WatchNode::WatchNode() : Node(),
    passthru_ (false)
{
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
} // WatchNode::Initialize


bool
WatchNode::Execute (vector<string>& inputs, const string& sandbox, json& vars)
{
    bool stat = true;

    LINFO << "Executing " << (isroot_ ? "root: " : "node: ") << name_;

    if (inputs.size() > 1) {
        LERROR << "Only one directory can be watched at a time.";
        return false;
    }

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
        else {
            RemoveWatches();

            OpenOutputs (sandbox);
            WriteOutputs ("EOF");
            CloseOutputs();
        }
    }

    return stat;
} // WatchNode::Execute


json
WatchNode::Serialize()
{
    auto json = Node::Serialize();
    json[id_]["passthru"] = passthru_;

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


bool
WatchNode::InitNotify()
{
    bool stat = true;

    return stat;
} // WatchNode::InitNotify


bool
WatchNode::Notify (const string& sandbox, const string& path)
{
    bool stat = true;

    std::vector<string> paths;
    paths.push_back (path);

    auto fs_path = filesystem::path (path);
    if (!is_directory (fs_path)) {
        return false;
    }

    for (const auto& input : paths) {
        handle_ = CreateFile(
            input.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            NULL
        );

        if (handle_ == INVALID_HANDLE_VALUE) {
            LDEBUG << "Watch failed on: " << input;
            stat = false;
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
    DWORD dwBytesReturned;
    char buffer[1024];
    FILE_NOTIFY_INFORMATION *pNotify;
    int offset;

    while (TRUE) {
        if (ReadDirectoryChangesW(
                handle_,
                buffer,
                sizeof(buffer),
                TRUE, // TRUE to monitor subdirectories as well
                FILE_NOTIFY_CHANGE_FILE_NAME |
                FILE_NOTIFY_CHANGE_DIR_NAME |
                FILE_NOTIFY_CHANGE_ATTRIBUTES |
                FILE_NOTIFY_CHANGE_SIZE |
                FILE_NOTIFY_CHANGE_LAST_WRITE |
                FILE_NOTIFY_CHANGE_CREATION,
                &dwBytesReturned,
                NULL,
                NULL))
        {
            offset = 0;
            bool transmit = false;

            do {
                pNotify = (FILE_NOTIFY_INFORMATION*) &buffer[offset];

                switch (pNotify->Action) {
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

                auto filename = wchar2string (pNotify->FileName);
                LDEBUG << filename;

                if (transmit) {
                    OpenOutputs (sandbox);
                    WriteOutputs (filename);
                    CloseOutputs();
                }

                offset += pNotify->NextEntryOffset;
            } while (pNotify->NextEntryOffset != 0);
        }
        else {
            LERROR << "ReadDirectoryChangesW failed with error: " << GetLastError();
            break;
        }
    }
}


void
WatchNode::RemoveWatches()
{
    CloseHandle (handle_);
    handle_ = nullptr;
    LDEBUG << "Removed watched file descriptors.";
} // WatchNode::RemoveWatches


void
WatchNode::RemoveMonitor() const
{
} // WatchNode::RemoveMonitor

}