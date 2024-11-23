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

#include "chainwindow.h"
#include "chainstyles.h"


static std::shared_ptr<NodeDelegateModelRegistry>
registerDataModels()
{
    auto ret = std::make_shared<NodeDelegateModelRegistry>();

    ret->registerModel<CommandModel>("");
    ret->registerModel<ConcatModel>("");
    ret->registerModel<DistroModel>("");
    ret->registerModel<FileListModel>("");
    ret->registerModel<FilterModel>("");
    ret->registerModel<WatchModel>("");

    return ret;
} // registerDataModels


ChainWindow::ChainWindow() :
    registry_ (registerDataModels()),
    compact_ (false),
    sidewidth_ (220),
    bottomheight_ (200),
    defaultsize_ (0, 0, 1280, 1024),
    compactsize_ (0, 0, 200, 400)
{
    setWindowTitle ("DaisyChain");
    setPalette (darkPalette());
    setAcceptDrops (true);

    trayicon_ = new QSystemTrayIcon (this);
    trayicon_->setIcon (QIcon (":daisy_alpha_24.png"));
    trayicon_->setVisible (true);

    // each tab introduces a new graph, view, and scene.
    auto model = std::make_shared<GraphModel>(registry_);
    auto scene = std::make_shared<ChainScene>(model.get(), this);
    auto view = std::make_shared<ChainView>();
    models_.push_back (model);
    scenes_.push_back (scene);
    views_.push_back (view);
    model_ = models_[0];
    scene_ = scenes_[0];
    view_ = views_[0];
    view_->setScene (scene_.get());

    el::Helpers::installLogDispatchCallback<LogFilter>("LogFilter");
    auto logfilter = el::Helpers::logDispatchCallback<LogFilter>("LogFilter");
    logfilter->set_logfile (model_->graphLog());
    logfilter->setEnabled (true);

    statusbar_ = statusBar();

    setupInputsDialog();
    setupUI();
    updateSignals();
    updateTitle();
    showGraph (compact_);
} // ChainWindow::ChainWindow


void
ChainWindow::setupActions()
{
    actions_["newtab"] = new QAction ("New", this);
    actions_["close"] = new QAction ("Close", this);
    actions_["open"] = new QAction ("Open ...", this);
    actions_["import"] = new QAction ("Import ...", this);
    actions_["save"] = new QAction ("Save", this);
    actions_["saveas"] = new QAction ("Save As ...", this);
    actions_["exit"] = new QAction ("Exit", this);
    actions_["select"] = new QAction ("Select ...", this);
    actions_["clearinputs"] = new QAction ("Clear", this);
    actions_["clearinputs"]->setIconVisibleInMenu (true);
    actions_["clearinputs"]->setIcon (QIcon::fromTheme ("delete"));
    actions_["files"] = new QAction ("Show", this);
    actions_["fit"] = new QAction ("Fit view", this);
    actions_["compact"] = new QAction ("Compact", this);
    actions_["print"] = new QAction ("Print", this);
    actions_["copy"] = new QAction ("Copy", this);
    actions_["paste"] = new QAction ("Paste", this);
    actions_["cleargraph"] = new QAction ("Clear", this);

    actions_["fit"]->setShortcut (Qt::Key_F);
    actions_["compact"]->setShortcut (Qt::Key_C);
    actions_["compact"]->setCheckable (true);
    actions_["compact"]->setChecked (false);

    connect (actions_["newtab"], &QAction::triggered, this, &ChainWindow::newTab);
    connect (actions_["close"], &QAction::triggered, this, qOverload<>(&ChainWindow::closeTab));
    connect (actions_["open"], &QAction::triggered, this, qOverload<>(&ChainWindow::load));
    connect (actions_["import"], &QAction::triggered, this, &ChainWindow::import);
    connect (actions_["save"], &QAction::triggered, this, &ChainWindow::save);
    connect (actions_["saveas"], &QAction::triggered, this, &ChainWindow::saveAs);
    connect (actions_["exit"], &QAction::triggered, this, &ChainWindow::close);
    connect (actions_["select"], &QAction::triggered, this, &ChainWindow::selectInputs);
    connect (actions_["clearinputs"], &QAction::triggered, this, &ChainWindow::clearInputs);
    connect (actions_["files"], &QAction::triggered, this, &ChainWindow::showInputs);
    connect (actions_["fit"], &QAction::triggered, this, &ChainWindow::fitInView);
    connect (actions_["compact"], &QAction::toggled, this, &ChainWindow::showGraph);

    connect (actions_["print"], &QAction::triggered, [&]() { model_->print(); });

    connect (actions_["copy"], &QAction::triggered, [&]() { view_->onCopySelectedObjects(); });
    connect (actions_["paste"], &QAction::triggered, [&]() {
        view_->onPasteObjects();
        scene_->updatePositions();
    });
    connect (actions_["cleargraph"], &QAction::triggered, this, &ChainWindow::clearGraph);
} // ChainWindow::setupActions


