#ifndef MPC_H
#define MPC_H

#include <QWidget>

class MPC : public QWidget {
    Q_OBJECT
public:
    explicit MPC(QWidget *parent = nullptr);

private:
    void setupUI();
};

#endif // MPC_H
