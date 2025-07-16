#include "player.h"
#include "ui_player.h"

Player::Player(QTcpSocket *s, QString id, QString devid, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Player)
{
    ui->setupUi(this);

    this->socket = s;
    this->appid = id;
    this->deviceid = devid;

    this->setWindowTitle("智能家居系统");

    flag =1;
    start_flag = 0;//没有开始
    suspend_flag = 0;//没有暂停

    QFont f("微软雅黑", 14);
    ui->playButton->setFont(f);
    ui->playButton->setText(">>");

    connect(socket, &QTcpSocket::connected, [this]()
    {
        QMessageBox::information(this, "连接提示", "连接服务器成功");
    });

    connect(socket, &QTcpSocket::disconnected, [this]()
    {
        QMessageBox::warning(this, "连接提升", "网络异常断开");
    });

    connect(socket, &QTcpSocket::readyRead, this, &Player::server_reply_slot);

    //创建两秒定时器上报数据
    timer = new QTimer();
    timer->start(2000);
    connect(timer, &QTimer::timeout, this, &Player::timeout_slot);
}

Player::~Player()
{
    delete ui;
}

void Player::server_reply_slot()//解析服务器返回的json，调用相应函数
{
    QByteArray recvData;

    player_recv_data(recvData);

    QJsonObject obj = QJsonDocument::fromJson(recvData).object();

    QString cmd = obj.value("cmd").toString();



    if(cmd == "info")
    {
        player_update_info(obj);

        if (flag)
        {
            player_get_music();
            flag = 0;
        }
    }
    else if(cmd == "upload_music")
    {
        player_update_music(obj);
    }
    else if(cmd == "app_start_reply")
    {
        player_start_reply(obj);
    }
    else if(cmd == "app_suspend_reply")
    {
        player_suspend_reply(obj);
    }
    else if(cmd == "app_continue_reply")
    {
        player_continue_reply(obj);
    }
    else if(cmd == "app_next_reply")
    {
        player_next_reply(obj);
    }
    else if(cmd == "app_prior_reply")
    {
        player_prior_reply(obj);
    }
    else if(cmd == "app_voice_up_reply" || cmd == "app_voice_down_reply")
    {
        player_voice_reply(obj);
    }
    else if(cmd == "app_circle_reply" || cmd == "app_sequence_reply")
    {
        player_mode_reply(obj);
    }
}

void Player::player_mode_reply(QJsonObject &obj)
{
    QString result = obj.value("result").toString();

    if(result == "offline")
    {
        ui->circleButton->setChecked(false);
        ui->sequenceButton->setChecked(true);

        QMessageBox::warning(this, "播放提示", "音箱离线，播放失败");
    }
}

void Player::player_voice_reply(QJsonObject &obj)
{
    QString result = obj.value("result").toString();

    if(result == "offline")
    {
        QMessageBox::warning(this, "播放提示", "音箱离线，播放失败");
    }
    else if(result == "success")
    {
        int volume = obj.value("voice").toInt();
        QFont f("微软雅黑", 14);
        ui->volumeLabel->setFont(f);
        ui->volumeLabel->setText(QString::number(volume));
    }
}

void Player::player_next_reply(QJsonObject &obj)
{
    QString result = obj.value("result").toString();

    if(result == "offline")
    {
        QMessageBox::warning(this, "播放提示", "音箱离线，播放失败");
    }
    else if(result == "success")
    {
        QString music = obj.value("music").toString();
        QFont f("微软雅黑", 14);
        ui->musicLabel->setFont(f);
        ui->musicLabel->setText(music);
    }
}

void Player::player_prior_reply(QJsonObject &obj)
{
    QString result = obj.value("result").toString();

    if(result == "offline")
    {
        QMessageBox::warning(this, "播放提示", "音箱离线，播放失败");
    }
    else if(result == "success")
    {
        QString music = obj.value("music").toString();
        QFont f("微软雅黑", 14);
        ui->musicLabel->setFont(f);
        ui->musicLabel->setText(music);
    }
}

void Player::player_continue_reply(QJsonObject &obj)//解析音箱针对按继续键后的回复
{
    QString result = obj.value("result").toString();

    if(result == "offline")
    {
        QMessageBox::warning(this, "播放提示", "音箱离线，播放失败");
    }
    else if(result == "success")
    {
        suspend_flag = 0;
        QFont f("微软雅黑", 14);
        ui->playButton->setFont(f);
        ui->playButton->setText("||");
    }
}

void Player::player_suspend_reply(QJsonObject &obj)//解析音箱针对按暂停键后的回复
{
    QString result = obj.value("result").toString();

    if(result == "offline")
    {
        QMessageBox::warning(this, "播放提示", "音箱离线，播放失败");
    }
    else if(result == "success")
    {
        suspend_flag = 1;
        QFont f("微软雅黑", 14);
        ui->playButton->setFont(f);
        ui->playButton->setText(">>");
    }
}