void
ChainWindow::setupMenu()
{
    // menubar
    auto menubar = menuBar();
    auto font = menubar->font();
    font.setPixelSize (12);
    menubar->setFont (font);

    // graphmenu
    auto graphmenu = menubar->addMenu ("File");
    graphmenu->setPalette (darkPalette());
    font = graphmenu->font();
    font.setPixelSize (11);
    graphmenu->setFont (font);
    graphmenu->addAction (actions_["newtab"]);
    graphmenu->addAction (actions_["close"]);
    graphmenu->addSeparator();
    graphmenu->addAction (actions_["open"]);
    graphmenu->addAction (actions_["import"]);
    graphmenu->addAction (actions_["save"]);
    graphmenu->addAction (actions_["saveas"]);
    graphmenu->addSeparator();
    graphmenu->addAction (actions_["print"]);
    graphmenu->addSeparator();
    graphmenu->addAction (actions_["exit"]);

    // editmenu
    auto editmenu = menubar->addMenu ("Edit");
    editmenu->setPalette (darkPalette());
    editmenu->setFont (font);
    editmenu->addAction (actions_["copy"]);
    editmenu->addAction (actions_["paste"]);
    editmenu->addSeparator();
    editmenu->addAction (actions_["cleargraph"]);

    // nodemenu
    auto nodemenu = scene_->createSceneMenu (QPointF());
    nodemenu->setTitle ("Nodes");
    nodemenu->setAttribute (Qt::WA_DeleteOnClose, false);
    menubar->addMenu (nodemenu);

    // filesmenu_
    filesmenu_ = menubar->addMenu ("Inputs");
    filesmenu_->setPalette (darkPalette());
    filesmenu_->setFont (font);
    filesmenu_->addAction (actions_["select"]);
    filesmenu_->addAction (actions_["files"]);
    filesmenu_->addSeparator();
    filesmenu_->addAction (actions_["clearinputs"]);

    // viewmenu
    auto viewmenu = menubar->addMenu ("View");
    viewmenu->setPalette (darkPalette());
    viewmenu->setFont (font);
    viewmenu->addAction (actions_["compact"]);
    auto loggeraction = findChild<QDockWidget*> ("loggerdock")->toggleViewAction();
    loggeraction->setShortcut (Qt::Key_L);
    viewmenu->addAction (loggeraction);
    auto browseraction = findChild<QDockWidget*> ("browserdock")->toggleViewAction();
    browseraction->setShortcut (Qt::Key_B);
    browseraction->setText ("Browser");
    viewmenu->addAction (browseraction);
    auto notesaction = findChild<QDockWidget*> ("notesdock")->toggleViewAction();
    notesaction->setShortcut (Qt::Key_N);
    viewmenu->addAction (notesaction);
    viewmenu->addAction (actions_["fit"]);
} // ChainWindow::setupMenu


void
ChainWindow::setupInputsDialog()
{
    inputsdialog_ = new QDialog (this);
    inputsdialog_->setWindowIcon (QIcon (":folder.svg"));
    inputsdialog_->setWindowTitle (QString::fromStdString ("Selected Files"));
    inputsdialog_->resize (1024, 512);
    inputsdialog_->setPalette (darkPalette());
    inputsdialog_->setSizeGripEnabled (true);

    auto textedit = new QTextEdit (inputsdialog_);
    textedit->setPalette (darkPalette());

    textedit->setObjectName ("dialogtextedit");
    textedit->setReadOnly (false);
    textedit->setUndoRedoEnabled (true);
    textedit->setStyleSheet (QString::fromStdString (chainScrollBarQss));

    textedit->setContextMenuPolicy (Qt::CustomContextMenu);
    connect (textedit, &QTextEdit::customContextMenuRequested, this, &ChainWindow::showInputsContextMenu);
    connect (textedit, &QTextEdit::textChanged, [&]() {
        const auto textedit_ = inputsdialog_->findChild<QTextEdit*> ("dialogtextedit");
        auto paths = textedit_->toPlainText().toStdString();
        model_->blockSignals (true);
        model_->updateInput (paths);
        model_->blockSignals (false);

        const auto btn = findChild<QWidget*> ("filesbtn");
        if (!paths.empty()) {
            btn->setGraphicsEffect (nullptr);
        }
        else {
            auto gfx = new QGraphicsColorizeEffect();
            gfx->setColor (QColor (0, 0, 0));
            gfx->setStrength (1.0);
            btn->setGraphicsEffect (gfx);
        }
        updateHud();
    });


    auto vlayout = new QVBoxLayout (inputsdialog_);
    vlayout->setContentsMargins (5, 5, 5, 5);
    vlayout->setSpacing (0);

    vlayout->addWidget (textedit);
} // ChainWindow::setupInputsDialog


