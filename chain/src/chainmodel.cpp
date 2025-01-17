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

#include <QMenu>
#include <QtGui>
#include "chainmodel.h"
#include "chainstyles.h"


ChainModel::ChainModel() :
    _nodename (""),
    _widget{nullptr},
    _layout{nullptr},
    _hlayout{nullptr},
    _batchChk{nullptr},
    _outputEdit{nullptr},
    _outputLabel{nullptr}
{
}


QJsonObject
ChainModel::save() const
{
    QJsonObject _json = NodeDelegateModel::save();

    _json["batch"]  = _batchChk->isChecked();
    _json["output"] = _outputEdit->text();
    _json["size"] = QJsonObject{{"x",_widget->size().width()}, {"y",_widget->size().height()}};

    return _json;
}


void
ChainModel::load (QJsonObject const &_json)
{
    QJsonValue val = _json["batch"];
    if (!val.isUndefined()) {
        auto checked = val.toBool();
        _batchChk->setChecked (checked);
    }

    val = _json["output"];
    if (!val.isUndefined()) {
        auto output = val.toString();
        _outputEdit->setText (output);
    }

    val = _json["size"];
    if (!val.isUndefined()) {
        auto x = val["x"].toInt();
        auto y = val["y"].toInt();
        if (_widget->size() != QSize (x, y)) {
            _widget->resize (x, y);
            onChanged();
        }
    }
}


QWidget*
ChainModel::embeddedWidget()
{
    if (!_widget) {
        _widget = new QWidget (nullptr);
        _widget->setObjectName ("chainmodel");
        _layout = new QVBoxLayout (_widget);
        _batchChk = new QCheckBox ("batch", _widget);
        _outputEdit = new QLineEdit (_widget);
        _outputLabel = new QLabel ("Output", _widget);

        auto palette = darkPalette();
        palette.setColor (QPalette::Window, QColor (0, 0, 0, 0));
        _widget->setPalette (palette);

        _batchChk->setObjectName ("_batchChk");
        _outputEdit->setObjectName ("_outputEdit");
#ifdef _WIN32
        _outputEdit->setPlaceholderText ("%INPUT%");
#else
        _outputEdit->setPlaceholderText ("${INPUT}");
#endif

        _hlayout = new QHBoxLayout();

        _hlayout->addWidget (_outputLabel);
        _hlayout->addWidget (_outputEdit);
        _hlayout->insertSpacing (2, 8);
        _hlayout->addWidget (_batchChk);

        _layout->setContentsMargins (0, 0, 0, 0);
        _layout->insertLayout (0, _hlayout);

        connect (_batchChk, &QCheckBox::checkStateChanged, this, &ChainModel::onChanged);
        connect (_outputEdit, &QLineEdit::textChanged, this, &ChainModel::onChanged);
    }

    return _widget;
}


unsigned int
ChainModel::nPorts (PortType portType) const
{
    unsigned int result = 1;

    switch (portType) {
    case PortType::In:
        result = 1;
        break;

    case PortType::Out:
        result = 1;

    default:
        break;
    } // switch

    return result;
} // ChainModel::nPorts


void
ChainModel::onChanged()
{
    Q_EMIT updated (this);
} // ChainModel::onChanged


NodeDataType
ChainModel::dataType (PortType portType, PortIndex) const
{
    switch (portType) {
    case PortType::In:
        return ChainReadData().type();

    case PortType::Out:
        return ChainWriteData().type();

    case PortType::None:
        break;
    } // switch

    return {};
} // ChainModel::dataType


std::shared_ptr<NodeData>
ChainModel::outData (PortIndex)
{
    return std::make_shared<ChainWriteData>();
} // ChainModel::outData
