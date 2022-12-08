#include<stdlib.h>

#define sum(a,b)  (a*b)


int main()
{


    int i= 3;
    int len;
    int  **a[5][6];

    len = sum(i++,sizeof(a)+5);


    printf("len = %d sizeof(int) =%d  sizeof(a) = %d \n",len,sizeof(int*),sizeof(a));
}
