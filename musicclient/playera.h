#ifndef PLAYER_H
#define PLAYER_H

#include <QWidget>
#include <QTcpSocket>


namespace Ui {
class Player;
}

class Player : public QWidget
{
    Q_OBJECT

public:
    explicit Player(QTcpSocket *, QString, QWidget *parent = 0);
    ~Player();

private:
    Ui::Player *ui;
    QTcpSocket *socket;
    QString appid;
};

#endif // PLAYER_H
