// MIT License
// Copyright (c) 2025 Stephen J. Parker
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text.

#include <fcntl.h>
#ifndef _WIN32
#include <unistd.h>
#include <cerrno>
#endif

#include <QGuiApplication>
#include <QMenu>
#include <QResizeEvent>
#include <utility>

#include "logwidget.h"
#include "loghighlighter.h"
#include "chainstyles.h"


LogWidget::LogWidget (const std::string& filename, QWidget* parent) :
    QTextEdit (parent), logfile_ (QString::fromStdString (filename))
{
    auto highlighter = new LogHighlighter();
    highlighter->setDocument (this->document());

    setPalette (darkPalette());

    setSizeAdjustPolicy (QAbstractScrollArea::AdjustToContents);
    setReadOnly (true);
    setUndoRedoEnabled (false);
    setMinimumWidth (0);
    //setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOn);
    setStyleSheet (QString::fromStdString (chainScrollBarQss));
    auto font = chain::chainfont();
    font.setPixelSize (11);
    setFont (font);
    setDocumentTitle (logfile_);

    watcher_ = new QFileSystemWatcher (this);

    setContextMenuPolicy (Qt::CustomContextMenu);

    actions_["clear"] = new QAction ("Clear Log", this);
    actions_["clear"]->setIconVisibleInMenu (true);
    actions_["clear"]->setIcon (QIcon::fromTheme ("delete"));

    actions_["info"] = new QAction ("INFO", this);
    actions_["info"]->setCheckable (true);

    actions_["warn"] = new QAction ("WARN", this);
    actions_["warn"]->setCheckable (true);

    actions_["error"] = new QAction ("ERROR", this);
    actions_["error"]->setCheckable (true);
    actions_["error"]->setChecked (true);
    logLevel (actions_["error"]);

    actions_["debug"] = new QAction ("DEBUG", this);
    actions_["debug"]->setCheckable (true);

    auto actiongroup = new QActionGroup (this);
    actiongroup->setExclusive (true);
    actiongroup->addAction (actions_["info"]);
    actiongroup->addAction (actions_["warn"]);
    actiongroup->addAction (actions_["error"]);
    actiongroup->addAction (actions_["debug"]);

    connect (actions_["clear"], &QAction::triggered, this, &LogWidget::clear);
    connect (actions_["clear"], &QAction::triggered, this, &LogWidget::storeFileOffset);
    connect (actiongroup, &QActionGroup::triggered, this, &LogWidget::logLevel);
    connect (this, &LogWidget::customContextMenuRequested, this, &LogWidget::showContextMenu);
    connect (watcher_, &QFileSystemWatcher::fileChanged, this, &LogWidget::readLogFile);
}


LogWidget::~LogWidget()
{
    for (const auto& [filename, handle] : openfiles_) {
#ifdef _WIN32
        CloseHandle (handle);
#else
        ::close (handle);
#endif
    }
}


