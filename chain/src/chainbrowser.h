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

#include <QFileSystemModel>
#include <QProcessEnvironment>
#include <QScreen>
#include <QTreeView>


class ChainFileSystemModel : public QFileSystemModel
{
Q_OBJECT

public:
    [[nodiscard]] QVariant headerData (int section, Qt::Orientation orientation, int role) const override
    {
        if (section == 0 && role == Qt::DisplayRole)
            return "Graph Files (.dcg)";
        else if (section == 0 && role == Qt::TextAlignmentRole)
            return Qt::AlignCenter;
        else
            return QFileSystemModel::headerData (section, orientation, role);
    };
};


class ChainBrowser : public QTreeView
{
    Q_OBJECT

public:
    ChainBrowser();
    ~ChainBrowser() override;

Q_SIGNALS:
    void sendFileSignal (QString&);

private Q_SLOTS:
    void sendFile (const QModelIndex&);

private:
    void setupUI();

    ChainFileSystemModel* model_;
};


