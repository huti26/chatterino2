#pragma once

#include <QCompleter>
#include <QKeyEvent>
#include <QTextEdit>
#include <pajlada/signals/signal.hpp>

class ResizingTextEdit : public QTextEdit
{
public:
    ResizingTextEdit();

    QSize sizeHint() const override;

    bool hasHeightForWidth() const override;

    pajlada::Signals::Signal<QKeyEvent *> keyPressed;

    void setCompleter(QCompleter *c);
    QCompleter *getCompleter() const;

protected:
    int heightForWidth(int) const override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QCompleter *completer = nullptr;
    bool completionInProgress = false;

    // hadSpace is set to true in case the "textUnderCursor" word was after a space
    QString textUnderCursor(bool *hadSpace = nullptr) const;

private slots:
    void insertCompletion(const QString &completion);
};
