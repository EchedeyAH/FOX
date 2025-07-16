#include "GANANCIAS.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QLabel>

GANANCIAS::GANANCIAS(QWidget *parent) : QWidget(parent) {
    setupUI();
}

void GANANCIAS::setupUI() {
    // G1 y G2
    QSlider *g1 = new QSlider(Qt::Horizontal);
    QSlider *g2 = new QSlider(Qt::Horizontal);
    g1->setRange(0, 1000);
    g2->setRange(0, 1000);

    QLabel *l1 = new QLabel("Ganancia G1 = 0.00");
    QLabel *l2 = new QLabel("Ganancia G2 = 0.00");

    connect(g1, &QSlider::valueChanged, this, [=](int val) {
        l1->setText("Ganancia G1 = " + QString::number(val / 100.0, 'f', 2));
    });
    connect(g2, &QSlider::valueChanged, this, [=](int val) {
        l2->setText("Ganancia G2 = " + QString::number(val / 100.0, 'f', 2));
    });

    auto layout = new QVBoxLayout;
    layout->addWidget(l1);
    layout->addWidget(g1);
    layout->addWidget(l2);
    layout->addWidget(g2);
    setLayout(layout);
}