void
ChainWindow::setupUI()
{
    setGeometry (compactsize_);

    auto statuslabel_ = new QLabel (
        "<a href=\"https://github.com/threadkill/daisychain\">v" DAISYCHAIN_VERSION "</a>");
    statuslabel_->setPalette (darkPalette());
    statuslabel_->setTextInteractionFlags (Qt::TextBrowserInteraction);
    statuslabel_->setOpenExternalLinks (true);
    statuslabel_->setFocusPolicy (Qt::NoFocus);

    statusbar_->addPermanentWidget (statuslabel_);
    statusbar_->setPalette (darkPalette());
    statusbar_->setContentsMargins (0, 0, 6, 0);

    logwidget_ = new LogWidget (model_->graphLog());
    logwidget_->setObjectName ("logger");

    loggerdock_ = new QDockWidget ("Log", this);
    loggerdock_->setContentsMargins (5, 0, 0, 5);
    loggerdock_->setObjectName ("loggerdock");
    loggerdock_->setAllowedAreas (Qt::AllDockWidgetAreas);
    loggerdock_->setWidget (logwidget_);
    auto font = loggerdock_->font();
    font.setPixelSize (12);
    loggerdock_->setFont (font);
    addDockWidget (Qt::BottomDockWidgetArea, loggerdock_);

    auto browserwidget = new QWidget();

    auto browser = new ChainBrowser();
    browser->setParent (browserwidget);
    browser->setObjectName ("browser");
    browser->setMinimumWidth (sidewidth_);

    auto browserlayout = new QVBoxLayout();
    browserlayout->setContentsMargins (5, 15, 0, 5);
    browserlayout->addWidget (browser);

    browserwidget->setLayout (browserlayout);

    auto browserdock = new QDockWidget ("", this);
    browserdock->setObjectName ("browserdock");
    browserdock->setAllowedAreas (Qt::AllDockWidgetAreas);
    browserdock->setWidget (browserwidget);
    addDockWidget (Qt::LeftDockWidgetArea, browserdock);

    auto noteswidget = new ChainNotes();

    auto notesdock = new QDockWidget ("Notes", this);
    notesdock->setObjectName ("notesdock");
    notesdock->setAllowedAreas (Qt::AllDockWidgetAreas);
    notesdock->setWidget (noteswidget);
    notesdock->setContentsMargins (0, 0, 5, 5);
    font = notesdock->font();
    font.setPixelSize (12);
    notesdock->setFont (font);
    addDockWidget (Qt::BottomDockWidgetArea, notesdock);

    auto central = new QWidget (this);
    setCentralWidget (central);

    auto winvlayout = new QVBoxLayout (central);
    winvlayout->setContentsMargins (0, 0, 0, 5);

    auto envlabel = new QLabel ("Global Variables");
    envlabel->setPalette (darkPalette());
    envlabel->setAlignment (Qt::AlignCenter);
    envlabel->setStyleSheet ("font-weight: bold;");
    font = envlabel->font();
    font.setPixelSize (12);
    envlabel->setFont (font);

    auto envpix = QPixmap (":daisy_alpha_24.png");

    auto envpixlabel = new QLabel();
    envpixlabel->setPixmap (envpix);

    auto envheaderlayout = new QHBoxLayout();
    envheaderlayout->setContentsMargins (0, 0, 0, 0);
    envheaderlayout->setSpacing (5);
    envheaderlayout->setAlignment (Qt::AlignCenter);
    envheaderlayout->addWidget (envpixlabel);
    envheaderlayout->addWidget (envlabel);

    auto envwidget = new EnvironWidget();
    envwidget->setObjectName ("envwidget");
    envwidget->setMinimumWidth (sidewidth_);

    auto execbtn = new QPushButton ("Execute");
    execbtn->setToolTip ("Execute graph in current tab.");
    execbtn->setPalette (darkPalette());
    execbtn->setObjectName ("execbtn");
    execbtn->setFocusPolicy (Qt::NoFocus);
    execbtn->setFixedHeight (32);
    execbtn->setStyleSheet ("font-weight: bold;");
    font = execbtn->font();
    font.setPixelSize (12);
    execbtn->setFont (font);
    auto execgfx = new QGraphicsColorizeEffect();
    execgfx->setColor (QColor (50, 120, 85));
    execgfx->setStrength (1.0);
    execbtn->setGraphicsEffect (execgfx);

    auto termbtn = new QPushButton ("Terminate");
    termbtn->setToolTip ("Terminate the current running graph.");
    termbtn->setPalette (darkPalette());
    termbtn->setObjectName ("termbtn");
    termbtn->setFocusPolicy (Qt::NoFocus);
    termbtn->setFixedHeight (32);
    termbtn->setStyleSheet ("font-weight: bold;");
    font = termbtn->font();
    font.setPixelSize (12);
    termbtn->setFont (font);
    termbtn->hide();
    auto termgfx = new QGraphicsColorizeEffect();
    termgfx->setColor (QColor (200, 80, 80));
    termgfx->setStrength (2.0);
    termbtn->setGraphicsEffect (termgfx);

    auto testbtn = new QPushButton ("Test");
    testbtn->setToolTip ("Test input pass-through.");
    testbtn->setPalette (darkPalette());
    testbtn->setObjectName ("testbtn");
    testbtn->setFocusPolicy (Qt::NoFocus);
    testbtn->setFixedHeight (32);
    testbtn->setFixedWidth (34);
    font = testbtn->font();
    font.setPixelSize (10);
    testbtn->setFont (font);

    auto filesbtn = new QPushButton();
    filesbtn->setToolTip ("Select file inputs. Right-click for more options.");
    filesbtn->setPalette (darkPalette());
    filesbtn->setObjectName ("filesbtn");
    filesbtn->setIcon (QIcon (":/folder.svg"));
    filesbtn->setFixedHeight (32);
    filesbtn->setFixedWidth (34);
    filesbtn->setVisible (true);
    filesbtn->setContextMenuPolicy (Qt::CustomContextMenu);
    auto gfx = new QGraphicsColorizeEffect();
    gfx->setColor (QColor (0, 0, 0));
    gfx->setStrength (1.0);
    filesbtn->setGraphicsEffect (gfx);

    auto execlayout = new QHBoxLayout();
    execlayout->setContentsMargins (0, 0, 0, 0);
    execlayout->addWidget (filesbtn);
    execlayout->addWidget (execbtn);
    execlayout->addWidget (termbtn);
    execlayout->addWidget (testbtn);
    execlayout->setStretchFactor (execbtn, 50);
    execlayout->insertSpacing (1, 1);
    execlayout->insertSpacing (3, 1);

    auto rightpanel = new QWidget();

    auto envlayout = new QVBoxLayout (rightpanel);
    envlayout->setContentsMargins (0, 0, 5, 0);
    envlayout->setSpacing (0);
    envlayout->addLayout (envheaderlayout);
    envlayout->addWidget (envwidget);
    envlayout->addLayout (execlayout);
    envlayout->insertSpacing (0, 5);
    envlayout->insertSpacing (2, 5);
    envlayout->insertSpacing (4, 5);

    auto tabsparent = new QWidget();
    tabsparent->setObjectName ("tabsparent");

    tabs_ = new QTabWidget (tabsparent);
    tabs_->setObjectName ("tabswidget");
    tabs_->setStyleSheet ("QTabWidget::pane {"
                         "border: 0px solid black;"
                         "}");
    tabs_->tabBar()->setStyleSheet(
        "QTabBar::close-button {"
        "image: url(:close.png);"
        "subcontrol-position: right;"
        "}"
        "QTabBar::close-button:hover {"
        "image: url(:close_hover.png);"
        "subcontrol-position: right;"
        "}");
    tabs_->setPalette (darkPalette());
    tabs_->setTabsClosable (true);
    tabs_->addTab (view_.get(), "");

    auto tabslayout = new QVBoxLayout (tabsparent);
    tabslayout->setContentsMargins (0, 2, 0, 0);
    tabslayout->addWidget (tabs_);

    auto hsplitter = new QSplitter (central);
    hsplitter->setObjectName ("envsplitter");
    hsplitter->addWidget (tabsparent);
    hsplitter->addWidget (rightpanel);
    hsplitter->setHandleWidth (6);
    hsplitter->setSizes ({defaultsize_.width(), compactsize_.width()});

    winvlayout->addWidget (hsplitter);

    setupActions();
    setupMenu();
    setupHud();

    connect (execbtn, &QPushButton::clicked, this, &ChainWindow::execute);
    connect (termbtn, &QPushButton::clicked, [&]() { model_->terminate(); });
    connect (testbtn, &QPushButton::clicked, this, &ChainWindow::test);
    connect (filesbtn, &QPushButton::customContextMenuRequested, this, &ChainWindow::showSelectInputsContextMenu);
    connect (filesbtn, &QPushButton::clicked, this, &ChainWindow::selectInputs);
    connect (tabs_, &QTabWidget::currentChanged, this, &ChainWindow::switchTab);
    connect (tabs_, &QTabWidget::tabCloseRequested, this, qOverload<int>(&ChainWindow::closeTab));
    connect (browser, &ChainBrowser::sendFileSignal, [&](QString& filename) {
        newTab();
        load (filename);
    });
    connect (noteswidget, &QTextEdit::textChanged, [&]() {
        auto widget = findChild<ChainNotes*> ("noteswidget");
        auto notes = widget->serialize();
        model_->updateNotes (notes);
    });
    connect (envwidget, &EnvironWidget::environWidgetChanged, [&](json& data_) {
        model_->updateEnviron (data_);
    });
    connect (logwidget_, &QTextEdit::textChanged, [&]() {
        QTextCursor cursor = logwidget_->textCursor();
        cursor.movePosition (QTextCursor::End);
        cursor.movePosition (QTextCursor::PreviousBlock);
        QString lastline = cursor.block().text().trimmed();
        statusbar_->showMessage (lastline);
    });

    switchTab (0);
} // ChainWindow::setupUI


