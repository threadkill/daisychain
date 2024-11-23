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

#include "graphmodel.h"
#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>
#include <QtCore/QJsonValue>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QTextEdit>
#include <utility>
#include <fstream>



GraphModel::GraphModel (std::shared_ptr<QtNodes::NodeDelegateModelRegistry> registry) :
    DataFlowGraphModel (std::move (registry)),
    graph_{nullptr},
    exec_thread{nullptr}
{
    graph_ = std::make_shared<daisychain::Graph>();
    graph_->PrepareFileSystem();
    graphlog_ = graph_->logfile();

    if (std::ofstream log_ (graphlog_.c_str()); log_) {
        log_.close();
    }
    else {
        LERROR << "Could not open logfile for reading/writing: " << graphlog_;
    }

    connect (this, &DataFlowGraphModel::nodeCreated, this, &GraphModel::addDaisyNode);
    connect (this, &DataFlowGraphModel::nodeDeleted, this, &GraphModel::removeDaisyNode);
    connect (this, &DataFlowGraphModel::connectionCreated, this, &GraphModel::connectDaisyNode);
    connect (this, &DataFlowGraphModel::connectionDeleted, this, &GraphModel::disconnectDaisyNode);
}


void
GraphModel::updateEnviron (json& env)
{
    graph_->set_environment (env);
} // GraphModel::updateEnviron


void
GraphModel::updateNotes (json& notes)
{
    graph_->set_notes (notes);
} // GraphModel::updateNotes


void
GraphModel::updateInput (const std::string& input)
{
    graph_->set_input (input);
    std::vector<string> inputs;
    m_split_input (input, inputs);
    num_inputs_ = static_cast<unsigned int>(inputs.size());
    Q_EMIT (inputUpdated());
} // GraphModel::updateInput


const std::string&
GraphModel::input()
{
    return graph_->input();
} // GraphModel::input


void
GraphModel::addDaisyNode (const NodeId& node)
{
    auto datamodel = delegateModel<ChainModel>(node);
    std::string uid;

    if (datamodel->name() == "Shell Command") {
        auto cmdnode = std::make_shared<CommandLineNode>();
        uid = cmdnode->id();
        graph_->AddNode (cmdnode);
    }
    else if (datamodel->name() == "Filter") {
        auto fltnode = std::make_shared<FilterNode>();
        uid = fltnode->id();
        graph_->AddNode (fltnode);
    }
    else if (datamodel->name() == "Concat") {
        auto catnode = std::make_shared<ConcatNode>();
        uid = catnode->id();
        graph_->AddNode (catnode);
    }
    else if (datamodel->name() == "Distro") {
        auto distronode = std::make_shared<DistroNode>();
        uid = distronode->id();
        graph_->AddNode (distronode);
    }
    else if (datamodel->name() == "FileList") {
        auto filelistnode = std::make_shared<FileListNode>();
        uid = filelistnode->id();
        graph_->AddNode (filelistnode);
    }
    else if (datamodel->name() == "Watch") {
        auto watchnode = std::make_shared<WatchNode>();
        uid = watchnode->id();
        graph_->AddNode (watchnode);
    }

    datamodel->setId (uid);
    uuid2nodeid_[uid] = node;
    nodeid2uuid_[node] = uid;

    updateDaisyNode (datamodel);
    modelToNodeUpdated (datamodel);

    connect (datamodel, &ChainModel::updated, this, &GraphModel::updateDaisyNode);
    connect (datamodel, &ChainModel::updated, this, &GraphModel::modelToNodeUpdated);
} // GraphModel::addDaisyNode


void GraphModel::modelToNodeUpdated (NodeDelegateModel* datamodel)
{
    auto chainmodel = dynamic_cast<ChainModel*> (datamodel);
    auto sz = datamodel->embeddedWidget()->size();

    auto& node = graph_->nodes().at (chainmodel->id());
    node->set_size ({sz.width(), sz.height()});

    auto qtnode = uuid2nodeid_[chainmodel->id()];
    Q_EMIT (nodeUpdated(qtnode));
}


