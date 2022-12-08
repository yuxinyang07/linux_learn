#include <linux/fs.h>        /*����file_operation�ṹ��*/
#include <linux/init.h>      /* ����module_init module_exit */
#include <linux/module.h>    /* ����LICENSE�ĺ� */
#include <linux/miscdevice.h>/*����miscdevice�ṹ��*/
#include <linux/kernel.h>    /*����printk�Ȳ�������*/
#include <asm/uaccess.h>     /*����copy_to_user��������*/
#include <linux/interrupt.h> /*����request_irq��������*/
#include <linux/of.h>        /*�豸��������صĺ���*/

#define MLX90640_MAGIC  'x'
#define TEST_MAX_NR 3 
#define MLX90640_GET_DATA    _IO(MLX90640_MAGIC,1 )
#define MLX90640_SET_DATA    _IO(MLX90640_MAGIC,2 )
#define TEST_KBUF _IO(TEST_MAGIC, 3)
#define LED_NUM_ON        _IOW('L',0x1122,int)
#define LED_NUM_OFF        _IOW('L',0x3344,int)
#define LED_ALL_ON        _IO('L',0x1234)
#define LED_ALL_OFF        _IO('L',0x5678)

#include <linux/of_gpio.h>   /*of_get_named_gpio�Ⱥ���*/
#include <linux/i2c.h>       /*I2C������غ���*/
#define BUFFSIZE 1664
struct i2c_client *mClient;
struct MLX90640_DATA
{
    uint16_t reg;
    uint16_t size;
    uint16_t buff[BUFFSIZE];
};

struct w_MLX90640_DATA
{
    uint16_t reg;
    uint16_t data;
};


void *private_data;	/* ˽������ */

static int tmp1075_read_regs(struct i2c_client *client, u8 reg, void *val, int len)
{
	int ret;
	struct i2c_msg msg[2];

	/* msg[0]Ϊ����Ҫ��ȡ���׵�ַ */
	msg[0].addr = client->addr;					/* ap3216c��ַ */
	msg[0].flags = 0;					/* ���Ϊ�������� */
	msg[0].buf = &reg;					/* ��ȡ���׵�ַ */
	msg[0].len = 1;						/* reg����*/

	/* msg[1]��ȡ���� */
	msg[1].addr = client->addr;			/* ap3216c��ַ */
	msg[1].flags = I2C_M_RD;			/* ���Ϊ��ȡ����*/
	msg[1].buf = val;					/* ��ȡ���ݻ����� */
	msg[1].len = len;					/* Ҫ��ȡ�����ݳ���*/

	ret = i2c_transfer(client->adapter, msg, 2);
	if(ret == 2) {
		ret = 0;
	} else {
		printk("i2c rd failed=%d reg=0x%x len=%d\n",ret, reg, len);
		ret = -EREMOTEIO;
	}
	return ret;
}
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
if(_IOC_TYPE(cmd) != MLX90640_MAGIC) return - EINVAL;
if(_IOC_NR(cmd) > TEST_MAX_NR) return - EINVAL;
	struct MLX90640_DATA *dev = (struct MLX90640_DATA *)arg;
	 struct i2c_client *client = file->private_data;
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
            if(mlx90640_read_data(client,data->reg,data->buff,data->size)!= 0){
                printk("read data failed ,reg:0x%x ,size :%d \n",data->reg,data->size);
                return -EFAULT;
            }
            memcpy(&buff,data->buff,1664);
            if(copy_to_user((void __user*)arg ,(void *)data,sizeof(struct MLX90640_DATA))){
                printk("copy_to_user error \n");
                return -EFAULT; 
            }
			printk("copy!\n");
            break;
    case MLX90640_SET_DATA:
			printk("copy!\n");
			printk("%x############\n",dev->reg);
			if(copy_from_user((void*)w_data,(void __user*)arg,sizeof(struct w_MLX90640_DATA))){
                printk("copy_from_user error \n");
                return -EFAULT;
            }
				printk("%x \n",w_data->reg);
				printk("%x \n",w_data->data);
            mlx90640_write_data(client,w_data->reg,w_data->data);
            break;
        default:
            printk("cmd: 0x%x error \n",cmd);
            break;
            
    }
