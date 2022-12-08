//ͷ�ļ�
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm-generic/ioctl.h>

#define LED_NUM_ON        _IOW('L',0x1122,int)
#define LED_NUM_OFF        _IOW('L',0x3344,int)
#define LED_ALL_ON        _IO('L',0x1234)
#define LED_ALL_OFF        _IO('L',0x5678)




//���������----����豸������
struct s5pv210_led{
    unsigned int major;
    struct class * cls;
    struct device * dev;
    int data;
};
struct s5pv210_led *led_dev;

volatile unsigned long *gpco_conf;
volatile unsigned long *gpco_data;


//ʵ���豸�����ӿ�
int led_open(struct inode *inode, struct file *filp)
{
    
    printk("--------^_^ %s------------\n",__FUNCTION__);

    return 0;
}
ssize_t led_write(struct file *filp, const char __user *buf, size_t size, loff_t *flags)
{
    int ret;
    printk("--------^_^ %s------------\n",__FUNCTION__);

    //Ӧ�ÿռ�����ת��Ϊ�ں˿ռ�����
    ret = copy_from_user(&led_dev->data,buf,size);
    if(ret != 0){
        printk("copy_from_user error!\n");
        return -EFAULT;
    }


    return size;
}

long led_ioctl(struct file *filp, unsigned int cmd , unsigned long args)
{
    int num = args+2;
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

    return 0;
}

int led_close(struct inode *inode, struct file *filp)
{
    printk("--------^_^ %s------------\n",__FUNCTION__);
      return 0;
}


static struct file_operations fops = {
    .open = led_open,
    .write = led_write,
    .unlocked_ioctl = led_ioctl,
    .release = led_close,
};


//���غ�����ж�غ���
static int __init led_init(void)   //���غ���-----������������ʱִ��
{
    int ret;
    printk("--------^_^ %s------------\n",__FUNCTION__);
    //0,ʵ�����豸����
    //����1 ---- Ҫ����Ŀռ�Ĵ�С
    //����2 ---- ����Ŀռ�ı�ʶ
    led_dev = kzalloc(sizeof(struct s5pv210_led),GFP_KERNEL);
    if(IS_ERR(led_dev)){
        printk("kzalloc error!\n");
        ret = PTR_ERR(led_dev);
        return -ENOMEM;
    }
    
    //1,�����豸��
    //��̬�������豸��
    led_dev->major = register_chrdev(0,"led_drv",&fops);
    if(led_dev->major < 0){
        printk("register_chrdev error!\n");
        ret =  -EINVAL;
        goto err_kfree;
    }

    //2,�����豸�ļ�-----/dev/led1
    led_dev->cls = class_create(THIS_MODULE,"led_cls");
    if(IS_ERR(led_dev->cls)){
        printk("class_create error!\n");
        ret = PTR_ERR(led_dev->cls);
        goto err_unregister;
    }
    
    led_dev->dev = device_create(led_dev->cls,NULL,MKDEV(led_dev->major,0),NULL,"led");
    if(IS_ERR(led_dev->dev)){
        printk("device_create error!\n");
        ret = PTR_ERR(led_dev->dev);
        goto err_class;
    }


    //3,Ӳ����ʼ��----��ַӳ��

    
    return 0;

err_class:
    class_destroy(led_dev->cls);

err_unregister:
    unregister_chrdev(led_dev->major,"led_drv");
    
err_kfree:
    kfree(led_dev);
    return ret;

    
}

static void __exit led_exit(void)   //ж�غ���-----��������ж��ʱִ��
{
    printk("--------^_^ %s------------\n",__FUNCTION__);
    device_destroy(led_dev->cls,MKDEV(led_dev->major,0));
    class_destroy(led_dev->cls);
    unregister_chrdev(led_dev->major,"led_drv");
    kfree(led_dev);
}

//��������֤
module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");