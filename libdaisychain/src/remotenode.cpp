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

#include "remotenode.h"


namespace daisychain {
using namespace std;


RemoteNode::RemoteNode() : Node() { Init_(); }


bool
RemoteNode::Init_()
{
    type_ = DaisyNodeType::DC_REMOTE;
    set_host_id ("");

    return true;
} // RemoteNode::Initialize_


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
