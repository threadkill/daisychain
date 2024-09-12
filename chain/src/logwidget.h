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
#include "lognotifier.h"
#include <QtWidgets/QTextEdit>


class LogWidget : public QTextEdit
{
    Q_OBJECT

public:
    explicit LogWidget (std::string, QWidget* parent = Q_NULLPTR);
    ~LogWidget() override;

public Q_SLOTS:
    void readLogFile(const std::string&);

    static void logLevel (QAction*);

    void showContextMenu (const QPoint&);

    void resizeEvent (QResizeEvent*) override;

    void setLogFile (const std::string&);

private Q_SLOTS:
    void storeFileOffset();

private:
    int fd{};
    LogNotifier* notifier;
    std::string logfile;
    std::map<std::string, QAction*> actions_;
    std::unordered_map<std::string, int> openfiles_;
    std::unordered_map<int, off_t > offsets_;
};