void
GraphModel::updateDaisyNode (NodeDelegateModel* datamodel)
{
    auto chainmodel = dynamic_cast<ChainModel*> (datamodel);
    auto id = chainmodel->id();
    auto sz = chainmodel->embeddedWidget()->size();
    auto& daisynode = graph_->nodes().at (id);
    const auto& node = daisynode.get();

    node->set_name (chainmodel->nodeName().toStdString());

    if (datamodel->name() == "Shell Command") {
        std::string cmd = datamodel->embeddedWidget()
                              ->findChild<QTextEdit*> ("_textEdit")
                              ->toPlainText()
                              .toStdString();
        std::string outputfile = datamodel->embeddedWidget()
                                     ->findChild<QLineEdit*> ("_outputEdit")
                                     ->text()
                                     .toStdString();
        bool batch = datamodel->embeddedWidget()->findChild<QCheckBox*> ("_batchChk")->isChecked();

        const auto& commandnode = dynamic_cast<CommandLineNode*> (daisynode.get());
        commandnode->set_command (cmd);
        commandnode->set_batch_flag (batch);
        commandnode->set_outputfile (outputfile);
        commandnode->set_size ({sz.width(), sz.height()});
    }
    else if (datamodel->name() == "Filter") {
        std::string flt = datamodel->embeddedWidget()
                              ->findChild<QLineEdit*> ("_lineEdit")
                              ->text()
                              .toStdString();
        bool regex = datamodel->embeddedWidget()->findChild<QCheckBox*> ("_regexChk")->isChecked();
        bool invert = datamodel->embeddedWidget()->findChild<QCheckBox*> ("_invertChk")->isChecked();

        const auto& filternode = dynamic_cast<FilterNode*> (daisynode.get());
        filternode->set_filter (flt);
        filternode->set_regex (regex);
        filternode->set_invert (invert);
        filternode->set_size ({sz.width(), sz.height()});
    }
    else if (datamodel->name() == "Watch") {
        bool passthru = datamodel->embeddedWidget()->findChild<QCheckBox*> ("_passthruChk")->isChecked();
        bool recursive = datamodel->embeddedWidget()->findChild<QCheckBox*> ("_recursiveChk")->isChecked();
        const auto& watchnode = dynamic_cast<WatchNode*> (daisynode.get());
        watchnode->set_passthru (passthru);
        watchnode->set_recursive (recursive);
    }
} // GraphModel::updateDaisyNode


void
GraphModel::updatePosition (const NodeId& node, const QPointF& position)
{
    auto datamodel = delegateModel<ChainModel>(node);
    std::string id = datamodel->id();
    auto& daisynode = graph_->nodes().at (id);

    daisynode->set_position (std::pair<float, float> (position.x(), position.y()));
} // GraphModel::updatePosition


void
GraphModel::updateNodeSize (const NodeId& node)
{
    auto datamodel = delegateModel<ChainModel>(node);
    std::string id = datamodel->id();
    auto sz = datamodel->embeddedWidget()->size();
    auto& daisynode = graph_->nodes().at (id);

    daisynode->set_size (std::pair<float, float> (sz.width(), sz.height()));
} // GraphModel::updatePosition


void
GraphModel::removeDaisyNode (const NodeId& node)
{
    graph_->RemoveNode (nodeid2uuid_[node]);
} // GraphModel::removeDaisyNode


void
GraphModel::connectDaisyNode (const ConnectionId& connection)
{
    auto i_node = delegateModel<ChainModel>(connection.inNodeId);
    auto o_node = delegateModel<ChainModel>(connection.outNodeId);

    bool connected = graph_->Connect (o_node->id(), i_node->id());

    if (!connected) {
        auto deleted = deleteConnection (connection);
        LWARN << "Connection failed: " << o_node->id() << " -> " << i_node->id();
        LERROR_IF (!deleted) << "Unable to disconnect node in UI.";
    }
} // GraphModel::connectDaisyNode


void
GraphModel::disconnectDaisyNode (const ConnectionId& connection)
{
    auto i_node = delegateModel<ChainModel>(connection.inNodeId);
    auto o_node = delegateModel<ChainModel>(connection.outNodeId);

    graph_->Disconnect (o_node->id(), i_node->id());
} // GraphModel::disconnectDaisyNode


