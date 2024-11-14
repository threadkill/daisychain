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

#include "node.h"


namespace daisychain {
using namespace std;
using json = nlohmann::ordered_json;


Node::Node () :
    id_ (m_gen_uuid()),
    position_ (std::pair<float, float> (0.0f, 0.0f)),
    size_ (std::pair<int, int> (0, 0)),
    type_ (DC_INVALID),
    batch_ (false),
    test_ (false),
    isroot_ (true),
    eofs_ (0),
    totalbytesread_ (0),
    totalbyteswritten_ (0)
{
    terminate_.store (false);
}


Node::~Node()
{
    Node::Cleanup();
    LDEBUG << LOGNODE << "destroyed.";
}


void
Node::Initialize (json& keydata, bool keep_uuid)
{
    json::iterator jit = keydata.begin();
    const auto& uuid = jit.key();
    auto data = keydata[uuid];

    if (keep_uuid) {
        set_id (uuid);
    }

    if (data.count ("name") && !data["name"].get<string>().empty()) {
        set_name (data["name"]);
    }
    else {
        set_name (DaisyNodeNameByType[type_]);
    }

    if (data.count ("position")) {
        set_position (std::pair<float, float> (data["position"][0], data["position"][1]));
    }
    if (data.count ("size")) {
        set_size (std::pair<int, int> (data["size"][0], data["size"][1]));
    }
}


json
Node::Serialize()
{
    json json_ = {
        {id_,
         {{"type", type_},
          {"name", name_},
          {"position", position_}}}
    };

    return json_;
}


#ifdef _WIN32
void
Node::Start (NodeThreadContext* ctx, const string& sandbox, json& vars, const string& threadname)
{
    context_ = ctx;
    thread_ = std::thread ([this, &sandbox, &vars, threadname]() {
        this->set_threadname (threadname);
        this->OpenWindowsPipes (sandbox);
        auto stat = this->Execute (sandbox, vars);
        this->CloseWindowsPipes();
        LINFO_IF (stat) << "<" << name_ << "> Finished.";
        LERROR_IF (!stat) << "<" << name_ << "> Failed.";
    });
}


void
Node::Start (NodeThreadContext* ctx, vector<string>& inputs, const string& sandbox, json& vars, const string& threadname)
{
    context_ = ctx;
    thread_ = std::thread ([this, &inputs, &sandbox, &vars, threadname]() {
        this->set_threadname (threadname);
        this->OpenWindowsPipes (sandbox);
        auto stat = this->Execute (inputs, sandbox, vars);
        this->CloseWindowsPipes();
        LINFO_IF (stat) << "<" << name_ << "> Finished.";
        LERROR_IF (!stat) << "<" << name_ << "> Failed.";
    });
}


void
Node::Join()
{
    if (thread_.joinable()) {
        thread_.join();
    }
}


void
Node::Stop()
{
    terminate_.store (true);
    SetEvent (terminate_event_);
}
#endif

bool
Node::Execute (const string& sandbox, json& env)
{
    std::vector<string> inputs;
    int eof = 0;

    OpenInputs (sandbox);

    do {
        eof = ReadInputs (inputs);
        if (terminate_.load()) return false;
    } while (batch_ && eof != -1);

    if (batch_) {
        concat_inputs (inputs);
    }

    return Execute (inputs, sandbox, env);
} // Execute


void
Node::Stats()
{
    LDEBUG << LOGNODE << "total bytes read: " << totalbytesread_;
    LDEBUG << LOGNODE << "total bytes written: " << totalbyteswritten_;
} // Stats


void
Node::AddInput (const string& fifo)
{
    inputs_.push_back (fifo);
    isroot_ = false;
} // AddInput


void
Node::RemoveInput (const string& fifo)
{
    inputs_.remove (fifo);

    if (inputs_.empty()) {
        isroot_ = true;
    }
} // RemoveInput


void
Node::OpenInputs (const string& sandbox)
{
#ifndef _WIN32
    for (const auto& fifo : inputs_) {
        string filepath = sandbox;
        filepath.append ("/");
        filepath.append (fifo);
        int fd = open (filepath.c_str(), O_RDWR | O_NONBLOCK);

        if (fd == -1) {
            LERROR << LOGNODE << "Cannot open fifo for reading: " << filepath;
            continue;
        }

        fd_in_[fifo] = fd;
    }
#endif
} // OpenInputs


void
Node::CloseInputs()
{
#ifndef _WIN32
    for (const auto& fd : fd_in_) {
        int stat = close (fd.second);

        if (stat == -1) {
            LERROR << LOGNODE << "Cannot close input file descriptor: " << fd.first;
            continue;
        }
    }
#endif
} // CloseInputs


void
Node::OpenOutputs (const string& sandbox)
{
#ifndef _WIN32
    for (const auto& fifo : outputs_) {
        string filepath = sandbox;
        filepath.append ("/");
        filepath.append (fifo);

        int fd = open (filepath.c_str(), O_WRONLY);

        if (fd == -1) {
            LERROR << LOGNODE << "Cannot open output for writing: " << filepath;
            continue;
        }

        fd_out_[fifo] = fd;
    }
#endif
} // OpenOutputs


void
Node::CloseOutputs()
{
#ifndef _WIN32
    for (const auto& fd : fd_out_) {
        int stat = close (fd.second);

        if (stat == -1) {
            LERROR << LOGNODE << "Cannot close output file descriptor: " << fd.first;
        }
    }
#endif
} // CloseOutputs


#ifndef _WIN32
int
Node::ReadInputs (vector<string>& inputs)
{
    struct pollfd pfds[fd_in_.size()];

    int i = 0;

    for (const auto& fd : fd_in_) {
        pfds[i].fd = fd.second;
        pfds[i].events = POLLIN;
        i++;
    }

    uint32_t BUFFSIZE = 8192;
    char cbuffer[BUFFSIZE + 1];
    string input;

    auto ret = poll (pfds, fd_in_.size(), 2);

    if (ret > 0) {
        i = 0;

        for (const auto& fd : fd_in_) {
            if (pfds[i].revents) {
                ssize_t numbytes = 0;

                do {
                    numbytes = read (fd.second, cbuffer, BUFFSIZE);
                    if (numbytes > 0) {
                        cbuffer[numbytes] = '\0';
                        input += cbuffer;
                        totalbytesread_ += numbytes;
                    }
                } while (numbytes > 0 || (numbytes == -1 && errno == EINTR));
            }

            i++;
        }

        m_split_input (input, inputs);
    }

    auto count = std::count (inputs.begin(), inputs.end(), "EOF");

    if (count) {
        eofs_ += int (count);
        LDEBUG << LOGNODE << "EOF COUNT: " << eofs_;
    }

    return (eofs_ == fd_in_.size()) ? -1 : eofs_;
} // ReadInputs


void
Node::WriteOutputs (const string& output)
{
    string token = output + '\n';

    struct pollfd pfds[fd_out_.size()];

    int i = 0;

    for (const auto& fd : fd_out_) {
        pfds[i].fd = fd.second;
        pfds[i].events = POLLOUT;
        i++;
    }

    size_t byteswritten = 0;
    auto totalbytes = token.size() * fd_out_.size();
    int ret = 0;

    do {
        ret = poll (pfds, fd_out_.size(), 2);

        if (ret > 0) {
            i = 0;

            for (const auto& fd : fd_out_) {
                if (pfds[i].revents) {
                    size_t numbytes = 0;
                    size_t tokensize = 0;

                    do {
                        tokensize = token.size();
                        numbytes = write (fd.second, token.c_str(), token.size());

                        if (numbytes == -1) {
                            LERROR << LOGNODE << "Cannot write to file descriptor: " << fd.first;
                            continue;
                        }
                        else if (numbytes == tokensize) {
                            pfds[i].events = 0;
                        }
                        else if (numbytes) {
                            token = output.substr (numbytes) + '\n';
                        }

                        byteswritten += numbytes;
                        totalbyteswritten_ += numbytes;
                    } while (numbytes < tokensize || (numbytes == -1 && errno == EINTR));
                }

                i++;
            }
        }
    } while (byteswritten < totalbytes);
} // WriteOutputs

#else
void
Node::OpenWindowsPipes (const string& sandbox_)
{
    // The GUI will allow duplicate connections on a concat node.
    // This was the path of least resistance to handle this.
    const std::set uniq_outputs (outputs_.begin(), outputs_.end());
    const std::set uniq_inputs (inputs_.begin(), inputs_.end());

    {
        std::unique_lock lock (context_->sync_mutex_);

        for (const auto& fifo : uniq_outputs) {
            auto pipename = get_pipename (sandbox_, fifo);

            HANDLE hwrite = CreateNamedPipeA(
                pipename.c_str(),
                PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE| PIPE_WAIT,
                1,
                0,
                0,
                0,
                nullptr);

            if (hwrite == INVALID_HANDLE_VALUE) {
                LERROR << "Error creating named pipe: " << pipename << " " << GetLastError();
                break;
            }

            fd_out_[fifo] = hwrite;
        }

        context_->nodes_ready[id_] = true;
        context_->sync_cv_.notify_all();
    }

    for (const auto& fifo : uniq_outputs) {
        auto pipename = get_pipename (sandbox_, fifo);
        BOOL connected = ConnectNamedPipe (fd_out_[fifo], nullptr);
        auto error = GetLastError();
        if (!connected && error != ERROR_PIPE_CONNECTED) {
            LERROR << LOGNODE << "Error connecting to named pipe: " << error << " " << pipename;
        }
        else {
            LDEBUG << LOGNODE << "Connected to named pipe: " << pipename;
            PipeInfo pipeInfo{};
            pipeInfo.handle = fd_out_[fifo];

            pipeInfo.event = CreateEvent (nullptr, TRUE, FALSE, nullptr);
            if (pipeInfo.event == nullptr) {
                LERROR << LOGNODE << "Failed to create write event.";
            }
            else {
                ZeroMemory (&pipeInfo.overlapped, sizeof(OVERLAPPED));
                pipeInfo.overlapped.hEvent = pipeInfo.event;
                write_events_.push_back (pipeInfo);
            }
        }
    }

    for (const auto& fifo : uniq_inputs) {
        auto pipename = get_pipename (sandbox_, fifo);

        {
            std::unique_lock lock (context_->sync_mutex_);
            vector<string> tokens;
            m_split (fifo, ".", tokens);
            string parentname = tokens[0];
            context_->sync_cv_.wait (lock, [&]() { return context_->nodes_ready[parentname]; });
        }

        while (!terminate_.load()) {
            HANDLE hread = CreateFileA(
                pipename.c_str(),
                GENERIC_READ,
                0,
                nullptr,
                OPEN_EXISTING,
                FILE_FLAG_OVERLAPPED,
                nullptr
            );

            if (hread != INVALID_HANDLE_VALUE) {
                LDEBUG << LOGNODE << "Handle created: " << fifo;
                fd_in_[fifo] = hread;
                break;
            }

            DWORD error = GetLastError();
            if (error != ERROR_PIPE_BUSY && error != ERROR_FILE_NOT_FOUND) {
                LERROR << LOGNODE << "Failed to open pipe: " << pipename << " " << GetLastError();
                return;
            }

            if (!WaitNamedPipeA (pipename.c_str(), 5000)) {
                LDEBUG << LOGNODE << "WaitNamedPipe failed: " << pipename;
            }
        }

        PipeInfo pipeInfo{};
        pipeInfo.handle = fd_in_[fifo];

        // Create an event for each pipe
        pipeInfo.event = CreateEvent (nullptr, TRUE, FALSE, nullptr);
        if (pipeInfo.event == nullptr) {
            LERROR << LOGNODE << "Failed to create read event.";
        }
        else {
            ZeroMemory (&pipeInfo.overlapped, sizeof(OVERLAPPED));
            pipeInfo.overlapped.hEvent = pipeInfo.event;
            read_events_.push_back (pipeInfo);
        }
    }

    terminate_event_ = CreateEvent (nullptr, TRUE, FALSE, nullptr);
    if (terminate_event_ == nullptr || terminate_event_ == INVALID_HANDLE_VALUE) {
        LERROR << LOGNODE << "Failed to create event. Error: " << GetLastError();
        return;
    }

    {
        std::unique_lock lock (context_->close_mutex_);
        ++context_->nodecount;
    }
} // OpenPipes


void
Node::CloseWindowsPipes()
{
    {
        std::unique_lock lock (context_->close_mutex_);
        --context_->nodecount;

        if (context_->nodecount == 0) {
            context_->close_cv_.notify_all();
        }
        else {
            context_->close_cv_.wait(lock, [&] { return context_->nodecount == 0; });
        }
    }

    for (const auto& [fifo, handle] : fd_out_) {
        FlushFileBuffers (handle);
        BOOL disconnected = DisconnectNamedPipe (handle);

        if (!disconnected) {
            LERROR << LOGNODE << "Error disconnecting named pipe: " << fifo;
        }
        else {
            LDEBUG << LOGNODE << "Closed output named pipe: " << fifo;
            CloseHandle (handle);
        }
    }

    for (const auto& [fifo, handle] : fd_in_) {
        CloseHandle (handle);
    }

    for (const auto& pipeinfo: read_events_) {
        CloseHandle (pipeinfo.event);
    }

    for (const auto& pipeinfo: write_events_) {
        CloseHandle (pipeinfo.event);
    }

    CloseHandle (terminate_event_);
    terminate_event_ = nullptr;

    fd_in_.clear();
    fd_out_.clear();
    write_events_.clear();
    read_events_.clear();
    Reset();
} // ClosePipes


int
Node::ReadInputs (std::vector<std::string>& inputs)
{
    if (terminate_.load())
        return -1;

    constexpr uint32_t BUFFSIZE = 8192;
    std::vector<HANDLE> events;                         // Events for overlapped writes to wait for
    std::vector<size_t> event_indices;                  // Map events to indices in fd_out_
    string input;
    size_t terminate_idx;

    size_t index = 0;
    // Issue asynchronous reads for all pipes before waiting for events
    for (auto& pipeinfo : read_events_) {
        DWORD bytesread_ = 0;
        DWORD error;
        pipeinfo.message.resize (BUFFSIZE);

        do {
            BOOL stat = ReadFile (pipeinfo.handle, &pipeinfo.message[0], BUFFSIZE, &bytesread_, &pipeinfo.overlapped);
            error = GetLastError();

            if (!stat) {
                if (error == ERROR_IO_PENDING) {
                    events.push_back (pipeinfo.overlapped.hEvent);
                    event_indices.push_back (index);
                    break;
                }
                if (error == ERROR_BROKEN_PIPE) {
                    LERROR << LOGNODE << "Broken pipe.";
                    return -1;
                }
                LERROR << LOGNODE << "Error initiating asynchronous read: " << error;
                return -1;
            }

            if (bytesread_) {
                pipeinfo.message.resize (bytesread_);
                input += pipeinfo.message;
                totalbytesread_ += bytesread_;
                LDEBUG << LOGNODE << "Synchronous bytes read: " << bytesread_;
            }
        } while (bytesread_ && error == ERROR_MORE_DATA);

        ++index;
    }

    std::vector completed (events.size(), false);

    if (!events.empty()) {
        events.push_back (terminate_event_);
        terminate_idx = events.size() - 1;
    }

    while (!events.empty() && !terminate_.load()) {
        // Wait for any of the named pipes to signal they have data
        const DWORD waitResult = WaitForMultipleObjects(
            static_cast<DWORD>(events.size()),  // Number of events
            events.data(),                      // Array of event handles
            FALSE,                              // Wait for any event
            INFINITE
        );

        if (waitResult == WAIT_FAILED) {
            LERROR << LOGNODE << "WaitForMultipleObjects failed.";
            return -1;
        }

        DWORD eventIndex = waitResult - WAIT_OBJECT_0;
        if (eventIndex >= read_events_.size()) {
            LERROR << LOGNODE << "Invalid index.";
            return -1;
        }
        if (eventIndex == terminate_idx) {
            LDEBUG << LOGNODE << "Termination event signaled.";
            ResetEvent (terminate_event_);
            return -1;
        }

        PipeInfo& pipeinfo = read_events_[event_indices[eventIndex]];

        // Use GetOverlappedResult to confirm the read operation and get the number of bytes read
        DWORD error;
        DWORD bytesread_ = 0;
        BOOL result = GetOverlappedResult (pipeinfo.handle, &pipeinfo.overlapped, &bytesread_, TRUE);

        if (!result) {
            error = GetLastError();
            if (error == ERROR_BROKEN_PIPE) {
                LERROR << LOGNODE << "Broken pipe.";
                return -1;
            }
            if (error != ERROR_MORE_DATA) {
                LERROR << LOGNODE << "GetOverlappedResult failed with error: " << error;
                return -1;
            }

            while (bytesread_ && error == ERROR_MORE_DATA) {
                input += pipeinfo.message;
                BOOL stat = ReadFile (pipeinfo.handle, &pipeinfo.message[0], BUFFSIZE, &bytesread_, &pipeinfo.overlapped);
                error = GetLastError();

                if (!stat && error != ERROR_MORE_DATA) {
                    if (error == ERROR_BROKEN_PIPE) {
                        LERROR << LOGNODE << "Broken pipe.";
                        return -1;
                    }
                    LERROR << LOGNODE << "Error initiating asynchronous read: " << error;
                    return -1;
                }

                if (bytesread_) {
                    pipeinfo.message.resize (bytesread_);
                    input += pipeinfo.message;
                    totalbytesread_ += bytesread_;
                    LDEBUG << LOGNODE << "Asynchronous bytes read: " << bytesread_;
                }
            }
        }
        else {
            pipeinfo.message.resize (bytesread_);
            input += pipeinfo.message;
            totalbytesread_ += bytesread_;
            LDEBUG << LOGNODE << "Asynchronous bytes read: " << bytesread_;
        }

        completed[eventIndex] = true;
        ResetEvent (pipeinfo.overlapped.hEvent);

        if (ranges::all_of (completed, [](bool complete) { return complete; })) {
            break;
        }
    }

    m_split_input (input, inputs);

    if (auto count = ranges::count (inputs, "EOF")) {
        eofs_ += static_cast<int>(count);
        LDEBUG << LOGNODE << "EOF COUNT: " << eofs_;
    }

    LDEBUG << LOGNODE << "Total Bytes read: " << totalbytesread_;

    return (eofs_ == fd_in_.size()) ? -1 : eofs_;
}


void
Node::WriteOutputs(const std::string& output)
{
    if (terminate_.load())
        return;

    if (output == "EOF") {
        LDEBUG << LOGNODE << "Writing EOF.";
    }

    std::string token = output + '\n';
    std::vector<DWORD> byteswritten (fd_out_.size(), 0); // Tracks bytes written for each pipe
    std::vector<HANDLE> events;                          // Events for overlapped writes to wait for
    std::vector<size_t> event_indices;                   // Map events to indices in fd_out_
    std::vector<bool> completed (fd_out_.size(), false); // Track if writes are complete

    size_t index = 0;
    for (auto it = fd_out_.begin(); it != fd_out_.end(); ++it, ++index) {
        HANDLE handle = it->second;

        write_events_[index].overlapped.Offset = 0;
        write_events_[index].overlapped.OffsetHigh = 0;

        ResetEvent (write_events_[index].event);

        // Issue the asynchronous write
        BOOL fSuccess = WriteFile(
            handle,
            token.data(),                     // Pointer to the buffer
            static_cast<DWORD>(token.size()), // Total size of the token to write
            &byteswritten[index],             // Immediate bytes written (if synchronous)
            &write_events_[index].overlapped        // Overlapped structure for async
        );

        if (fSuccess) {
            // Write completed synchronously
            totalbyteswritten_ += byteswritten[index];
            completed[index] = true;
            FlushFileBuffers (handle);
            LDEBUG << LOGNODE << "Write completed synchronously for pipe handle: " << handle;
        }
        else {
            DWORD error = GetLastError();
            if (error == ERROR_IO_PENDING) {
                // Write is pending, add the event handle to the list of events to wait for
                events.push_back (write_events_[index].event);
                event_indices.push_back (index); // Map this event to the current index
            }
            else {
                LERROR << LOGNODE << "WriteFile failed with error: " << error;
                return;
            }
        }
    }

    if (events.empty()) {
        return;
    }

    // Asynchronous I/O ...

    events.push_back (terminate_event_);
    size_t terminateEventIndex = events.size() - 1;

    // Loop until all writes are complete or the termination event is signaled
    while (!terminate_.load()) {
        DWORD waitResult = WaitForMultipleObjects(
            static_cast<DWORD>(events.size()), // Number of events
            events.data(),                     // Array of event handles
            FALSE,                             // Wait for any event
            INFINITE                           // No timeout
        );

        if (waitResult == WAIT_FAILED) {
            LERROR << LOGNODE << "WaitForMultipleObjects failed with error: " << GetLastError();
            return;
        }

        DWORD eventIndex = waitResult - WAIT_OBJECT_0;

        if (eventIndex == terminateEventIndex) {
            LDEBUG << LOGNODE << "Termination event signaled, exiting WriteOutputs.";
            return;
        }

        if (eventIndex < event_indices.size()) {
            size_t index = event_indices[eventIndex];
            HANDLE handle = std::next (fd_out_.begin(), index)->second;

            DWORD dwWritten = 0;
            BOOL overlappedResult = GetOverlappedResult(
                handle,
                &write_events_[index].overlapped,
                &dwWritten,
                FALSE
            );

            if (!overlappedResult) {
                DWORD error = GetLastError();
                LERROR << LOGNODE << "GetOverlappedResult failed with error: " << error;
                return;
            }

            totalbyteswritten_ += dwWritten;
            completed[index] = true;

            FlushFileBuffers (handle);
            LDEBUG << LOGNODE << "Write completed asynchronously for pipe handle: " << handle;

            ResetEvent (write_events_[index].event);

            // Adjust indices
            events.erase (events.begin() + eventIndex);
            event_indices.erase (event_indices.begin() + eventIndex);
            terminateEventIndex = events.size() - 1;
        }

        if (ranges::all_of (completed, [](bool complete) { return complete; })) {
            break;
        }
    }
}
#endif


void
Node::Cleanup()
{
    CloseOutputs();
    CloseInputs();
} // Cleanup


void
Node::Reset()
{
#ifndef _WIN32
    fd_in_.clear();
    fd_out_.clear();
#endif
    eofs_ = 0;
    totalbytesread_ = 0;
    totalbyteswritten_ = 0;
    terminate_.store (false);
}

int
Node::input_index (const string& id) const
{
    int idx = 0;

    for (const auto& input : inputs_) {
        if (input == id) {
            break;
        }
        idx++;
    }

    return idx;
} // input_index


std::map<string, vector<unsigned int>>
Node::input_indices() const
{
    int idx = 0;
    std::map<string, vector<unsigned int>> indices;

    for (const auto& input : inputs_) {
        if (!indices.contains (input)) {
            indices[input];
        }
        indices[input].push_back (idx);
        idx++;
    }

    return indices;
}

} // namespace daisychain
