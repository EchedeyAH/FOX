#ifndef PI_H
#define PI_H

#include <QWidget>

class PI : public QWidget {
    Q_OBJECT

public:
    explicit PI(QWidget *parent = nullptr);

private:
    void setupUI();
};

#endif // PI_H
