#include "link.h"

extern Node *head;
extern int g_start_flag;


int init_link()
{
	head = (Node *)malloc(sizeof(Node) * 1);
	if (NULL == head)
	{
		return -1;
	}

	head->next = NULL;
	head->prior = NULL;

	return 0;
}

void create_link(const char *s)
{
	struct json_object *obj = (struct json_object *)json_tokener_parse(s);//将接收到的字符串转换为json对象
	if(NULL == obj)
	{
		printf("不是一个json格式的字符串\n");
		return;
	}
	
	struct json_object *array;//解析json对象中的歌曲名（数组）
	array = (struct json_object *)json_object_object_get(obj, "music");
	int i = 0;
	for (i = 0; i < 5; i++)
	{
		struct json_object *music = (struct json_object *)json_object_array_get_idx(array,i);
		if(inserk_link((const char *)json_object_get_string(music)) == -1)
		{
			printf("插入链表失败\n");
			return;
		}
		json_object_put(music);
	}
	json_object_put(obj);
	json_object_put(array);

}

int inserk_link(const char *name)
{
	Node *p = head;

	while(p->next)
		p = p->next;

	Node *new = (Node *)malloc(sizeof(Node));
	if(NULL == new)
	{
		printf("malloc failure\n");
		return -1;
	}

	strcpy(new->music_name, name);
	new->next = NULL;
	new->prior = p;
	p->next = new;

	return 0; 

}
void traverse_link()
{
	Node *p = head->next;

	while (p)
	{
		printf("%s \n",p->music_name);
		p = p->next;
	}
}
/*
函数描述：找到下一首歌曲
函数参数：当前播放的歌曲 播放模式 存放下一首歌
函数返回值：如果歌曲存在，返回0，不存在返回－1
*/
int find_next_music(char *cur, int mode, char *next)
{
	Node *p = head->next;

	while(p)
	{
		if (strstr(p->music_name, cur) != NULL)
		{
			break;
		}
		p = p->next; 
	}
	//已经找到链表中对应当前歌曲的结点
	if(mode == CIRCLE)
	{
		strcpy(next, p->music_name);
		return;
	}
	else if(mode == SEQUENCE)
	{
		if(p->next != NULL)
		{
			strcpy(next, p->next->music_name);
			return 0;
		}
		else
		{
			return -1;
		}
	}
}
/*
根据当前歌曲找到上一首歌，如果为第一首，则从头开始播放第一首。
*/
void find_prior_music(char *cur, char *prior)
{
	if(NULL == cur || NULL == prior)
		return;
	
	Node *p = head->next;

	if(strstr(p->music_name, cur))//如果正在播放链表中第一首，返回值还是这首歌
	{
		strcpy(prior, p->music_name);
		return;
	}

	p = p->next;//从第二首开始遍历
	while(p)
	{
		if(strstr(p->music_name,cur))
		{
			strcpy(prior, p->prior->music_name);
			return;
		}
		p = p->next;//指针往下走（遍历链表）
	}

	printf("遍历链表失败.....\n");



}

void clear_link()
{
	Node *p = head->next;
	while(p)
	{
		head->next = p->next;
		free(p);
		p = head->next;
	}
}
/*
链表中歌曲播放完毕，调用该函数更新列表（信号SIGUSR1触发）
*/
void update_music()
{
	g_start_flag = 0;//修改标志位，防止start_play卡住

	//必须回收子进程，防止player.c中出现僵尸进程
	Shm s;
	get_shm(&s);//将共享内存中参数读到s中
	int status;
	waitpid(s.child_pid, &status, 0);//回收子进程

	char singer[128] = {0};
	get_singer(singer);

	clear_link();//清空链表
	//请求新的歌曲数据
	get_music(singer);//此时在父进程中创建新链表

	//开始播放
	start_play();
}

void get_singer(char *s)
{
	if(head->next == NULL)
		return;
	// 链表中：其他/1.mp3，让p指向/，只拷贝/之前的字段
	char *begin = head->next->music_name;
	char *p = begin;
	while(*p != '/')
		p++;

	strncpy(s, begin, p - begin);
}
