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


#define FPGA_FIRMWARE_PATH  "fw_spi_fpga.bin"


  struct  sysfstest_dev {
    struct platform_device *pdev;
    struct cdev *cdev;
    struct device *dev;
    struct work_struct work;
    struct class  *class;
     dev_t   dev_id;
    int major;
};

struct sysfstest_dev   *sysfs_test = NULL;

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

static ssize_t sys_test(struct device *dev,struct device_attribute *attr,const char *buf ,size_t count)
{

    int ret ;
    printk("sysfs test \n");
    return ret;

}

static void work_func(struct work_struct *work)
{
    int ret = 0;
    const struct firmware *fw =NULL;
    struct sysfstest_dev *fsdev  = container_of(work,struct sysfstest_dev,work);
    ret = request_firmware(&fw,FPGA_FIRMWARE_PATH,&sysfs_test->pdev->dev);
    if(ret == 0 ){
        printk("request_firmwar success \n");
    }

}

static DEVICE_ATTR(test_fw, S_IWUSR, NULL, sys_test);
static int sysfstest_probe(struct platform_device *pdev)
{


     int ret;
     int status = 0 ;

     sysfs_test  = devm_kzalloc(&pdev->dev, sizeof(struct sysfstest_dev), GFP_KERNEL);
#if 1
     ret = alloc_chrdev_region(&sysfs_test->dev_id,0,1,"sysfs_test");
     if(ret < 0){
         printk("register chrdev failed \n");
         return -1;

     }
     sysfs_test->major = MAJOR(sysfs_test->dev_id);
     sysfs_test->cdev = cdev_alloc();
     cdev_init(sysfs_test->cdev,&sysfs_fops);
     ret = cdev_add(sysfs_test->cdev,sysfs_test->dev_id,1);
     if(ret){
         printk("add cdev failed \n");
         status = -1;
         goto err1;
     }
     sysfs_test->class = class_create(THIS_MODULE,"sysfs_test");
     if(IS_ERR(sysfs_test->class)){
         printk("class curete failed \n");
         status = -1;
         goto err2;
     }
     sysfs_test->dev = device_create(sysfs_test->class,NULL,MKDEV(sysfs_test->major,0),NULL,"sysfs_test");
     sysfs_test->pdev = pdev;
    if(device_create_file(&pdev->dev,&dev_attr_test_fw)){
        printk("creat_file failed \n");
    }
     if(sysfs_test->dev == NULL){
         printk("device create filed \n");
         status = -1;
         goto err3;
     }

     INIT_WORK(&sysfs_test->work,work_func);
     schedule_work(&sysfs_test->work);
    return status;
err3:
    class_destroy(sysfs_test->class);
    sysfs_test->class = 0;


err2:
    cdev_del(sysfs_test->cdev);


err1:
    unregister_chrdev_region(sysfs_test->dev_id,1);
#endif
    return status;
 
}


static int sysfstest_remove(struct platform_device *pdev)
{
    return 0;

}

static const struct of_device_id sysfstest_ids[] = {
    { .compatible = "sysfstest-ids"},
    {}
};


static struct platform_driver sysfstest_driver = {
    .driver = {
        .name = "sysfs_test",
        .of_match_table = of_match_ptr(sysfstest_ids),

    },
    .probe = sysfstest_probe,
    .remove = sysfstest_remove,
};




static  int __init sysfstest_init(void )
{
       return platform_driver_register(&sysfstest_driver);
}






static void  __exit sysfstest_exit(void )
{
    return platform_driver_unregister(&sysfstest_driver);
}






module_init(sysfstest_init);
module_exit(sysfstest_exit);
MODULE_LICENSE("GPL");

