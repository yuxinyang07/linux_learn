#include <linux/bitrev.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/spi/spi.h>
#include <linux/wait.h>
#include <linux/param.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/irqreturn.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/hwmon-sysfs.h>
#include <linux/hwmon.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/sysfs.h>
#include <linux/types.h>
#include <linux/sysfs.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
#define DRV_NAME "radar_gpio"


/*    异步信号结构体变量    */
static struct fasync_struct  *radar_async;

int key_val;
struct radar_gpio_data
{
    struct device *dev;
    int radar_gpio;
    int radar_gpio_val;
    int irq_radar_gpio;
    int radar_gpio_wait_flag;
    wait_queue_head_t gpio_radar_ctrl_wait;

};


static int radar_drv_open(struct inode*  inode, struct file* filp)
{
    printk("open device \n");
   //  struct radar_gpio_data* dev;
    // dev = container_of(inode->i_cdev,struct radar_gpio_data ,dev );
   //  filp->private_data = dev;
     return 0;


}

static int radar_fasync (int fd, struct file *file, int on)
{
         return fasync_helper(fd, file, on, &radar_async);
}


ssize_t radar_drv_read(struct file *file,char __user *buf,size_t size,loff_t  *ppos)
{
    int fd;
//    struct radar_gpio_data *data =(struct radar_gpio_data *)file->private_data;

    printk("gpio val =%d \n",key_val);
  //  if(size != 1)
 //       return -EINVAL;
 //   if(file->f_flags & O_NONBLOCK)
 //   {
 //       if(!data->radar_gpio_wait_flag)
 //           return -EAGAIN;
 //   }
 //   else
 //   {
 //       wait_event_interruptible(data->gpio_radar_ctrl_wait,data->radar_gpio_wait_flag);
  //  }
//    data->radar_gpio_wait_flag = 0;
    fd = copy_to_user(buf,&key_val,sizeof(key_val));
    if(fd <  0){
        return -EAGAIN;

    }

    return 0;


}


static  irqreturn_t  radar_gpio_irq_handler(int irq ,void *dev_id)
{
    struct radar_gpio_data *data = (struct radar_gpio_data *)dev_id;
    printk("%s enter \n",__func__);
    data->radar_gpio_wait_flag = 1;
    data->radar_gpio_val =  gpio_get_value(data->radar_gpio);
    key_val = data->radar_gpio_val;
    printk("%d enter \n",data->radar_gpio_val);
    wake_up_interruptible(&data->gpio_radar_ctrl_wait);
    kill_fasync(&radar_async, SIGIO, POLL_IN);  
    return IRQ_HANDLED;
}


static struct file_operations radar_drv_fops={
         .owner = THIS_MODULE,
         .read = radar_drv_read,
         .open = radar_drv_open,
         .fasync= radar_fasync,     //初始化异步信号函数
};

static struct miscdevice radar_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "radar",
    .fops = &radar_drv_fops,
};


static int radar_gpio_probe(struct platform_device *pdev)
{
    int ret = 0;
    int irq_flags;
    struct radar_gpio_data * data = NULL;
    int irq_gpio = 0;
    int irq = (-1);
    printk("enter \n");
    misc_register(&radar_dev);
    data = devm_kzalloc(&pdev->dev,sizeof(struct radar_gpio_data),GFP_KERNEL);
    if(!data){
        return -EINVAL;
    }
    data->dev = &pdev->dev;
    dev_set_drvdata(data->dev,data);
      irq_gpio = of_get_named_gpio_flags(pdev->dev.of_node, "irq_gpio", 0,(enum of_gpio_flags *)&irq_flags);
       if (!gpio_is_valid(irq_gpio))
         {
            printk(KERN_ERR"invalid irq_gpio: %d\n", irq_gpio);
             return -1;
         }

         irq = gpio_to_irq(irq_gpio);

         if(!irq)
         {
         printk(KERN_ERR"irqGPIO: %d get irq failed!\n", irq);
         return -1;
         }
     printk(KERN_ERR"irq_gpio: %d, irq: %d", irq_gpio, irq);
    data->radar_gpio = irq_gpio;
    data->irq_radar_gpio = irq;
    ret = devm_request_any_context_irq(&pdev->dev,data->irq_radar_gpio,radar_gpio_irq_handler,IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,dev_name(&pdev->dev),data);

    if(ret){
        return ret;    
    }
    data->radar_gpio_wait_flag = 0;
    init_waitqueue_head(&data->gpio_radar_ctrl_wait);
    return ret;
}



static struct of_device_id radar_of_match[] = {
    {.compatible = "radar-gpio"},
     {}
 };

MODULE_DEVICE_TABLE(of, radar_of_match);

static int radar_gpio_remove(struct platform_device *dev)
{
    struct radar_gpio_data  *data;

    printk("%s enter.\n", __func__);
    return 0;
}

static struct platform_driver radar_gpio_driver =
{
    .driver ={
        .name = "radar_gpio",
        .of_match_table = radar_of_match,
    },
    .probe = radar_gpio_probe,
    .remove = radar_gpio_remove,

};















module_platform_driver(radar_gpio_driver);



MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("radar driver");
