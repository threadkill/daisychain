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

#include "logger.h"
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QGraphicsItem>
#include <QtNodes/NodeDelegateModel>
#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/GraphicsView>


using QtNodes::GraphicsView;
using QtNodes::NodeId;
using QtNodes::NodeRole;
using QtNodes::NodeDelegateModel;
using QtNodes::BasicGraphicsScene;
using QtNodes::NodeGraphicsObject;
using QtNodes::DataFlowGraphicsScene;


class ChainView : public GraphicsView
{
    Q_OBJECT

public:

    ChainView();

    void setScene (ChainScene*);

public Q_SLOTS:

    void droppedFromWindow (QDropEvent*);

    void createRenameContextMenu (QtNodes::NodeId, QPointF);

protected Q_SLOTS:

    void dropEvent (QDropEvent*) override;

    void dragEnterEvent (QDragEnterEvent*) override;

    void dragMoveEvent (QDragMoveEvent*) override;

private:

    void keyReleaseEvent (QKeyEvent*) override;

    QMenu* renameMenu_{};
    QLineEdit* renameTxt_{};
    QAction* renameAction_{};
};
