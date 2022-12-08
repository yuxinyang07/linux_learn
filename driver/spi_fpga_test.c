#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


typedef unsigned char u8;

#define SPI_FPGA_IOC_MAGIC              'k'
#define SPI_FPGA_IOC_MAXNR              5

#define SPI_GPIO_IOC_SET_MODE           _IOW(SPI_FPGA_IOC_MAGIC, 1, u8)
#define SPI_GPIO_IOC_GET_MODE           _IOR(SPI_FPGA_IOC_MAGIC, 1, u8)

#define SPI_GPIO_IOC_SET_LEN            _IOW(SPI_FPGA_IOC_MAGIC, 2, u8)
#define SPI_GPIO_IOC_GET_LEN            _IOR(SPI_FPGA_IOC_MAGIC, 2, u8)

#define SPI_GPIO_IOC_TRANS              _IO(SPI_FPGA_IOC_MAGIC, 3)


uint8_t test_buff[16] = {0x00, 0x01, 0x02, 0x03,
                         0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0A, 0x0B,
                         0x0C, 0x0D, 0x0E, 0x0F};


int main(int argc, char **argv)
{
    int fd  = -1;
    int ret = 0;
    int i   = 0;
    unsigned char cmd = 0x55;

    fd = open("/dev/spi_fpga", O_RDWR);
    if (fd < 0)
    {
        perror("open device failed!\r\n");
        return -1;
    }

    ret = ioctl(fd, SPI_GPIO_IOC_SET_MODE, 0);
    if (ret < 0)
    {
        perror("set spi mode failed!\r\n");
        close(fd);
        return -2;
    }
    
    for(int n = 0; n < 10; n++){
    // trans cmd
    ret = ioctl(fd, SPI_GPIO_IOC_SET_LEN, 0x01);
    if (ret < 0)
    {
        perror("set spi trans len failed!\r\n");
        close(fd);
        return -3;
    }
    ret = ioctl(fd, SPI_GPIO_IOC_TRANS, 0x55);
    if (ret < 0)
    {
        perror("trans cmd failed!\r\n");
        close(fd);
        return -4;
    }
    printf("[app] Trans cmd:0x%02x.\r\n", cmd);
    sleep(1);
    }

    for(int m = 0; m < 10; m++) {
    // trans data
    ret = ioctl(fd, SPI_GPIO_IOC_SET_LEN, 0x10);
    if (ret < 0)
    {
        perror("set spi trans len failed!\r\n");
        close(fd);
        return -3;
    }
    ret = ioctl(fd, SPI_GPIO_IOC_TRANS,test_buff);
    if (ret < 0)
    {
        perror("trans data failedQ!\r\n");
        close(fd);
        return -4;
    }
    printf("[app] Trans data:\r\n");
    for(i = 0; i < sizeof(test_buff); i++)
    {   
        printf("0x%02x ", test_buff[i]);
        if ((i + 1) % 4 == 0)
            printf("\r\n");
    }
    sleep(1);
    }

    close(fd);

    return 0;
}
