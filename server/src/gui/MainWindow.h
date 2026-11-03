#pragma once

#include <QMainWindow>

#include <memory>

class GameServer;
class QStackedWidget;
class QTimer;
class Room;
class RoomPage;
class RoundPage;
class TitlePage;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void startRoom(const QString& password);
    void endRoomClicked();
    bool confirmEndRoom();
    void endRoom();
    void refreshFromRoom();
    void toggleFullScreen();

    void startRound();
    void earlyStop();
    void showNextAnswer();
    void showImpostor();

    void countdownTick();
    void countdownFinished();
    void stopCountdown();

    QStackedWidget* pages_;
    TitlePage* titlePage_;
    RoomPage* roomPage_;
    RoundPage* roundPage_;


    std::unique_ptr<Room> room_;
    std::unique_ptr<GameServer> server_;

    QTimer* countdownTickTimer_;
    QTimer* countdownCloseTimer_;
    int countdownRoundId_ = 0;
    int countdownSecondsLeft_ = 0;
};
