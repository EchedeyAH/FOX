#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include "Ventanas/Elipsoide.h"
#include "Ventanas/Fuzzy.h"
#include "Ventanas/Ganancias.h"
#include "Ventanas/MPC.h"
#include "Ventanas/PI.h"
#include "Ventanas/Proporcional.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::FOXControl)
{
    ui->setupUi(this);

    ui->ESC->setCheckable(true);
    // 🔧 CONECTAR BOTÓN ESC AL SLOT
    connect(ui->ESC, &QPushButton::toggled, this, &MainWindow::on_ESC_toggled);

    this->setWindowIcon(QIcon(":/imagenes/logo.png"));

    // ================= [AJUSTE DE ALTURA DE VISTAS] =================
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(ui->centralwidget->layout());
    if (layout) {
        layout->setStretch(0, 4);  // 40%
        layout->setStretch(1, 6);  // 60%
    }

    // ================= [VEHÍCULO + MOTORES] =================
    vehicleOverlay = new QWidget(this);
    vehicleOverlay->setMinimumSize(100, 100);
    vehicleOverlay->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    carImage = new QLabel(vehicleOverlay);
    carImage->setScaledContents(true);

    m1 = new QPushButton("M1", vehicleOverlay);
    m2 = new QPushButton("M2", vehicleOverlay);
    m3 = new QPushButton("M3", vehicleOverlay);
    m4 = new QPushButton("M4", vehicleOverlay);

    QSize btnSize(40, 25);
    m1->setFixedSize(btnSize);
    m2->setFixedSize(btnSize);
    m3->setFixedSize(btnSize);
    m4->setFixedSize(btnSize);

    QVBoxLayout *wrapper = new QVBoxLayout;
    wrapper->addWidget(vehicleOverlay);
    wrapper->setContentsMargins(0, 0, 0, 0);

    if (QLayout *old = ui->VehiculoWidget->layout()) delete old;
    ui->VehiculoWidget->setLayout(wrapper);

    // ================= [ESTILOS Y FUNCIONALIDAD DE BOTONES] =================
    QString normalStyle = "background-color: lightgray; font-weight: bold;";
    QString activeStyle = "background-color: green; color: white; font-weight: bold;";

    m1->setStyleSheet(normalStyle);
    m2->setStyleSheet(normalStyle);
    m3->setStyleSheet(normalStyle);
    m4->setStyleSheet(normalStyle);

    motorStates[m1] = false;
    motorStates[m2] = false;
    motorStates[m3] = false;
    motorStates[m4] = false;

    connect(m1, &QPushButton::clicked, this, [=]() { toggleMotorButton(m1, normalStyle, activeStyle); });
    connect(m2, &QPushButton::clicked, this, [=]() { toggleMotorButton(m2, normalStyle, activeStyle); });
    connect(m3, &QPushButton::clicked, this, [=]() { toggleMotorButton(m3, normalStyle, activeStyle); });
    connect(m4, &QPushButton::clicked, this, [=]() { toggleMotorButton(m4, normalStyle, activeStyle); });

    // ================= [VENTANAS EN TABS] =================
    ui->tabWidget->addTab(new MPC(this), "MPC");
    ui->tabWidget->addTab(new ELIPSOIDE(this), "ELIPSOIDE");
    ui->tabWidget->addTab(new PI(this), "PI");
    ui->tabWidget->addTab(new GANANCIAS(this), "GANANCIAS");
    ui->tabWidget->addTab(new Fuzzy(this), "FUZZY");
    ui->tabWidget->addTab(new Proporcional(this), "PROPORCIONAL");

    advertenciaIcono = new QLabel(ui->tabWidget);
    advertenciaIcono->setAlignment(Qt::AlignCenter);
    advertenciaIcono->setStyleSheet("background: transparent;");
    advertenciaIcono->setVisible(true);
    advertenciaIcono->raise();

    QPixmap pix(":/imagenes/advertencia.png");
    if (pix.isNull()) {
        qDebug() << "❌ No se pudo cargar advertencia.png";
    } else {
        advertenciaIcono->setPixmap(pix.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    QSize size = vehicleOverlay->size();
    int w = size.width();
    int h = size.height();

    int imageW = w * 0.6;
    int imageH = h * 0.6;
    int imageX = (w - imageW) / 2;
    int imageY = (h - imageH) / 2;

    carImage->setGeometry(imageX, imageY, imageW, imageH);
    QPixmap pix(":/imagenes/coche.PNG");
    if (!pix.isNull()) {
        carImage->setPixmap(pix.scaled(imageW, imageH, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    int btnW = imageW * 0.2;
    int btnH = imageH * 0.12;
    QSize scaledBtnSize(btnW, btnH);

    m1->setFixedSize(scaledBtnSize);
    m2->setFixedSize(scaledBtnSize);
    m3->setFixedSize(scaledBtnSize);
    m4->setFixedSize(scaledBtnSize);

    m1->move(imageX + imageW * 0.15 - btnW / 2, imageY - btnH);
    m2->move(imageX + imageW * 0.8 - btnW / 2, imageY - btnH);
    m3->move(imageX + imageW * 0.15 - btnW / 2, imageY + imageH);
    m4->move(imageX + imageW * 0.8 - btnW / 2, imageY + imageH);

    if (advertenciaIcono) {
        int areaW = ui->tabWidget->width();
        int areaH = ui->tabWidget->height();
        int imgW = areaW;
        int imgH = areaH;

        QPixmap pix(":/imagenes/advertencia.png");
        if (!pix.isNull()) {
            advertenciaIcono->setPixmap(pix.scaled(imgW, imgH, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }

        advertenciaIcono->setGeometry(
            (areaW - imgW) / 2,
            (areaH - imgH) / 2,
            imgW,
            imgH
            );
    }

}

void MainWindow::toggleMotorButton(QPushButton *btn, const QString &normalStyle, const QString &activeStyle)
{
    bool isActive = motorStates[btn];
    motorStates[btn] = !isActive;

    if (motorStates[btn]) {
        btn->setStyleSheet(activeStyle);
    } else {
        btn->setStyleSheet(normalStyle);
    }
}

void MainWindow::on_ESC_toggled(bool checked)
{
    if (checked) {
        ui->ESC->setStyleSheet("background-color: green; color: white;");
        ui->ESC->setText("ESC ON");
    } else {
        ui->ESC->setStyleSheet("background-color: red; color: white;");
        ui->ESC->setText("ESC OFF");
    }
    actualizarEstadoESC(checked);
}



void MainWindow::on_TX_clicked()
{
    QString datos = "Mensaje de prueba";
    qDebug() << "⬆️ Enviando datos:" << datos;

    ui->TX->setStyleSheet("background-color: orange; color: white;");
    QTimer::singleShot(100, this, [this]() {
        ui->TX->setStyleSheet("");
    });
}

void MainWindow::actualizarEstadoESC(bool activo)
{
    advertenciaIcono->setVisible(!activo);

    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        ui->tabWidget->setTabEnabled(i, activo);
    }
}



