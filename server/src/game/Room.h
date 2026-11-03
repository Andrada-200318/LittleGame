#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <vector>

#include "QuestionBank.h"
#include "Round.h"

class TCPSocket;

enum class PlayerStatus { Waiting, Answered, NoAnswer, Left, JoinsNextRound };

struct PlayerRow {
    QString name;
    PlayerStatus status;
};

struct RoomSnapshot {
    QStringList playerNames;
    int roundId = 0;
    Round::State roundState = Round::State::Answering;
    std::vector<PlayerRow> playerRows;
    QStringList revealedAnswers;
    QString impostorName;
    QString normalQuestion;
    QString impostorQuestion;
    bool usesHardcodedQuestions = false;
};


class Room : public QObject {
    Q_OBJECT

public:
    Room(const std::string& password, const QString& questionsFilePath);

    bool join(int clientId, const std::string& password, const std::string& username,
              const std::shared_ptr<TCPSocket>& socket);

    void removePlayer(int clientId);

    void submitAnswer(int clientId, int roundId, const std::string& text);

    void startRound();

    int earlyStop();

    void closeAnswers(int roundId);

    void revealNext();
    void showImpostor();

    void close();

    RoomSnapshot snapshot() const;

signals:

    void changed();

private:
    struct Player {
        int clientId;
        QString name;
        std::shared_ptr<TCPSocket> socket;
    };

    void refuseJoin(int clientId, TCPSocket& socket, const char* code, const char* message);
    void rejectAnswer(int clientId, TCPSocket& socket, int roundId, const char* code);
    void closeAnswersLocked();

    mutable std::mutex mutex_;
    const std::string password_;
    std::vector<Player> players_;
    bool closed_ = false;

    std::unique_ptr<Round> round_;
    QuestionBank questionBank_;
    std::mt19937 random_;
};
