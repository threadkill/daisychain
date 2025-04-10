// MIT License
// Copyright (c) 2025 Stephen J. Parker
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text.

#include "remotenode.h"


namespace daisychain {
using namespace std;


RemoteNode::RemoteNode()
{
    type_ = DaisyNodeType::DC_REMOTE;
    set_host_id ("");
}


bool
RemoteNode::Execute (const string& sandbox)
{
    return true;
} // RemoteNode::Execute


json
RemoteNode::Serialize()
{
    return {
        {id_, {{"name", name_}, {"type", type_}, {"host_id", host_id_}}}
    };
} // RemoteNode::Serialize


void
RemoteNode::set_host_id (const string& host_id)
{
    host_id_ = host_id;
} // RemoteNode::set_host_id


string
RemoteNode::host_id()
{
    return host_id_;
} // RemoteNode::host_id
} // namespace daisychain
