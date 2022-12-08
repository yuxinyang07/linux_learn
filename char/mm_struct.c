
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
int init_val =10;
int uninit_val;
const *pp = "dsfsdf";
int main()
{
    const char *p = "abcd";
    unsigned  char *addr;
    unsigned char *date;
    printf(" test addr = %lx\n",main);
    printf(" const data addr  = %lx\n",p);
    printf("init data addr = %lx \n ",&init_val);
    printf("uninit data addr = %lx \n ",&uninit_val);
    printf("const pp  = %lx \n ",pp);
    int *p1 = (int *)malloc(4096);   
    int *p2 = (int *)malloc(4096);  
    int *p3 = (int *)malloc(4096);  
    int *p4 = (int *)malloc(4096);  
    
    printf("heap  p1 = %lx \n ",p1);
    printf("heap  p2 = %lx \n ",p2);
    printf("heap  p3 = %lx \n ",p3);
    printf("heap  p4 = %lx \n ",p4);
    printf("stack  p1 = %lx \n ",&p1);
    printf("stack  p2 = %lx \n ",&p2);
    printf("stack  p3 = %lx \n ",&p3);
    printf("stack  p4 = %lx \n ",&p4);

    FILE * fp ;
    int fd;
    float a = -1.3333;
    float b,c;
    int ret;
    struct stat buf = {0};
    fp = fopen("/home/rk3399/learn/char/test.txt","r+");
    if(fp ==NULL ){
        printf("open file failed \n");
        return -1;
    }
    fd = open("/home/rk3399/learn/char/test.txt",O_RDWR|O_CREAT,00777);
    if(fd < 0){
        printf("open test.txt failed \n");
        return -1;
    }

    fprintf(fp,"%f %f",a,a);
    rewind(fp);
    //获取文件字节大小
    ret = fstat(fd,&buf);
    addr = (unsigned char *)mmap(NULL,buf.st_size,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
    if(addr == NULL){
        printf("mmap files failed \n ");
    }
    printf("buf = %lx,",addr);
    fscanf(fp,"%f %f ",&b,&c);
    fgetc(fp);
    fscanf(fp,"%s",date);

    printf("b = %f c = %f \n",b,c);
    printf("%s!!!!!!!!! \n",date);
    while(1){
        sleep(1);
    }
        
    fclose(fp);
}
