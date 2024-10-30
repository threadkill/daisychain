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

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11_json/pybind11_json.hpp>
#include "graph.h"


using namespace daisychain;
using json = nlohmann::ordered_json;
namespace py = pybind11;


class PyNode : public Node
{
    using Node::Node;

    bool Execute (const string& sandbox, json& vars) override {
        PYBIND11_OVERLOAD (bool, Node, Execute, sandbox, vars);
    }

    bool Execute (vector<string>& inputs, const string& sandbox, json& vars) override {
        PYBIND11_OVERLOAD_PURE (bool, Node, Execute, inputs, sandbox, vars);
    }

    json Serialize() override {
        PYBIND11_OVERLOAD_PURE (json, Node, Serialize);
    }

    void Initialize (json& data, bool keep_uuid) override {
        PYBIND11_OVERLOAD_PURE (void, Node, Initialize, data, keep_uuid);
    }
};


PYBIND11_MODULE (pydaisychain, m) {

    configureLogger("info");

    m.def ("configureLogger", [](const std::string& level)
                                { return configureLogger (level); }
    );

    py::class_<Graph> (m, "Graph")
        .def (py::init<>())
        .def (py::init<string&>())
        .def ("Initialize", &Graph::Initialize, "", py::arg ("filename") = "")
        .def ("Serialize", &Graph::Serialize)
        .def ("Parse", &Graph::Parse)
        .def ("Save", &Graph::Save, "", py::arg ("filename") = "")
        .def ("PrepareFileSystem", &Graph::PrepareFileSystem)
        .def ("Execute", static_cast<bool (Graph::*)()> (&Graph::Execute))
        .def ("Execute", py::overload_cast<const string&> (&Graph::Execute))
        .def ("Execute", py::overload_cast<const string&, json&> (&Graph::Execute))
        .def ("Execute", py::overload_cast<const string&, const string&> (&Graph::Execute))
        .def ("Test", &Graph::Test)
        .def ("Terminate", &Graph::Terminate)
        .def ("Cleanup", &Graph::Cleanup)
        .def ("Print", &Graph::Print)
        .def ("CreateNode", &Graph::CreateNode)
        .def ("AddNode", &Graph::AddNode, py::keep_alive<1,2>())
        .def ("RemoveNode", &Graph::RemoveNode)
        .def ("Connect", &Graph::Connect)
        .def ("Disconnect", &Graph::Disconnect)
        .def ("get_node", &Graph::get_node)
        .def ("set_filename", &Graph::set_filename)
        .def ("filename", &Graph::filename)
        .def ("set_sandbox", &Graph::set_sandbox)
        .def ("sandbox", &Graph::sandbox)
        .def ("set_environment", &Graph::set_environment)
        .def ("environment", &Graph::environment)
        .def ("set_notes", &Graph::set_notes)
        .def ("notes", &Graph::notes)
        .def ("set_input", &Graph::set_input)
        .def ("input", &Graph::input)
        .def ("set_cleanup_flag", &Graph::set_cleanup_flag)
        .def ("cleanup_flag", &Graph::cleanup_flag)
        .def ("set_test_flag", &Graph::set_test_flag)
        .def ("test_flag", &Graph::test_flag)
        .def ("running", &Graph::running)
        .def ("nodes", &Graph::nodes)
        .def ("edges", &Graph::edges)
        ;

    py::enum_<DaisyNodeType> (m, "DaisyNodeType")
        .value ("DC_INVALID", DaisyNodeType::DC_INVALID)
        .value ("DC_COMMANDLINE", DaisyNodeType::DC_COMMANDLINE)
        .value ("DC_REMOTE", DaisyNodeType::DC_REMOTE)
        .value ("DC_FILTER", DaisyNodeType::DC_FILTER)
        .value ("DC_CONCAT", DaisyNodeType::DC_CONCAT)
        .value ("DC_DISTRO", DaisyNodeType::DC_DISTRO)
        .value ("DC_FILELIST", DaisyNodeType::DC_FILELIST)
        .value ("DC_WATCHNODE", DaisyNodeType::DC_WATCH)
        .export_values()
        ;

    py::class_<Node, PyNode, std::shared_ptr<Node>> (m, "Node")
        .def (py::init<>())
        .def ("Initialize", &Node::Initialize)
        .def ("Serialize", &Node::Serialize)
        .def ("Execute", py::overload_cast<const string&, json&> (&Node::Execute))
        .def ("Execute", py::overload_cast<vector<string>&, const string&, json&> (&Node::Execute))
        .def ("Stats", &Node::Stats)
        .def ("AddInput", &Node::AddInput)
        .def ("RemoveInput", &Node::RemoveInput)
        .def ("AddOutput", &Node::AddOutput)
        .def ("RemoveOutput", &Node::RemoveOutput)
        .def ("OpenInputs", &Node::OpenInputs)
        .def ("ReadInputs", &Node::ReadInputs)
        .def ("CloseInputs", &Node::CloseInputs)
        .def ("OpenOutputs", &Node::OpenOutputs)
        .def ("WriteOutputs", &Node::WriteOutputs)
        .def ("CloseOutputs", &Node::CloseOutputs)
        .def ("Cleanup", &Node::Cleanup)
        .def ("type", &Node::type)
        .def ("set_id", static_cast<void (Node::*)()> (&Node::set_id))
        .def ("set_id", py::overload_cast<const string&> (&Node::set_id))
        .def ("id", &Node::id)
        .def ("set_name", &Node::set_name)
        .def ("name", &Node::name)
        .def ("set_position", &Node::set_position)
        .def ("position", &Node::position)
        .def ("set_size", &Node::set_position)
        .def ("size", &Node::position)
        .def ("is_root", &Node::is_root)
        .def ("set_batch_flag", &Node::set_batch_flag)
        .def ("batch_flag", &Node::batch_flag)
        .def ("set_test_flag", &Node::set_test_flag)
        .def ("test_flag", &Node::test_flag)
        .def ("set_outputfile", &Node::set_outputfile)
        .def ("outputfile", &Node::outputfile)
        .def ("input_index", &Node::input_index)
        .def ("input_indices", &Node::input_indices)
        .def ("shell_expand", &Node::shell_expand)
        .def ("concat_inputs", &Node::concat_inputs)
        ;

    py::class_<CommandLineNode, Node, std::shared_ptr<CommandLineNode>> (m, "CommandLineNode")
        .def (py::init<>())
        .def (py::init<const string&>())
        .def ("Execute", py::overload_cast<vector<string>&, const string&, json&> (&CommandLineNode::Execute))
        .def ("Initialize", &CommandLineNode::Initialize)
        .def ("Serialize", &CommandLineNode::Serialize)
        .def ("set_command", &CommandLineNode::set_command)
        .def ("command", &CommandLineNode::command)
        ;

    py::class_<FilterNode, Node, std::shared_ptr<FilterNode>> (m, "FilterNode")
        .def (py::init<>())
        .def (py::init<string, bool, bool>())
        .def ("Execute", py::overload_cast<vector<string>&, const string&, json&> (&FilterNode::Execute))
        .def ("Initialize", &FilterNode::Initialize)
        .def ("Serialize", &FilterNode::Serialize)
        .def ("set_filter", &FilterNode::set_filter)
        .def ("filter", &FilterNode::filter)
        .def ("set_regex", &FilterNode::set_regex)
        .def ("regex", &FilterNode::regex)
        .def ("set_invert", &FilterNode::set_invert)
        .def ("negate", &FilterNode::invert)
        ;

    py::class_<ConcatNode, Node, std::shared_ptr<ConcatNode>> (m, "ConcatNode")
        .def (py::init<>())
        .def ("Execute", py::overload_cast<vector<string>&, const string&, json&> (&ConcatNode::Execute))
        ;

    py::class_<DistroNode, Node, std::shared_ptr<DistroNode>> (m, "DistroNode")
        .def (py::init<>())
        .def ("Execute", py::overload_cast<vector<string>&, const string&, json&> (&DistroNode::Execute))
        ;

    py::class_<FileListNode, Node, std::shared_ptr<FileListNode>> (m, "FileListNode")
        .def (py::init<>())
        .def ("Execute", py::overload_cast<vector<string>&, const string&, json&> (&FileListNode::Execute))
        ;

    py::class_<WatchNode, Node, std::shared_ptr<WatchNode>> (m, "WatchNode")
        .def (py::init<>())
        .def ("Execute", py::overload_cast<vector<string>&, const string&, json&> (&WatchNode::Execute))
        .def ("Initialize", &WatchNode::Initialize)
        .def ("Serialize", &WatchNode::Serialize)
        .def ("Cleanup", &WatchNode::Cleanup)
        .def ("passthru", &WatchNode::passthru)
        .def ("set_passthru", &WatchNode::set_passthru)
        ;
}

