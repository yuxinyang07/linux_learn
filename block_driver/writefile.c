#include<stdio.h>
#include <unistd.h>
#include<fcntl.h>




int main()
{

    int fd ;
    unsigned char *buf ;
    int len;

    buf = "hello world";
    fd = open("./test.txt",O_RDWR);
    if(fd < 0){
        printf("open  file failed \n ");
        return -1;
    }
    len = write(fd,buf, sizeof("hello world"));
    if(len < 0){
        printf("write  file failed \n ");
        return -1;

    }

    return 0;
}

