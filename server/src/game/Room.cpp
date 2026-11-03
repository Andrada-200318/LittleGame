#include "Room.h"

#include <QtDebug>

#include <algorithm>

#include "../net/TCPSocket.h"
#include "Protocol.h"

Room::Room(const std::string& password, const QString& questionsFilePath)
    : password_(password), questionBank_(questionsFilePath), random_(std::random_device{}()) {
}

bool Room::join(int clientId, const std::string& password, const std::string& username,
                const std::shared_ptr<TCPSocket>& socket) {
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (closed_) {
            return false;
        }

        if (password != password_) {
            refuseJoin(clientId, *socket, Protocol::kWrongPassword, "Wrong password");
            return false;
        }

        const QString name = QString::fromStdString(username).trimmed();
        const auto nameBytes = static_cast<std::size_t>(name.toUtf8().size());
        if (name.isEmpty() || nameBytes > Protocol::kMaxUsernameBytes
            || name.contains('|') || name.contains(',')) {
            refuseJoin(clientId, *socket, Protocol::kInvalidName, "Invalid name");
            return false;
        }

        for (const Player& player : players_) {
            if (player.name.compare(name, Qt::CaseInsensitive) == 0) {
                refuseJoin(clientId, *socket, Protocol::kNameTaken, "Name already taken");
                return false;
            }
        }

        if (static_cast<int>(players_.size()) >= Protocol::kMaxPlayers) {
            refuseJoin(clientId, *socket, Protocol::kRoomFull, "Room is full");
            return false;
        }

        players_.push_back({clientId, name, socket});

        const char* status = round_ ? Protocol::kStatusWaitNextRound : Protocol::kStatusLobby;

        // Sent with the lock held, so a ROUND_START can never overtake this JOIN_OK.
        socket->sendLine(Protocol::format(Protocol::kJoinOk, {name.toStdString(), status}));
        qDebug("[client %d] joined as %s", clientId, qUtf8Printable(name));
    }

    // Emitted after the lock is released: nothing that reacts to it should run under our mutex.
    emit changed();
    return true;
}

void Room::refuseJoin(int clientId, TCPSocket& socket, const char* code, const char* message) {
    socket.sendLine(Protocol::format(Protocol::kJoinError, {code, message}));
    qDebug("[client %d] join refused: %s", clientId, code);
}

void Room::removePlayer(int clientId) {
    {
        std::lock_guard<std::mutex> lock(mutex_);

        const auto player = std::find_if(players_.begin(), players_.end(), [clientId](const Player& p) {
            return p.clientId == clientId;
        });
        if (player == players_.end()) {
            return;
        }

        qDebug("[client %d] removed %s", clientId, qUtf8Printable(player->name));
        players_.erase(player);

        if (round_) {
            round_->markLeft(clientId);

            if (!closed_ && round_->acceptsAnswers() && round_->everyoneConnectedAnswered()) {
                closeAnswersLocked();
            }
        }
    }
    emit changed();
}

void Room::submitAnswer(int clientId, int roundId, const std::string& text) {
    {
        std::lock_guard<std::mutex> lock(mutex_);

        const auto player = std::find_if(players_.begin(), players_.end(), [clientId](const Player& p) {
            return p.clientId == clientId;
        });
        if (closed_ || player == players_.end()) {
            return;
        }
        TCPSocket& socket = *player->socket;

        if (!round_ || round_->id() != roundId) {
            rejectAnswer(clientId, socket, roundId, Protocol::kWrongRound);
            return;
        }
        if (!round_->isParticipant(clientId)) {
            rejectAnswer(clientId, socket, roundId, Protocol::kNotInRound);
            return;
        }
        if (round_->hasAnswered(clientId)) {
            rejectAnswer(clientId, socket, roundId, Protocol::kAlreadyAnswered);
            return;
        }
        if (!round_->acceptsAnswers()) {
            rejectAnswer(clientId, socket, roundId, Protocol::kAnswersClosed);
            return;
        }

        const QString answer = QString::fromStdString(text).trimmed();
        const auto answerBytes = static_cast<std::size_t>(answer.toUtf8().size());
        if (answer.isEmpty() || answerBytes > Protocol::kMaxAnswerBytes) {
            rejectAnswer(clientId, socket, roundId, Protocol::kInvalidAnswer);
            return;
        }

        round_->recordAnswer(clientId, answer);
        socket.sendLine(Protocol::format(Protocol::kAnswerOk, {std::to_string(roundId)}));
        qDebug("[client %d] answer accepted for round %d", clientId, roundId);

        if (round_->everyoneConnectedAnswered()) {
            closeAnswersLocked();
        }
    }
    emit changed();
}

