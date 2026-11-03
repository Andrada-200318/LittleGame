#pragma once

#include <QStringList>
#include <QWidget>

class QLabel;

class RoomHeader : public QWidget {
    Q_OBJECT

public:
    explicit RoomHeader(int pointSize, QWidget* parent = nullptr);

    void setRoomInfo(const QStringList& addresses, int port, const QString& password);

private:
    QLabel* addressesLabel_;
    QLabel* portLabel_;
    QLabel* passwordLabel_;
};
