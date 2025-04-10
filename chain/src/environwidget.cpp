// # MIT License
// # Copyright (c) 2025 Stephen J. Parker
// # SPDX-License-Identifier: MIT
// # See LICENSE file for full license text.
//

#include <QtWidgets/QHeaderView>
#include <QtWidgets/QVBoxLayout>
#include "environwidget.h"
#include "chainstyles.h"


EnvironWidget::EnvironWidget (QWidget* parent) : QWidget (parent), table_ (new QTableWidget (this))
{
    setupUI();
}


void
EnvironWidget::setupUI()
{
    setObjectName ("environWidget");

    auto hlayout = new QHBoxLayout (this);

    hlayout->setContentsMargins (0, 0, 0, 0);
    hlayout->setSpacing (0);

    table_->setFocusPolicy (Qt::ClickFocus);

    auto palette = darkPalette();
    palette.setColor (QPalette::Window, QColor (0, 0, 0, 127));
    table_->setPalette (palette);

    table_->setObjectName ("tableWidget");
    table_->setColumnCount (2);
    table_->setRowCount (0);
    table_->verticalHeader()->setVisible (false);
    table_->horizontalHeader()->setVisible (true);
    table_->horizontalHeader()->setSectionResizeMode (QHeaderView::Stretch);
    table_->horizontalHeader()->resizeSection (0, 120);
    table_->horizontalHeader()->resizeSection (1, 120);
    table_->horizontalHeader()->setFixedHeight (22);
    auto font = table_->horizontalHeader()->font();
    font.setPixelSize (11);
    table_->horizontalHeader()->setFont (font);
    table_->setHorizontalHeaderLabels ({"Name", "Value"});
    table_->setContentsMargins (0, 0, 0, 0);
    addRow ("", "");

    hlayout->addWidget (table_);

    connect (table_, &QTableWidget::itemChanged, this, &EnvironWidget::updateTable);
} // EnvironWidget::setupUI


json
EnvironWidget::serialize()
{
    json environ_;

    for (auto row = 0; row < table_->rowCount(); ++row) {
        auto key = table_->item (row, 0)->text().toStdString();
        auto value = table_->item (row, 1)->text().toStdString();

        if (!key.empty() && !value.empty()) {
            environ_[key] = value;
        }
    }

    return environ_;
} // EnvironWidget::serialize


void
EnvironWidget::populateUI (const json& environ_)
{
    // Block here to avoid recursive signaling.
    table_->blockSignals (true);

    table_->clearContents();

    while (table_->rowCount()) {
        table_->removeRow (0);
    }

    if (environ_ != nullptr) {
        for (const auto& [key, value] : environ_.items()) {
            addRow (key.c_str(), value.get<std::string>().c_str());
        }
    }

    addRow ("", "");
    table_->blockSignals (false);
} // EnvironWidget::populateUI


void
EnvironWidget::addRow (const QString& key, const QString& value)
{
    table_->blockSignals (true);

    auto row = table_->rowCount();

    table_->insertRow (row);

    auto keyitem = new QTableWidgetItem (key);
    auto keyfont = keyitem->font();

    keyfont.setPixelSize (12);
    keyfont.setBold (true);
    keyitem->setFont (keyfont);
    keyitem->setForeground (QColor (200, 200, 200));
    keyitem->setFlags (Qt::ItemIsEditable | Qt::ItemIsEnabled);
    keyitem->setTextAlignment (Qt::AlignRight | Qt::AlignVCenter);
    table_->setItem (row, 0, keyitem);

    auto valueitem = new QTableWidgetItem (value);
    auto valuefont = valueitem->font();

    valuefont.setPixelSize (11);
    valueitem->setFont (valuefont);
    valueitem->setForeground (QColor (175, 175, 175));
    valueitem->setFlags (Qt::ItemIsEditable | Qt::ItemIsEnabled);
    valueitem->setTextAlignment (Qt::AlignLeft | Qt::AlignVCenter);
    table_->setItem (row, 1, valueitem);

    if (key.isEmpty() && value.isEmpty()) {
        valueitem->setFlags (Qt::ItemIsEnabled);
    }

    table_->blockSignals (false);
} // EnvironWidget::addRow


void
EnvironWidget::updateTable()
{
    table_->blockSignals (true);

    for (auto row = 0; row < table_->rowCount(); ++row) {
        auto key = table_->item (row, 0);
        auto keystr = key->text();

        if (!keystr.isEmpty()) {
            auto val = table_->item (row, 1);
            val->setFlags (Qt::ItemIsEditable | Qt::ItemIsEnabled);
        }
        else {
            // remove empties.
            table_->removeRow (row);
        }
    }

    auto keystr = table_->item (table_->rowCount() - 1, 0)->text();
    auto valstr = table_->item (table_->rowCount() - 1, 1)->text();

    if (!keystr.isEmpty() && !valstr.isEmpty()) {
        addRow ("", "");
    }

    table_->blockSignals (false);

    auto environ_ = serialize();

    Q_EMIT (environWidgetChanged (environ_));
} // EnvironWidget::updateTable
