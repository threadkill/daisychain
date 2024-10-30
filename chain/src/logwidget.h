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
