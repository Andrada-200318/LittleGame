#include "LogPanel.h"

#include <QPlainTextEdit>
#include <QPushButton>
#include <QTime>
#include <QVBoxLayout>

std::atomic<LogPanel*> LogPanel::instance_{nullptr};
QtMessageHandler LogPanel::previousHandler_ = nullptr;

LogPanel::LogPanel(QWidget* parent) : QWidget(parent), logView_(new QPlainTextEdit(this)) {
    auto* toggleButton = new QPushButton("Show log", this);
    toggleButton->setCheckable(true);

    logView_->setReadOnly(true);
    logView_->setMaximumBlockCount(1000);
    logView_->setFixedHeight(180);
    logView_->setVisible(false);

    QFont logFont = logView_->font();
    logFont.setPointSize(11);
    logView_->setFont(logFont);

    connect(toggleButton, &QPushButton::toggled, this, [this, toggleButton](bool checked) {
        logView_->setVisible(checked);
        toggleButton->setText(checked ? "Hide log" : "Show log");
    });
    connect(this, &LogPanel::messageLogged, logView_, &QPlainTextEdit::appendPlainText, Qt::QueuedConnection);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(9, 0, 9, 9);
    layout->addWidget(toggleButton, 0, Qt::AlignLeft);
    layout->addWidget(logView_);

    instance_ = this;
    previousHandler_ = qInstallMessageHandler(&LogPanel::handleMessage);
}

LogPanel::~LogPanel() {
    qInstallMessageHandler(previousHandler_);
    instance_ = nullptr;
}

void LogPanel::handleMessage(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    if (LogPanel* panel = instance_.load()) {
        emit panel->messageLogged(QTime::currentTime().toString("hh:mm:ss.zzz  ") + message);
    }
    if (previousHandler_ != nullptr) {
        previousHandler_(type, context, message);
    }
}
