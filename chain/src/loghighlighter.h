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

#include <QtWidgets>


class LogHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit LogHighlighter (QTextDocument* parent=nullptr);

protected:
    void highlightBlock (const QString& text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat infoFormat;
    QTextCharFormat warnFormat;
    QTextCharFormat errorFormat;
    QTextCharFormat debugFormat;
    QTextCharFormat testFormat;
    QTextCharFormat sourceFormat;
    QTextCharFormat nodenameFormat;
    QTextCharFormat perfFormat;
};