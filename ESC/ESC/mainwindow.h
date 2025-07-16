#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QWidget>
#include <QMap>  // Necesario para motorStates

QT_BEGIN_NAMESPACE
namespace Ui {
class FOXControl;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_ESC_toggled(bool checked);
    void on_TX_clicked();

private:
    QWidget *vehicleOverlay;
    QLabel *carImage;
    QPushButton *m1;
    QPushButton *m2;
    QPushButton *m3;
    QPushButton *m4;
    QLabel *advertenciaIcono;
    void actualizarEstadoESC(bool activo);

    QMap<QPushButton*, bool> motorStates;
    void toggleMotorButton(QPushButton *btn, const QString &normalStyle, const QString &activeStyle);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::FOXControl *ui;
};

#endif // MAINWINDOW_H
