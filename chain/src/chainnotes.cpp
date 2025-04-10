// MIT License
// Copyright (c) 2025 Stephen J. Parker
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text.

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


