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

#include <fcntl.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include <QGuiApplication>
#include <QMenu>
#include <QResizeEvent>
#include <QThread>
#include <utility>

#include "logwidget.h"
#include "loghighlighter.h"
#include "chainstyles.h"


LogWidget::LogWidget (std::string filename, QWidget* parent) :
    QTextEdit (parent),
    notifier{nullptr},
    logfile (std::move (filename))
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
    setDocumentTitle (QString::fromStdString (logfile));

    notifier = new LogNotifier();
    thread_ = new QThread();

    connect (thread_, &QThread::started, notifier, &LogNotifier::monitor);
    connect (notifier, &LogNotifier::fileChanged, this, &LogWidget::readLogFile);

    notifier->moveToThread (thread_);
    thread_->start();

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
    thread_->quit();
}


#ifdef _WIN32
void
LogWidget::setLogFile (const std::string& filename)
{
    logfile = filename;

    // Check if the file is already open
    if (!openfiles_.count (filename)) {
        // Try to open the file with read access
        fd = CreateFileA (
            logfile.c_str(),
            GENERIC_READ,
            FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

        if (fd == INVALID_HANDLE_VALUE) {
            LERROR << "Could not open log file for reading: " + logfile + " " << GetLastError();
            return;
        } else {
            // Store the handle for reuse
            openfiles_[filename] = fd;
            notifier->setLogFile(logfile);
        }
    } else {
        fd = openfiles_[filename]; // Reuse existing file handle
    }

    // If there is an offset for this file, seek to it
    if (offsets_.count (filename)) {
        SetFilePointer (fd, offsets_[filename], nullptr, FILE_BEGIN);
    } else {
        SetFilePointer (fd, 0, nullptr, FILE_BEGIN); // Seek to the beginning if no offset
    }

    clear();
    readLogFile (logfile); // Custom function to read the log file
}

#else
void
LogWidget::setLogFile (const std::string& filename)
{
    logfile = filename;
    if (!openfiles_.count (filename)) {
        fd = ::open (logfile.c_str(), O_RDONLY);
        if (fd  == -1) {
            LERROR << "Could not open log file for reading: " << logfile;
        }
        else {
            openfiles_[filename] = fd;
            notifier->setLogFile (logfile);
        }
    }
    else {
        fd = openfiles_[filename];
    }

    if (offsets_.count (fd)) {
        lseek (fd, offsets_[fd], SEEK_SET);
    }
    else {
        lseek (fd, 0, SEEK_SET);
    }
    clear();
    readLogFile (logfile);
}
#endif


#ifdef _WIN32
void LogWidget::readLogFile(const std::string& filename) {
    if (logfile != filename) {
        return; // Ensure we are reading the correct log file
    } else {
        fd = openfiles_[filename];
    }

    const DWORD BUFFSIZE = 8192; // Buffer size for reading
    char cbuffer[BUFFSIZE + 1]; // Extra byte for null terminator
    DWORD numbytes = 0; // Number of bytes read
    BOOL result;

    do {
        result = ReadFile(fd, cbuffer, BUFFSIZE, &numbytes, nullptr);
        if (result && numbytes > 0) {
            cbuffer[numbytes] = '\0';
            moveCursor (QTextCursor::End);
            insertPlainText (QString::fromUtf8 (cbuffer, static_cast<int> (numbytes)));
            moveCursor (QTextCursor::End); // Move cursor again to the end
        } else if (!result) {
            DWORD error = GetLastError();
            if (error != ERROR_HANDLE_EOF && error != ERROR_IO_PENDING) {
                LERROR << "Error reading log file: " + filename;
                return;
            }
        }
    } while (numbytes > 0);
} // LogWidget::readLogFile

#else

void
LogWidget::readLogFile(const std::string& filename)
{
    if (logfile != filename) {
        return;
    }
    else {
        fd = openfiles_[filename];
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
    SetFilePointer (fd, offsets_[logfile], nullptr, FILE_CURRENT);
#else
    offsets_[fd] = lseek (fd, 0, SEEK_CUR);
#endif
}