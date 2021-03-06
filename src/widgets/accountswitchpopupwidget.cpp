#include "widgets/accountswitchpopupwidget.hpp"
#include "debug/log.hpp"
#include "widgets/settingsdialog.hpp"

#include <QHBoxLayout>
#include <QLayout>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

namespace chatterino {
namespace widgets {

AccountSwitchPopupWidget::AccountSwitchPopupWidget(QWidget *parent)
    : QWidget(parent)
{
    this->setWindowFlags(Qt::FramelessWindowHint);

    this->setContentsMargins(0, 0, 0, 0);

    this->ui.accountSwitchWidget = new AccountSwitchWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout(this);
    this->ui.accountSwitchWidget->setFocusPolicy(Qt::NoFocus);
    vbox->addWidget(this->ui.accountSwitchWidget);

    // vbox->setSizeConstraint(QLayout::SetMinimumSize);

    auto hbox = new QHBoxLayout();
    auto manageAccountsButton = new QPushButton(this);
    manageAccountsButton->setText("Manage Accounts");
    hbox->addWidget(manageAccountsButton);
    vbox->addLayout(hbox);

    connect(manageAccountsButton, &QPushButton::clicked, []() {
        SettingsDialog::showDialog(SettingsDialog::PreferredTab::Accounts);  //
    });

    this->setLayout(vbox);
}

void AccountSwitchPopupWidget::refresh()
{
    this->ui.accountSwitchWidget->refresh();
}

void AccountSwitchPopupWidget::focusOutEvent(QFocusEvent *)
{
    this->hide();
}

void AccountSwitchPopupWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    painter.fillRect(this->rect(), QColor(255, 255, 255));
}

}  // namespace widgets
}  // namespace chatterino
