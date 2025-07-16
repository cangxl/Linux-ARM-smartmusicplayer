#include "player.h"

int g_shmid = 0;           //共享内存返回值
int g_start_flag = 0;      //开始播放标志位
int g_suspend_flag = 0;    //暂停播放标志位


extern int g_mixerfd;
extern Node *head;

int init_shm()
{
	//创建共享内存
	g_shmid = shmget(SHMKEY,SHMSIZE,IPC_CREAT | IPC_EXCL);
	if (-1 == g_shmid)
	{
		perror("shmget");
		return -1;
	}

	//映射
	void *addr = shmat(g_shmid,NULL,0);
	if((void *)-1 == addr)
	{
		perror("shmat");
		return -1;
	}

	Shm s;
	memset(&s,0,sizeof(s));
	s.parent_pid = getpid();//直接将父进程pid存入共享内存
	s.mode = SEQUENCE;
	memcpy(addr,&s,sizeof(s));   //将数据s写入共享内存

	shmdt(addr);            //解除映射

}

void get_shm(Shm *s)//将共享内存映射至addr，存入结构体s
{
	void *addr = shmat(g_shmid, NULL, 0);
	if((void *)-1 == addr)
	{
		perror("shmat");
		return ;
	}

	memcpy(s, addr, sizeof(Shm));

	shmdt(addr);
}

void set_shm(Shm s)//通过addr建立映射，将结构体s中的数据拷贝到共享内存（g_shmid）
{
	void *addr = shmat(g_shmid, NULL, 0);
    if((void *)-1 == addr)
    {
		perror("shmat");
	    	return ;
	}
	     
	memcpy(addr, &s, sizeof(Shm));
		
	shmdt(addr);
}

void get_volume(int *v)
{
	//获取音量大小
	int volume;
	if(ioctl(g_mixerfd, SOUND_MIXER_READ_VOLUME, v) == -1)//从mixer中获取音量参数
	{
		perror("ioctl");
		return ;
	} 
	*v /= 257;
}

void get_music(const char *singer)
{
	//发送请求json
	struct json_object *obj = json_object_new_object();
	json_object_object_add(obj, "cmd", json_object_new_string("get_music_list"));
	json_object_object_add(obj, "singer", json_object_new_string(singer));

	socket_send_data(obj);

	char msg[1024] = {0};
	socket_recv_data(msg);


	//形成链表
	create_link(msg);
	
	//上传音乐数据给服务器，服务器传给app
	upload_music_list();


}

int start_play()
{
	if(g_start_flag == 1)	//已开始播放
		return -1;

	if(head->next == NULL)
		return -1;

	char name[32] = {0};
	strcpy(name, head->next->music_name);//从链表头节点中拷贝歌名给name
	
	//初始化音量
	int volume = DFL_VOL;
	volume *=257;
	ioctl(g_mixerfd, SOUND_MIXER_WRITE_VOLUME, &volume);

	g_start_flag = 1;

	play_music(name);
	
	return 0;
}


/*子进程收到SIGUSR2信号，触发该函数*/
void child_quit()
{
	g_start_flag = 0;
}

void play_music(char *n)
{
	pid_t child_pid = fork();//创建一个子进程
	if(-1 == child_pid)
	{
		perror("fork failure");
		return;
	}
	else if (0 == child_pid)
	{
	//子进程返回0，创建成功
		
		close(0);//关闭标准输入（防止影响父进程）
		signal(SIGUSR2, child_quit);
		child_process(n);
		exit(0);
	}
	else
	{
		return;
	}
}

