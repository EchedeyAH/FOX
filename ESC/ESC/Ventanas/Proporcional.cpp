#include "Proporcional.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QLabel>

Proporcional::Proporcional(QWidget *parent) : QWidget(parent) {
    setupUI();
}

void Proporcional::setupUI() {
    // ---------- K APROP ----------
    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setMinimum(0);
    slider->setMaximum(1000);
    slider->setSingleStep(1);

    QLabel *sliderLabel = new QLabel("K_APROP = 0.00");

    connect(slider, &QSlider::valueChanged, this, [=](int val) {
        double realVal = val / 100.0;
        sliderLabel->setText("K_APROP = " + QString::number(realVal, 'f', 2));
    });

    QHBoxLayout *labelsLayout = new QHBoxLayout;
    for (int i = 0; i <= 10; ++i) {
        QLabel *label = new QLabel(QString::number(i));
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-size: 10px; color: gray;");
        labelsLayout->addWidget(label);
        labelsLayout->setStretch(i, 1);
    }

    QVBoxLayout *kpropLayout = new QVBoxLayout;
    kpropLayout->addWidget(sliderLabel);
    kpropLayout->addWidget(slider);
    kpropLayout->addLayout(labelsLayout);

    // ---------- YAW ----------
    QSlider *yawSlider = new QSlider(Qt::Horizontal);
    yawSlider->setMinimum(0);
    yawSlider->setMaximum(100);
    yawSlider->setSingleStep(1);

    QLabel *yawLabel = new QLabel("Umbral Yaw = 0.00");

    connect(yawSlider, &QSlider::valueChanged, this, [=](int val) {
        double realVal = val / 100.0;
        yawLabel->setText("Umbral Yaw = " + QString::number(realVal, 'f', 2));
    });

    QHBoxLayout *yawMarks = new QHBoxLayout;
    for (int i = 0; i <= 10; ++i) {
        QLabel *label = new QLabel(QString::number(i / 10.0, 'f', 1));
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-size: 10px; color: gray;");
        yawMarks->addWidget(label);
        yawMarks->setStretch(i, 1);
    }

    QVBoxLayout *yawLayout = new QVBoxLayout;
    yawLayout->addWidget(yawLabel);
    yawLayout->addWidget(yawSlider);
    yawLayout->addLayout(yawMarks);

    // ---------- BETA ----------
    QSlider *betaSlider = new QSlider(Qt::Horizontal);
    betaSlider->setMinimum(0);
    betaSlider->setMaximum(1000);
    betaSlider->setSingleStep(1);

    QLabel *betaLabel = new QLabel("Umbral Beta = 0.00");

    connect(betaSlider, &QSlider::valueChanged, this, [=](int val) {
        double realVal = val / 100.0;
        betaLabel->setText("Umbral Beta = " + QString::number(realVal, 'f', 2));
    });

    QHBoxLayout *betaMarks = new QHBoxLayout;
    for (int i = 0; i <= 10; ++i) {
        QLabel *label = new QLabel(QString::number(i));
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-size: 10px; color: gray;");
        betaMarks->addWidget(label);
        betaMarks->setStretch(i, 1);
    }

    QVBoxLayout *betaLayout = new QVBoxLayout;
    betaLayout->addWidget(betaLabel);
    betaLayout->addWidget(betaSlider);
    betaLayout->addLayout(betaMarks);

    // Layout principal
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(kpropLayout);
    mainLayout->addLayout(yawLayout);
    mainLayout->addLayout(betaLayout);

    setLayout(mainLayout);
}