void
ChainWindow::setupHud()
{
    auto inputcount = new QLabel ("Inputs:");
    auto font = inputcount->font();
    font.setPixelSize (10);
    inputcount->setFont (font);
    inputcount->setAlignment (Qt::AlignRight|Qt::AlignVCenter);
    inputcountlabel_.setFont (font);
    inputcountlabel_.setAlignment (Qt::AlignLeft|Qt::AlignVCenter);

    auto nodecount = new QLabel ("Nodes:");
    nodecount->setFont (font);
    nodecount->setAlignment (Qt::AlignRight|Qt::AlignVCenter);
    nodecountlabel_.setFont (font);
    nodecountlabel_.setAlignment (Qt::AlignLeft|Qt::AlignVCenter);

    auto connections = new QLabel ("Connections:");
    connections->setFont (font);
    connections->setAlignment (Qt::AlignRight|Qt::AlignVCenter);
    connectionslabel_.setFont (font);
    connectionslabel_.setAlignment (Qt::AlignLeft|Qt::AlignVCenter);

    auto hudlayout = new QGridLayout();
    hudlayout->setOriginCorner (Qt::TopLeftCorner);
    hudlayout->setVerticalSpacing (0);
    hudlayout->setColumnMinimumWidth (0, 50);
    hudlayout->setColumnMinimumWidth (1, 50);
    hudlayout->addWidget (inputcount, 0, 0);
    hudlayout->addWidget (&inputcountlabel_, 0, 1);
    hudlayout->addWidget (nodecount, 1, 0);
    hudlayout->addWidget (&nodecountlabel_, 1, 1);
    hudlayout->addWidget (connections, 2, 0);
    hudlayout->addWidget (&connectionslabel_, 2, 1);

    auto tabswidget = findChild<QTabWidget*> ("tabswidget");
    auto hud = new QWidget (tabswidget);
    hud->setLayout (hudlayout);
    hud->setPalette (darkPalette());
    hud->move (0, 25);
} // ChainWindow::setupHud


