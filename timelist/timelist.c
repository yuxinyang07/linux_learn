
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/jiffies.h>



struct timer_list mytimer;







static void myfunc(unsigned long data)
{
    printk("~~~%s \n",(char*)data);
    mod_timer(&mytimer,jiffies + HZ*4);
    
}












static int __init timer_list_init(void)
{
    init_timer(&mytimer);
    mytimer.function = myfunc;
    mytimer.data = (unsigned long )"hello world";
    mytimer.expires = jiffies + HZ*4;
    add_timer(&mytimer);
    return 0;
}




static void __exit timer_list_exit(void)
{
    del_timer(&mytimer);
}













module_init(timer_list_init);
module_exit(timer_list_exit);

MODULE_LICENSE("GPL");

