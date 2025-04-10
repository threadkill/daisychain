// MIT License
// Copyright (c) 2025 Stephen J. Parker
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text.

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


