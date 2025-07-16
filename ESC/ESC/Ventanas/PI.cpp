#include "PI.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QLabel>

PI::PI(QWidget *parent) : QWidget(parent) {
    setupUI();
}

void PI::setupUI() {
    // ---------- Kp ----------
    QSlider *kpSlider = new QSlider(Qt::Horizontal);
    kpSlider->setMinimum(0);
    kpSlider->setMaximum(1000);
    kpSlider->setSingleStep(1);

    QLabel *kpLabel = new QLabel("Kp = 0.00");

    connect(kpSlider, &QSlider::valueChanged, this, [=](int val) {
        double realVal = val / 100.0;
        kpLabel->setText("Kp = " + QString::number(realVal, 'f', 2));
    });

    QHBoxLayout *kpMarks = new QHBoxLayout;
    for (int i = 0; i <= 10; ++i) {
        QLabel *label = new QLabel(QString::number(i));
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-size: 10px; color: gray;");
        kpMarks->addWidget(label);
        kpMarks->setStretch(i, 1);
    }

    QVBoxLayout *kpLayout = new QVBoxLayout;
    kpLayout->addWidget(kpLabel);
    kpLayout->addWidget(kpSlider);
    kpLayout->addLayout(kpMarks);

    // ---------- Ki ----------
    QSlider *kiSlider = new QSlider(Qt::Horizontal);
    kiSlider->setMinimum(0);
    kiSlider->setMaximum(1000);
    kiSlider->setSingleStep(1);

    QLabel *kiLabel = new QLabel("Ki = 0.00");

    connect(kiSlider, &QSlider::valueChanged, this, [=](int val) {
        double realVal = val / 100.0;
        kiLabel->setText("Ki = " + QString::number(realVal, 'f', 2));
    });

    QHBoxLayout *kiMarks = new QHBoxLayout;
    for (int i = 0; i <= 10; ++i) {
        QLabel *label = new QLabel(QString::number(i));
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-size: 10px; color: gray;");
        kiMarks->addWidget(label);
        kiMarks->setStretch(i, 1);
    }

    QVBoxLayout *kiLayout = new QVBoxLayout;
    kiLayout->addWidget(kiLabel);
    kiLayout->addWidget(kiSlider);
    kiLayout->addLayout(kiMarks);

    // Layout principal
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(kpLayout);
    mainLayout->addLayout(kiLayout);

    setLayout(mainLayout);
}