void
ChainWindow::updateSignals()
{
    auto envwidget = findChild<EnvironWidget*>("envwidget");

    connect (model_.get(), &GraphModel::inputUpdated, this, &ChainWindow::updateInputs);
    connect (model_.get(), &GraphModel::environUpdated, envwidget, &EnvironWidget::populateUI);
    connect (model_.get(), &GraphModel::finished, this, &ChainWindow::finished);

    connect (model_.get(), &GraphModel::notesUpdated, [&](const json& data_) {
        auto widget = findChild<ChainNotes*> ("noteswidget");
        widget->clear();
        if (data_.contains ("text")) {
            widget->setPlainText (QString::fromStdString (data_["text"]));
        }
    });

    connect (scene_.get(), &ChainScene::changed, this, &ChainWindow::updateHud);
}


void
ChainWindow::newTab()
{
    auto model = std::make_shared<GraphModel> (registry_);
    auto scene = std::make_shared<ChainScene> (model.get(), this);
    auto view = std::make_shared<ChainView>();
    view->setScene (scene.get());
    tabs_->addTab (view.get(), "");

    models_.push_back (model);
    scenes_.push_back (scene);
    views_.push_back (view);

    auto index = scenes_.size() - 1;

    blockSignals (true);
    model_ = models_[index];
    scene_ = scenes_[index];
    view_ = views_[index];
    blockSignals (false);

    updateSignals();

    el::Helpers::installLogDispatchCallback<LogFilter>(model_->graphLog());
    auto logfilter = el::Helpers::logDispatchCallback<LogFilter>(model_->graphLog());
    logfilter->set_logfile (model_->graphLog());
    logfilter->setEnabled (true);

    tabs_->setCurrentIndex (int(index));
}


