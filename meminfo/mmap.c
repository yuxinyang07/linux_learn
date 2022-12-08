#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/mman.h>



#define MAP_SIZE (100*1024*1024)


int main(int argc ,char *argv[])
{
    char *p;
    char val;
    int i;
    puts("before mmap ok,please exec 'free -m!");
    sleep(5);


    p = mmap(NULL,MAP_SIZE,PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS,-1,0);

    if(p == NULL){
        perror("fail to malloc");
        return -1;
    }
    puts("after  mmap ok,please exec 'free -m!");
    sleep(5);

    for(i = 0; i < MAP_SIZE; i++){
        val = p[i];
    }
    puts("read ok,please exec 'free -m '!");
    sleep(5);


    memset(p ,0x55,MAP_SIZE);
    puts("write ok,please exec 'free -m '!");

    pause();
    return 0;

}

