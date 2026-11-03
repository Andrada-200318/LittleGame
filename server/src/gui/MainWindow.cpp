#include "MainWindow.h"

#include <QCloseEvent>
#include <QCoreApplication>
#include <QKeySequence>
#include <QMessageBox>
#include <QShortcut>
#include <QStackedWidget>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>

#include <string>

#include "../game/Protocol.h"
#include "../game/Room.h"
#include "../net/GameServer.h"
#include "../net/TCPSocket.h"
#include "LogPanel.h"
#include "RoomHeader.h"
#include "RoomPage.h"
#include "RoundPage.h"
#include "TitlePage.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      pages_(new QStackedWidget(this)),
      titlePage_(new TitlePage(pages_)),
      roomPage_(new RoomPage(pages_)),
      roundPage_(new RoundPage(pages_)),
      countdownTickTimer_(new QTimer(this)),
      countdownCloseTimer_(new QTimer(this)) {
    setWindowTitle("Little Impostor Game");
    setMinimumSize(1000, 700);

    pages_->addWidget(titlePage_);
    pages_->addWidget(roomPage_);
    pages_->addWidget(roundPage_);

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(pages_, 1);
    layout->addWidget(new LogPanel(central));
    setCentralWidget(central);

    auto* fullScreenShortcut = new QShortcut(QKeySequence(Qt::Key_F11), this);
    connect(fullScreenShortcut, &QShortcut::activated, this, &MainWindow::toggleFullScreen);

    connect(titlePage_, &TitlePage::startRequested, this, &MainWindow::startRoom);
    connect(roomPage_, &RoomPage::startRoundRequested, this, &MainWindow::startRound);
    connect(roomPage_, &RoomPage::endRoomRequested, this, &MainWindow::endRoomClicked);
    connect(roundPage_, &RoundPage::earlyStopRequested, this, &MainWindow::earlyStop);
    connect(roundPage_, &RoundPage::showNextAnswerRequested, this, &MainWindow::showNextAnswer);
    connect(roundPage_, &RoundPage::showImpostorRequested, this, &MainWindow::showImpostor);
    connect(roundPage_, &RoundPage::newRoundRequested, this, &MainWindow::startRound);
    connect(roundPage_, &RoundPage::endRoomRequested, this, &MainWindow::endRoomClicked);

    countdownTickTimer_->setInterval(1000);
    countdownCloseTimer_->setInterval(Protocol::kEarlyStopSeconds * 1000);
    countdownCloseTimer_->setSingleShot(true);
    connect(countdownTickTimer_, &QTimer::timeout, this, &MainWindow::countdownTick);
    connect(countdownCloseTimer_, &QTimer::timeout, this, &MainWindow::countdownFinished);
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent* event) {
    if (room_ && !confirmEndRoom()) {
        event->ignore();
        return;
    }
    endRoom();
    QMainWindow::closeEvent(event);
}

void MainWindow::endRoomClicked() {
    if (confirmEndRoom()) {
        endRoom();
    }
}

bool MainWindow::confirmEndRoom() {
    const auto answer = QMessageBox::question(this, "End room", "End the room for every player?",
                                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return answer == QMessageBox::Yes;
}

void MainWindow::toggleFullScreen() {
    if (isFullScreen()) {
        showNormal();
    } else {
        showFullScreen();
    }
}

void MainWindow::startRoom(const QString& password) {
    auto room = std::make_unique<Room>(password.toStdString(),
                                       QCoreApplication::applicationDirPath() + "/questions.txt");
    auto server = std::make_unique<GameServer>(*room);

    // Connected before the server starts, so no change can be missed. Queued: changed() is emitted
    // on client threads, and widgets may only be touched on the GUI thread. Qt delivers the call
    // through the GUI thread's event loop.
    connect(room.get(), &Room::changed, this, &MainWindow::refreshFromRoom, Qt::QueuedConnection);

    std::string error;
    if (!server->start(Protocol::kPort, error)) {
        QMessageBox::critical(this, "Start room", QString::fromStdString(error));
        return;
    }

    room_ = std::move(room);
    server_ = std::move(server);

    QStringList addresses;
    for (const auto& [adapter, address] : TCPSocket::localIPv4Addresses()) {
        addresses.append(QString::fromStdString(adapter + ": " + address));
    }
    roomPage_->header()->setRoomInfo(addresses, Protocol::kPort, password);
    roundPage_->header()->setRoomInfo(addresses, Protocol::kPort, password);

    refreshFromRoom();
}

void MainWindow::endRoom() {
    if (!room_) {
        return;
    }

    stopCountdown();

    room_->close();
    server_->stop();
    server_.reset();
    room_.reset();

    pages_->setCurrentWidget(titlePage_);
}

void MainWindow::refreshFromRoom() {
    if (!room_) {
        return;
    }

    const RoomSnapshot snapshot = room_->snapshot();

    if (snapshot.roundState != Round::State::Countdown) {
        stopCountdown();
    }

    if (snapshot.roundId == 0) {
        roomPage_->showSnapshot(snapshot);
        pages_->setCurrentWidget(roomPage_);
    } else {
        roundPage_->showSnapshot(snapshot);
        pages_->setCurrentWidget(roundPage_);
    }
}

void MainWindow::startRound() {
    if (room_) {
        room_->startRound();
    }
}

void MainWindow::earlyStop() {
    if (!room_) {
        return;
    }

    const int roundId = room_->earlyStop();
    if (roundId == 0) {
        return;
    }

    countdownRoundId_ = roundId;
    countdownSecondsLeft_ = Protocol::kEarlyStopSeconds;
    roundPage_->showCountdown(countdownSecondsLeft_);
    countdownTickTimer_->start();
    countdownCloseTimer_->start();
}

void MainWindow::countdownTick() {
    --countdownSecondsLeft_;
    if (countdownSecondsLeft_ > 0) {
        roundPage_->showCountdown(countdownSecondsLeft_);
    }
}

void MainWindow::countdownFinished() {
    stopCountdown();
    if (room_) {
        room_->closeAnswers(countdownRoundId_);
    }
}

void MainWindow::stopCountdown() {
    countdownTickTimer_->stop();
    countdownCloseTimer_->stop();
}

void MainWindow::showNextAnswer() {
    if (room_) {
        room_->revealNext();
    }
}

void MainWindow::showImpostor() {
    if (room_) {
        room_->showImpostor();
    }
}
