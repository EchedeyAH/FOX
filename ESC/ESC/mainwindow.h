#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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
    void on_M1_toggled(bool checked);
    void on_M2_toggled(bool checked);
    void on_M3_toggled(bool checked);
    void on_M4_toggled(bool checked);
    void on_TX_clicked();


private:
    Ui::FOXControl *ui;
};
#endif // MAINWINDOW_H
