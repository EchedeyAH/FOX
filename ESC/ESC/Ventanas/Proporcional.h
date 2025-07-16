#ifndef PROPORCIONAL_H
#define PROPORCIONAL_H

#include <QWidget>

class QSlider;
class QLabel;

class Proporcional : public QWidget {
    Q_OBJECT

public:
    explicit Proporcional(QWidget *parent = nullptr);

private:
    void setupUI();
};

#endif // PROPORCIONAL_H
