#include<linux/init.h>
#include<linux/module.h>
#include<linux/sched.h>





MODULE_LICENSE("GPL");
static int pid =-1;
module_param(pid,int ,S_IRUSR | S_IWUSR);







static int __init  killd_init(void)
{
    struct task_struct *p;
    
    for_each_process(p){
        if(p->pid == pid){
            printk(" process state = %ld \n",p->state);
            set_task_state(p,TASK_UNINTERRUPTIBLE);
            return 0;
        }
    }
    return 0;
}




static void __exit  killd_exit(void)
{
}










module_init(killd_init);
module_exit(killd_exit);

