#include "RoomHeader.h"

#include <QFont>
#include <QLabel>
#include <QVBoxLayout>

RoomHeader::RoomHeader(int pointSize, QWidget* parent)
    : QWidget(parent),
      addressesLabel_(new QLabel(this)),
      portLabel_(new QLabel(this)),
      passwordLabel_(new QLabel(this)) {
    auto* layout = new QVBoxLayout(this);

    QFont headerFont = font();
    headerFont.setPointSize(pointSize);

    for (QLabel* label : {addressesLabel_, portLabel_, passwordLabel_}) {
        label->setFont(headerFont);
        label->setTextFormat(Qt::PlainText);
        layout->addWidget(label);
    }
}

void RoomHeader::setRoomInfo(const QStringList& addresses, int port, const QString& password) {
    addressesLabel_->setText(addresses.join('\n'));
    portLabel_->setText(QString("Port: %1").arg(port));
    passwordLabel_->setText(QString("Password: %1").arg(password));
}