void
GraphModel::saveGraph (QString& filename)
{
    if (!filename.isEmpty()) {
        if (!filename.endsWith ("dcg", Qt::CaseInsensitive)) {
            filename += ".dcg";
        }

        graph_->Save (filename.toStdString());
    }
} // GraphModel::saveGraph


void
GraphModel::loadGraph (QString& filename, bool import)
{
    if (!QFileInfo::exists (filename)) {
        return;
    }

    if (import) {
        std::ifstream filein (filename.toStdString());

        if (!filein.is_open()) {
            return;
        }

        json json_graph;
        filein >> json_graph;
        filein.close();

        graph_->Parse (json_graph);
        Q_EMIT (clearSelection());
    }
    else {
        Q_EMIT (clearScene());
        if (!graph_->Initialize (filename.toStdString())) {
            return;
        }
    }

    disconnect (this, &DataFlowGraphModel::nodeCreated, this, &GraphModel::addDaisyNode);

    for (const auto& [id, node] : graph_->nodes()) {
        // skip pre-existing nodes (as a result of an import).
        /*
        if (nodes().count (QUuid (QString::fromStdString (id)))) {
            continue;
        }
         */

        auto nid = createNodeFromNode (node);
        if (nid >= 0) {
            Q_EMIT (nodeUpdated (nid));
        }

    }

    if (!import) {
        Q_EMIT (clearSelection());
    }

    connect (this, &DataFlowGraphModel::nodeCreated, this, &GraphModel::addDaisyNode);

    disconnect (this, &DataFlowGraphModel::connectionCreated, this, &GraphModel::connectDaisyNode);

    for (const auto& edge : graph_->edges()) {
        auto node = graph_->nodes().at (edge.second);
        auto connection_str = edge.first + "." + edge.second;
        auto indices_map = node->input_indices();
        auto indices = indices_map[connection_str];

        QtNodes::ConnectionId connection_id = {uuid2nodeid_[edge.first], 0, uuid2nodeid_[edge.second], 0};

        for (auto index : indices) {
            connection_id = {uuid2nodeid_[edge.first], 0, uuid2nodeid_[edge.second], index};
            if (!connectionExists (connection_id)) {
                break;
            }
        }

        if (connectionPossible (connection_id)) {
            addConnection (connection_id);
        }

        if (!connectionExists (connection_id)) {
            disconnectDaisyNode (connection_id);
        }
    }

    connect (this, &DataFlowGraphModel::connectionCreated, this, &GraphModel::connectDaisyNode);

    emitAll();
} // GraphModel::loadGraph


void
GraphModel::emitAll()
{
    Q_EMIT (inputUpdated());
    Q_EMIT (environUpdated (graph_->environment()));
    Q_EMIT (notesUpdated (graph_->notes()));
}


