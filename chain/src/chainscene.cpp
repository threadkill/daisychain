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

#include "chainscene.h"

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGraphicsObject>
#include <QtWidgets/QTreeWidget>


ChainScene::ChainScene (GraphModel* model, QWidget* parent) :
    DataFlowGraphicsScene (static_cast<DataFlowGraphModel&>(*model), parent)
{
    // when moving multiple selected nodes, only a single nodeMoved signal is sent so we force update all positions.
    connect (this, &DataFlowGraphicsScene::nodeMoved, this, &ChainScene::updatePositions);
    connect (this, &DataFlowGraphicsScene::nodeClicked, dynamic_cast<GraphModel*>(&graphModel()), &GraphModel::updateNodeSize);
    connect (dynamic_cast<GraphModel*>(&graphModel()), &DataFlowGraphModel::nodeCreated, this, &ChainScene::updateGraphicsObject);
    connect (dynamic_cast<GraphModel*>(&graphModel()), &GraphModel::clearSelection, this, &ChainScene::clearSelection);
    connect (dynamic_cast<GraphModel*>(&graphModel()), &GraphModel::clearScene, this, &ChainScene::clearScene);
    connect (dynamic_cast<GraphModel*>(&graphModel()), &GraphModel::nodeUpdated, this, &BasicGraphicsScene::onNodeUpdated);
}


void
ChainScene::updateGraphicsObject (const NodeId qtnode)
{
    const auto model = dynamic_cast<GraphModel*>(&graphModel());
    const auto datamodel = model->delegateModel<ChainModel>(qtnode);

    if (datamodel->name() == "Shell Command") {
        reinterpret_cast<QGraphicsObject*>(nodeGraphicsObject(qtnode))->setToolTip (
            "Shell Command:\n"
            " - Executes any command-line program in a shell environment.\n"
            " - Shell variables can be passed in using the variables panel.\n"
            " - When batch is checked, execution is deferred until all inputs are read.\n"
            " - Batched inputs are concatenated into a single, newline-separated string.\n"
            " - Output field can be set to ${STDOUT} to capture output from a command.");
    }
    else if (datamodel->name() == "Filter") {
        reinterpret_cast<QGraphicsObject*>(nodeGraphicsObject(qtnode))->setToolTip (
            "Filter:\n"
            " - Matches inputs against glob or regular expression patterns.\n"
            " - Unmatched inputs are filtered out.\n"
            " - Use invert to filter out matches.");
    }
    else if (datamodel->name() == "Concat") {
        reinterpret_cast<QGraphicsObject*>(nodeGraphicsObject(qtnode))->setToolTip (
            "Concat:\n"
            " - Concatenates all inputs into a single output.");
    }
    else if (datamodel->name() == "Distro") {
        reinterpret_cast<QGraphicsObject*>(nodeGraphicsObject(qtnode))->setToolTip (
            "Distro:\n"
            " - Distributes inputs evenly across outputs (round-robin).");
    }
    else if (datamodel->name() == "FileList") {
        reinterpret_cast<QGraphicsObject*>(nodeGraphicsObject(qtnode))->setToolTip (
            "FileList:\n"
            " - Sets output based on the contents of the input file.");
    }
    else if (datamodel->name() == "Watch") {
        reinterpret_cast<QGraphicsObject*>(nodeGraphicsObject(qtnode))->setToolTip (
            "Watch:\n"
            " - Watches all file inputs for changes.\n"
            " - Recursively watches directories for file changes.\n"
            " - Writes changed files to output.\n"
            " - 'Passthru' processes all inputs before enabling watch.\n"
            " - Execution continues until program exit.");
    }

    reinterpret_cast<QGraphicsObject*>(nodeGraphicsObject(qtnode))->setSelected (true);
}


void
ChainScene::updatePositions()
{
    auto graph_ = dynamic_cast<GraphModel*>(&graphModel());

    for (auto id : selectedNodes()) {
        auto obj = reinterpret_cast<QGraphicsObject*>(nodeGraphicsObject(id));
        graph_->updatePosition (id, obj->scenePos());
    }
}


QMenu*
ChainScene::createSceneMenu (QPointF const scenePos)
{
    QMenu *modelMenu = DataFlowGraphicsScene::createSceneMenu (scenePos);
    if (auto* treeView = modelMenu->findChild<QTreeView*>()) {
        treeView->setSortingEnabled(true);
        treeView->sortByColumn (0, Qt::AscendingOrder);
    }

    return modelMenu;
}
