#pragma once

#include "widgets/window.hpp"

namespace chatterino {
namespace singletons {

class WindowManager
{
public:
    WindowManager();

    ~WindowManager() = delete;

    void showSettingsDialog();
    void showAccountSelectPopup(QPoint point);

    void layoutVisibleChatWidgets(Channel *channel = nullptr);
    void repaintVisibleChatWidgets(Channel *channel = nullptr);
    void repaintGifEmotes();
    // void updateAll();

    widgets::Window &getMainWindow();
    widgets::Window &getSelectedWindow();
    widgets::Window &createWindow(widgets::Window::WindowType type);

    int windowCount();
    widgets::Window *windowAt(int index);

    void save();
    void initialize();
    void closeAll();

    pajlada::Signals::NoArgSignal repaintGifs;
    pajlada::Signals::Signal<Channel *> layout;

private:
    bool initialized = false;

    std::vector<widgets::Window *> windows;

    widgets::Window *mainWindow = nullptr;
    widgets::Window *selectedWindow = nullptr;

    void encodeChannel(IndirectChannel channel, QJsonObject &obj);
    IndirectChannel decodeChannel(const QJsonObject &obj);
};

}  // namespace singletons
}  // namespace chatterino
