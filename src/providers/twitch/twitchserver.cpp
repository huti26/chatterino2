#include "twitchserver.hpp"

#include "application.hpp"
#include "providers/twitch/ircmessagehandler.hpp"
#include "providers/twitch/twitchaccount.hpp"
#include "providers/twitch/twitchhelpers.hpp"
#include "providers/twitch/twitchmessagebuilder.hpp"
#include "singletons/accountmanager.hpp"
#include "util/posttothread.hpp"

#include <cassert>

using namespace Communi;
using namespace chatterino::singletons;

namespace chatterino {
namespace providers {
namespace twitch {

TwitchServer::TwitchServer()
    : whispersChannel(new Channel("/whispers", Channel::TwitchWhispers))
    , mentionsChannel(new Channel("/mentions", Channel::TwitchMentions))
    , watchingChannel(Channel::getEmpty(), Channel::TwitchWatching)
{
    qDebug() << "init TwitchServer";
}

void TwitchServer::initialize()
{
    getApp()->accounts->Twitch.currentUserChanged.connect(
        [this]() { util::postToThread([this] { this->connect(); }); });
}

void TwitchServer::initializeConnection(IrcConnection *connection, bool isRead, bool isWrite)
{
    std::shared_ptr<TwitchAccount> account = getApp()->accounts->Twitch.getCurrent();

    qDebug() << "logging in as" << account->getUserName();

    QString username = account->getUserName();
    //    QString oauthClient = account->getOAuthClient();
    QString oauthToken = account->getOAuthToken();

    if (!oauthToken.startsWith("oauth:")) {
        oauthToken.prepend("oauth:");
    }

    connection->setUserName(username);
    connection->setNickName(username);
    connection->setRealName(username);

    if (!account->isAnon()) {
        connection->setPassword(oauthToken);

        // fourtf: ignored users
        //        this->refreshIgnoredUsers(username, oauthClient, oauthToken);
    }

    connection->sendCommand(IrcCommand::createCapability("REQ", "twitch.tv/membership"));
    connection->sendCommand(IrcCommand::createCapability("REQ", "twitch.tv/commands"));
    connection->sendCommand(IrcCommand::createCapability("REQ", "twitch.tv/tags"));

    connection->setHost("irc.chat.twitch.tv");
    connection->setPort(6667);
}

std::shared_ptr<Channel> TwitchServer::createChannel(const QString &channelName)
{
    TwitchChannel *channel = new TwitchChannel(channelName, this->getReadConnection());

    channel->sendMessageSignal.connect(
        [this](auto chan, auto msg) { this->sendMessage(chan, msg); });

    return std::shared_ptr<Channel>(channel);
}

void TwitchServer::privateMessageReceived(IrcPrivateMessage *message)
{
    QString channelName;
    if (!TrimChannelName(message->target(), channelName)) {
        return;
    }

    this->onPrivateMessage.invoke(message);
    auto chan = this->getChannelOrEmpty(channelName);

    if (chan->isEmpty()) {
        return;
    }

    messages::MessageParseArgs args;

    TwitchMessageBuilder builder(chan.get(), message, args);

    if (!builder.isIgnored()) {
        messages::MessagePtr _message = builder.build();
        if (_message->flags & messages::Message::Highlighted) {
            this->mentionsChannel->addMessage(_message);
        }

        chan->addMessage(_message);
    }
}

void TwitchServer::messageReceived(IrcMessage *message)
{
    //    this->readConnection
    if (message->type() == IrcMessage::Type::Private) {
        // We already have a handler for private messages
        return;
    }

    const QString &command = message->command();

    if (command == "ROOMSTATE") {
        IrcMessageHandler::getInstance().handleRoomStateMessage(message);
    } else if (command == "CLEARCHAT") {
        IrcMessageHandler::getInstance().handleClearChatMessage(message);
    } else if (command == "USERSTATE") {
        IrcMessageHandler::getInstance().handleUserStateMessage(message);
    } else if (command == "WHISPER") {
        IrcMessageHandler::getInstance().handleWhisperMessage(message);
    } else if (command == "USERNOTICE") {
        IrcMessageHandler::getInstance().handleUserNoticeMessage(message);
    } else if (command == "MODE") {
        IrcMessageHandler::getInstance().handleModeMessage(message);
    } else if (command == "NOTICE") {
        IrcMessageHandler::getInstance().handleNoticeMessage(
            static_cast<IrcNoticeMessage *>(message));
    }
}

void TwitchServer::writeConnectionMessageReceived(IrcMessage *message)
{
    switch (message->type()) {
        case IrcMessage::Type::Notice: {
            IrcMessageHandler::getInstance().handleWriteConnectionNoticeMessage(
                static_cast<IrcNoticeMessage *>(message));
        } break;
    }
}

std::shared_ptr<Channel> TwitchServer::getCustomChannel(const QString &channelName)
{
    if (channelName == "/whispers") {
        return whispersChannel;
    }

    if (channelName == "/mentions") {
        return mentionsChannel;
    }

    return nullptr;
}

void TwitchServer::forEachChannelAndSpecialChannels(std::function<void(ChannelPtr)> func)
{
    std::lock_guard<std::mutex> lock(this->channelMutex);

    for (std::weak_ptr<Channel> &weak : this->channels) {
        std::shared_ptr<Channel> chan = weak.lock();
        if (!chan) {
            continue;
        }

        func(chan);
    }

    func(this->whispersChannel);
    func(this->mentionsChannel);
}

std::shared_ptr<Channel> TwitchServer::getChannelOrEmptyByID(const QString &channelID)
{
    {
        std::lock_guard<std::mutex> lock(this->channelMutex);

        for (const auto &weakChannel : this->channels) {
            auto channel = weakChannel.lock();
            if (!channel) {
                continue;
            }

            auto twitchChannel = std::dynamic_pointer_cast<TwitchChannel>(channel);
            if (!twitchChannel) {
                continue;
            }

            if (twitchChannel->roomID == channelID) {
                return twitchChannel;
            }
        }
    }

    return Channel::getEmpty();
}

QString TwitchServer::cleanChannelName(const QString &dirtyChannelName)
{
    return dirtyChannelName.toLower();
}

}  // namespace twitch
}  // namespace providers
}  // namespace chatterino
