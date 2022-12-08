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
#define DRV_NAME "esp32_gpio"


/*    异步信号结构体变量    */

int esp32_gpio;

static int esp32_drv_open(struct inode*  inode, struct file* filp)
{
    printk("open device \n");
   //  struct esp32_gpio_data* dev;
    // dev = container_of(inode->i_cdev,struct esp32_gpio_data ,dev );
   //  filp->private_data = dev;
     return 0;


}


ssize_t esp32_drv_write(struct file *file,char __user *buf,size_t size,loff_t  *ppos)
{
    int fd;
    int val;

    copy_from_user(&val,buf,sizeof(val));
    printk("val = %d \n",val);
    gpio_direction_output(esp32_gpio, val);

    return 0;

}
ssize_t esp32_drv_read(struct file *file,char __user *buf,size_t size,loff_t  *ppos)
{
    int fd;
    int val;
    gpio_direction_input(esp32_gpio);
    val = gpio_get_value(esp32_gpio);
    printk("gpio val =%d \n",val);
    fd = copy_to_user(buf,&val,sizeof(val));
    if(fd <  0){
        return -EAGAIN;

    }

    return 0;


}


static struct file_operations esp32_drv_fops={
         .owner = THIS_MODULE,
         .read = esp32_drv_read,
         .write = esp32_drv_write,
         .open = esp32_drv_open,
};

static struct miscdevice esp32_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "esp32",
    .fops = &esp32_drv_fops,
};


static int esp32_gpio_probe(struct platform_device *pdev)
{
    int ret = 0;
    printk("enter \n");
    misc_register(&esp32_dev);
      esp32_gpio = of_get_named_gpio(pdev->dev.of_node, "gpios", 0);
       if (!gpio_is_valid(esp32_gpio))
         {
            printk(KERN_ERR"invalid irq_gpio: %d\n", esp32_gpio);
             return -1;
         }
     ret = gpio_request(esp32_gpio,"esp32");
     if(ret < 0 ){
         printk("reguest gpio failed %d \n",esp32_gpio);
         return -1;
     }

}



static struct of_device_id esp32_of_match[] = {
    {.compatible = "esp32-gpio"},
     {}
 };

MODULE_DEVICE_TABLE(of, esp32_of_match);

static int esp32_gpio_remove(struct platform_device *dev)
{

    printk("%s enter.\n", __func__);
    return 0;
}

static struct platform_driver esp32_gpio_driver =
{
    .driver ={
        .name = "esp32_gpio",
        .of_match_table = esp32_of_match,
    },
    .probe = esp32_gpio_probe,
    .remove = esp32_gpio_remove,

};















module_platform_driver(esp32_gpio_driver);



MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("esp32 driver");
