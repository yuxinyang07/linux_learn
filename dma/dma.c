
/*
Function description:When we call dmatest_read(),it will transmit src memory data
to dst memory,then print dst memory data by dma_callback_func(void) function.
*/
#include<linux/module.h>
#include<linux/init.h>
#include<linux/fs.h>
#include<linux/sched.h>
 
#include<linux/device.h>
#include<linux/string.h>
#include<linux/errno.h>
 
#include<linux/types.h>
#include<linux/slab.h>
#include<linux/dmaengine.h>
#include<linux/dma-mapping.h>
 
#include<asm/uaccess.h>
 
 
 
#define DEVICE_NAME "dma_test"
 
unsigned char dmatest_major;
static struct class *dmatest_class;
 
struct dma_chan *chan;
 //bus address
dma_addr_t dma_src;
dma_addr_t dma_dst;
//virtual address
char *src = NULL;
char *dst = NULL ;
struct dma_device *dev;
struct dma_async_tx_descriptor *tx = NULL;
enum dma_ctrl_flags flags;
dma_cookie_t cookie;
 
 
//When dma transfer finished,this function will be called.
void dma_callback_func(void *arg)
{
	int i;
	printk("dma transfer ok.\n");
 
	for (i = 0; i < 512; ){
		printk("dst[%d]:%c ", i, dst[i]);
		i += 10;
	}
	printk("\n");
}
 
int dmatest_open(struct inode *inode, struct file *filp)
{
	return 0;
}
 
int dmatest_release(struct inode *inode, struct file *filp)
{
	return 0;
}
 
static ssize_t dmatest_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	int ret = 0;
	//alloc a desc,and set dst_addr,src_addr,data_size.
	tx = dev->device_prep_dma_memcpy(chan, dma_dst, dma_src, 512, flags);
	if (!tx){
		printk("Failed to prepare DMA memcpy");
	}
	
	tx->callback = dma_callback_func;//set call back function
	tx->callback_param = NULL;
	cookie = tx->tx_submit(tx); //submit the desc
	if (dma_submit_error(cookie)){
		printk("Failed to do DMA tx_submit");
	}
	
	printk("begin dma transfer.\n");	
	dma_async_issue_pending(chan);	//begin dma transfer
	
	return ret;
}
 
static ssize_t dmatest_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	int ret = 0;
	return ret;
}
 
static const struct file_operations dmatest_fops = {
	.owner = THIS_MODULE,
	.read = dmatest_read,
	.write = dmatest_write,
	.open = dmatest_open,
	.release = dmatest_release,
};
 
 
int dmatest_init(void)
{
	int i;
	dma_cap_mask_t mask;
    printk("%d %s \n",__LINE__,__func__);	
	//the first parameter 0 means allocate major device number automatically
	dmatest_major = register_chrdev(0, DEVICE_NAME, &dmatest_fops);
	if (dmatest_major < 0) 
		return dmatest_major;
	//create a dmatest class

   
 	dmatest_class = class_create(THIS_MODULE,DEVICE_NAME);
 	if (IS_ERR(dmatest_class))
		return -1;
	//create a dmatest device from this class
    printk("%d %s \n",__LINE__,__func__);	
   
	device_create(dmatest_class,NULL,MKDEV(dmatest_major,0),NULL,DEVICE_NAME);
	printk("device node create ok.\n");
 
	//alloc 512B src memory and dst memory
	src = dma_alloc_coherent(NULL, 512, &dma_src, GFP_KERNEL);
	printk("src = 0x%x, dma_src = 0x%x\n",src, dma_src);
	
	dst = dma_alloc_coherent(NULL, 512, &dma_dst, GFP_KERNEL);
	printk("dst = 0x%x, dma_dst = 0x%x\n",dst, dma_dst);
		
	for (i = 0; i < 512; i++){
		*(src + i) = 'a';
	}
 
	dma_cap_zero(mask);
	dma_cap_set(DMA_MEMCPY, mask);	//direction:memory to memory
	chan = dma_request_channel(mask,NULL,NULL); 	//request a dma channel
	if(chan)
		printk("dma channel id = %d\n",chan->chan_id);
	else{
		printk("dma_request_channel faild.\n");
		return -1;
	}
	
	flags = DMA_CTRL_ACK | DMA_PREP_INTERRUPT;
	dev = chan->device;
	
	return 0;
}
 
void dmatest_exit(void)
{
	unregister_chrdev(dmatest_major,DEVICE_NAME);//release major device number
	device_destroy(dmatest_class,MKDEV(dmatest_major,0));//destroy globalmem device
	class_destroy(dmatest_class);//destroy globalmem class
	
	//free memory and dma channel
	dma_free_coherent(NULL, 512, src, &dma_src);
	dma_free_coherent(NULL, 512, dst, &dma_dst);
	
	dma_release_channel(chan);
}
 
 
module_init(dmatest_init);
module_exit(dmatest_exit);
 
MODULE_LICENSE("GPL");
