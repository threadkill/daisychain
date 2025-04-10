// # MIT License
// # Copyright (c) 2025 Stephen J. Parker
// # SPDX-License-Identifier: MIT
// # See LICENSE file for full license text.
//

#include "filternode.h"
#ifdef _WIN32
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#else
#include <fnmatch.h>
#endif
#include <regex>
#include <utility>


#if defined(__APPLE__)
#define FNM_EXTMATCH 0
#endif // if defined(__APPLE__)


namespace daisychain {
using namespace std;


FilterNode::FilterNode() :
    regex_ (false),
    invert_ (false)
{
    type_ = DaisyNodeType::DC_FILTER;
    set_name (DaisyNodeNameByType[type_]);
}


FilterNode::FilterNode (string filter, bool is_regex = false, bool negate = false) :
    regex_ (is_regex),
    invert_ (negate),
    filter_ (std::move (filter))
{
    type_ = DaisyNodeType::DC_FILTER;
    set_name (DaisyNodeNameByType[type_]);
}


void
FilterNode::Initialize (json& keydata, bool keep_uuid)
{
    Node::Initialize (keydata, keep_uuid);

    json::iterator jit = keydata.begin();
    const auto& uuid = jit.key();
    auto data = keydata[uuid];

    set_filter (data["filter"]);
    set_regex (data["regex"]);

    if (data.count ("negate")) {
        set_invert (data["negate"]);
    }
}


bool
FilterNode::Execute (vector<string>& inputs, const string& sandbox, json& vars)
{
    LINFO << "Executing " << (isroot_ ? "root: " : "node: ") << name_;

    std::regex expr;

    if (regex_) {
        try {
            expr = std::regex (filter_);
        }
        catch (const std::regex_error& e) {
            LERROR << "regex_error caught: " << e.what();
            OpenOutputs (sandbox);
            WriteOutputs ("EOF");
            CloseOutputs();

            return false;
        }
    }

    if (isroot_) {
        LDEBUG << "Root: " << name_;

        for (auto& input : inputs) {
            bool match = false;

            if (regex_) {
                match = std::regex_match (input, expr);
            }
#ifdef _WIN32
            else if (PathMatchSpec (input.c_str(), filter_.c_str())) {
#else
            else if (fnmatch (filter_.c_str(), input.c_str(), FNM_PERIOD | FNM_EXTMATCH) == 0) {
#endif
                match = true;
            }

            if (match ^ invert_) {
                LDEBUG << "Matched: " << input;
                OpenOutputs (sandbox);
                WriteOutputs (input);
                CloseOutputs();
            }
            else {
                LDEBUG << "No Match." << input;
            }
        }
    }
    else {
        while (eofs_ <= fd_in_.size() && !terminate_.load()) {
            for (auto& input : inputs) {
                if (input != "EOF") {
                    bool match = false;

                    if (regex_) {
                        match = std::regex_match (input, expr);
                    }
#ifdef _WIN32
                    else if (PathMatchSpec (input.c_str(), filter_.c_str())) {
#else
                    else if (fnmatch (filter_.c_str(), input.c_str(), FNM_PERIOD | FNM_EXTMATCH) == 0) {
#endif
                        match = true;
                    }

                    if (match ^ invert_) {
                        LDEBUG << "Matched: " << input;
                        OpenOutputs (sandbox);
                        WriteOutputs (input);
                        CloseOutputs();
                    }
                    else {
                        LDEBUG << "No Match." << input;
                    }
                }
            }

            inputs.clear();

            if (eofs_ == fd_in_.size()) {
                break;
            }
            ReadInputs (inputs);
        }
        CloseInputs();
    }

    // all processing is done for this node. Send EOF downstream.
    OpenOutputs (sandbox);
    WriteOutputs ("EOF");
    CloseOutputs();
    Stats();
    Reset();

    return true;
} // FilterNode::Execute


json
FilterNode::Serialize()
{
    auto json_ = Node::Serialize();
    json_[id_]["filter"] = filter_;
    json_[id_]["regex"] = regex_;
    json_[id_]["negate"] = invert_;

    if (size_ != std::pair<int, int>(0,0)) {json_[id_]["size"] = size_;}

    return json_;
} // FilterNode::Serialize


void
FilterNode::set_regex (bool is_regex)
{
    regex_ = is_regex;
} // FilterNode::set_regex


bool
FilterNode::regex() const
{
    return regex_;
} // FilterNode::regex


void
FilterNode::set_invert (bool invert)
{
    invert_ = invert;
} // FilterNode::set_invert


bool
FilterNode::invert() const
{
    return invert_;
} // FilterNode::invert


void
FilterNode::set_filter (const string& filter)
{
    filter_ = filter;
} // FilterNode::set_filter


string
FilterNode::filter()
{
    return filter_;
} // FilterNode::filter
} // namespace daisychain
