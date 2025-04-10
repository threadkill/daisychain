// MIT License
// Copyright (c) 2025 Stephen J. Parker
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text.

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

    void setId (const std::string& id) { id_ = id; }

    const std::string& id() { return id_; } // id

Q_SIGNALS:

    void updated (NodeDelegateModel*);

protected Q_SLOTS:

    void onChanged();

protected:
    std::string id_;
    QString _nodename;
    QWidget* _widget;
    QVBoxLayout* _layout;
    QHBoxLayout* _hlayout;
    QCheckBox* _batchChk;
    QLineEdit* _outputEdit;
    QLabel* _outputLabel;
};
