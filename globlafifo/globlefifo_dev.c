#include <linux/module.h>
#include <linux/platform_device.h>





struct platform_device globle_fifo= {

	.name = "globle_fifo",
	.id = -1,	
 
		
	
};
struct globle_fifo *globlefifo;

static int __init  globle_fifo_init(void)
{
	int ret;
	ret = platform_device_register(&globlefifo);
	if(ret < 0){
		printk("register platform device failed \n");
		return -1;
	}
	return 0;
}


static void  __exit  globle_fifo_exit(void)
{
	platform_device_unregister(&globlefifo);
}









module_init(globle_fifo_init);
module_exit(globle_fifo_exit);
MODULE_AUTHOR("YU");
MODULE_LICENSE("GPL v2");


