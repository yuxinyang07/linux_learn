 #include <linux/fs.h>
 #include <linux/module.h>
 #include <linux/moduleparam.h>
 #include <linux/init.h>
 #include <linux/vmalloc.h>
 #include <linux/blkdev.h>
 #include <linux/genhd.h>
 #include <linux/errno.h>
 #include <linux/hdreg.h>
 #include <linux/version.h>

#define MY_DEVICE_NAME "ramdisk"

static  int   diskmb =256;
static int mybdrv_ma_no,disk_size;
static unsigned  char *ramdisk;
static spinlock_t  lock;
static struct request_queue * my_request_queue;
unsigned short sector_size = 512;
struct gendisk * my_gd;


static void my_request(struct request_queue *q)
{

}



static int   my_ioctl(struct block_device *bdev,fmode_t mode,unsigned int cmd, 
        unsigned  long arg)

{
    return 0;
}


static const struct block_device_operations mybdrv_fops  = {
    .owner = THIS_MODULE,
    .ioctl = my_ioctl,
};







static  int  __init  ramdisk_init(void)
{

    disk_size = diskmb * 1024*1024;
    spin_lock_init(&lock);
    ramdisk = vmalloc(disk_size);
    if(!ramdisk){
        return -ENOMEM;
    }
    my_request_queue =  blk_init_queue(my_request,&lock);
    if(!my_request_queue){
        vfree(ramdisk);
        return -ENOMEM;
    }
    blk_queue_logical_block_size(my_request_queue,sector_size);
    mybdrv_ma_no = register_blkdev(0,MY_DEVICE_NAME); 
    if(mybdrv_ma_no < 0){
        pr_err("failed registering mybvdr   %d  \n",mybdrv_ma_no);
        vfree(ramdisk);
        return mybdrv_ma_no;
    }
    my_gd = alloc_disk(16);
    if(!my_gd){
        unregister_blkdev(mybdrv_ma_no,MY_DEVICE_NAME);
        vfree(ramdisk);
        return -ENOMEM;

    }
    my_gd->major = mybdrv_ma_no;
    my_gd->first_minor = 0;
    my_gd->fops = &mybdrv_fops;
    strcpy(my_gd->disk_name,MY_DEVICE_NAME);
    my_gd->queue = my_request_queue;
    set_capacity(my_gd,disk_size/sector_size);
    add_disk(my_gd);
    
    

    return 0;

}











static  void  __exit   ramdisk_exit(void)
{
    



}



















module_init(ramdisk_init);
module_exit(ramdisk_exit);

MODULE_LICENSE("GPL");





