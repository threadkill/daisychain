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

#include <QtCore/QObject>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <iostream>
#include <QtNodes/NodeDelegateModel>

#include "chainreaddata.h"
#include "chainwritedata.h"


using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;


class ChainModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    ChainModel();

    QJsonObject save() const override;

    void load (QJsonObject const &) override;

    QWidget* embeddedWidget() override; // embeddedWidget

    QString caption() const override
    {
        if (_nodename.isEmpty()) {
            return name();
        }
        else {
            return _nodename;
        }
    } // caption

    bool captionVisible() const override { return true; } // captionVisible

    void setNodeName (const QString& name) { _nodename = name; }

    QString& nodeName() { return _nodename; }

    unsigned int nPorts (PortType portType) const override;

    NodeDataType dataType (PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData> outData (PortIndex port) override;

    void setInData (std::shared_ptr<NodeData>, PortIndex const) override {}

    void setId (const std::string& id) { _id = id; }

    const std::string& id() { return _id; } // id

Q_SIGNALS:

    void updated (NodeDelegateModel*);

protected Q_SLOTS:

    void onChanged();

protected:
    std::string _id;
    QString _nodename;
    QWidget* _widget;
    QVBoxLayout* _layout;
    QHBoxLayout* _hlayout;
    QCheckBox* _batchChk;
    QLineEdit* _outputEdit;
    QLabel* _outputLabel;
};
