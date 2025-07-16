#ifndef DATABASE_H
#define DATABASE_H

#include <iostream>
#include <mysql/mysql.h>
#include <string.h>


class DataBase
{
	private:
		MYSQL *mysql;        //数据库句柄
	public:
		DataBase();
		~DataBase();
		bool database_connect();//连接数据库
		void database_disconnect();//断开（释放）数据库
		bool database_init_table();//建立表格
		bool database_user_exist(std::string);//检测用户是否存在
		void database_add_user(std::string, std::string);//添加用户信息
		bool database_password_correct(std::string, std::string);//检查密码是否正确
		bool database_user_bind(std::string, std::string &);//检查是否与device绑定
		void database_user_bindid(std::string, std::string );//绑定appid与deviceid
};



#endif

