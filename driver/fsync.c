#include <sys/types.h>    
#include <sys/stat.h>    
#include <stdio.h>
#include <string.h>
#include <poll.h>               
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>


int fd,ret;
void my_signal_fun(int signame)      //有信号来了
{
       
	   read( fd, &ret, 1);              //读取驱动层数据
	      printf("signal   sdio update ~~~state=0X%x\r\n",ret); 

}

 
/*useg:    fourthtext   */
int main(int argc,char **argv)
{
	  int oflag;
	  unsigned int val=0;      
	  fd=open("/dev/dev_fpga",O_RDWR); 
	  if(fd<0)
	{
		printf("can't open!!!\n");
		return -1;
	}

	signal(SIGIO,my_signal_fun); //指定的信号SIGIO与处理函数my_signal_run对应
	fcntl( fd, F_SETOWN, getpid());  //指定进程作为fd 的属主,发送pid给驱动
	oflag=fcntl( fd, F_GETFL);   //获取fd的文件标志状态
	fcntl( fd, F_SETFL, oflag|FASYNC);  //添加FASYNC状态标志，调用驱动层.fasync成员函数 
	 while(1)
	 {
		sleep(1);                  //做其它的事情
	   read( fd, &ret, 1);              //读取驱动层数据
	      printf("sdio update ~~~state=0X%x\r\n",ret); 
	}
	 return 0;

}

