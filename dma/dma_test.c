#include<linux/module.h>
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
#include<linux/cdev.h>
#include<linux/of_device.h>

struct dma_chan *test_chan;
struct dma_slave_config config;
dma_cookie_t  cookit;
struct cdev *cdev;
dev_t  devid;
struct device *dma_dev;
struct class  *dma_class = NULL;

dma_addr_t src_handle;    
dma_addr_t dst_handle;    

    static void* src =NULL;
    static void* dst =NULL;


int cmp( char *cs,  char*ct)
{
    unsigned char c1,c2;
    int res=0;
    int count = 100;
    do{
        c1 = *cs++;
        c2= *ct++;
        res = c1 - c2;
        if(res){
            printk("%x  !=  %x\n",c1 ,c2 );

 //           break;
        }

    }while(--count);
    return res;
}


void  callback_func(void *arg)
{
    printk("finish dma\n");
     int i;
    unsigned char *su1;
    unsigned char *su2;
    dma_sync_single_for_cpu(dma_dev,src_handle,4096,DMA_BIDIRECTIONAL);
    dma_sync_single_for_cpu(dma_dev,dst_handle,4096,DMA_BIDIRECTIONAL);
    su1 = (unsigned char * )src;
    su2 = (unsigned char * )dst;
    printk("dma trans  success\n");
//    for(i = 0;i < 100 ;i ++){
//        printk("su1 =%x su2 =%x \n",su1[i],su2[i]);
//    }

    if(!cmp(su1,su2)){
        printk("src copy success \n");
    }else{

        printk("src copy failed  \n");
    }

}

static int dma_open(struct inode * inode ,struct file *file)
{
    printk("dma open \n");

    return 0;
}


struct file_operations  dma_ops = {
    .open = dma_open,

};


static int __init  dma_test_init(void)
{
    struct dma_async_tx_descriptor *tx = NULL;
    int ret;
    int i =0;
    unsigned char *su1;
    unsigned char *su2;
     ret = alloc_chrdev_region(&devid,0,1,"dma_test");
    if(ret <0){
        printk("alloc chrdev failed \n");
        return -1;
    }
     cdev = cdev_alloc();
     cdev_init(cdev ,&dma_ops);

     if(cdev_add(cdev,devid,1) < 0){
        printk("alloc chrdev failed \n");
        return -1;
     }
    dma_class = class_create(THIS_MODULE,"dma_test") ;
    if(dma_class==NULL){
       printk("class create failed \n") ;
       return -1;
    }
    dma_dev = device_create(dma_class,NULL,devid,NULL,"dma_test") ;
    if(dma_dev==NULL){
       printk("device create failed \n") ;
       return -1;
    }
    

    of_dma_configure(dma_dev,dma_dev->of_node);

//    src = __get_free_page(GFP_KERNEL);
//    dst = __get_free_page(GFP_KERNEL);
    dma_cap_mask_t mask;
    dma_cap_zero(mask);
    dma_cap_set(DMA_MEMCPY,mask);
    test_chan =   dma_request_channel(mask,NULL,NULL);
    src = kmalloc(1024*4,GFP_KERNEL);
    dst = kmalloc(1024*4,GFP_KERNEL);
    memset(src,0xaa,4096);
    memset(dst,0x55,4096);
     su1 = (unsigned char *)src;
     su2=(unsigned char *)dst;
     for(i = 0;i < 100 ;i ++){
     printk("su1 =%x su2 =%x \n",su1[i],su2[i]);
 }
    src_handle = dma_map_single(dma_dev,src,4096,DMA_BIDIRECTIONAL);
    dst_handle = dma_map_single(dma_dev,dst,4096,DMA_BIDIRECTIONAL);
    if(dma_mapping_error(dma_dev,src_handle) || dma_mapping_error(dma_dev,dst_handle) ){
        printk("map single failed \n");
        return -1;
    }
    config.direction = DMA_MEM_TO_MEM;
    config.src_addr = src_handle;
    config.dst_addr = dst_handle;
    config.src_addr_width = 2;
    config.dst_addr_width = 2;
    config.src_maxburst = 1;
    config.dst_maxburst = 1;

    dmaengine_slave_config(test_chan,&config);

    tx = test_chan->device->device_prep_dma_memcpy(test_chan,src_handle,dst_handle,4096,DMA_CTRL_ACK | DMA_PREP_INTERRUPT);
    tx->callback = callback_func;
     cookit= dmaengine_submit(tx);
     dma_async_issue_pending(test_chan);
    return 0;
}












static void __exit dma_test_exit(void)
{
        dma_unmap_single(dma_dev,src_handle,4096,DMA_BIDIRECTIONAL);
    dma_unmap_single(dma_dev,dst_handle,4096,DMA_BIDIRECTIONAL);
    dma_release_channel(test_chan);
   

    kfree(src);
    kfree(dst); 
    device_destroy(dma_class,devid);
    class_destroy(dma_class);
    unregister_chrdev_region(devid,1);
    
}
















module_init(dma_test_init);
module_exit(dma_test_exit);
MODULE_LICENSE("GPL");

