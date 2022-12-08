#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
















int main()
{
    int fd ;
    unsigned char  *buff;
    char data[32];
    fd = open("/dev/mem",O_RDWR);
    if(fd < 0){
        printf("open device failed \n");

    }

    buff = mmap(NULL,4096,PROT_READ | PROT_WRITE,MAP_SHARED ,fd ,0x49000000);
    if(buff == MAP_FAILED ){
        printf("mmap failed \n");
        return -1;
    }

    memcpy(buff,"hello world!!!",15);
//    if(read(fd,data,15)== -1 ){
//        printf("read buff error \n");
        return -1;
//    }
//    puts(buff);
    pause();
    while(1);
    return 0;
}
