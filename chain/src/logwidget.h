// MIT License
// Copyright (c) 2025 Stephen J. Parker
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text.

#pragma once

#include "chainfonts.h"
#include "chainstyles.h"
#include "logger.h"
#include <QtWidgets/QTextEdit>
#include <QtCore/QFileSystemWatcher>
#ifdef _WIN32
#include <windows.h>
#endif


class LogWidget : public QTextEdit
{
    Q_OBJECT

public:
    explicit LogWidget (const std::string&, QWidget* parent = Q_NULLPTR);
    ~LogWidget() override;

public Q_SLOTS:
    void readLogFile (const QString&);

    static void logLevel (QAction*);

    void showContextMenu (const QPoint&);

    void resizeEvent (QResizeEvent*) override;

    void setLogFile (const std::string&);

private Q_SLOTS:
    void storeFileOffset();

private:
    QString logfile_;
    std::map<std::string, QAction*> actions_;
    QFileSystemWatcher* watcher_;
#ifdef _WIN32
    HANDLE fd{};
    std::unordered_map<QString, HANDLE> openfiles_;
    std::unordered_map<QString, LARGE_INTEGER> offsets_;
#else
    int fd{};
    std::unordered_map<QString, int> openfiles_;
    std::unordered_map<int, off_t > offsets_;
#endif
};
