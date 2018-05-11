#include "widgets/helper/splitheader.hpp"

#include "application.hpp"
#include "providers/twitch/twitchchannel.hpp"
#include "providers/twitch/twitchserver.hpp"
#include "singletons/resourcemanager.hpp"
#include "singletons/thememanager.hpp"
#include "util/layoutcreator.hpp"
#include "util/urlfetch.hpp"
#include "widgets/helper/label.hpp"
#include "widgets/split.hpp"
#include "widgets/splitcontainer.hpp"
#include "widgets/tooltipwidget.hpp"

#include <QByteArray>
#include <QDrag>
#include <QMimeData>
#include <QPainter>

#ifdef USEWEBENGINE
#include "widgets/streamview.hpp"
#endif

using namespace chatterino::providers::twitch;

namespace chatterino {
namespace widgets {

SplitHeader::SplitHeader(Split *_split)
    : BaseWidget(_split)
    , split(_split)
{
    auto app = getApp();
    this->setMouseTracking(true);

    util::LayoutCreator<SplitHeader> layoutCreator(this);
    auto layout = layoutCreator.emplace<QHBoxLayout>().withoutMargin();
    {
        // dropdown label
        auto dropdown = layout.emplace<RippleEffectButton>(this).assign(&this->dropdownButton);
        dropdown->setMouseTracking(true);
        dropdown->setPixmap(app->resources->splitHeaderContext->getPixmap());
        this->addDropdownItems(dropdown.getElement());
        QObject::connect(dropdown.getElement(), &RippleEffectButton::clicked, this, [this] {
            QTimer::singleShot(80, [&] {
                this->dropdownMenu.move(
                    this->dropdownButton->mapToGlobal(QPoint(0, this->dropdownButton->height())));
                this->dropdownMenu.show();
            });
        });

        layout->addStretch(1);

        // channel name label
        //        auto title = layout.emplace<Label>(this).assign(&this->titleLabel);
        auto title = layout.emplace<SignalLabel>().assign(&this->titleLabel);
        title->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        title->setMouseTracking(true);
        QObject::connect(this->titleLabel, &SignalLabel::mouseDoubleClick, this,
                         &SplitHeader::mouseDoubleClickEvent);
        QObject::connect(this->titleLabel, &SignalLabel::mouseMove, this,
                         &SplitHeader::mouseMoveEvent);

        layout->addStretch(1);

        // moderation mode
        auto moderator = layout.emplace<RippleEffectButton>(this).assign(&this->moderationButton);

        QObject::connect(moderator.getElement(), &RippleEffectButton::clicked, this, [this] {
            this->split->setModerationMode(!this->split->getModerationMode());
        });

        this->updateModerationModeIcon();
    }

    // ---- misc
    this->layout()->setMargin(0);
    this->scaleChangedEvent(this->getScale());

    this->updateChannelText();

    this->initializeChannelSignals();

    this->split->channelChanged.connect([this]() {
        this->initializeChannelSignals();  //
    });
}

SplitHeader::~SplitHeader()
{
    this->onlineStatusChangedConnection.disconnect();
}

void SplitHeader::addDropdownItems(RippleEffectButton *label)
{
    // clang-format off
    this->dropdownMenu.addAction("Add new split", this->split, &Split::doAddSplit, QKeySequence(tr("Ctrl+T")));
    this->dropdownMenu.addAction("Close split", this->split, &Split::doCloseSplit, QKeySequence(tr("Ctrl+W")));
//    this->dropdownMenu.addAction("Move split", this, SLOT(menuMoveSplit()));
    this->dropdownMenu.addAction("Popup", this->split, &Split::doPopup);
    this->dropdownMenu.addAction("Open viewer list", this->split, &Split::doOpenViewerList);
    this->dropdownMenu.addSeparator();
#ifdef USEWEBENGINE
    this->dropdownMenu.addAction("Start watching", this, [this]{
        ChannelPtr _channel = this->split->getChannel();
        TwitchChannel *tc = dynamic_cast<TwitchChannel *>(_channel.get());

        if (tc != nullptr) {
            StreamView *view = new StreamView(_channel, "https://player.twitch.tv/?channel=" + tc->name);
            view->setAttribute(Qt::WA_DeleteOnClose, true);
            view->show();
        }
    });
#endif
    this->dropdownMenu.addAction("Change channel", this->split, &Split::doChangeChannel, QKeySequence(tr("Ctrl+R")));
    this->dropdownMenu.addAction("Clear chat", this->split, &Split::doClearChat);
    this->dropdownMenu.addAction("Open in web browser", this->split, &Split::doOpenChannel);
#ifndef USEWEBENGINE
    this->dropdownMenu.addAction("Open web player", this->split, &Split::doOpenPopupPlayer);
#endif
    this->dropdownMenu.addAction("Open in Streamlink", this->split, &Split::doOpenStreamlink);
    this->dropdownMenu.addSeparator();
    this->dropdownMenu.addAction("Reload channel emotes", this, SLOT(menuReloadChannelEmotes()));
    this->dropdownMenu.addAction("Manual reconnect", this, SLOT(menuManualReconnect()));
    this->dropdownMenu.addSeparator();
    this->dropdownMenu.addAction("Show changelog", this, SLOT(menuShowChangelog()));
    // clang-format on
}

void SplitHeader::initializeChannelSignals()
{
    // Disconnect any previous signal first
    this->onlineStatusChangedConnection.disconnect();

    auto channel = this->split->getChannel();
    TwitchChannel *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());

