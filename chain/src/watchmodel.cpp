// # MIT License
// # Copyright (c) 2025 Stephen J. Parker
// # SPDX-License-Identifier: MIT
// # See LICENSE file for full license text.
//

#include "watchmodel.h"


WatchModel::WatchModel() :
    ChainModel(),
    _passthruChk{nullptr},
    _recursiveChk{nullptr}
{
}


QJsonObject
WatchModel::save() const
{
    QJsonObject _json = ChainModel::save();

    _json["passthru"]  = _passthruChk->isChecked();
    _json["recursive"] = _recursiveChk->isChecked();

    return _json;
}


void
WatchModel::load (QJsonObject const &_json)
{
    ChainModel::load (_json);

    QJsonValue val = _json["passthru"];
    if (!val.isUndefined()) {
        auto checked = val.toBool();
        _passthruChk->setChecked (checked);
    }

    val = _json["recursive"];
    if (!val.isUndefined()) {
        auto invert = val.toBool();
        _recursiveChk->setChecked (invert);
    }
}


QWidget*
WatchModel::embeddedWidget()
{
    if (!_widget) {
        _widget = ChainModel::embeddedWidget();
        _widget->setObjectName ("watchmodel");

        _batchChk->setEnabled (false);
        _batchChk->setVisible (false);
        _outputEdit->setEnabled (false);
        _outputEdit->setVisible (false);
        _outputLabel->setVisible (false);

        _passthruChk = new QCheckBox ("passthru");
        _passthruChk->setParent (_widget);
        _passthruChk->setChecked (false);
        _passthruChk->setObjectName ("_passthruChk");

        _recursiveChk = new QCheckBox ("recursive");
        _recursiveChk->setParent (_widget);
        _recursiveChk->setChecked (true);
        _recursiveChk->setObjectName ("_recursiveChk");

        auto hlayout = new QHBoxLayout();
        hlayout->addWidget (_recursiveChk);
        auto spacer = new QSpacerItem (10, 0, QSizePolicy::Minimum, QSizePolicy::Minimum);
        hlayout->addItem (spacer);
        hlayout->addWidget (_passthruChk);

        _layout->addLayout (hlayout);

        connect (_passthruChk, &QCheckBox::checkStateChanged, this, &WatchModel::onChanged);
        connect (_recursiveChk, &QCheckBox::checkStateChanged, this, &WatchModel::onChanged);
    }

    return _widget;
}