void
ChainWindow::closeTab()
{
    if (model_->running()) {
        auto button = QMessageBox::question (this, tr ("Daisy"), tr ("Graph is still running and will be terminated. Close anyway?"));
        if (button == QMessageBox::No) {
            return;
        }
    }

    blockSignals (true);
    model_->terminate();
    clearGraph();

    const auto index = tabs_->currentIndex();
    const auto newindex = index > 0 ? index - 1 : 0;
    tabs_->setCurrentIndex (newindex);

    view_  = nullptr;
    scene_ = nullptr;
    model_ = nullptr;

    if (tabs_->count() > 1) {
        tabs_->removeTab (index);

        auto v_it = views_.begin();
        std::advance (v_it, index);
        views_.erase (v_it);

        auto s_it = scenes_.begin();
        std::advance (s_it, index);
        scenes_.erase (s_it);

        auto m_it = models_.begin();
        std::advance (m_it, index);
        models_.erase (m_it);

    }

    switchTab (newindex);
    blockSignals (false);
} // ChainWindow::closeTab


void
ChainWindow::closeTab (int index)
{
    blockSignals (true);
    auto cur_index= tabs_->currentIndex();

    if (tabs_->count() > 1) {
        if (cur_index == index) {
            closeTab();
        }
        else {
            cur_index = index < cur_index ? cur_index - 1 : cur_index;
            tabs_->setCurrentIndex (index);
            switchTab (index);
            closeTab();
            tabs_->setCurrentIndex (cur_index);
            switchTab (cur_index);
        }
    }
    blockSignals (false);
} // ChainWindow::closeTab


void
ChainWindow::switchTab (int index)
{
    if (index < 0) return;

    blockSignals (true);
    model_ = models_[index];
    scene_ = scenes_[index];
    view_ = views_[index];
    blockSignals (false);

    el::Helpers::setThreadName (model_->graphLog());
    logwidget_->setLogFile (model_->graphLog());
    updateTitle();

    auto termbtn = findChild<QPushButton*>("termbtn");
    auto execbtn = findChild<QPushButton*>("execbtn");
    auto testbtn = findChild<QPushButton*>("testbtn");
    auto filesbtn = findChild<QPushButton*>("filesbtn");
    if (model_ && model_->running()) {
        filesbtn->setEnabled (false);
        execbtn->setEnabled (false);
        execbtn->hide();
        termbtn->setEnabled (true);
        termbtn->show();
        testbtn->setEnabled (false);
        view_->setEnabled (false);
        auto gfx = new QGraphicsColorizeEffect();
        gfx->setColor (QColor (0, 0, 0));
        gfx->setStrength (1.0);
        view_->setGraphicsEffect(gfx);

    }
    else {
        filesbtn->setEnabled (true);
        execbtn->setEnabled (true);
        execbtn->show();
        termbtn->setEnabled (false);
        termbtn->hide();
        testbtn->setEnabled (true);
        view_->setEnabled (true);
        view_->setGraphicsEffect (nullptr);
    }

    model_->emitAll();
} // ChainWindow::switchTab


