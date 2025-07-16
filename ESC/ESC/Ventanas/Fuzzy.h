#ifndef FUZZY_H
#define FUZZY_H

#include <QWidget>

class Fuzzy : public QWidget {
    Q_OBJECT

public:
    explicit Fuzzy(QWidget *parent = nullptr);

private:
    void setupUI();
};

#endif // FUZZY_H
