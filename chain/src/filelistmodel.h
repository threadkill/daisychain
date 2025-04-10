// # MIT License
// # Copyright (c) 2025 Stephen J. Parker
// # SPDX-License-Identifier: MIT
// # See LICENSE file for full license text.
//

#pragma once

#include "chainmodel.h"


class FileListModel : public ChainModel
{
    Q_OBJECT

public:
    FileListModel();

    ~FileListModel() override = default;

    QWidget* embeddedWidget() override; // embeddedWidget

    static QString Name() { return {"FileList"}; } // Name

    QString name() const override { return FileListModel::Name(); } // name
};
