#include "TitlePage.h"

#include <QFont>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "../game/Protocol.h"

TitlePage::TitlePage(QWidget* parent) : QWidget(parent) {
    auto* title = new QLabel("Little Impostor Game", this);
    title->setAlignment(Qt::AlignCenter);
    QFont titleFont = title->font();
    titleFont.setPointSize(56);
    titleFont.setBold(true);
    title->setFont(titleFont);

    auto* startButton = new QPushButton("Start room", this);
    connect(startButton, &QPushButton::clicked, this, &TitlePage::askForPassword);

    auto* layout = new QVBoxLayout(this);
    layout->addStretch();
    layout->addWidget(title);
    layout->addWidget(startButton, 0, Qt::AlignCenter);
    layout->addStretch();
}

void TitlePage::askForPassword() {
    bool ok = false;
    const QString password = QInputDialog::getText(this, "Start room", "Room password:",
                                                   QLineEdit::Normal, QString(), &ok);
    if (!ok) {
        return;
    }

    const auto characters = password.toUcs4().size();
    if (characters < 1 || characters > Protocol::kMaxPasswordChars || password.contains('|')) {
        QMessageBox::warning(this, "Start room",
                             QString("The password must have 1 to %1 characters and no | character.")
                                 .arg(Protocol::kMaxPasswordChars));
        return;
    }

    emit startRequested(password);
}
