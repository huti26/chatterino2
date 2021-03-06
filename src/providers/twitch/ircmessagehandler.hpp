#pragma once

#include <IrcMessage>

namespace chatterino {
namespace providers {
namespace twitch {

class IrcMessageHandler
{
    IrcMessageHandler() = default;

public:
    static IrcMessageHandler &getInstance();

    void handleRoomStateMessage(Communi::IrcMessage *message);
    void handleClearChatMessage(Communi::IrcMessage *message);
    void handleUserStateMessage(Communi::IrcMessage *message);
    void handleWhisperMessage(Communi::IrcMessage *message);
    void handleUserNoticeMessage(Communi::IrcMessage *message);
    void handleModeMessage(Communi::IrcMessage *message);
    void handleNoticeMessage(Communi::IrcNoticeMessage *message);
    void handleWriteConnectionNoticeMessage(Communi::IrcNoticeMessage *message);
};

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
