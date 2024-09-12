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

#include "loghighlighter.h"


LogHighlighter::LogHighlighter (QTextDocument* parent) : QSyntaxHighlighter (parent)
{
    HighlightingRule rule;

    rule.pattern = QRegularExpression ("^.*INFO\\].*$");
    infoFormat.setForeground (QColor (200, 200, 200));
    rule.format = infoFormat;
    highlightingRules.append (rule);

    rule.pattern = QRegularExpression ("^.*DEBUG\\].*$");
    debugFormat.setForeground (QColor (101, 137, 177));
    rule.format = debugFormat;
    highlightingRules.append (rule);

    rule.pattern = QRegularExpression ("^.*WARNING\\].*$");
    warnFormat.setForeground (QColor (168, 128, 87));
    rule.format = warnFormat;
    highlightingRules.append (rule);

    rule.pattern = QRegularExpression ("^.*ERROR\\].*$");
    errorFormat.setForeground (QColor (200, 80, 80));
    rule.format = errorFormat;
    highlightingRules.append (rule);

    rule.pattern = QRegularExpression ("^.*PERF\\].*$");
    perfFormat.setForeground (QColor (50, 120, 85));
    rule.format = perfFormat;
    highlightingRules.append (rule);

    rule.pattern = QRegularExpression ("^.*TEST\\].*$");
    testFormat.setForeground (QColor (133, 133, 205));
    rule.format = testFormat;
    highlightingRules.append (rule);

    rule.pattern = QRegularExpression ("\\{.*\\}$");
    sourceFormat.setForeground (QColor (120, 120, 120));
    sourceFormat.setFontItalic (true);
    rule.format = sourceFormat;
    highlightingRules.append (rule);

    rule.pattern = QRegularExpression ("\\<.*?\\>");
    //nodenameFormat.setForeground (QColor (154, 179, 203));
    nodenameFormat.setFontItalic (true);
    rule.format = nodenameFormat;
    //highlightingRules.append (rule);
}


void
LogHighlighter::highlightBlock (const QString& text)
{
    for (const auto& rule : std::as_const (highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch (text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat (static_cast<int>(match.capturedStart()), static_cast<int>(match.capturedLength()), rule.format);
        }
    }

    setCurrentBlockState (0);
}