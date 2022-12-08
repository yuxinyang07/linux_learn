#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/sysfs.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm-generic/ioctl.h>

#include <linux/miscdevice.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/of.h>
#define MLX90640_MAGIC  'b'
#define MLX90640_GET_DATA    _IOWR(MLX90640_MAGIC,0x1a,unsigned long )
#define MLX90640_SET_DATA    _IOW(MLX90640_MAGIC,0x1b, unsigned long)

#define LED_NUM_ON        _IOW('L',0x1122,int)
#define LED_NUM_OFF        _IOW('L',0x3344,int)
#define LED_ALL_ON        _IO('L',0x1234)
#define LED_ALL_OFF        _IO('L',0x5678)

#define DRV_NAME  "mlx90640"
#define BUFFSIZE 1664
struct MLX90640_DATA
{
    uint16_t reg;
    uint16_t size;
    uint16_t buff[BUFFSIZE];
};

struct w_MLX90640_DATA
{
    uint16_t reg;
    uint16_t size;
    uint16_t data;
};
struct i2c_client *mClient;

int  i2c_master_recv_(const struct i2c_client *client,char *msgbuf,uint16_t *data,uint16_t nMemAddressRead)
{
    struct i2c_adapter *adap = client->adapter;
    struct i2c_msg msg[2];
    int ret;
    uint16_t bytesRemaining = nMemAddressRead * 2 ;
    int cnt = 0;
    int i = 0;
    uint16_t *p =data;
    char i2cData[1664];

    msg[0].addr = client->addr;
    msg[0].flags = I2C_M_TEN;
    msg[0].len = 2;
    msg[0].buf = msgbuf;

    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD | I2C_M_NOSTART;
    msg[1].len = bytesRemaining;
    msg[1].buf = i2cData;
    memset(i2cData,0,bytesRemaining);
    ret = i2c_transfer(adap,msg,2);
    for(cnt = 0; cnt < nMemAddressRead; cnt++)
    {
        i = cnt << 1;
        *p++ = ((uint16_t)i2cData[i] << 8) | i2cData[i+1];
    }
    return 0;
}

static int mlx90640_write_data(struct i2c_client *client,uint16_t reg,uint16_t buf)
{
    int ret =0;
    unsigned char  buff[4];
    struct i2c_adapter *adap; 
    adap = client->adapter;
    buff[0] = reg >> 8;
    buff[1] = reg;
    buff[2] = buf >>8;
    buff[3] = buf;

    ret = i2c_master_send(client,buff,4);
    return ret;
}

static int mlx90640_read_data(struct i2c_client *client ,uint16_t  reg,uint16_t *data,uint16_t nMemAddressRead)
{
    int ret;
    u8 msgbuf[2];
    msgbuf[0] = reg >> 8;
    msgbuf[1] = reg;
    ret = i2c_master_recv_(client,msgbuf,data,nMemAddressRead);
    return ret;
}


static long mlx90640_unlocked_ioctl(struct file *file,unsigned int cmd ,unsigned long arg)
{
    struct MLX90640_DATA data[1];
    struct w_MLX90640_DATA w_data[1];
    uint16_t buff[1664];
  //  uint16_t *buff = kzalloc(1664,GFP_KERNEL);
//    if(cmd >= SE_IOC_SIZE)
//        return -EINVAL;
    printk("test~~~~~~~~~~~~~~~~~~~~~~~");
#if 1
    switch(cmd)
    {
    case MLX90640_GET_DATA:
        if(copy_from_user((void*)data,(void __user*)arg,(sizeof(struct MLX90640_DATA)))){
                printk("copy_from_user error \n");
                return -EFAULT;
            }
            if(mlx90640_read_data(mClient,data->reg,data->buff,data->size)!= 0){
                printk("read data failed ,reg:0x%x ,size :%d \n",data->reg,data->size);
                return -EFAULT;
            }
            memcpy(&buff,data->buff,1664);
            if(copy_to_user((void __user*)arg ,(void *)data,sizeof(struct MLX90640_DATA))){
                printk("copy_to_user error \n");
                return -EFAULT; 
            }
            break;
    case MLX90640_SET_DATA:
            if(copy_from_user((void*)w_data,(void __user*)arg,sizeof(struct w_MLX90640_DATA))){
                printk("copy_from_user error \n");
                return -EFAULT;
            }
            mlx90640_write_data(mClient,w_data->reg,w_data->data);
            break;
        default:
            printk("cmd: 0x%x error \n",cmd);
            break;
            
    }
#else

    int num = arg+2;
    printk("--------^_^ %s------------\n",__FUNCTION__);
    switch(cmd){
        case LED_NUM_ON:        //将某个灯点亮
			printk("copy_fr!\n");
            break;
        case LED_NUM_OFF:        //将某个灯点灭掉
		printk("copy!\n");

            break;
        case LED_ALL_ON:        //两个灯同时亮
           printk("cop!\n");

            break;
        case LED_ALL_OFF:        //两个灯同时灭
		printk("copy_from_user !\n");

            break;
        default:
            printk("unknow cmd!\n");
    }


#endif
   // kfree(buff);
    return 0;
}
static int mlx90640_open(struct inode*  inode, struct file* file)
{
    printk("open device \n");
    return 0;
    
}

static struct file_operations mlx90640_fop  = {
    .owner = THIS_MODULE,
    .open = mlx90640_open,
    .unlocked_ioctl = mlx90640_unlocked_ioctl,
    .compat_ioctl = mlx90640_unlocked_ioctl,
};


static struct miscdevice mlx90640_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "mlx90640-dev",
    .fops = &mlx90640_fop,
};

static struct of_device_id mlx90640_of_match[] = {
    {.compatible = "mlx90640"},
    {}
};
static const struct i2c_device_id mlx90640_id[] = {
    {DRV_NAME ,0 },
    {}
};


static int mlx90640_probe(struct i2c_client *client ,const struct i2c_device_id *id)
{
    int ret = 0;
    printk("probe driver \n");
    mClient = client;
    if(!i2c_check_functionality(client->adapter,I2C_FUNC_I2C)){
        ret = -ENODEV;
        printk("i2c_check_functionality fail \n");
            kfree(client);
            return ret;
    }
    ret = misc_register(&mlx90640_miscdev);
    if(ret < 0) {
        printk("register misc device fail \n");
        return ret;
    }
    return 0;
    
}

static int mlx90640_remove(struct i2c_client *client)
{
    misc_deregister(&mlx90640_miscdev);
    return 0;
}




static struct i2c_driver mlx90640_driver = {
    .driver ={
		.owner = THIS_MODULE,
        .name = DRV_NAME,
        .of_match_table = mlx90640_of_match,
    },
    .id_table = mlx90640_id,
    .probe = mlx90640_probe,
    .remove = mlx90640_remove,
};






static int __init mlx90640_init(void)
{
    int ret = 0;
    ret = i2c_add_driver(&mlx90640_driver);
    if(ret < 0){
        printk("i2c_add_driver fail \n");
        return ret;
    }
    printk("init ret ;%d \n",ret);
    return ret;
}

static void  __exit mlx90640_exit(void)
{
    i2c_del_driver(&mlx90640_driver);
}







module_init(mlx90640_init)
module_exit(mlx90640_exit)
MODULE_DESCRIPTION("i2c mlx90640 support");
MODULE_LICENSE("GPL v2");
