// MIT License
// Copyright (c) 2025 Stephen J. Parker
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text.

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

    void resizeEvent (QResizeEvent*) override;

private:

    void keyReleaseEvent (QKeyEvent*) override;

    QMenu* renameMenu_{};
    QLineEdit* renameTxt_{};
    QAction* renameAction_{};
    QLabel* dragndrop_label_{};
};
