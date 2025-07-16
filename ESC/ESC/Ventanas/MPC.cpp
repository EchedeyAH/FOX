#include "MPC.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QLabel>

MPC::MPC(QWidget *parent) : QWidget(parent) {
    setupUI();
}

void MPC::setupUI() {
    // K_MPC
    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setRange(0, 1000);
    QLabel *label = new QLabel("K_MPC = 0.00");
    connect(slider, &QSlider::valueChanged, this, [=](int val) {
        label->setText("K_MPC = " + QString::number(val / 100.0, 'f', 2));
    });

    QHBoxLayout *marks = new QHBoxLayout;
    for (int i = 0; i <= 10; ++i) {
        QLabel *l = new QLabel(QString::number(i));
        l->setAlignment(Qt::AlignCenter);
        marks->addWidget(l);
        marks->setStretch(i, 1);
    }

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(slider);
    layout->addLayout(marks);
    setLayout(layout);
}

