#pragma once

#include <QWidget>

class TitlePage : public QWidget {
    Q_OBJECT

public:
    explicit TitlePage(QWidget* parent = nullptr);

signals:
    void startRequested(const QString& password);

private:
    void askForPassword();
};
