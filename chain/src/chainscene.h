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

#include "chainstyles.h"
#include "graphmodel.h"
#include "graph.h"
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/DataFlowGraphModel>

using daisychain::CommandLineNode;
using daisychain::ConcatNode;
using daisychain::DistroNode;
using daisychain::FileListNode;
using daisychain::FilterNode;
using daisychain::WatchNode;
using daisychain::Graph;
using QtNodes::ConnectionId;
using QtNodes::NodeDelegateModelRegistry;
using QtNodes::DataFlowGraphicsScene;
using QtNodes::DataFlowGraphModel;
using QtNodes::NodeGraphicsObject;
using QtNodes::NodeId;
using QtNodes::PortType;


class ChainScene : public DataFlowGraphicsScene
{
    Q_OBJECT

public:
    explicit ChainScene (GraphModel* model, QWidget* parent);

public Q_SLOTS:

    void updateGraphicsObject (QtNodes::NodeId);

    void updatePositions();

    QMenu* createSceneMenu (QPointF scenePos) override;
};
