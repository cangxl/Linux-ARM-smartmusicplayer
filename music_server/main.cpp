#include<iostream>
#include "server.h"

int main()
{
	Server s;   //创建服务器对象
	s.listen(IP, PORT); //监听客户端链接
	
	return 0;
}
