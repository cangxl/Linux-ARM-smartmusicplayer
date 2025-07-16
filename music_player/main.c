#include "main.h"

fd_set READSET;
int g_maxfd = 0;
Node *head = NULL;      //链表头节点地址 头指针

int main()
{
	system("./init.sh");
	//初始化 select
	init_select();
	//初始化设备
	if (init_device() == -1)
	{
		printf("初始化设备失败\n");
		//exit(1);
	}
	//初始化共享内存
	if(init_shm() == -1)
	{
		printf("初始化共享内存失败\n");
		return -1;
	}
	//初始化链表
	if (init_link() == -1)
	{
		printf("链表初始化失败\n");
		return -1;
	}
	//初始化网络(TCP)
	if (init_socket() == -1)
	{
		printf("初始化网络失败\n");
		return -1;
	}

	printf("连接服务器成功...\n");

	//获取音乐
	get_music("其他");

	//traverse_link();

	//处理孙进程送来的信号
	signal(SIGUSR1, update_music);

	m_select();
	return 0;
}
