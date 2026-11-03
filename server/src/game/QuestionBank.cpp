#include "QuestionBank.h"

#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QtDebug>

#include <algorithm>

namespace {

const std::vector<QuestionPair> kHardcodedPairs = {
    {"What's your favorite food?", "What's your favorite dessert?"},
    {"How many hours do you sleep?", "How many hours a day are you on your phone?"},
    {"Which country would you most like to visit?", "Which country would you never want to visit?"},
};

}

QuestionBank::QuestionBank(const QString& filePath) {
    loadFile(filePath);

    if (pairs_.empty()) {
        pairs_ = kHardcodedPairs;
        usesHardcodedPairs_ = true;
        qDebug("Using the hardcoded question pairs");
    }

    for (std::size_t i = 0; i < pairs_.size(); ++i) {
        bag_.push_back(i);
    }
    bagPosition_ = bag_.size();
}

void QuestionBank::loadFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug("Could not open %s", qUtf8Printable(filePath));
        return;
    }

    QTextStream stream(&file);
    int lineNumber = 0;
    while (!stream.atEnd()) {
        ++lineNumber;
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        const QStringList sides = line.split('|');
        if (sides.size() != 2 || sides[0].trimmed().isEmpty() || sides[1].trimmed().isEmpty()) {
            qDebug("questions.txt: skipped line %d", lineNumber);
            continue;
        }
        pairs_.push_back({sides[0].trimmed().toStdString(), sides[1].trimmed().toStdString()});
    }
    qDebug("questions.txt: loaded %d pairs", static_cast<int>(pairs_.size()));
}

QuestionPair QuestionBank::next(std::mt19937& random) {
    if (bagPosition_ == bag_.size()) {
        std::shuffle(bag_.begin(), bag_.end(), random);
        bagPosition_ = 0;
    }
    return pairs_[bag_[bagPosition_++]];
}

bool QuestionBank::usesHardcodedPairs() const {
    return usesHardcodedPairs_;
}
