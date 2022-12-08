#include<stdio.h>
typedef unsigned short int      uint16_t;

 struct MLX90640_DATA
{
        uint16_t reg;
            uint16_t size;
};




int main()
{
    struct MLX90640_DATA data;
    struct MLX90640_DATA *p;
    data.reg = 0x0013;
    p = &data;

    printf("%x \n",p->reg);
    
}
