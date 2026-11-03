#pragma once

#include <QString>

#include <cstddef>
#include <random>
#include <string>
#include <vector>

struct QuestionPair {
    std::string normal;
    std::string impostor;
};

class QuestionBank {
public:
    explicit QuestionBank(const QString& filePath);

    QuestionPair next(std::mt19937& random);

    bool usesHardcodedPairs() const;

private:
    void loadFile(const QString& filePath);

    std::vector<QuestionPair> pairs_;
    std::vector<std::size_t> bag_;
    std::size_t bagPosition_ = 0;
    bool usesHardcodedPairs_ = false;
};
