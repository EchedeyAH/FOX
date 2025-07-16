#include "Fuzzy.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QLabel>

Fuzzy::Fuzzy(QWidget *parent) : QWidget(parent) {
    setupUI();
}

void Fuzzy::setupUI() {
    // ------- K_FUZZY -------
    QSlider *kSlider = new QSlider(Qt::Horizontal);
    kSlider->setMinimum(0);
    kSlider->setMaximum(1000);  // 0.00 - 10.00
    kSlider->setSingleStep(1);

    QLabel *kLabel = new QLabel("K_Fuzzy = 0.00");

    connect(kSlider, &QSlider::valueChanged, this, [=](int val) {
        double realVal = val / 100.0;
        kLabel->setText("K_Fuzzy = " + QString::number(realVal, 'f', 2));
    });

    QHBoxLayout *kMarks = new QHBoxLayout;
    for (int i = 0; i <= 10; ++i) {
        QLabel *label = new QLabel(QString::number(i));
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-size: 10px; color: gray;");
        kMarks->addWidget(label);
        kMarks->setStretch(i, 1);
    }

    QVBoxLayout *kLayout = new QVBoxLayout;
    kLayout->addWidget(kLabel);
    kLayout->addWidget(kSlider);
    kLayout->addLayout(kMarks);

    // ------- THETA_FRONT -------
    QSlider *thetaSlider = new QSlider(Qt::Horizontal);
    thetaSlider->setMinimum(0);
    thetaSlider->setMaximum(100);  // 0.00 - 1.00
    thetaSlider->setSingleStep(1);

    QLabel *thetaLabel = new QLabel("Theta_Front = 0.00");

    connect(thetaSlider, &QSlider::valueChanged, this, [=](int val) {
        double realVal = val / 100.0;
        thetaLabel->setText("Theta_Front = " + QString::number(realVal, 'f', 2));
    });

    QHBoxLayout *thetaMarks = new QHBoxLayout;
    for (int i = 0; i <= 10; ++i) {
        QLabel *label = new QLabel(QString::number(i / 10.0, 'f', 1));
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-size: 10px; color: gray;");
        thetaMarks->addWidget(label);
        thetaMarks->setStretch(i, 1);
    }

    QVBoxLayout *thetaLayout = new QVBoxLayout;
    thetaLayout->addWidget(thetaLabel);
    thetaLayout->addWidget(thetaSlider);
    thetaLayout->addLayout(thetaMarks);

    // Layout principal
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(kLayout);
    mainLayout->addLayout(thetaLayout);

    setLayout(mainLayout);
}
