#include "RoomPage.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include "../game/Protocol.h"
#include "../game/Room.h"
#include "RoomHeader.h"

RoomPage::RoomPage(QWidget* parent)
    : QWidget(parent),
      header_(new RoomHeader(28, this)),
      playerList_(new QListWidget(this)),
      playerCountLabel_(new QLabel(this)),
      questionsWarningLabel_(new QLabel("questions.txt is missing or empty: using the 3 built-in question pairs", this)),
      startRoundButton_(new QPushButton("Start round", this)) {
    QFont largeFont = font();
    largeFont.setPointSize(22);
    playerList_->setFont(largeFont);
    playerCountLabel_->setFont(largeFont);

    questionsWarningLabel_->setStyleSheet("color: #c98a00;");

    auto* endRoomButton = new QPushButton("End room", this);
    connect(startRoundButton_, &QPushButton::clicked, this, &RoomPage::startRoundRequested);
    connect(endRoomButton, &QPushButton::clicked, this, &RoomPage::endRoomRequested);

    auto* buttons = new QHBoxLayout();
    buttons->addStretch();
    buttons->addWidget(startRoundButton_);
    buttons->addWidget(endRoomButton);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(header_);
    layout->addWidget(playerCountLabel_);
    layout->addWidget(playerList_);
    layout->addWidget(questionsWarningLabel_);
    layout->addLayout(buttons);
}

RoomHeader* RoomPage::header() const {
    return header_;
}

void RoomPage::showSnapshot(const RoomSnapshot& snapshot) {
    playerList_->clear();
    playerList_->addItems(snapshot.playerNames);
    playerCountLabel_->setText(QString("%1 / %2 players").arg(snapshot.playerNames.size()).arg(Protocol::kMaxPlayers));
    startRoundButton_->setEnabled(snapshot.playerNames.size() >= Protocol::kMinPlayers);
    questionsWarningLabel_->setVisible(snapshot.usesHardcodedQuestions);
}
