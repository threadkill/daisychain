// # MIT License
// # Copyright (c) 2025 Stephen J. Parker
// # SPDX-License-Identifier: MIT
// # See LICENSE file for full license text.
//

#include "chainscene.h"

#include "chainstyles.h"

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
            "Shell Command:\n\n"
            " - Executes any command-line program in a shell environment.\n"
            " - Shell variables can be passed in using the global variables panel.\n"
            " - When batch is checked, execution is deferred until all inputs are read.\n"
            " - Batched inputs are concatenated into a single string.\n"
            " - SANDBOX variable is set to the temp location created for each graph.\n"
            " - INPUT variable is set automatically for reference in the command field.\n"
            " - DIRNAME, FILENAME, STEM and EXT variables are defined for each input.\n"
            " - OUTPUT variable is set to INPUT but can be overridden in the output field.\n"
            " - STDOUT variable is captured automatically if a program writes to stdout."
            );
    }
    else if (datamodel->name() == "Filter") {
        reinterpret_cast<QGraphicsObject*>(nodeGraphicsObject(qtnode))->setToolTip (
            "Filter:\n"
            " - Matches inputs against glob or regular expression patterns.\n"
            " - Unmatched inputs are filtered out.\n"
            " - Use invert to exclude the matches.");
    }
    else if (datamodel->name() == "Concat") {
        reinterpret_cast<QGraphicsObject*>(nodeGraphicsObject(qtnode))->setToolTip (
            "Concat:\n"
            " - Concatenates all inputs into a single output.\n");
    }
    else if (datamodel->name() == "Distro") {
        reinterpret_cast<QGraphicsObject*>(nodeGraphicsObject(qtnode))->setToolTip (
            "Distro:\n"
            " - Distributes inputs across multiple outputs (round-robin).\n"
            " - Skips busy outputs and continously scans for ready outputs."
            );
    }
    else if (datamodel->name() == "FileList") {
        reinterpret_cast<QGraphicsObject*>(nodeGraphicsObject(qtnode))->setToolTip (
            "FileList:\n"
            " - Reads the contents of the input file and writes outputs line-by-line.");
    }
    else if (datamodel->name() == "Watch") {
        reinterpret_cast<QGraphicsObject*>(nodeGraphicsObject(qtnode))->setToolTip (
            "Watch:\n"
            " - Watches all file inputs for changes.\n"
            " - Recursively watches directories for file changes.\n"
            " - Writes changed files to output.\n"
            " - 'Passthru' processes all inputs before enabling watch.\n"
            " - Execution continues until termination or program exit.");
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
    QMenu *menu = DataFlowGraphicsScene::createSceneMenu (scenePos);
    menu->setPalette (darkPalette());

    if (auto* treeview = menu->findChild<QTreeView*>()) {
        treeview->setSortingEnabled (true);
        treeview->sortByColumn (0, Qt::AscendingOrder);
    }

    return menu;
}
