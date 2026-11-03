#include "RoundPage.h"

#include <QColor>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include "../game/Protocol.h"
#include "../game/Room.h"
#include "RoomHeader.h"

namespace {

QString statusText(PlayerStatus status) {
    switch (status) {
        case PlayerStatus::Waiting:        return "waiting";
        case PlayerStatus::Answered:       return "✓ answered";
        case PlayerStatus::NoAnswer:       return "no answer";
        case PlayerStatus::Left:           return "left";
        case PlayerStatus::JoinsNextRound: return "joins next round";
    }
    return QString();
}

QColor statusColor(PlayerStatus status) {
    switch (status) {
        case PlayerStatus::Waiting:        return QColor("#c98a00");
        case PlayerStatus::Answered:       return QColor("#2e9e4f");
        case PlayerStatus::NoAnswer:       return QColor("#d0453a");
        case PlayerStatus::Left:           return QColor("#8a8a8a");
        case PlayerStatus::JoinsNextRound: return QColor("#3f8fd8");
    }
    return QColor();
}

}

RoundPage::RoundPage(QWidget* parent)
    : QWidget(parent),
      header_(new RoomHeader(14, this)),
      roundLabel_(new QLabel(this)),
      playerList_(new QListWidget(this)),
      answerList_(new QListWidget(this)),
      impostorLabel_(new QLabel(this)),
      earlyStopButton_(new QPushButton("Early stop", this)),
      showNextAnswerButton_(new QPushButton("Show next answer", this)),
      showImpostorButton_(new QPushButton("Show impostor", this)),
      newRoundButton_(new QPushButton("New round", this)) {
    QFont roundFont = roundLabel_->font();
    roundFont.setPointSize(30);
    roundFont.setBold(true);
    roundLabel_->setFont(roundFont);

    QFont largeFont = font();
    largeFont.setPointSize(22);
    playerList_->setFont(largeFont);
    answerList_->setFont(largeFont);
    impostorLabel_->setFont(largeFont);

    impostorLabel_->setTextFormat(Qt::PlainText);
    impostorLabel_->setWordWrap(true);

    auto* endRoomButton = new QPushButton("End room", this);

    connect(earlyStopButton_, &QPushButton::clicked, this, &RoundPage::earlyStopRequested);
    connect(showNextAnswerButton_, &QPushButton::clicked, this, &RoundPage::showNextAnswerRequested);
    connect(showImpostorButton_, &QPushButton::clicked, this, &RoundPage::showImpostorRequested);
    connect(newRoundButton_, &QPushButton::clicked, this, &RoundPage::newRoundRequested);
    connect(endRoomButton, &QPushButton::clicked, this, &RoundPage::endRoomRequested);

    auto* lists = new QHBoxLayout();
    lists->addWidget(playerList_);
    lists->addWidget(answerList_);

    auto* buttons = new QHBoxLayout();
    buttons->addWidget(earlyStopButton_);
    buttons->addWidget(showNextAnswerButton_);
    buttons->addWidget(showImpostorButton_);
    buttons->addWidget(newRoundButton_);
    buttons->addStretch();
    buttons->addWidget(endRoomButton);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(header_);
    layout->addWidget(roundLabel_);
    layout->addLayout(lists);
    layout->addWidget(impostorLabel_);
    layout->addLayout(buttons);
}

RoomHeader* RoundPage::header() const {
    return header_;
}

void RoundPage::showSnapshot(const RoomSnapshot& snapshot) {
    using State = Round::State;
    const State state = snapshot.roundState;

    roundLabel_->setText(QString("Round %1").arg(snapshot.roundId));

    playerList_->clear();
    for (const PlayerRow& row : snapshot.playerRows) {
        auto* item = new QListWidgetItem(QString("%1: %2").arg(row.name, statusText(row.status)), playerList_);
        item->setForeground(statusColor(row.status));
    }

    answerList_->clear();
    answerList_->addItems(snapshot.revealedAnswers);

    earlyStopButton_->setEnabled(state == State::Answering);
    showNextAnswerButton_->setEnabled(state == State::Revealing);
    showImpostorButton_->setVisible(state == State::AllRevealed || state == State::ImpostorShown);
    showImpostorButton_->setEnabled(state == State::AllRevealed);
    newRoundButton_->setEnabled(state == State::ImpostorShown && snapshot.playerNames.size() >= Protocol::kMinPlayers);

    if (state != State::Countdown) {
        earlyStopButton_->setText("Early stop");
    }

    const bool impostorShown = state == State::ImpostorShown;
    impostorLabel_->setVisible(impostorShown);
    impostorLabel_->setText(impostorShown
        ? QString("The impostor was %1\nEveryone's question: %2\nImpostor's question: %3")
              .arg(snapshot.impostorName, snapshot.normalQuestion, snapshot.impostorQuestion)
        : QString());
}

void RoundPage::showCountdown(int secondsLeft) {
    earlyStopButton_->setText(QString("Closing in %1…").arg(secondsLeft));
}
