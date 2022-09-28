#include <linux/module.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/of_device.h>

#define GLOBLEFIFO_MINOR 88
#define GLOBLEFIFO_LEN 1000

static struct globalfifo_dev  {
	struct cdev cdev; 
 	unsigned  char MEM[GLOBLEFIFO_LEN];
	struct mutex mutex;
	wait_queue_head_t *read_t;
	wait_queue_head_t *write_t;
	struct miscdevice misc_dev;
};
static int globalfifo_open(struct inode *inode, struct file *filp)
{
	return 0;
}
static int  globalfifo_read(struct file *filp, char __user *buff, size_t len, loff_t * loff)
{
	return 0;
}
static int  globalfifo_write(struct file *filp, const char __user *buff, size_t  len, loff_t * loff)
{
	return 0;
}

struct file_operations globalfifo_fop = {
	.owner = THIS_MODULE,
	.open = globalfifo_open,
	.read = globalfifo_read,
	.write = globalfifo_write,
};

static int globlefifo_probe(struct platform_device * pdev)
{
	struct globalfifo_dev *dev ;
	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	dev->misc_dev.name ="globle_fifo";
	dev->misc_dev.minor = GLOBLEFIFO_MINOR;
	dev->misc_dev.fops =&globalfifo_fop;
	misc_register(&dev->misc_dev);
	printk("match fifo \n");
}


static int globlefifo_remove(struct platform_device *pdev)
{
	return 0;
}

	
static struct platform_driver globlefifo = {
	.probe = globlefifo_probe,
	.remove = globlefifo_remove,
	.driver = {
		.name = "globle_fifo",
		.owner = THIS_MODULE,
	},
};








static int __init globlefifo_driver_init(void)
{
	return platform_driver_register(&globlefifo);
}



static void __exit globlefifo_driver_exit(void)
{
	platform_driver_unregister(&globlefifo);

}









module_init(globlefifo_driver_init);
module_exit(globlefifo_driver_exit);
MODULE_AUTHOR("YU");
MODULE_LICENSE("GPL v2");