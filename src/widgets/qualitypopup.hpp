#pragma once

#include "basewindow.hpp"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace chatterino {
namespace widgets {

class QualityPopup : public BaseWindow
{
public:
    QualityPopup(const QString &_channelName, QStringList options);
    static void showDialog(const QString &_channelName, QStringList options);

private:
    struct {
        QVBoxLayout vbox;
        QComboBox selector;
        QDialogButtonBox buttonBox;
        QPushButton okButton;
        QPushButton cancelButton;
    } ui;

    QString channelName;

    void okButtonClicked();
    void cancelButtonClicked();
};

}  // namespace widgets
}  // namespace chatterino