#ifdef _WIN32
void
LogWidget::setLogFile (const std::string& filename)
{
    logfile_ = QString::fromStdString (filename);

    if (!openfiles_.contains (logfile_)) {
        fd = CreateFileA (
            filename.c_str(),
            GENERIC_READ,
            FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

        if (fd == INVALID_HANDLE_VALUE) {
            LERROR << "Could not open log file for reading: " + filename + " " << GetLastError();
            return;
        }
        openfiles_[logfile_] = fd;
    } else {
        fd = openfiles_[logfile_];
    }

    // If there is an offset for this file, seek to it
    if (offsets_.contains (logfile_)) {
        SetFilePointerEx (fd, offsets_[logfile_], nullptr, FILE_BEGIN);
    } else {
        SetFilePointerEx (fd, {0}, nullptr, FILE_BEGIN);
    }

    clear();
    readLogFile (logfile_);
    watcher_->addPath (logfile_);
}

#else
void
LogWidget::setLogFile (const std::string& filename)
{
    logfile_ = QString::fromStdString (filename);
    if (!openfiles_.contains (logfile_)) {
        fd = ::open (filename.c_str(), O_RDONLY);
        if (fd  == -1) {
            LERROR << "Could not open log file for reading: " << filename;
        }
        else {
            openfiles_[logfile_] = fd;
        }
    }
    else {
        fd = openfiles_[logfile_];
    }

    if (offsets_.contains (fd)) {
        lseek (fd, offsets_[fd], SEEK_SET);
    }
    else {
        lseek (fd, 0, SEEK_SET);
    }

    clear();
    readLogFile (logfile_);
    watcher_->addPath (logfile_);
}
#endif


#ifdef _WIN32
void
LogWidget::readLogFile (const QString& filename) {
    if (logfile_ != filename) {
        return;
    }

    fd = openfiles_[logfile_];
    constexpr DWORD BUFFSIZE = 8192; // Buffer size for reading
    char cbuffer[BUFFSIZE + 1]; // Extra byte for null terminator
    DWORD numbytes = 0; // Number of bytes read

    do {
        BOOL result = ReadFile (fd, cbuffer, BUFFSIZE, &numbytes, nullptr);
        if (result && numbytes > 0) {
            cbuffer[numbytes] = '\0';
            moveCursor (QTextCursor::End);
            insertPlainText (QString::fromUtf8 (cbuffer, static_cast<int> (numbytes)));
            moveCursor (QTextCursor::End);
        } else if (!result) {
            DWORD error = GetLastError();
            if (error != ERROR_HANDLE_EOF && error != ERROR_IO_PENDING) {
                LERROR << "Error reading log file: " + filename.toStdString();
                return;
            }
        }
    } while (numbytes > 0);
} // LogWidget::readLogFile

#else

void
LogWidget::readLogFile(const QString& filename)
{
    if (logfile_ != filename) {
        return;
    }
    else {
        fd = openfiles_[logfile_];
    }

    uint32_t BUFFSIZE = 8192;
    char cbuffer[BUFFSIZE + 1];
    ssize_t numbytes = 0;

    do {
        numbytes = read (fd, cbuffer, BUFFSIZE);
        if (numbytes > 0) {
            cbuffer[numbytes] = '\0';
            moveCursor (QTextCursor::End);
            insertPlainText (QString::fromUtf8 (cbuffer, int (numbytes)));
            moveCursor (QTextCursor::End);
        }
    } while (numbytes > 0 || (numbytes == -1 && errno == EINTR));
} // LogWidget::readLogFile
#endif


void
LogWidget::logLevel (QAction* action)
{
    auto level = action->text().toLower().toStdString();
    if (level == "info") {
        el::Loggers::setLoggingLevel (el::Level::Info);
    }
    else if (level == "warn") {
        el::Loggers::setLoggingLevel (el::Level::Warning);
    }
    else if (level == "error") {
        el::Loggers::setLoggingLevel (el::Level::Error);
    }
    else if (level == "debug") {
        el::Loggers::setLoggingLevel (el::Level::Debug);
    }
    else if (level == "off") {
        el::Loggers::setLoggingLevel (el::Level::Unknown);
    }
} // LogWidget::logLevel


void
LogWidget::showContextMenu (const QPoint& pt)
{
    auto menu = createStandardContextMenu (mapToGlobal (pt));
    menu->setPalette (darkPalette());
    auto font = menu->font();
    font.setPixelSize (11);
    menu->setFont (font);
    menu->addSeparator();
    menu->addAction (actions_["clear"]);
    menu->addSeparator();
    menu->addAction (actions_["info"]);
    menu->addAction (actions_["warn"]);
    menu->addAction (actions_["error"]);
    menu->addAction (actions_["debug"]);

    menu->exec (mapToGlobal (pt));
} // LogWidget::showContextMenu


void
LogWidget::resizeEvent (QResizeEvent* event)
{
    QTextEdit::resizeEvent (event);

    // qreal dpi = QGuiApplication::primaryScreen()->physicalDotsPerInch();
    // qreal ldpi = QGuiApplication::primaryScreen()->logicalDotsPerInch();

    verticalScrollBar()->setValue (verticalScrollBar()->maximum());
} // LogWidget::resizeEvent


void
LogWidget::storeFileOffset()
{
#ifdef _WIN32
    LARGE_INTEGER currentPosition;

    SetFilePointerEx (fd, {0}, &currentPosition, FILE_CURRENT);
    offsets_[logfile_] = currentPosition;
#else
    offsets_[fd] = lseek (fd, 0, SEEK_CUR);
#endif
}