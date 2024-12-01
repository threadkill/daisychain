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

#include <QHeaderView>
#include "chainbrowser.h"
#include "chainstyles.h"


ChainBrowser::ChainBrowser() : QTreeView(), model_ (new ChainFileSystemModel)
{
    setPalette (darkPalette());
    setupUI();
    connect (this,
             SIGNAL (doubleClicked (const QModelIndex&)),
             this,
             SLOT (sendFile (const QModelIndex&)));
}


ChainBrowser::~ChainBrowser() = default;


void
ChainBrowser::setupUI()
{
    auto graph_dir
        = QProcessEnvironment::systemEnvironment().value ("DAISYCHAIN_GRAPHS", QDir::homePath());

    setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOff);

    model_->setFilter (QDir::AllDirs | QDir::AllEntries | QDir::NoDotAndDotDot);
    model_->setRootPath ("");
    model_->setNameFilterDisables (false);
    model_->setNameFilters ({"*.dcg"});

#ifndef __APPLE__
    model_->setOption (QFileSystemModel::DontUseCustomDirectoryIcons);
    model_->setOption (QFileSystemModel::DontWatchForChanges);
#endif // ifndef __APPLE__

    auto palette = this->palette();
    palette.setColor (QPalette::Text, QColor (200, 200, 200));
    setPalette (palette);

    setModel (model_);
    setRootIndex (model_->index (""));

    for (int i = 1; i < model_->columnCount(); ++i) {
        hideColumn (i);
    }

    setDragEnabled (true);
    setAcceptDrops (false);
    setAnimated (false);
    setIndentation (8);
    setSortingEnabled (true);
    sortByColumn (0, Qt::AscendingOrder);
    resizeColumnToContents (0);

    auto index = model_->index (graph_dir);
    scrollTo (index);
    expand (index);
    setCurrentIndex (index);

    auto font = QTreeView::font();
    font.setPixelSize (12);
    setFont (font);

    auto header_ = header();
    font = header_->font();
    font.setPixelSize (11);
    header_->setFont (font);
    header_->setWindowIconText ("Graph Files (*.dcg)");
    header_->setFixedHeight (22);
    header_->setDefaultAlignment (Qt::AlignCenter);
} // ChainBrowser::setupUI


void
ChainBrowser::sendFile (const QModelIndex& idx)
{
    auto fileinfo = model_->fileInfo (idx);

    if (fileinfo.isFile()) {
        auto path = model_->filePath (idx);
        Q_EMIT (sendFileSignal (path));
    }
} // ChainBrowser::sendFile
