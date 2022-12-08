#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/interrupt.h>


struct test_dev {
    struct device *dev;
};

struct test_dev dev;

struct tasklet_struct test_tasklet;



static irqreturn_t  test_irq(int irq ,void *dev_id)
{

    tasklet_schedule(&test_tasklet);
}




void tasklet_sched(unsigned long data)
{
    struct task_struct  *task;
    task = current;
    printk("pid ,status, name \n",task->pid,task->status,task->name);

}




static int __init tasklet_init(void)
{
    


    tasklet_init(&test_tasklet,tasklet_sched,(unsigned long)dev);
    request_irq(irq,test_irq,0,"test_tasklet",dev);

}







static int __exit tasklet_exit(void)
{

}












moudule_init(tasklet_init);
moudule_exit(tasklet_exit);
MODULE_LICENSE("GPL");
