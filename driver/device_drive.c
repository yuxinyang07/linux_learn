# include <linux/module.h>
# include <linux/fs.h>
# include <linux/uaccess.h>
# include <linux/init.h>
# include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/sysfs.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>

# define DEMO_NAME "my_demo_dev"


static  signed count = 1;
  struct test_dev {
     dev_t dev;
     struct device *device;
     struct cdev *demo_cdev;

};

static struct test_dev  *testdev = NULL;


static int demodrv_open(struct inode *inode, struct file *file)
{
	int major = MAJOR(inode->i_rdev);
	int minor = MINOR(inode->i_rdev);

	printk("%s: major=%d, minor=%d\n",__func__,major,minor);

	return 0;
}


static ssize_t demodrv_read(struct file *file, char __user *buf,size_t lbuf,loff_t *ppos)
{
	printk("%s enter\n",__func__);
	
	return 0;
}


static ssize_t demodrv_write(struct file *file, const char __user *buf,size_t count,loff_t *f_pos)
{
	printk("%s enter\n",__func__);
	
	return 0;
}


static const struct file_operations demodrv_fops = {
	.owner = THIS_MODULE,
	.open = demodrv_open,
	.read = demodrv_read,
	.write = demodrv_write
};


static int __init simple_char_init(void)
{
	int ret;

    printk("[spi_fpga] %s(%d)\r\n", __FUNCTION__, __LINE__);
	ret = alloc_chrdev_region(testdev->dev,0,count,DEMO_NAME);
	if(ret)
	{
		printk("failed to allocate char device region\n");
		return ret;
	}
    printk("[spi_fpga] %s(%d)\r\n", __FUNCTION__, __LINE__);
	testdev->demo_cdev = cdev_alloc();
	if(!testdev->demo_cdev) 
	{
		printk("cdev_alloc failed\n");
		goto unregister_chrdev;
	}

    printk("[spi_fpga] %s(%d)\r\n", __FUNCTION__, __LINE__);
	cdev_init(testdev->demo_cdev,&demodrv_fops);

    printk("[spi_fpga] %s(%d)\r\n", __FUNCTION__, __LINE__);
	ret = cdev_add(testdev->demo_cdev,testdev->dev,count);
    printk("[spi_fpga] %s(%d)\r\n", __FUNCTION__, __LINE__);
	if(ret)
	{
		printk("cdev_add failed\n");
		goto cdev_fail;
	}

	printk("successed register char device: %s\n",DEMO_NAME);
	printk("Major number = %d,minor number = %d\n",MAJOR(testdev->dev),MINOR(testdev->dev));

	return 0;

cdev_fail:
	cdev_del(testdev->demo_cdev);

unregister_chrdev:
	unregister_chrdev_region(testdev->dev,count);

	return ret;
} 


static void __exit simple_char_exit(void)
{
	printk("removing device\n");

	if(testdev->demo_cdev)
		cdev_del(testdev->demo_cdev);

	unregister_chrdev_region(testdev->dev,count);
}

module_init(simple_char_init);
module_exit(simple_char_exit);

MODULE_LICENSE("GPL");
