#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>


int main()
{
	int fd = open("/dev/mixer",O_RDWR);
	if(-1 == fd)
	{
		perror("open");
		return -1;
	}

	//获取音量大小
	int volume;
	if(ioctl(fd, SOUND_MIXER_READ_VOLUME, &volume) == -1)//从mixer中获取音量参数
	{
		perror("ioctl");
		return -1;
	}

	volume /= 257;

	printf("当前系统音量为 %d\n",volume);
	
	volume = 70;
	volume *=257;
	if(ioctl(fd,SOUND_MIXER_WRITE_VOLUME,&volume) == -1)
	{
		perror("ioctl");
		return -1;
	}

	return 0;
}
