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
