#include "playera.h"
#include "ui_player.h"

Player::Player(QTcpSocket *s, QString id, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Player)
{
    ui->setupUi(this);

    this->socket = s;
    this->appid = id;
}

Player::~Player()
{
    delete ui;
}
