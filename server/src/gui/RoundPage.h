#pragma once

#include <QWidget>

class QLabel;
class QListWidget;
class QPushButton;
class RoomHeader;
struct RoomSnapshot;

class RoundPage : public QWidget {
    Q_OBJECT

public:
    explicit RoundPage(QWidget* parent = nullptr);

    RoomHeader* header() const;
    void showSnapshot(const RoomSnapshot& snapshot);

    void showCountdown(int secondsLeft);

signals:
    void earlyStopRequested();
    void showNextAnswerRequested();
    void showImpostorRequested();
    void newRoundRequested();
    void endRoomRequested();

private:
    RoomHeader* header_;
    QLabel* roundLabel_;
    QListWidget* playerList_;
    QListWidget* answerList_;
    QLabel* impostorLabel_;
    QPushButton* earlyStopButton_;
    QPushButton* showNextAnswerButton_;
    QPushButton* showImpostorButton_;
    QPushButton* newRoundButton_;
};