void Player::player_start_reply(QJsonObject &obj)//解析音箱针对按开始键后的回复
{
    QString result = obj.value("result").toString();

    if(result == "offline")
    {
        QMessageBox::warning(this, "播放提示", "音箱离线，播放失败");
    }
    else if(result == "failure")
    {
        QMessageBox::warning(this, "播放提示", "音箱启动失败");
    }
    else if(result == "success")
    {
        start_flag = 1;
        QFont f("微软雅黑", 14);
        ui->playButton->setFont(f);
        ui->playButton->setText("||");
    }
}

void Player::player_get_music()
{
    QJsonObject obj;
    obj.insert("cmd","app_get_music");

    player_send_data(obj);
}

void Player::player_update_music(QJsonObject &obj)
{
    QJsonArray arr = obj.value("music").toArray();

    QFont f("黑体", 14);
    ui->textEdit->setFont(f);

    QString result;
    for (int i = 0; i < arr.size(); i++)
    {
        result.append(arr.at(i).toString());
        result.append('\n');
    }

    ui->textEdit->setText(result);
}

void Player::timeout_slot()
{
    QJsonObject obj;
    obj.insert("cmd","app_info");
    obj.insert("appid",appid);
    obj.insert("deviceid", deviceid);

    player_send_data(obj);
}

void Player::player_send_data(QJsonObject &obj)
{
    QByteArray sendData;
    QByteArray ba = QJsonDocument(obj).toJson();

    int size = ba.size();
    sendData.insert(0, (const char *)&size, 4);
    sendData.append(ba);

    socket->write(sendData);
}

void Player::player_update_info(QJsonObject &obj)
{
    QString cur_music = obj.value("cur_music").toString();
    QString dev_id = obj.value("deviceid").toString();
    QString status = obj.value("status").toString();

    int volume = obj.value("volume").toInt();
    int mode = obj.value("mode").toInt();

    QFont f("微软雅黑", 14);
    ui->devidLabel->setFont(f);
    ui->devidLabel->setText(dev_id);

    ui->musicLabel->setFont(f);
    ui->musicLabel->setText(cur_music);

    ui->volumeLabel->setFont(f);
    ui->volumeLabel->setText(QString::number(volume));

    if(mode == SEQUENCE)
    {
        ui->sequenceButton->setChecked(true);
    }
    else if(mode == CIRCLE)
    {
        ui->circleButton->setChecked(true);
    }

    ui->playButton->setFont(f);
    if(status == "start")//开始播放
    {
        ui->playButton->setText("||");
        start_flag = 1;
        suspend_flag = 0;
    }
    else if (status == "stop")//停止播放
    {
        ui->playButton->setText(">>");
        start_flag = 0;
        suspend_flag = 0;
    }
    else if (status == "suspend")//暂停播放
    {
        ui->playButton->setText(">>");
        start_flag = 1;
        suspend_flag = 1;
    }
}

void Player::player_recv_data(QByteArray &ba)
{
    char buf[1024] = {0};
    qint64 size = 0;

    while(true)
    {
        size += socket->read(buf + size, sizeof(int) - size);
        if (size >= sizeof(int))
            break;
    }

    int len = *(int *)buf;
    size = 0;
    memset(buf, 0, sizeof(buf));

    while(true)
    {
        size += socket->read(buf + size, len - size);
        if (size >= len)
            break;
    }

    ba.append(buf);
    qDebug() << "收到服务器" << len << "字节：" << ba;
}

void Player::on_playButton_clicked()
{
    if(start_flag == 0)//未开始播放
    {
        QJsonObject obj;
        obj.insert("cmd","app_start");//开始播放

        player_send_data(obj);
    }
    else if(start_flag == 1 && suspend_flag == 1)//开始后暂停
    {
        QJsonObject obj;
        obj.insert("cmd","app_continue");//继续播放

        player_send_data(obj);
    }
    else if(start_flag == 1 && suspend_flag == 0)//正在播放
    {
        QJsonObject obj;
        obj.insert("cmd","app_suspend");//继续播放

        player_send_data(obj);
    }
}

void Player::on_priorButton_clicked()
{
    QJsonObject obj;
    obj.insert("cmd","app_prior");


    player_send_data(obj);
}

void Player::on_nextButton_clicked()
{
    QJsonObject obj;
    obj.insert("cmd","app_next");


    player_send_data(obj);
}

void Player::on_upButton_clicked()
{
    QJsonObject obj;
    obj.insert("cmd","app_voice_up");


    player_send_data(obj);
}

void Player::on_downButton_clicked()
{
    QJsonObject obj;
    obj.insert("cmd","app_voice_down");


    player_send_data(obj);
}

void Player::on_circleButton_clicked()
{
    QJsonObject obj;
    obj.insert("cmd","app_circle");


    player_send_data(obj);
    ui->circleButton->setChecked(true);
}

void Player::on_sequenceButton_clicked()
{
    QJsonObject obj;
    obj.insert("cmd","app_sequence");


    player_send_data(obj);
    ui->sequenceButton->setChecked(true);
}

void Player::closeEvent(QCloseEvent *e)
{
    QJsonObject obj;
    obj.insert("cmd","app_offline");


    player_send_data(obj);

    e->accept();
}
