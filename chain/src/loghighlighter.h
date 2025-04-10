// MIT License
// Copyright (c) 2025 Stephen J. Parker
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text.

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