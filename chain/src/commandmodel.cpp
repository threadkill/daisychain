// # MIT License
// # Copyright (c) 2025 Stephen J. Parker
// # SPDX-License-Identifier: MIT
// # See LICENSE file for full license text.
//

#include "commandmodel.h"
#include "chainfonts.h"


CommandModel::CommandModel() : ChainModel(), _textEdit{nullptr}
{
}


QJsonObject
CommandModel::save() const
{
    QJsonObject _json = ChainModel::save();

    _json["command"] = _textEdit->toPlainText();

    return _json;
}


void
CommandModel::load (QJsonObject const &_json)
{
    ChainModel::load (_json);

    QJsonValue val = _json["command"];
    if (!val.isUndefined()) {
        auto output = val.toString();
        _textEdit->setPlainText (output);
    }
}


QWidget*
CommandModel::embeddedWidget()
{
    if (!_widget) {
        _widget = ChainModel::embeddedWidget();
        _widget->setObjectName ("commandmodel");

#ifdef _WIN32
        std::string placeholdertext = "Global variables are accessible as environment variables."
                                      "\nBy default, OUTPUT=%INPUT%."
                                      "\nWhen OUTPUT is set below, it can be referenced as %OUTPUT% by the command."
                                      "\nShell variable expansion rules apply.";
#else
        std::string placeholdertext = "Global variables are accessible as environment variables."
                                      "\nBy default, OUTPUT=${INPUT}."
                                      "\nWhen OUTPUT is set below, it can be referenced as %OUTPUT% by the command."
                                      "\nShell variable expansion rules apply.";
#endif
        _textEdit = new QPlainTextEdit ("");
        _textEdit->setParent (_widget);
        _textEdit->setObjectName ("_textEdit");
        _textEdit->setPlaceholderText (placeholdertext.c_str());
        _textEdit->setFont (chain::chainfont());
        _textEdit->setMinimumWidth (350);
        _textEdit->setMinimumHeight (100);
        _textEdit->setSizePolicy (QSizePolicy (QSizePolicy::Minimum, QSizePolicy::Ignored));

        auto palette = _textEdit->palette();
        palette.setColor (QPalette::Base, QColor (0, 0, 0));
        palette.setColor (QPalette::Text, QColor (69, 165, 117));
        _textEdit->setPalette (palette);

        _layout->insertWidget (0, _textEdit);

        connect (_textEdit, &QPlainTextEdit::textChanged, this, &CommandModel::onChanged);

        _textEdit->resize (350, 100);
    }


    return _widget;
}