    if (twitchChannel) {
        this->managedConnections.emplace_back(twitchChannel->updateLiveInfo.connect([this]() {
            this->updateChannelText();  //
        }));
    }
}

void SplitHeader::scaleChangedEvent(float scale)
{
    int w = 28 * scale;

    this->setFixedHeight(w);
    this->dropdownButton->setFixedWidth(w);
    this->moderationButton->setFixedWidth(w);
    //    this->titleLabel->setFont(
    //        singletons::FontManager::getInstance().getFont(FontStyle::Medium, scale));
}

void SplitHeader::updateChannelText()
{
    auto indirectChannel = this->split->getIndirectChannel();
    auto channel = this->split->getChannel();

    QString title = channel->name;

    if (indirectChannel.getType() == Channel::TwitchWatching) {
        title = "watching: " + (title.isEmpty() ? "none" : title);
    }

    TwitchChannel *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());

    if (twitchChannel != nullptr) {
        const auto &streamStatus = twitchChannel->GetStreamStatus();

        if (streamStatus.live) {
            this->isLive = true;
            this->tooltip = "<style>.center    { text-align: center; }</style>"
                            "<p class = \"center\">" +
                            streamStatus.title + "<br><br>" + streamStatus.game + "<br>" +
                            (streamStatus.rerun ? "Vod-casting" : "Live") + " for " +
                            streamStatus.uptime + " with " +
                            QString::number(streamStatus.viewerCount) +
                            " viewers"
                            "</p>";
            if (streamStatus.rerun) {
                title += " (rerun)";
            } else {
                title += " (live)";
            }
        }
    }

    if (title.isEmpty()) {
        title = "<empty>";
    }

    this->isLive = false;
    this->titleLabel->setText(title);
    this->tooltip = "";
}

void SplitHeader::updateModerationModeIcon()
{
    auto app = getApp();

    this->moderationButton->setPixmap(this->split->getModerationMode()
                                          ? app->resources->moderationmode_enabled->getPixmap()
                                          : app->resources->moderationmode_disabled->getPixmap());

    bool modButtonVisible = false;
    ChannelPtr channel = this->split->getChannel();

    TwitchChannel *tc = dynamic_cast<TwitchChannel *>(channel.get());

    if (tc != nullptr && tc->hasModRights()) {
        modButtonVisible = true;
    }

    this->moderationButton->setVisible(modButtonVisible);
}

void SplitHeader::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(rect(), this->themeManager->splits.header.background);
    painter.setPen(this->themeManager->splits.header.border);
    painter.drawRect(0, 0, width() - 1, height() - 1);
}

void SplitHeader::mousePressEvent(QMouseEvent *event)
{
    this->dragging = true;

    this->dragStart = event->pos();
}

void SplitHeader::mouseMoveEvent(QMouseEvent *event)
{
    if (!this->dragging && this->isLive) {
        auto tooltipWidget = TooltipWidget::getInstance();
        tooltipWidget->moveTo(this, event->globalPos());
        tooltipWidget->setText(tooltip);
        tooltipWidget->show();
    }

    if (this->dragging) {
        if (std::abs(this->dragStart.x() - event->pos().x()) > 12 ||
            std::abs(this->dragStart.y() - event->pos().y()) > 12) {
            this->split->drag();
            this->dragging = false;
        }
    }
}

void SplitHeader::leaveEvent(QEvent *event)
{
    TooltipWidget::getInstance()->hide();
    BaseWidget::leaveEvent(event);
}

void SplitHeader::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        this->split->doChangeChannel();
    }
}

void SplitHeader::rightButtonClicked()
{
}

void SplitHeader::themeRefreshEvent()
{
    QPalette palette;
    palette.setColor(QPalette::Foreground, this->themeManager->splits.header.text);

    //    this->dropdownButton->setPalette(palette);
    this->titleLabel->setPalette(palette);
    //    this->moderationLabel->setPalette(palette);
}

void SplitHeader::menuMoveSplit()
{
}

void SplitHeader::menuReloadChannelEmotes()
{
}

void SplitHeader::menuManualReconnect()
{
    auto app = getApp();

    // fourtf: connection
    app->twitch.server->connect();
}

void SplitHeader::menuShowChangelog()
{
}

}  // namespace widgets
}  // namespace chatterino
