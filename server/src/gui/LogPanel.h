#pragma once

#include <QString>
#include <QWidget>
#include <QtGlobal>

#include <atomic>

class QPlainTextEdit;


class LogPanel : public QWidget {
    Q_OBJECT

public:
    explicit LogPanel(QWidget* parent = nullptr);
    ~LogPanel() override;

signals:

    void messageLogged(const QString& message);

private:
    static void handleMessage(QtMsgType type, const QMessageLogContext& context, const QString& message);

    static std::atomic<LogPanel*> instance_;
    static QtMessageHandler previousHandler_;

    QPlainTextEdit* logView_;
};
