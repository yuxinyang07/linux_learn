#include<stdio.h>






struct list{

    int a;
    char b;
    double c;
};




int main()
{
    struct  list *a=NULL;
    struct list p;
    p.a = 2;
    p.b= 3;
    p.c=24;

    a= (struct list*)(&p.b - (unsigned long)(&((struct list*)0)->b));
    printf("%x %x %x  %ld\n",a,&p,&p.b, (unsigned long)(&((struct list*)0)->b));

    
}

