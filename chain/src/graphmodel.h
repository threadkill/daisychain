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

#pragma once

#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include "graph.h"
#include "commandmodel.h"
#include "concatmodel.h"
#include "distromodel.h"
#include "filelistmodel.h"
#include "filtermodel.h"
#include "watchmodel.h"
#include <QFutureWatcher>
#include <future>


using QtNodes::DataFlowGraphModel;
using QtNodes::ConnectionId;
using QtNodes::NodeDelegateModelRegistry;
using QtNodes::NodeId;
using daisychain::Graph;
using daisychain::CommandLineNode;
using daisychain::ConcatNode;
using daisychain::DistroNode;
using daisychain::FileListNode;
using daisychain::FilterNode;
using daisychain::WatchNode;
using daisychain::Graph;


class GraphModel : public QtNodes::DataFlowGraphModel
{
    Q_OBJECT

public:
    explicit GraphModel (std::shared_ptr<QtNodes::NodeDelegateModelRegistry> registry);

    void loadGraph (QString& filename, bool import = false);

    void saveGraph (QString& filename);

    const std::string& graphLog() { return graphlog_; }

    int createNodeFromNode (const std::shared_ptr<daisychain::Node>&);

    void emitAll();

    const std::string& input();

    unsigned int num_inputs() const { return num_inputs_; }

    std::shared_ptr<Graph> graph() { return graph_; }

public Q_SLOTS:

    void updateEnviron (json&);

    void updateNotes (json&);

    void updateInput (const std::string&);

    void updateDaisyNode (QtNodes::NodeDelegateModel*);

    void updatePosition (const QtNodes::NodeId&, const QPointF&);

    void updateNodeSize (const QtNodes::NodeId&);

    void execute();

    void test();

    void terminate();

    bool running();

    void print();

Q_SIGNALS:

    void nodeUpdated (const QtNodes::NodeId&);

    void environUpdated (const json&);

    void inputUpdated();

    void notesUpdated (const json&);

    void clearSelection();

    void clearScene();

    void finished();

private Q_SLOTS:

    void modelToNodeUpdated (QtNodes::NodeDelegateModel*);

    void addDaisyNode (const QtNodes::NodeId&);

    void removeDaisyNode (const QtNodes::NodeId&);

    void connectDaisyNode (const QtNodes::ConnectionId&);

    void disconnectDaisyNode (const QtNodes::ConnectionId&);

private:
    std::unordered_map<std::string, QtNodes::NodeId> uuid2nodeid_;
    std::unordered_map<QtNodes::NodeId, std::string> nodeid2uuid_;
    std::shared_ptr<Graph> graph_;
    std::string graphlog_;
    QThread* exec_thread;
    unsigned int num_inputs_{0};
};