/*
子进程：
1.创建孙进程，孙进程调用mplayer播放音乐
2.等待孙进程结束
*/
void child_process(char *n)
{
	while(g_start_flag)
		{
			pid_t grand_pid = fork();//创建孙进程
			if(-1 == grand_pid)
			{
	    		perror("fork failure");
				return;
			}
			else if (0 == grand_pid)
			{
				//孙进程返回0，创建成功
				close(0);//关闭标准输入
				Shm s;
				memset(&s, 0, sizeof(s));
				if(strlen(n) == 0)    //第二次进入循环（自动播放下一首歌）
				{
					grand_get_shm(&s);//孙进程获取共享内存中数据


					if(find_next_music(s.cur_music, s.mode, n) == -1)
					{
						//歌曲播放完了   通知父进程与子进程

						kill(s.parent_pid,SIGUSR1);//此时父子进程pid已经读取在s里，SIGUSR1或2为用户自定义信号
						kill(s.child_pid,SIGUSR2);
						usleep(100000);//防止进程间接收信号延时，收到信号前就触发exit
						exit(0);
					}
				}
				char *arg[7] = {0};
				char music_path[128] = {0};
				strcpy(music_path,URL);
				strcat(music_path, n);

				arg[0] = "mplayer";
				arg[1] = music_path;
				arg[2] = "-slave";
				arg[3] = "-quiet";
				arg[4] = "-input";
				arg[5] = "file=./cmd_fifo";
				arg[6] = NULL;
		
				grand_get_shm(&s);//(从共享内存中取信息)
				char *p = n;      //去除歌手名字（删掉/前的文字）
				while(*p != '/')
					p++;
				strcpy(s.cur_music, p + 1);

				grand_set_shm(s);  //修改共享内存（子进程id 孙进程id 音乐名)

				printf("---grand %s\n",s.cur_music);

#ifdef ARM
				execv("/bin/mplayer",arg);
#else
				execv("/usr/local/bin/mplayer",arg);
#endif
			}		
			else         //子进程内容：等待孙进程结束
			{	
				memset(n, 0, sizeof(n));
				int status;
				wait(&status);

				usleep(100000);
			}
		}
}

void grand_set_shm(Shm s)
{
	//修改共享内存（子进程id 孙进程id 音乐名）
	int shmid = shmget(SHMKEY, SHMSIZE, 0);
	if (-1 == shmid)
	{
		perror("grand shmget");
		return;
	}
 
	void *addr = shmat(shmid, NULL, 0);
	if ((void *)-1 == addr)
	{
	    perror("grand shmat");
		return;
	}
	 
	
	s.child_pid = getppid();
	s.grand_pid = getpid();
	
	 
	memcpy(addr, &s, sizeof(s));//完成上传数据至共享内存
	shmdt(addr);
}

void grand_get_shm(Shm *s)
{
	int shmid = shmget(SHMKEY, SHMSIZE, 0);
	if (-1 == shmid)
	{
		perror("grand shmget");
		return;
	}
 
	void *addr = shmat(shmid, NULL, 0);
	if ((void *)-1 == addr)
	{
		perror("grand shmat");
		return;
	}
	memcpy(s, addr, sizeof(Shm));

	shmdt(addr);
 }

void write_fifo(const char *cmd)
{
	int fd = open("cmd_fifo", O_WRONLY);
	if(-1 == fd)
	{
		perror("fifo open");
		return;
	}

	if(write(fd, cmd, strlen(cmd)) == -1)
	{
		perror("fifo write");
		return;
	}

	close(fd);
}

void stop_play()
{
	if(g_start_flag == 0)
		return;
	//通知子进程
	Shm s;
	get_shm(&s);
	kill(s.child_pid, SIGUSR2);//使g_start_flag变成0
	
	//结束mplayer进程（fifo）
	write_fifo("stop\n");
	
	//回收子进程
	int status;
	waitpid(s.child_pid, &status, 0);

	g_start_flag = 0;//重置标志位状态
}

void suspend_play()
{
	if(g_start_flag == 0 || g_suspend_flag == 1)
		return;
	printf("-- 暂停播放 --\n");

	write_fifo("pause\n");

	g_suspend_flag = 1;
}

