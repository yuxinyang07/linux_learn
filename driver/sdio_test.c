 #include <linux/module.h>
 #include <linux/init.h>
 #include <linux/kernel.h>
 #include <linux/sched.h>
 #include <linux/mutex.h>
 #include <linux/seq_file.h>
 #include <linux/circ_buf.h>
 #include <linux/kfifo.h>
 #include <linux/slab.h>
 #include <linux/cdev.h>
 #include <asm/io.h>
 #include <asm/uaccess.h>
 #include <asm-generic/ioctl.h>
 #include <linux/mmc/core.h>
 #include <linux/mmc/card.h>
 #include <linux/mmc/sdio_func.h>
 #include <linux/mmc/sdio_ids.h>
 #include <linux/uaccess.h>

#define SDIO_DEV_NAME  "sdio_fpga"
extern void sunxi_mmc_rescan_card(unsigned ids);

struct sdio_fpgadev {
    uint32_t  magic;
    struct sdio_func  *func;
    struct cdev *c_dev;
    struct device *dev;

};

static dev_t  fpga_devid;
static struct class * fpga_class;
 


static ssize_t fpga_read(struct file *fp,char __user *buf,size_t len,loff_t *offset)
{
    return 0;
}



static ssize_t fpga_write(struct file *fp,const char __user *buf,size_t len,loff_t *offset)
{
    return 0;
}


static int fpga_open(struct inode *inode,struct file * file)
{
    return 0;
}


static int fpga_release(struct inode *inode,struct file * file)
{

    return 0;

}

static  long fpga_ioctl(struct file *file,unsigned int  cmd,unsigned long arg )
{
    return 0;
}


static  int fpga_mmap(struct file *file,struct vm_area_struct *vma)
{
    return 0;

}
static struct file_operations  fpga_ops = {
    .open = fpga_open,
    .release = fpga_release,
    .read = fpga_read,
    .write = fpga_write,
    .compat_ioctl = fpga_ioctl,
    .mmap = fpga_mmap,
};



static int fpga_probe(struct sdio_func *port,const struct sdio_device_id  *dev_id)
{
    int ret;
    printk("module probe \n");
    struct sdio_fpgadev  *fpgadev;
    fpgadev = devm_kzalloc(&port->dev,sizeof(struct sdio_fpgadev),GFP_KERNEL);
    if(!fpgadev){
        printk("kmalloc falled \n");
        return -ENOMEM;
    }
    fpgadev->magic = 0x12345678;
    fpgadev->func = port;
    sdio_set_drvdata(port,fpgadev);

    fpgadev->dev = device_create(fpga_class,NULL,fpga_devid,NULL,"fpga_dev");
    fpgadev->c_dev = cdev_alloc();
    if(!fpgadev->c_dev ){
        printk("cdev_alloc failed  \n");
        return -ENOMEM;

    }
    cdev_init(fpgadev->c_dev,&fpga_ops);
    ret = cdev_add(fpgadev->c_dev,fpga_devid,1);
    if(ret < 0){
        goto err;    
    }

    return ret;

err:
    device_destroy(fpga_class,fpga_devid);

    return ret;



}

void fpga_remove(struct sdio_func * port)
{

}


static struct sdio_device_id fpga_ids[] = {
    {SDIO_DEVICE_CLASS(SDIO_CLASS_UART)},
    {},

};

struct  sdio_driver  fpga_sdio =  {
    .name = "fgpa-sdio",
    .probe = fpga_probe,
    .remove = fpga_remove,
    .id_table = fpga_ids,
};





static int __init sdio_fpga_init(void)
{
    int ret;

    printk("%d ,%s ~~~~~~~~~yxy~~~~~~~~~\n",__LINE__,__func__);
    sunxi_mmc_rescan_card(1);
    ret = alloc_chrdev_region(&fpga_devid,0,1,"sdio_test");
    if(ret < 0){
        return ret;
    }
    fpga_class = class_create(THIS_MODULE,SDIO_DEV_NAME);
    if(fpga_class == NULL){
        printk("create class failed \n");
        goto err1;

    }
    

    ret = sdio_register_driver(&fpga_sdio);
    if(ret < 0){
        printk("register driver failed \n");
        goto err2;
    }        
    return 0;
err2:
    class_destroy(fpga_class);
    fpga_class = NULL;


err1:
    unregister_chrdev_region(fpga_devid,1);
    fpga_devid = 0;
    
    return ret ;
}

















static void  __exit sdio_fpga_exit(void)
{
    printk("sdio fgpa driver exit \n");
    sdio_unregister_driver(&fpga_sdio);
    if(fpga_class){
        class_destroy(fpga_class);
        fpga_class =NULL;
    }
    if(fpga_devid){
        unregister_chrdev_region(fpga_devid,1);
        fpga_devid = 0;
    }

    
}









module_init(sdio_fpga_init);
module_exit(sdio_fpga_exit);

MODULE_LICENSE("GPL");

