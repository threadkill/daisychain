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

#include "filtermodel.h"
#include "chainfonts.h"


FilterModel::FilterModel() :
    ChainModel(),
    _lineEdit{nullptr},
    _regexChk{nullptr},
    _invertChk{nullptr}
{
}


QJsonObject
FilterModel::save() const
{
    QJsonObject _json = ChainModel::save();

    _json["regex"]  = _regexChk->isChecked();
    _json["invert"] = _invertChk->isChecked();
    _json["filter"] = _lineEdit->text();

    return _json;
}


void
FilterModel::load (QJsonObject const &_json)
{
    ChainModel::load (_json);

    QJsonValue val = _json["regex"];
    if (!val.isUndefined()) {
        auto checked = val.toBool();
        _regexChk->setChecked (checked);
    }

    val = _json["invert"];
    if (!val.isUndefined()) {
        auto invert = val.toBool();
        _invertChk->setChecked (invert);
    }

    val = _json["filter"];
    if (!val.isUndefined()) {
        auto filter = val.toString();
        _lineEdit->setText (filter);
    }
}


QWidget*
FilterModel::embeddedWidget()
{
    if (!_widget) {
        _widget = ChainModel::embeddedWidget();
        _widget->setObjectName ("filtermodel");

        _batchChk->setEnabled (false);
        _batchChk->setVisible (false);
        _outputEdit->setEnabled (false);
        _outputEdit->setVisible (false);
        _outputLabel->setVisible (false);

        _lineEdit = new QLineEdit ("");
        _regexChk = new QCheckBox ("use regex");
        _invertChk = new QCheckBox ("invert");

        _lineEdit->setParent (_widget);
        _lineEdit->setObjectName ("_lineEdit");
        _lineEdit->setPlaceholderText ("Glob or Regex");
        _lineEdit->setFont (chain::chainfont());
        _lineEdit->setStyleSheet ("background-color: rgb(89, 131, 177); color: white;");

        _regexChk->setParent (_widget);
        _regexChk->setObjectName ("_regexChk");
        _regexChk->setChecked (false);

        _invertChk->setParent (_widget);
        _invertChk->setObjectName ("_invertChk");
        _invertChk->setChecked (false);

        auto hlayout = new QHBoxLayout();
        hlayout->addWidget (_regexChk);
        hlayout->addStretch (100);
        hlayout->addWidget (_invertChk);

        _layout->addWidget (_lineEdit);
        _layout->addLayout (hlayout);

        _widget->setMaximumHeight (50);

        connect (_lineEdit, &QLineEdit::textChanged, this, &FilterModel::onChanged);
        connect (_regexChk, &QCheckBox::checkStateChanged, this, &FilterModel::onChanged);
        connect (_invertChk, &QCheckBox::checkStateChanged, this, &FilterModel::onChanged);
    }

    return _widget;
}