void continue_play()
{
	if(g_start_flag == 0 || g_suspend_flag == 0)
		return;

	printf("-- 继续播放 --\n");

	write_fifo("pause\n");

	g_suspend_flag = 0;
}
/*
函数描述：播放下一首
读共享内存看播放状态，没歌了用get_music取
用fifo传loadfile指令给mplayer，因此需要拼接歌曲名
*/
void next_play()
{
	if(g_start_flag == 0)
		return;

	Shm s;
	get_shm(&s);
	char music[128] = {0};
	printf("-----%s\n",s.cur_music);
	if(find_next_music(s.cur_music, SEQUENCE, music) == -1)
	{
		//链表里没歌了
		stop_play();
		char singer[128] = {0};
		get_singer(singer);

		clear_link();
		get_music(singer);
		start_play();

		if(g_suspend_flag == 1)
			g_suspend_flag == 0;
		return;
	}

	char path[256] = {0};
	strcat(path, URL);
	strcat(path, music);

	char cmd[256] = {0};
	sprintf(cmd, "loadfile %s\n",path);

	write_fifo(cmd);

	//更新共享内存（不关mplayer，孙进程不会触发grand_set_shm，共享内存不变）
	int i;
	for(i = 0;i < sizeof(music);i++)
	{
		if(music[i] == '/')
			break;
	}
	
	strcpy(s.cur_music,music + i + 1);
	set_shm(s);//裁掉歌手名，写入共享内存

	if(g_suspend_flag == 1)
		g_suspend_flag == 0;
}

void prior_play()
{
	if(g_start_flag == 0)
		return;

	Shm s;
	get_shm(&s);
	char music[128] = {0};
	find_prior_music(s.cur_music, music);

	char path[256] = {0};
	strcat(path, URL);
	strcat(path, music);

	char cmd[256] = {0};
	sprintf(cmd, "loadfile %s\n",path);
	 
	write_fifo(cmd);
	 
	//更新共享内存（不关mplayer，孙进程不会触发grand_set_shm，共享内存不变）
	int i;
	for(i = 0;i < sizeof(music);i++)
		{
			if(music[i] == '/')
	             break;
	    }
	
	strcpy(s.cur_music,music + i + 1);
	set_shm(s);//裁掉歌手名，写入共享内存

	if(g_suspend_flag == 1)
		g_suspend_flag = 0;
}


void voice_up()
{
	//获取音量大小
	int volume;
	if(ioctl(g_mixerfd, SOUND_MIXER_READ_VOLUME, &volume) == -1)//从mixer中获取音量参数
	{
		perror("ioctl");
	    return ;
    }
	volume /= 257;

	if(volume <= 95)
	{
		volume += 5;
	}
	else if(volume > 95 && volume < 100)
	{
		volume = 100;
	}
	else if(volume == 100)
	{
		printf("音量已经达到最大...\n");
		return;
	}
    
	volume *= 257;

	if(ioctl(g_mixerfd, SOUND_MIXER_WRITE_VOLUME, &volume) == -1)//将更新后的音量参数写入mixer
	{
		perror("ioctl");
        return ;
	}

	printf("---- 增加音量 ----\n");
}

void voice_down()
{
	//获取音量大小
	int volume;
	if(ioctl(g_mixerfd, SOUND_MIXER_READ_VOLUME, &volume) == -1)//从mixer中获取音量参数
	{
		perror("ioctl");
		return ;
	}
	volume /= 257;
 
	if(volume >= 5)
	{
        volume -= 5;
    }
	else if(volume < 5 && volume > 0)
    {
        volume = 0;
    }
	else if(volume == 0)
	{
        printf("音量已经达到最小...\n");
        return;
    }
 
    volume *= 257;

    if(ioctl(g_mixerfd, SOUND_MIXER_WRITE_VOLUME, &volume) == -1)//将更新后的音量参数写入mixer
    {
        perror("ioctl");
        return ;
    }
 
    printf("---- 减小音量 ----\n");
}

void circle_play()
{
	Shm s;
	get_shm(&s);
	s.mode = CIRCLE;
	set_shm(s);
	printf("---- 单曲循环 ----\n");
}

void sequence_play()
{
	Shm s;
	get_shm(&s);
	s.mode = SEQUENCE;
	set_shm(s);
	printf("---- 顺序播放 ----\n");
}

void singer_play(const char *singer)
{
	//停止播放
	stop_play();

	//清空链表
	clear_link();

	//获取歌曲
	get_music(singer);

	//开始播放
	start_play();
}