int
GraphModel::createNodeFromNode (const std::shared_ptr<daisychain::Node>& node)
{
    NodeId qtnode{};
    ChainModel* datamodel = nullptr;

    auto xy = node->position();
    QPointF position (xy.first, xy.second);
    auto sz = node->size();
    QPoint size_ (sz.first, sz.second);

    switch (node->type()) {
    case daisychain::DC_COMMANDLINE: {
        qtnode = addNode("Shell Command");

        const auto& commandnode = dynamic_cast<CommandLineNode*> (node.get());
        const QString name = QString::fromStdString (commandnode->name());
        const QString cmd = QString::fromStdString (commandnode->command());
        bool batch = commandnode->batch_flag();
        const QString outputfile = QString::fromStdString (commandnode->outputfile());

        datamodel = delegateModel<ChainModel>(qtnode);
        datamodel->setNodeName (name);
        auto widget = datamodel->embeddedWidget();
        widget->findChild<QTextEdit*> ("_textEdit")->setText (cmd);
        widget->findChild<QLineEdit*> ("_outputEdit")->setText (outputfile);
        widget->findChild<QCheckBox*> ("_batchChk")->setChecked (batch);
    } break;
    case daisychain::DC_REMOTE:
        return -1;
    case daisychain::DC_FILTER: {
        qtnode = addNode("Filter");

        const auto& filternode = dynamic_cast<FilterNode*> (node.get());
        const QString name = QString::fromStdString (filternode->name());
        const QString flt = QString::fromStdString (filternode->filter());
        bool regex = filternode->regex();
        bool invert = filternode->invert();

        datamodel = delegateModel<ChainModel>(qtnode);
        datamodel->setNodeName (name);
        datamodel->embeddedWidget()->findChild<QLineEdit*> ("_lineEdit")->setText (flt);
        datamodel->embeddedWidget()->findChild<QCheckBox*> ("_regexChk")->setChecked (regex);
        datamodel->embeddedWidget()->findChild<QCheckBox*> ("_invertChk")->setChecked (invert);
    } break;
    case daisychain::DC_CONCAT: {
        qtnode = addNode("Concat");

        const auto& concatnode = dynamic_cast<ConcatNode*> (node.get());
        const QString name = QString::fromStdString (concatnode->name());

        datamodel = delegateModel<ChainModel>(qtnode);
        datamodel->setNodeName (name);
    } break;
    case daisychain::DC_DISTRO: {
        qtnode = addNode("Distro");

        const auto& distronode = dynamic_cast<DistroNode*> (node.get());
        const QString name = QString::fromStdString (distronode->name());

        datamodel = delegateModel<ChainModel>(qtnode);
        datamodel->setNodeName (name);
    } break;
    case daisychain::DC_FILELIST: {
        qtnode = addNode("FileList");

        const auto& filelistnode = dynamic_cast<FileListNode*> (node.get());
        const QString name = QString::fromStdString (filelistnode->name());

        datamodel = delegateModel<ChainModel>(qtnode);
        datamodel->setNodeName (name);
    } break;
    case daisychain::DC_WATCH: {
        qtnode = addNode("Watch");

        const auto& watchnode = dynamic_cast<WatchNode*> (node.get());
        const QString name = QString::fromStdString (watchnode->name());
        auto passthru = watchnode->passthru();
        auto recursive = watchnode->recursive();

        datamodel = delegateModel<ChainModel>(qtnode);
        datamodel->setNodeName (name);
        datamodel->embeddedWidget()->findChild<QCheckBox*> ("_passthruChk")->setChecked (passthru);
        datamodel->embeddedWidget()->findChild<QCheckBox*> ("_recursiveChk")->setChecked (recursive);
    } break;
    case daisychain::DC_INVALID:
        return -1;
    } // switch

    if (!datamodel) return -1;

    if (datamodel->resizable() && size_ != QPoint{0,0}) {
        //setNodeData (qtnode, QtNodes::NodeRole::Size, size_);
        datamodel->embeddedWidget()->resize (size_.x(), size_.y());
    }
    setNodeData (qtnode, QtNodes::NodeRole::Position, position);

    datamodel->setId (node->id());
    connect (datamodel, &ChainModel::updated, this, &GraphModel::updateDaisyNode);

    uuid2nodeid_[node->id()] = qtnode;
    nodeid2uuid_[qtnode] = node->id();

    return qtnode;
}


void
GraphModel::execute()
{
    blockSignals (true);
    exec_thread = QThread::create ([this] {
        el::Helpers::setThreadName (this->graphLog());
        this->graph_->Execute();
    });
    connect (exec_thread, SIGNAL (finished()), this, SIGNAL (finished()));
    exec_thread->start();
    blockSignals (false);
} // GraphModel::execute


void
GraphModel::test()
{
    blockSignals (true);
    exec_thread = QThread::create ([this] {
        el::Helpers::setThreadName (this->graphLog());
        this->graph_->Test();
    });
    connect (exec_thread, SIGNAL (finished()), this, SIGNAL (finished()));
    exec_thread->start();
    blockSignals (false);
}


void
GraphModel::terminate()
{
    if (running()) {
        graph_->Terminate();
    }
    while (running()) {
        std::this_thread::sleep_for (std::chrono::milliseconds(100));
    }
}


bool
GraphModel::running()
{
    bool running = false;
    if (exec_thread && exec_thread->isRunning()) {
        running = true;
    }

    return running;
}


void
GraphModel::print()
{
    graph_->Print();
} // GraphModel::print
