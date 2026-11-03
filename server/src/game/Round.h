#pragma once

#include <QString>
#include <QStringList>

#include <cstddef>
#include <random>
#include <string>
#include <vector>

#include "QuestionBank.h"


class Round {
public:
    enum class State { Answering, Countdown, Revealing, AllRevealed, ImpostorShown };

    struct Participant {
        int clientId;
        QString name;
        bool left = false;
        bool answered = false;
        QString answer;
    };

    Round(int id, std::vector<Participant> participants, QuestionPair questions, std::mt19937& random);

    int id() const;
    State state() const;
    const std::vector<Participant>& participants() const;

    bool acceptsAnswers() const;

    bool isParticipant(int clientId) const;
    bool hasAnswered(int clientId) const;
    bool isImpostor(int clientId) const;
    const std::string& questionFor(int clientId) const;

    void recordAnswer(int clientId, const QString& answer);
    void markLeft(int clientId);

    bool everyoneConnectedAnswered() const;

    void startCountdown();
    void closeAnswers(std::mt19937& random);
    void revealNext();
    void showImpostor();

    QStringList revealedLines() const;
    QString impostorName() const;
    const QuestionPair& questions() const;

private:
    Participant* find(int clientId);
    const Participant* find(int clientId) const;

    int id_;
    State state_ = State::Answering;
    std::vector<Participant> participants_;
    QuestionPair questions_;
    int impostorClientId_ = 0;
    std::vector<std::size_t> revealOrder_;
    std::size_t revealedCount_ = 0;
};
