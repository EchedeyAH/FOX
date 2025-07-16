#ifndef ELIPSOIDE_H
#define ELIPSOIDE_H

#include <QWidget>

class ELIPSOIDE : public QWidget {
    Q_OBJECT
public:
    explicit ELIPSOIDE(QWidget *parent = nullptr);

private:
    void setupUI();
};

#endif // ELIPSOIDE_H
