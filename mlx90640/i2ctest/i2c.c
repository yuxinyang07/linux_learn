#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
typedef unsigned short int      uint16_t;
#define TMP1075_DEV "/dev/mlx90640-dev"
#define MLX90640_MAGIC  'x'
#define TEST_MAX_NR 3 
#define MLX90640_GET_DATA    _IO(MLX90640_MAGIC,1 )
#define MLX90640_SET_DATA    _IO(MLX90640_MAGIC,2 )
#define TEST_KBUF _IO(TEST_MAGIC, 3)
#define LED_NUM_ON        _IOW('L',0x1122,int)
#define LED_NUM_OFF        _IOW('L',0x3344,int)
#define LED_ALL_ON        _IO('L',0x1234)
#define LED_ALL_OFF        _IO('L',0x5678)
typedef struct MLX90640_DATA
{
    uint16_t reg;
    uint16_t size;
    uint16_t buff[1664];
};

typedef struct w_MLX90640_DATA {
    uint16_t reg;
    uint16_t data;
};
int main(int argc, char * const argv[])
{
    int fd = 0;
    unsigned char data[3];;
    int ret = 0;
int i = 0;
    struct w_MLX90640_DATA  data2;
    struct MLX90640_DATA  data3;
     data2.reg = 0x8000;
    data2.data = 0x0030;
   memset(&data3,0,sizeof(struct MLX90640_DATA)); 
    data3.reg = 0x2400;
    data3.size = 832;
      for(i = 0 ; i < 832 ; i ++)
{
	printf("%d \n",data3.buff[i]);
}

    sleep(5);

    fd = open(TMP1075_DEV, O_RDONLY);

	if(fd <= 0)
	{
		perror("open err\r\n");
		exit(1);
	}
    while(1){
        ret = read(fd, data, 2);
	    if(ret == -1) {
            perror("Failed to read.\n");
            exit(1);
        }

	ioctl(fd,MLX90640_SET_DATA,&data2);
	ioctl(fd,MLX90640_GET_DATA,&data3);
      for(i = 0 ; i < 832 ; i ++)
{
	printf("%d \n",data3.buff[i]);
}

    sleep(5);

#if 0
    //第一个灯闪
    ioctl(fd,LED_NUM_ON,1);
    sleep(1);
    ioctl(fd,LED_NUM_OFF,1);
    sleep(1);
    
    //第二个灯闪
    ioctl(fd,LED_NUM_ON,2);
    sleep(1);
    ioctl(fd,LED_NUM_OFF,2);
    sleep(1);

    //两个灯同时闪
    ioctl(fd,LED_ALL_ON);
    sleep(1);
    ioctl(fd,LED_ALL_OFF);
    sleep(1);
#endif
        printf("temp register data = 0x%02x%02x\r\n",data[0],data[1]);

    }   
    return 0;
}