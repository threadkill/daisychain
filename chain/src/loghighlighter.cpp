// MIT License
// Copyright (c) 2025 Stephen J. Parker
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text.

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