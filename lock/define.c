#include<stdlib.h>






#define spin_lock_mutex(lock, flags) \
        do { spin_lock(lock); (void)(flags); } while (0)




int spin_lock(int lock)
{
    printf("lock =%d \n",lock);
}






void main()
{

    int i =5;
    unsigned long flags;


    spin_lock_mutex(i,flags);
}
