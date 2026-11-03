#include "Round.h"

#include <algorithm>
#include <utility>

Round::Round(int id, std::vector<Participant> participants, QuestionPair questions, std::mt19937& random)
    : id_(id), participants_(std::move(participants)), questions_(std::move(questions)) {
    std::uniform_int_distribution<std::size_t> pick(0, participants_.size() - 1);
    impostorClientId_ = participants_[pick(random)].clientId;
}

int Round::id() const {
    return id_;
}

Round::State Round::state() const {
    return state_;
}

const std::vector<Round::Participant>& Round::participants() const {
    return participants_;
}

bool Round::acceptsAnswers() const {
    return state_ == State::Answering || state_ == State::Countdown;
}

Round::Participant* Round::find(int clientId) {
    for (Participant& participant : participants_) {
        if (participant.clientId == clientId) {
            return &participant;
        }
    }
    return nullptr;
}

const Round::Participant* Round::find(int clientId) const {
    for (const Participant& participant : participants_) {
        if (participant.clientId == clientId) {
            return &participant;
        }
    }
    return nullptr;
}

bool Round::isParticipant(int clientId) const {
    return find(clientId) != nullptr;
}

bool Round::hasAnswered(int clientId) const {
    const Participant* participant = find(clientId);
    return participant != nullptr && participant->answered;
}

bool Round::isImpostor(int clientId) const {
    return clientId == impostorClientId_;
}

const std::string& Round::questionFor(int clientId) const {
    return isImpostor(clientId) ? questions_.impostor : questions_.normal;
}

void Round::recordAnswer(int clientId, const QString& answer) {
    if (Participant* participant = find(clientId)) {
        participant->answered = true;
        participant->answer = answer;
    }
}

void Round::markLeft(int clientId) {
    if (Participant* participant = find(clientId)) {
        participant->left = true;
    }
}

bool Round::everyoneConnectedAnswered() const {
    for (const Participant& participant : participants_) {
        if (!participant.left && !participant.answered) {
            return false;
        }
    }
    return true;
}

void Round::startCountdown() {
    if (state_ == State::Answering) {
        state_ = State::Countdown;
    }
}

void Round::closeAnswers(std::mt19937& random) {
    if (!acceptsAnswers()) {
        return;
    }

    revealOrder_.clear();
    for (std::size_t i = 0; i < participants_.size(); ++i) {
        revealOrder_.push_back(i);
    }
    std::shuffle(revealOrder_.begin(), revealOrder_.end(), random);

    state_ = State::Revealing;
}

void Round::revealNext() {
    if (state_ != State::Revealing) {
        return;
    }
    ++revealedCount_;
    if (revealedCount_ == revealOrder_.size()) {
        state_ = State::AllRevealed;
    }
}

void Round::showImpostor() {
    if (state_ == State::AllRevealed) {
        state_ = State::ImpostorShown;
    }
}

QStringList Round::revealedLines() const {
    QStringList lines;
    for (std::size_t i = 0; i < revealedCount_; ++i) {
        const Participant& participant = participants_[revealOrder_[i]];
        QString line = QString("%1 — %2").arg(participant.name,
                                              participant.answered ? participant.answer : QString("(no answer)"));
        if (participant.left) {
            line += " (left)";
        }
        lines.append(line);
    }
    return lines;
}

QString Round::impostorName() const {
    const Participant* impostor = find(impostorClientId_);
    return impostor != nullptr ? impostor->name : QString();
}

const QuestionPair& Round::questions() const {
    return questions_;
}