void
ChainWindow::fitInView()
{
    QRectF bounds;

    if (scene_->selectedNodes().empty())
        bounds = scene_->itemsBoundingRect();
    else {
        auto group = scene_->createItemGroup (scene_->selectedItems());
        bounds = group->boundingRect();
        scene_->destroyItemGroup (group);
    }
    view_->setSceneRect (bounds);
    view_->fitInView (bounds, Qt::KeepAspectRatio);
} // ChainWindow::fitInView


void
ChainWindow::updateTitle()
{
    QString title = "DaisyChain";
    QString filename = model_->graph()->filename().c_str();
    QString tabname;

    if (!filename.isEmpty()) {
        QString basename = filename.split ('/').last();
        tabname = basename.split ('.').first();
        if (compact_) {
            title = basename;
        }
        else {
            title += " - " + filename;
        }
    }
    else {
        tabname = "Untitled";
    }

    setWindowTitle (title);

    auto index = tabs_->currentIndex();
    tabs_->setTabText (index, tabname);
} // ChainWindow::updateTitle


void
ChainWindow::showGraph (bool compact)
{
    auto splitter = findChild<QSplitter*> ("envsplitter");
    auto testbtn = findChild<QPushButton*> ("testbtn");
    auto browserdock = findChild<QDockWidget*> ("browserdock");
    auto loggerdock = findChild<QDockWidget*> ("loggerdock");
    auto notesdock = findChild<QDockWidget*> ("notesdock");

    compact_ = compact;

    if (compact) {
        browserdock->close();
        splitter->setSizes ({0, compactsize_.width()});
        testbtn->setVisible (false);
        loggerdock->hide();
        notesdock->hide();

        // doing this twice to work around a bug.
        resize (compactsize_.width(), compactsize_.height());
        resize (compactsize_.width(), compactsize_.height());
    }
    else {
        browserdock->show();
        loggerdock->show();
        notesdock->show();
        resize (defaultsize_.width(), defaultsize_.height());
        splitter->setSizes ({defaultsize_.width(), compactsize_.width()});
        testbtn->setVisible (true);
        resizeDocks ({browserdock}, {sidewidth_}, Qt::Horizontal);
        resizeDocks ({loggerdock}, {bottomheight_}, Qt::Vertical);
        resizeDocks ({loggerdock, notesdock}, {1000, 300}, Qt::Horizontal);
        fitInView();
    }

    updateTitle();

    if (isVisible()) {
        // Center the window.
        auto screen = QGuiApplication::primaryScreen();
        auto center = screen->geometry().center();

        auto geo = frameGeometry();
        geo.moveCenter (center);
        move (geo.topLeft());
    }
} // ChainWindow::showGraph


void
ChainWindow::clearGraph()
{
    scene_->clearScene();
    model_->graph()->set_filename ("");
    model_->graph()->Initialize();
    model_->emitAll();
    clearInputs();
    updateTitle();
} // ChainWindow::clearGraph


void
ChainWindow::load()
{
    std::string graph_dir;

    const char* tmp = getenv ("DAISY_GRAPH_DIR");

    if (tmp == nullptr) {
        graph_dir = QDir::homePath().toStdString();
    }
    else {
        graph_dir = tmp;
    }

    auto filename = QFileDialog::getOpenFileName (nullptr,
                                                  tr ("Open Daisy Graph"),
                                                  QString::fromStdString (graph_dir),
                                                  tr ("Daisy Graph Files (*.dcg)"));

    load (filename);
} // ChainWindow::load


void
ChainWindow::load (QString& filename)
{
    if (!filename.isEmpty()) {
        clearGraph();
        model_->loadGraph (filename);
        fitInView();
    }

    updateTitle();
} // ChainWindow::load


void
ChainWindow::import()
{
}


void
ChainWindow::save()
{
    auto filename = QString::fromStdString (model_->graph()->filename());

    if (!filename.isEmpty()) {
        model_->saveGraph (filename);
    }
    else {
        saveAs();
    }
} // ChainWindow::save


void
ChainWindow::saveAs()
{
    auto filename = QFileDialog::getSaveFileName (
        nullptr, tr ("Save Daisy Graph"), QDir::homePath(), tr ("Daisy Graph Files (*.dcg)"));

    model_->saveGraph (filename);
    updateTitle();
} // ChainWindow::saveAs


void
ChainWindow::clearInputs()
{
    model_->updateInput ("");
    updateInputs();
} // ChainWindow::clearInputs


