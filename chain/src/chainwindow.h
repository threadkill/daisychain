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

#include "graphmodel.h"
#include "chainbrowser.h"
#include "chainnotes.h"
#include "chainreaddata.h"
#include "chainscene.h"
#include "chainview.h"
#include "commandmodel.h"
#include "concatmodel.h"
#include "distromodel.h"
#include "environwidget.h"
#include "filtermodel.h"
#include "logger.h"
#include "logwidget.h"
#include "logfilter.h"
#include "utils.h"
#include <QtWidgets>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtNodes/NodeData>


using QtNodes::NodeDelegateModelRegistry;


class ChainWindow : public QMainWindow
{
    Q_OBJECT

public:

    ChainWindow();

private:

    void setupActions();

    void setupMenu();

    void setupHud();

    void setupUI();

    void setupInputsDialog();

    void updateSignals();

private Q_SLOTS:

    void newTab();

    void closeTab();

    void closeTab (int);

    void switchTab (int);

    void fitInView();

    void updateTitle();

    void clearGraph();

    void load();

    void load (QString&);

    void import();

    void save();

    void saveAs();

    void clearInputs();

    void selectInputs();

    void showInputs();

    void showInputsContextMenu (const QPoint&);

    void showSelectInputsContextMenu (const QPoint&);

    void updateInputs();

    void updateHud();

    void showGraph (bool);

    void dragEnterEvent (QDragEnterEvent*) override;

    void dropEvent (QDropEvent*) override;

    void closeEvent (QCloseEvent*) override;

    void execute();

    void test();

    void finished();

private:

    std::map<std::string, QAction*> actions_;
    std::shared_ptr<NodeDelegateModelRegistry> registry_;
    std::shared_ptr<GraphModel> model_;
    std::shared_ptr<ChainScene> scene_;
    std::shared_ptr<ChainView> view_;
    std::vector<std::shared_ptr<ChainScene>> scenes_;
    std::vector<std::shared_ptr<ChainView>> views_;
    std::vector<std::shared_ptr<GraphModel>> models_;
    QDialog* inputsdialog_{};
    QRect defaultsize_;
    QRect compactsize_;
    QLabel inputcountlabel_;
    QLabel nodecountlabel_;
    QLabel connectionslabel_;
    QSystemTrayIcon* trayicon_;
    QMenu* traymenu_{};
    QMenu* filesmenu_{};
    QTabWidget* tabs_{};
    LogWidget* logwidget_{};
    QDockWidget* loggerdock_{};
    bool compact_;
    int sidewidth_;
    int bottomheight_;
};
