#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#define MLX90640_MAGIC  'x'
#define TEST_MAX_NR 3 
#define MLX90640_GET_DATA    _IO(MLX90640_MAGIC,1 )
#define MLX90640_SET_DATA    _IO(MLX90640_MAGIC,2 )
#define TEST_KBUF _IO(TEST_MAGIC, 3)


typedef unsigned short int      uint16_t;
#define MLX90640_DEV   "/dev/mlx90640-dev"
typedef struct MLX90640_DATA
{
    uint16_t reg;
    uint16_t size;
    uint16_t buff[1664];
};


typedef struct w_MLX90640_DATA
{
    uint16_t reg;
    uint16_t data;
};



int i2c_write(int fd ,struct w_MLX90640_DATA* data)
{
    int ret = ioctl(fd,MLX90640_SET_DATA,data);
    if(ret < 0) {
        printf("MLX90640_SET_DATA fail \n");
        return ret;
    }
    return 0;
}

int i2c_read(int fd,struct  MLX90640_DATA* data)
{
    int ret = ioctl(fd,MLX90640_GET_DATA,data);
    if(ret < 0) {
        printf("MLX90640_GET_DATA fail \n");
        return ret;
    }
    return 0;

}





int main()
{
    int fd;
    int i = 0;
    uint16_t value =0;
    fd = open(MLX90640_DEV,O_RDWR);
    struct MLX90640_DATA  data3;
    struct w_MLX90640_DATA  data2;
   
    memset(&data3,0,sizeof(struct MLX90640_DATA));
    data3.reg = 0x800d;
    data3.size = 1;
    

    if(fd < 0){
        printf("open device fail \n");
        return fd;
    }
    

    i2c_read(fd,&data3);
    printf("data3 = %x  \n",data3.buff[0]);
    value = (data3.buff[0]  & 0xFC7F) | value;
    printf("value = %x  \n",value);
    value = 0x1100;
    data2.reg = 0x800D;
    data2.data = value;
    
    i2c_write(fd,&data2);
    
    i2c_read(fd,&data3);
    printf("value = %x  \n",data3.buff[0]);

}
