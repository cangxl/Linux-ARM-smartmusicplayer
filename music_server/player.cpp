#include "player.h"
#include "server.h"


Player::Player()
{
	info = new std::list<PlayerInfo>;
}

Player::~Player()
{
	if(info)
		delete info;
}

void Player::player_update_list(struct bufferevent *bev, Json::Value &v, Server *ser)
{
	auto it = info->begin();//链表迭代器
	//遍历链表，如果设备id存在，更新并转发给app；
	for(; it != info->end(); it++)
	{
		if(it->deviceid == v["deviceid"].asString())//找到了对应的设备id
		{
			debug("设备已经存在，更新链表信息....");
			it->cur_music = v["cur_music"].asString();
			it->volume = v["volume"].asInt();
			it->mode = v["mode"].asInt();	
			it->d_time = time(NULL);

			if(it->a_bev)//如果app在线
			{
				debug("app已在线，转发数据");
				ser->server_send_data(it->a_bev, v);
			}

			return;
		}


	}
	//如果不存在，新建节点
	PlayerInfo p;
	p.deviceid = v["deviceid"].asString();
	p.cur_music = v["cur_music"].asString();
	p.volume = v["volume"].asInt();
	p.mode = v["mode"].asInt();
	p.d_time = time(NULL);
	p.d_bev = bev;
	p.a_bev = NULL;

	info->push_back(p);

	debug("初次上报数据，建立节点");

}

void Player::player_app_update_list(struct bufferevent *bev, Json::Value &v)
{
	for (auto it = info->begin(); it != info->end(); it++)        //app更新数据前提是音箱在线，无需else
	{
		if(it->deviceid == v["deviceid"].asString())
		{
			it->a_time = time(NULL);
			it->appid = v["appid"].asString();
			it->a_bev = bev;
		debug("app数据更新update");
		}

	}
}

void Player::player_traverse_list()
{
	debug("定时器事件：遍历链表");
	for (auto it = info->begin(); it != info->end(); it++)
	{
		if(time(NULL) - it->d_time > 6)   //音箱2秒上传一次数据，若上传数据时间与现时间超过6秒，视为离线
		{
			info->erase(it);          //把链表里这组app-音箱信息删了
		}

		if(it->a_bev)       //app上传过数据，a_bev就有值
		{
			if(time(NULL) - it->a_time > 6)
			{
				it->a_bev = NULL;       //离线了将a_bev置为空
			}
		}
	}
}

void Player::debug(const char *s)
{
	time_t cur = time(NULL);
	struct tm *t = localtime(&cur);

	std::cout << "[ " << t->tm_hour <<" : " << t->tm_min << " : ";
	std::cout << t->tm_sec << " ] " << s << std::endl;
}

void Player::player_upload_music(Server *ser, struct bufferevent *bev, Json::Value &v)
{
	for (auto it = info->begin(); it != info->end();it++)
	{
		if(it->d_bev == bev)      //找到音箱
		{
			if(it->a_bev != NULL) //确定app在线
			{
				ser->server_send_data(it->a_bev, v);      //将歌曲信息转发到app端
				debug("app在线，歌曲信息转发成功");
			}
			else//app不在线
			{
				debug("app离线，无法转发歌曲信息");
			}
			break;
		}
	}
}

void Player::player_option(Server *ser,struct bufferevent *bev, Json::Value &v)
{
	//判断音箱是否在线（a_bev与d_bev通过保活机制绑定，有a_bev就有d_bev）
	for(auto it = info->begin();it != info->end();it++)
	{
		if(it->a_bev == bev)   //音箱在线
		{
			ser->server_send_data(it->d_bev, v);
			debug("音箱在线，成功转发指令");
			return;
		}

	}
	//音箱离线
	Json::Value value;
	std::string cmd = v["cmd"].asString();
	cmd += "_reply";
	value["cmd"] = cmd;
	value["result"] = "offline";

	ser->server_send_data(bev, value);

	debug("音箱离线，转发失败");
}

void Player::player_reply_option(Server *ser,struct bufferevent *bev, Json::Value &v)
{
	for(auto it = info->begin();it !=info->end();it++)
	{
		if(it->d_bev == bev) //app在线
		{
			if(it->a_bev)
			{
				ser->server_send_data(it->a_bev, v);
                        	debug("app在线，成功回复指令");
				break;
			}

		}
	}
}

void Player::player_offline(struct bufferevent *bev)
{
	for (auto it = info->begin();it != info->end(); it++)
	{
		if(it->d_bev == bev)//音箱下线
		{
			debug("音箱异常离线");
			info->erase(it);
			break;
		}

		if(it ->a_bev == bev)//app下线
		{
			debug("app异常离线");
			it->a_bev = NULL;
			break;
		}
	}
}

void Player::player_app_offline(struct bufferevent *bev)
{
	for (auto it = info->begin();it != info->end();it++)
	{
		if(it->a_bev == bev)
		{
			debug("app正常下线");
			it->a_bev = NULL;
			break;
		}
	}
}

void Player::player_get_music(Server *ser,struct bufferevent *bev, Json::Value &v)
{
	for(auto it = info->begin(); it != info->end(); it++)
	{
		if(it->a_bev == bev)
		{
			ser->server_send_data(it->d_bev, v);
		}
	}
}
