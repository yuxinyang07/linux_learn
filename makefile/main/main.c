#include "../include/myinclude.h"                                                                                            
#include<pthread.h>



void func(void *args)
{
    printf("thread func\n");
}




int main(int argc, char const *argv[])
{
    pthread_t ntid;
    pthread_create(&ntid,NULL,func,NULL);
    print1();  
	print2();  

    return 0;
}