#else

    int num = arg+2;
    printk("--------^_^ %s------------\n",__FUNCTION__);
    switch(cmd){
        case LED_NUM_ON:        //��ĳ���Ƶ���
			printk("copy_fr!\n");
            break;
        case LED_NUM_OFF:        //��ĳ���Ƶ����
		printk("copy!\n");

            break;
        case LED_ALL_ON:        //������ͬʱ��
           printk("cop!\n");

            break;
        case LED_ALL_OFF:        //������ͬʱ��
		printk("copy_from_user !\n");

            break;
        default:
            printk("unknow cmd!\n");
    }


#endif
   // kfree(buff);
    return 0;
}
static int tmp1075_open(struct inode *inode, struct file *file)
{
	file->private_data = private_data;
	return 0;
}
/* ����һ�����豸��,read���� */
//��IICĳ���Ĵ������裺д��Ĵ����������ַ��ʱ����Ϊд������ֵ�������ַ��ʱ����Ϊд����
ssize_t tmp1075_read(struct file *file, char __user *array, size_t size, loff_t *ppos)
{
	long res;
	unsigned char data[2];
	struct i2c_client *dev = (struct i2c_client *)file->private_data;
	tmp1075_read_regs(dev,0x00,data,2);
	res = copy_to_user(array, data, sizeof(data));
	return 0;
}

/*�ַ��豸�����������Ϊ����Ӳ����file_operations�ṹ��д��������*/
static const struct file_operations tmp1075_ctl={
         .owner          = THIS_MODULE,
		 .open           = tmp1075_open,
		 .read           = tmp1075_read,
		.unlocked_ioctl = mlx90640_unlocked_ioctl,
		.compat_ioctl = mlx90640_unlocked_ioctl,
};

/*�����豸�����豸��Ϊ10���ַ��豸�������ͨ�ַ��豸��ʹ�ø���*/
static struct miscdevice tmp1075_miscdev = {
         .minor          = 255,
         .name           = "mlx90640-dev",//�豸�ڵ�����
         .fops           = &tmp1075_ctl,
};
/*i2c������probe���������������豸ƥ���Ժ�˺����ͻ�ִ��*/
static int tmp1075_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	char res;
	/*ע�������豸����*/
	printk("ap3216c_probe\r\n");
	printk("ap3216c_probe addr111 =%x\r\n",client->addr);
	res = misc_register(&tmp1075_miscdev);
	printk(KERN_ALERT"tmp1075_probe = %d\n",res);
	private_data = client;
	return 0;
}
/*i2c������remove�������Ƴ�i2c������ʱ��˺�����ִ��*/
static int tmp1075_remove(struct i2c_client *client)
{
	/*�ͷ������豸*/
	misc_deregister(&tmp1075_miscdev);
	printk("tmp1075_remove\r\n");
	return 0;
}
/* ��ͳƥ�䷽ʽID�б� */
static const struct i2c_device_id tmp1075_id[] = {
	{"mlx90640", 0},  
	{}
};
/* �豸��ƥ���б� */
static const struct of_device_id tmp1075_of_match[] = {
	{ .compatible = "mlx90640" },
	{ /* Sentinel */ }
};

/* i2c�����ṹ�� */	
static struct i2c_driver tmp1075_driver = {
	.probe = tmp1075_probe,
	.remove = tmp1075_remove,
	.driver = {
			.owner = THIS_MODULE,
		   	.name = "mlx90640",//i2c�������ֺ�i2c�豸����ƥ��һ�£����ܽ���probe��������platformһ��
		   	.of_match_table = tmp1075_of_match, //compatible����ƥ���豸��
		   },
	.id_table = tmp1075_id,
};
static int __init tmp1075_init(void)
{
	int res;
	res = i2c_add_driver(&tmp1075_driver);
	printk("tmp1075_init = %d \r\n",res);
	return 0;
}

static void __exit tmp1075_exit(void)
{
	i2c_del_driver(&tmp1075_driver);
	printk(KERN_ALERT"tmp1075_exit\r\n");
}

/*����ģ��ļ��غ�ж�����*/
module_init(tmp1075_init);
module_exit(tmp1075_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yxy ");
MODULE_DESCRIPTION("i2c mlx90640 support");