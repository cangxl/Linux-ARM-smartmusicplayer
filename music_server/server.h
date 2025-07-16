#ifndef SERVER_H
#define SERVER_H

#include <string.h>
#include <event.h>
#include <event2/listener.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "database.h"
#include <stdlib.h>
#include <list>
#include <time.h>
#include <jsoncpp/json/json.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include "player.h"


#define IP  "172.28.130.152"
#define PORT 8000

#define SEQUENCE  1
#define CIRCLE    2





class Server
{
private:
	struct event_base *m_base;       //事件集合
	DataBase *m_database;            //数据库对象
	Player *m_p;			 //调用player对象
public:
	Server();                 //构造函数
	~Server();			  //析构函数
	void listen(const char *ip, int port);//启动服务器监听
	struct event_base *server_get_base();
	void server_read_data(struct bufferevent *,char *);
	void server_get_music(struct bufferevent *, std::string);
	void server_send_data(struct bufferevent *, Json::Value &);
	void server_player_handler(struct bufferevent *, Json::Value &);
	void server_start_timer();
	void server_app_register(struct bufferevent *, Json::Value &);
	void server_app_login(struct bufferevent *, Json::Value &);
	void server_app_bind(struct bufferevent *, Json::Value &);
	void server_client_offline(struct bufferevent *);
	void server_app_offline(struct bufferevent *);
	void debug(const char *);

	static void listener_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int socklen, void *);
	static void  read_cb(struct bufferevent *bev, void *ctx);
	static void event_cb(struct bufferevent *, short, void *);
	static void timeout_cb(evutil_socket_t, short, void *);

};




#endif
