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

#include "chainnotes.h"
#include "chainfonts.h"
#include "chainstyles.h"


ChainNotes::ChainNotes (QWidget* parent) :
    QPlainTextEdit (parent)
{
    setupUI();
    setPlainText ("");
    setPlaceholderText ("Notes are saved with the graph.");
}


void
ChainNotes::setupUI()
{
    setObjectName ("noteswidget");
    setPalette (darkPalette());
    //setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOn);
    setStyleSheet (QString::fromStdString (chainScrollBarQss));
    auto font = chain::chainfont();
    font.setPixelSize (11);
    setFont (font);
}


json
ChainNotes::serialize()
{
    json notes;
    notes["text"] = QPlainTextEdit::toPlainText().toStdString();

    return notes;
} // ChainNotes::serialize


