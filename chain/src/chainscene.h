// MIT License
// Copyright (c) 2025 Stephen J. Parker
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text.

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
