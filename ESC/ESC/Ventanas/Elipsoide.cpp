#include "ELIPSOIDE.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QLabel>

ELIPSOIDE::ELIPSOIDE(QWidget *parent) : QWidget(parent) {
    setupUI();
}

void ELIPSOIDE::setupUI() {
    QSlider *alphaSlider = new QSlider(Qt::Horizontal);
    alphaSlider->setRange(0, 100);

    QLabel *label = new QLabel("Alpha (Escala) = 0.00");
    connect(alphaSlider, &QSlider::valueChanged, this, [=](int val) {
        label->setText("Alpha (Escala) = " + QString::number(val / 100.0, 'f', 2));
    });

    QHBoxLayout *marks = new QHBoxLayout;
    for (int i = 0; i <= 10; ++i) {
        QLabel *l = new QLabel(QString::number(i / 10.0, 'f', 1));
        l->setAlignment(Qt::AlignCenter);
        marks->addWidget(l);
        marks->setStretch(i, 1);
    }

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(alphaSlider);
    layout->addLayout(marks);
    setLayout(layout);
}
