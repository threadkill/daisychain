// MIT License
// Copyright (c) 2025 Stephen J. Parker
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text.

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

    void keyReleaseEvent (QKeyEvent*) override;

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

    void onPaste();

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
    QStatusBar* statusbar_{};

    bool compact_;
    int sidewidth_;
    int bottomheight_;
};
