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

#include <QWidgetAction>

#include "graphmodel.h"
#include "chainscene.h"
#include "chainview.h"



ChainView::ChainView() : GraphicsView()
{
    setAcceptDrops (true);

    dragndrop_label_ = new QLabel ("Drag-n-Drop\n Input Files", this);
    dragndrop_label_->setStyleSheet ("color: rgba(255,255,255,32); background-color: rgba(0,0,0,0);");
    dragndrop_label_->setFont (QFont("Arial", 32, QFont::Bold));
    dragndrop_label_->adjustSize();
    dragndrop_label_->setAttribute (Qt::WA_TransparentForMouseEvents);
}


void ChainView::setScene (ChainScene* chainscene)
{
    GraphicsView::setScene (chainscene);
    connect (chainscene, &ChainScene::nodeContextMenu, this, &ChainView::createRenameContextMenu);

    connect (dynamic_cast<GraphModel*>(&chainscene->graphModel()), &GraphModel::inputUpdated, [&] {
        const auto _chainscene = dynamic_cast<ChainScene*>(scene());
        const auto _model = dynamic_cast<GraphModel*>(&_chainscene->graphModel());
        dragndrop_label_->setVisible (_model->input().empty());
    });
}

void
ChainView::droppedFromWindow (QDropEvent* event)
{
    dropEvent (event);
} // ChainView::droppedFromWindow


void
ChainView::createRenameContextMenu (const QtNodes::NodeId node, const QPointF pos)
{
    auto menu = new QMenu (this);
    auto font = menu->font();
    font.setPixelSize (11);
    menu->setFont (font);

    renameAction_ = new QAction ("Rename", this);

    menu->addAction (renameAction_);

    renameMenu_ = new QMenu (this);
    renameMenu_->setAttribute(Qt::WA_DeleteOnClose);
    renameTxt_ = new QLineEdit (renameMenu_);
    renameTxt_->setClearButtonEnabled (true);

    auto renameTxt_Action = new QWidgetAction (renameMenu_);
    renameTxt_Action->setDefaultWidget (renameTxt_);

    renameMenu_->addAction (renameTxt_Action);

    connect (renameAction_, &QAction::triggered, [&]() {
        auto view_pos = mapFromScene (pos);
        auto global_pos = mapToGlobal (view_pos);
        renameTxt_->setFocus();
        renameMenu_->exec (global_pos);
    });

    connect (renameTxt_, &QLineEdit::editingFinished, [&]() {
        auto _scene = reinterpret_cast<DataFlowGraphicsScene*> (this->scene());
        auto _model = dynamic_cast<GraphModel*>(&_scene->graphModel());
        auto _delegate = _model->delegateModel<ChainModel>(node);
        auto text = renameTxt_->text();
        _delegate->setNodeName (text);
        _model->updateDaisyNode (_delegate);
        renameMenu_->close();
    });

    auto view_pos = mapFromScene (pos);
    auto global_pos = mapToGlobal (view_pos);
    menu->exec (global_pos);
} // ChainView::createRenameContextMenu


void
ChainView::dropEvent (QDropEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        const auto chainscene = dynamic_cast<ChainScene*> (scene());
        const auto model = dynamic_cast<GraphModel*>(&chainscene->graphModel());
        std::string paths;
        QString graph;

        auto urls = event->mimeData()->urls();

        for (const auto& url : urls) {
            if (url.toLocalFile().endsWith (".dcg")) {
                graph = url.toLocalFile();
            }
            else {
                auto path = url.toLocalFile().toStdString();
                paths += path + '\n';
            }
        }

        event->acceptProposedAction();

        if (!paths.empty()) {
            auto input = model->input();
            model->updateInput (input+paths);
            dragndrop_label_->hide();
        }

        if (!graph.isEmpty()) {
            auto button = QMessageBox::question (this, tr ("Daisy"), tr ("Import graph?"));

            if (button == QMessageBox::Yes) {
                chainscene->clearSelection();
                model->loadGraph (graph, true);
                setSceneRect (chainscene->itemsBoundingRect());
                auto scenerect = chainscene->sceneRect();
                fitInView (scenerect, Qt::KeepAspectRatio);
            }
        }
    }
} // ChainView::dropEvent


void
ChainView::dragEnterEvent (QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat ("text/uri-list")) {
        event->acceptProposedAction();
    }
} // ChainView::dragEnterEvent


void
ChainView::dragMoveEvent (QDragMoveEvent* event)
{
    // overriding this to no-op. otherwise, drag-n-drop inputs doesn't work properly.
}


void
ChainView::keyReleaseEvent (QKeyEvent *event)
{
    GraphicsView::keyReleaseEvent (event);

    if (event->matches (QKeySequence::Paste)) {
        auto _scene = reinterpret_cast<ChainScene*> (this->scene());
        _scene->updatePositions();
    }
}


void
ChainView::resizeEvent (QResizeEvent* event)
{
    GraphicsView::resizeEvent (event);

    int x = (viewport()->width() - dragndrop_label_->width()) / 2;
    int y = (viewport()->height() - dragndrop_label_->height()) / 2;
    dragndrop_label_->move (x, y);
}
