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




  struct  sysfs_dev {
    struct cdev *cdev;
    struct device *dev;
    struct class  *class;
     dev_t   dev_id;
    int major;
};

struct sysfs_dev   *sysfs = NULL;

 int sysfs_open(struct inode* inode,struct file* file )
{
    printk("sysfs_open() \n");
    return 0;
}


static int sysfs_release(struct inode* inode,struct file* file )
{
    printk("sysfs_release() \n");
    return 0;
}


 ssize_t sysfs_read(struct file *file,char __user* buf ,size_t len ,loff_t *off)
{
    printk("sysfs_read() \n");
    return 0;
}


ssize_t sysfs_write(struct file *file,const char __user* buf ,size_t len ,loff_t *off)
{
    printk("sysfs_write() \n");
    return 0;

}


static    struct  file_operations sysfs_fops =  {
    .owner = THIS_MODULE,
    .open  = sysfs_open,
    .release = sysfs_release,
    .read = sysfs_read,
    .write = sysfs_write,
};





static  int __init sysfstest_init(void )
{
     int ret;
     int status = 0 ;
     printk("%s %d \n ",__func__,__LINE__);
     sysfs  = devm_kzalloc(sysfs->dev, sizeof(struct sysfs_dev), GFP_KERNEL);
     printk("%s %d \n ",__func__,__LINE__);
#if 1
     ret = alloc_chrdev_region(&sysfs->dev_id,0,1,"sysfs_test");
     if(ret < 0){
         printk("register chrdev failed \n");
         return -1;

     }
     printk("%s %d \n ",__func__,__LINE__);
     sysfs->major = MAJOR(sysfs->dev_id);
     printk("%s %d \n ",__func__,__LINE__);
     cdev_init(sysfs->cdev,&sysfs_fops);
     printk("%s %d \n ",__func__,__LINE__);
     ret = cdev_add(sysfs->cdev,sysfs->dev_id,1);
     printk("%s %d \n ",__func__,__LINE__);
     if(ret){
         printk("add cdev failed \n");
         status = -1;
         goto err1;
     }
     printk("%s %d \n ",__func__,__LINE__);
     sysfs->class = class_create(THIS_MODULE,"sysfs_test");
     if(IS_ERR(sysfs->class)){
         printk("class curete failed \n");
         status = -1;
         goto err2;
     }
     printk("%s %d \n ",__func__,__LINE__);
     sysfs->dev = device_create(sysfs->class,NULL,MKDEV(sysfs->major,0),NULL,"sysfs_test");
     if(sysfs->dev == NULL){
         printk("device create filed \n");
         status = -1;
         goto err3;
     }
          
    return status;
err3:
    class_destroy(sysfs->class);
    sysfs->class = 0;


err2:
    cdev_del(sysfs->cdev);


err1:
    unregister_chrdev_region(sysfs->dev_id,1);
#endif
    return status;
    
}





static void  __exit sysfs_exit(void )
{



}







module_init(sysfstest_init);
module_exit(sysfs_exit);
MODULE_LICENSE("GPL");

