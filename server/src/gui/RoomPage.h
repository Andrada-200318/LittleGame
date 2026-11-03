#pragma once

#include <QWidget>

class QLabel;
class QListWidget;
class QPushButton;
class RoomHeader;
struct RoomSnapshot;

class RoomPage : public QWidget {
    Q_OBJECT

public:
    explicit RoomPage(QWidget* parent = nullptr);

    RoomHeader* header() const;
    void showSnapshot(const RoomSnapshot& snapshot);

signals:
    void startRoundRequested();
    void endRoomRequested();

private:
    RoomHeader* header_;
    QListWidget* playerList_;
    QLabel* playerCountLabel_;
    QLabel* questionsWarningLabel_;
    QPushButton* startRoundButton_;
};