void Room::rejectAnswer(int clientId, TCPSocket& socket, int roundId, const char* code) {
    socket.sendLine(Protocol::format(Protocol::kAnswerRejected, {std::to_string(roundId), code}));
    qDebug("[client %d] answer rejected: %s", clientId, code);
}

void Room::startRound() {
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (closed_ || static_cast<int>(players_.size()) < Protocol::kMinPlayers) {
            return;
        }
        if (round_ && round_->state() != Round::State::ImpostorShown) {
            return;
        }

        std::vector<Round::Participant> participants;
        for (const Player& player : players_) {
            participants.push_back({player.clientId, player.name});
        }

        const int roundId = round_ ? round_->id() + 1 : 1;
        round_ = std::make_unique<Round>(roundId, std::move(participants), questionBank_.next(random_), random_);

        for (const Player& player : players_) {
            const char* role = round_->isImpostor(player.clientId) ? Protocol::kRoleImpostor : Protocol::kRoleNormal;
            player.socket->sendLine(Protocol::format(Protocol::kRoundStart,
                                                     {std::to_string(roundId), role, round_->questionFor(player.clientId)}));
        }
        qDebug("Round %d started with %d players", roundId, static_cast<int>(players_.size()));
    }
    emit changed();
}

int Room::earlyStop() {
    int roundId = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (closed_ || !round_ || round_->state() != Round::State::Answering) {
            return 0;
        }
        round_->startCountdown();
        roundId = round_->id();
        qDebug("Round %d: early stop", roundId);
    }
    emit changed();
    return roundId;
}

void Room::closeAnswers(int roundId) {
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (closed_ || !round_ || round_->id() != roundId || !round_->acceptsAnswers()) {
            return;
        }
        closeAnswersLocked();
    }
    emit changed();
}

void Room::closeAnswersLocked() {
    round_->closeAnswers(random_);

    for (const Player& player : players_) {
        if (round_->isParticipant(player.clientId)) {
            player.socket->sendLine(Protocol::format(Protocol::kAnswersClosed, {std::to_string(round_->id())}));
        }
    }
    qDebug("Round %d: answers closed", round_->id());
}

void Room::revealNext() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!round_) {
            return;
        }
        round_->revealNext();
    }
    emit changed();
}

void Room::showImpostor() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!round_) {
            return;
        }
        round_->showImpostor();
    }
    emit changed();
}

void Room::close() {
    {
        std::lock_guard<std::mutex> lock(mutex_);

        closed_ = true;
        for (const Player& player : players_) {
            player.socket->sendLine(Protocol::format(Protocol::kRoomClosed, {"Host ended the room"}));

            player.socket->shutdown();
        }
    }
    emit changed();
}

RoomSnapshot Room::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);

    RoomSnapshot snapshot;
    snapshot.usesHardcodedQuestions = questionBank_.usesHardcodedPairs();
    for (const Player& player : players_) {
        snapshot.playerNames.append(player.name);
    }

    if (!round_) {
        return snapshot;
    }

    snapshot.roundId = round_->id();
    snapshot.roundState = round_->state();

    for (const Round::Participant& participant : round_->participants()) {
        PlayerStatus status = PlayerStatus::NoAnswer;
        if (participant.left) {
            status = PlayerStatus::Left;
        } else if (participant.answered) {
            status = PlayerStatus::Answered;
        } else if (round_->acceptsAnswers()) {
            status = PlayerStatus::Waiting;
        }
        snapshot.playerRows.push_back({participant.name, status});
    }
    for (const Player& player : players_) {
        if (!round_->isParticipant(player.clientId)) {
            snapshot.playerRows.push_back({player.name, PlayerStatus::JoinsNextRound});
        }
    }

    snapshot.revealedAnswers = round_->revealedLines();

    if (round_->state() == Round::State::ImpostorShown) {
        snapshot.impostorName = round_->impostorName();
        snapshot.normalQuestion = QString::fromStdString(round_->questions().normal);
        snapshot.impostorQuestion = QString::fromStdString(round_->questions().impostor);
    }
    return snapshot;
}