void
ChainWindow::selectInputs()
{
    auto home_dir = QDir::homePath().toStdString();
    auto filenames = QFileDialog::getOpenFileNames (
        nullptr, tr ("Select Input Files"), QString::fromStdString (home_dir));

    std::string paths;

    for (const auto& filename : filenames) {
        paths += filename.toStdString() + '\n';
    }

    if (!paths.empty()) {
        model_->updateInput (paths);
    }
} // ChainWindow::selectInputs


void
ChainWindow::updateInputs()
{
    const auto input = model_->input();

    const auto textedit = inputsdialog_->findChild<QTextEdit*> ("dialogtextedit");
    textedit->blockSignals (true);
    textedit->clear();
    textedit->setText (QString::fromStdString (input));
    textedit->blockSignals (false);

    const auto btn = findChild<QWidget*> ("filesbtn");
    if (!input.empty()) {
        btn->setGraphicsEffect (nullptr);
    }
    else {
        auto gfx = new QGraphicsColorizeEffect();
        gfx->setColor (QColor (0, 0, 0));
        gfx->setStrength (1.0);
        btn->setGraphicsEffect (gfx);
    }

    updateHud();
} // ChainWindow::updateInputs


void
ChainWindow::showInputs()
{
    if (inputsdialog_->isVisible()) {
        inputsdialog_->close();
    }
    else {
        inputsdialog_->show();
    }
} // ChainWindow::showInputs


void
ChainWindow::showInputsContextMenu (const QPoint& pt)
{
    auto textedit = inputsdialog_->findChild<QTextEdit*> ("dialogtextedit");
    auto clearaction = new QAction ("Clear Inputs", textedit);
    clearaction->setIconVisibleInMenu (true);
    clearaction->setIcon (QIcon::fromTheme ("delete"));
    connect (clearaction, &QAction::triggered, this, &ChainWindow::clearInputs);

    auto menu = textedit->createStandardContextMenu (textedit->mapToGlobal (pt));
    menu->setPalette (darkPalette());
    auto font = menu->font();
    font.setPixelSize (11);
    menu->setFont (font);
    menu->addSeparator();
    menu->addAction (clearaction);

    menu->exec (textedit->mapToGlobal (pt));
} // ChainWindow::showInputsContextMenu


void
ChainWindow::showSelectInputsContextMenu (const QPoint& pt)
{
    auto filesbtn = findChild<QPushButton*> ("filesbtn");
    filesmenu_->exec (filesbtn->mapToGlobal (pt));
} // ChainWindow::showInputsContextMenu


void
ChainWindow::updateHud()
{
    QString text;
    text.setNum (model_->num_inputs());
    inputcountlabel_.setText (text);
    text.setNum (model_->graph()->nodes().size());
    nodecountlabel_.setText (text);
    text.setNum (model_->graph()->edges().size());
    connectionslabel_.setText (text);
} // ChainWindow::updateHud


void
ChainWindow::dragEnterEvent (QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat ("text/uri-list")) {
        event->acceptProposedAction();
    }
} // ChainWindow::dragEnterEvent


void
ChainWindow::dropEvent (QDropEvent* event)
{
    view_->droppedFromWindow (event);
} // ChainWindow::dropEvent


void
ChainWindow::closeEvent (QCloseEvent* event)
{
    bool accept = true;
    for (const auto& model : models_) {
        if (model->running()) {
            auto button = QMessageBox::question (this, tr ("Daisy"), tr ("At least one graph is still running and will be terminated. Close anyway?"));
            if (button == QMessageBox::No) {
                accept = false;
                break;
            }
        }
    }

    if (accept) {
        disconnect (tabs_, &QTabWidget::currentChanged, this, &ChainWindow::switchTab);

        for (const auto& model : models_) {
            model->blockSignals (true);
            model->terminate();
        }
        event->accept();
    }
    else {
        event->ignore();
    }

    trayicon_->setVisible (false);
    trayicon_->deleteLater();
} // ChainWindow::closeEvent


void
ChainWindow::execute()
{
    model_->execute();
    switchTab (tabs_->currentIndex());
} // ChainWindow::execute


void
ChainWindow::test()
{
    model_->test();
    switchTab (tabs_->currentIndex());
}


void
ChainWindow::finished()
{
    QMessageBox::information (this, tr ("Daisy"), tr ("Finished."), QMessageBox::Ok);
    switchTab (tabs_->currentIndex());
} // ChainWindow::finished
