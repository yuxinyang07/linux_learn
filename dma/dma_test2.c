
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/dmaengine.h>
#include <linux/uaccess.h>	/* For copy_to_user/put_user/... */
#include <linux/dma-mapping.h>
 
 
#define DRIVER_NAME 		"axidma"
#define AXIDMA_IOC_MAGIC 			'A'
#define AXIDMA_IOCGETCHN			_IO(AXIDMA_IOC_MAGIC, 0)
#define AXIDMA_IOCCFGANDSTART 		_IO(AXIDMA_IOC_MAGIC, 1)
#define AXIDMA_IOCGETSTATUS 		_IO(AXIDMA_IOC_MAGIC, 2)
#define AXIDMA_IOCRELEASECHN 		_IO(AXIDMA_IOC_MAGIC, 3)
 
struct dma_chan *dmaTest_chan = NULL;
static int dmaTest_Flag = -1;
#define DMA_STATUS_UNFINISHED	0
#define DMA_STATUS_FINISHED		1 
static void *src11;
static u64 src_phys;
 
#define BUF_SIZE  (512)
static void *dst11;
static u64 dst_phys;
 
static int axidma_open(struct inode *inode, struct file *file)
{
	printk("Open: do nothing\n");
	
	return 0;
}
 
static int axidma_release(struct inode *inode, struct file *file)
{
	printk("Release: do nothing\n");
	return 0;
}
 
static ssize_t axidma_write(struct file *file, const char __user *data, size_t len, loff_t *ppos)
{
	printk("Write: do nothing\n");
	return 0;
}
 
static void dma_complete_func(void *status)
{
	printk("dma_complete_func dma_complete!\n");
}
 
static long axidma_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct dma_device *dma_dev;
	struct dma_async_tx_descriptor *tx = NULL;
	dma_cap_mask_t mask;
	dma_cookie_t cookie;
	enum dma_ctrl_flags flags;
	unsigned char *su1, *su2;
	unsigned char count;
 
	switch(cmd)
	{
		case AXIDMA_IOCGETCHN:
		{ 
			dma_cap_zero(mask);
			dma_cap_set(DMA_MEMCPY, mask); 
			dmaTest_chan = dma_request_channel(mask, NULL, NULL);
			if(!dmaTest_chan){
				printk("dma request channel failed\n");
				goto error;
			}
 
			printk("dmaTest_chan = %d\n",dmaTest_chan->chan_id);
			break;
		}
		case AXIDMA_IOCCFGANDSTART:
		{			
			dma_dev = dmaTest_chan->device;
			src11 = dma_alloc_wc(dma_dev->dev, BUF_SIZE, &src_phys, GFP_KERNEL);
			if (NULL == src11)
			{
				printk("can't alloc buffer for src\n");
				return -ENOMEM;
			}
			
			dst11 = dma_alloc_wc(dma_dev->dev, BUF_SIZE, &dst_phys, GFP_KERNEL);
			if (NULL == dst11)
			{
				printk("can't alloc buffer for dst\n");
				return -ENOMEM;
			}
			memset(src11, 0xAA, BUF_SIZE);
			memset(dst11, 0x55, BUF_SIZE);
 
			flags = DMA_CTRL_ACK | DMA_PREP_INTERRUPT;
      		tx=dma_dev->device_prep_dma_memcpy(dmaTest_chan, dst_phys, src_phys, BUF_SIZE, flags);
			if(!tx){
				printk("Failed to prepare DMA memcpy\n");
				goto error;
			}
 
			//printk("\r\nsrc_addr = 0x%llx, dst_addr = 0x%llx,  len = %d\r\n", src_phys, dst_phys, BUF_SIZE);
 
			tx->callback = dma_complete_func;
			dmaTest_Flag = DMA_STATUS_UNFINISHED;
			tx->callback_param = &dmaTest_Flag;
			cookie =  dmaengine_submit(tx);
			if(dma_submit_error(cookie)){
				printk("Failed to dma tx_submit\n");
				goto error;
			}
			dma_async_issue_pending(dmaTest_chan);
			break;
		}
		case AXIDMA_IOCGETSTATUS:
		{
			if (memcmp(src11, dst11, BUF_SIZE) == 0)
			{
				printk("COPY OK\n");
			}
			else
			{
				printk("COPY ERROR\n");
			}
			
			for (su1 = src11, su2 = dst11, count = 0; count < 16; ++su1, ++su2, count++)
			{
				//printk("%x %x\n", *su1, *su2);
			}
			
			break;
		}
		case AXIDMA_IOCRELEASECHN:
		{
			dma_release_channel(dmaTest_chan);
			dmaTest_Flag = DMA_STATUS_UNFINISHED;
		}
		break;
		default:
			printk("Don't support cmd [%d]\n", cmd);
			break;
	}
	return 0;
error:
	return -EFAULT;
}
 
/*
 *    Kernel Interfaces
 */
 
static struct file_operations axidma_fops = {
    .owner        = THIS_MODULE,
    .llseek        = no_llseek,
    .write        = axidma_write,
    .unlocked_ioctl = axidma_unlocked_ioctl,
    .open        = axidma_open,
    .release    = axidma_release,
};
 
static struct miscdevice axidma_miscdev = {
    .minor        = MISC_DYNAMIC_MINOR,
    .name        = DRIVER_NAME,
    .fops        = &axidma_fops,
};
 
static int __init axidma_init(void)
{
    int ret = 0;
 
    ret = misc_register(&axidma_miscdev);
    if(ret) {
        printk (KERN_ERR "cannot register miscdev (err=%d)\n", ret);
		return ret;
    }
 
	return 0;
}
 
static void __exit axidma_exit(void)
{    
    misc_deregister(&axidma_miscdev);
}
 
module_init(axidma_init);
module_exit(axidma_exit);
 
MODULE_AUTHOR("ygy");
MODULE_DESCRIPTION("Axi Dmac Driver");

MODULE_LICENSE("GPL");
