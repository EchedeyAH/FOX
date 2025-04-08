#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::FOXControl)
{
    ui->setupUi(this);
    // Establecer el icono de la ventana
    this->setWindowIcon(QIcon(":/imagenes/logo.png"));

    // Cargar imagen desde recursos
    QPixmap pix(":/imagenes/coche.PNG");
    if (pix.isNull()) {
        qDebug() << "No se pudo cargar la imagen";
    } else {
        QSize labelSize = ui->VEHICULO->size();
        QPixmap scaledPix = pix.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui->VEHICULO->setPixmap(scaledPix);
    }


    // ------------- K APROP ------------------------------------------
    // Crear slider dinámicamente
    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setMinimum(0);
    slider->setMaximum(1000);
    slider->setSingleStep(1);

    // Etiqueta para mostrar el valor
    QLabel *sliderLabel = new QLabel("K_APROP = 0.00");

    // Conexión del valor
    connect(slider, &QSlider::valueChanged, this, [=](int val) {
        double realVal = val / 100.0;
        sliderLabel->setText("K_APROP = " + QString::number(realVal, 'f', 2));
    });

    // Crear etiquetas numéricas debajo del slider (0 a 10)
    QHBoxLayout *labelsLayout = new QHBoxLayout;
    for (int i = 0; i <= 10; ++i) {
        QLabel *label = new QLabel(QString::number(i));
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-size: 10px; color: gray;");
        labelsLayout->addWidget(label);
        labelsLayout->setStretch(i, 1);  // espaciado uniforme
    }

    // Organizar todo verticalmente
    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addWidget(sliderLabel);     // texto dinámico
    vLayout->addWidget(slider);          // el slider
    vLayout->addLayout(labelsLayout);    // etiquetas debajo

    // Asignar el layout al contenedor
    ui->kpropContainer->setLayout(vLayout);


    // --- UMBRAL YAW ---
    QSlider *yawSlider = new QSlider(Qt::Horizontal);
    yawSlider->setMinimum(0);
    yawSlider->setMaximum(100);  // 0.00 - 10.00
    yawSlider->setSingleStep(1);

    QLabel *yawLabel = new QLabel("Umbral Yaw = 0.00");

    connect(yawSlider, &QSlider::valueChanged, this, [=](int val) {
        double realVal = val / 100.0;
        yawLabel->setText("Umbral Yaw = " + QString::number(realVal, 'f', 2));
    });

    QHBoxLayout *yawMarks = new QHBoxLayout;
    for (int i = 0; i <= 10; ++i) {
        double val = i / 10.0;  // genera: 0.0, 0.1, ..., 1.0
        QLabel *label = new QLabel(QString::number(val, 'f', 1));
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-size: 10px; color: gray;");
        yawMarks->addWidget(label);
        yawMarks->setStretch(i, 1);  // espaciado uniforme
    }

    QVBoxLayout *yawLayout = new QVBoxLayout;
    yawLayout->addWidget(yawLabel);
    yawLayout->addWidget(yawSlider);
    yawLayout->addLayout(yawMarks);

    ui->yawContainer->setLayout(yawLayout);


    // --- UMBRAL BETA ---
    QSlider *betaSlider = new QSlider(Qt::Horizontal);
    betaSlider->setMinimum(0);
    betaSlider->setMaximum(1000);  // 0.00 - 10.00
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

    ui->betaContainer->setLayout(betaLayout);









}

MainWindow::~MainWindow()
{
    delete ui;
}

// ACTIVACIÓN DEL MODO PREDETERMINADO, EN EL QUE SE ENVIA EQUITATIVAMENTE ENERGÍA A CADA RUEDA
void MainWindow::on_ESC_toggled(bool checked)
{
    if (checked) {
        ui->ESC->setStyleSheet("background-color: green; color: white;");
        ui->ESC->setText("ESC ON");
    } else {
        ui->ESC->setStyleSheet("background-color: red; color: white;");
        ui->ESC->setText("ESC OFF");
    }
}

 // ACTIVACIÓN O DESACTIVACIÓN DE MOTORES
void MainWindow::on_M1_toggled(bool checked)
{
    if (checked) {
        ui->M1->setStyleSheet("background-color: #00aa00; color: white;"); // verde
    } else {
        ui->M1->setStyleSheet(""); // estilo neutro por defecto
    }
}

void MainWindow::on_M2_toggled(bool checked)
{
    if (checked) {
        ui->M2->setStyleSheet("background-color: #00aa00; color: white;"); // verde
    } else {
        ui->M2->setStyleSheet(""); // estilo neutro por defecto
    }
}

void MainWindow::on_M3_toggled(bool checked)
{
    if (checked) {
        ui->M3->setStyleSheet("background-color: #00aa00; color: white;"); // verde
    } else {
        ui->M3->setStyleSheet(""); // estilo neutro por defecto
    }
}

void MainWindow::on_M4_toggled(bool checked)
{
    if (checked) {
        ui->M4->setStyleSheet("background-color: #00aa00; color: white;"); // verde
    } else {
        ui->M4->setStyleSheet(""); // estilo neutro por defecto
    }
}


// TRANSMISIÓN DEL TIPO DE CONTROLADOR QUE SE QUIERE USAR
void MainWindow::on_TX_clicked()
{
    // Simular el envío de datos
    QString datos = "Mensaje de prueba";
    qDebug() << "🔼 Enviando datos:" << datos;

    // Cambiar color del botón a naranja al hacer click
    ui->TX->setStyleSheet("background-color: orange; color: white;");
    // (Opcional) volver al estilo original después de un tiempo (por ejemplo 300 ms)
    QTimer::singleShot(100, this, [this]() {
        ui->TX->setStyleSheet("");  // Restaurar estilo original
    });
}


