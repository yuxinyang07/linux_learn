#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>

#define FPAG_CMD_CNT	1
#define FPAG_CMD_NAME	"fpga_cmd"

typedef struct  {
    dev_t devid;				/* 设备号 */
    struct cdev cdev;			/* cdev */
    struct class* class;		/* 类 */
    struct device* device;		/* 设备 */
    struct device_node* nd; 	/* 设备节点 */
    int major;					/* 主设备号 */
    void* private_data;			/* 私有数据 */
}fpga_cmd_dev_t;
static fpga_cmd_dev_t fpga_cmd_dev = {0};

/*
 * 读取16bit寄存器
 */
static int spi_read_halfword_regs(fpga_cmd_dev_t* dev, uint8_t reg, void* buf, int len)
{
    int ret = -1;
    unsigned char txdata[4];
    unsigned char* rxdata;
    struct spi_message m;
    struct spi_transfer* t;
    struct spi_device* spi = (struct spi_device*)dev->private_data;

    if (len != 2)
    {
        return -EINVAL;
    }

    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);	/* 申请内存 */
    if (!t) {
        return -ENOMEM;
    }

    rxdata = kzalloc(sizeof(char) * (len + 2), GFP_KERNEL);	/* 申请内存 */
    if (!rxdata) {
        goto out1;
    }

    txdata[0] = 0x03;		    /* 发送数据 */
    txdata[1] = reg;
    txdata[2] = 0xFF;
    txdata[3] = 0xFF;
    t->tx_buf = txdata;			/* 要发送的数据 */
    t->rx_buf = rxdata;			/* 要读取的数据 */
    t->len = len + 2;		    /* t->len=发送的长度+读取的长度 */
    spi_message_init(&m);		/* 初始化spi_message */
    spi_message_add_tail(t, &m);/* 将spi_transfer添加到spi_message队列 */
    ret = spi_sync(spi, &m);	/* 同步发送 */
    if (ret) {
        goto out2;
    }

    memcpy(buf, rxdata + 2, len);  /* 只需要读取的数据 */

out2:
    kfree(rxdata);					/* 释放内存 */
out1:
    kfree(t);						/* 释放内存 */
    return ret;
}

/*
 * 写入16bit寄存器
 */
static int spi_write_halfword_regs(fpga_cmd_dev_t* dev, uint8_t reg, void* buf, int len)
{
    int ret = -1;
    uint8_t* inbuf = (uint8_t*)buf;
    unsigned char txdata[4];
    struct spi_message m;
    struct spi_transfer* t;
    struct spi_device* spi = (struct spi_device*)dev->private_data;
    
    if (len != 2)
    {
        return -EINVAL;
    }
    
    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);	/* 申请内存 */
    if (!t) {
        return -ENOMEM;
    }

    txdata[0] = 0x02;		    /* 发送数据 */
    txdata[1] = reg;
    txdata[2] = inbuf[0];
    txdata[3] = inbuf[1];
    t->tx_buf = txdata;			/* 要发送的数据 */
    t->rx_buf = NULL;			/* 要读取的数据 */
    t->len = len + 2;		    /* t->len=发送的长度+读取的长度 */
    spi_message_init(&m);		/* 初始化spi_message */
    spi_message_add_tail(t, &m);/* 将spi_transfer添加到spi_message队列 */
    ret = spi_sync(spi, &m);	/* 同步发送 */
    if (ret) {
        goto out1;
    }
out1:
    kfree(t);						/* 释放内存 */
    return ret;
}


static int fpga_cmd_open(struct inode* inode, struct file* filp)
{
    filp->private_data = &fpga_cmd_dev; /* 设置私有数据 */
    return 0;
}

static ssize_t fpga_cmd_read(struct file* filp, char __user* buf, size_t cnt, loff_t* off)
{
    uint8_t data[16];
    int read_byte = 0;
    long err = 0;
    int ret;
    fpga_cmd_dev_t* dev = (fpga_cmd_dev_t*)filp->private_data;
    if (cnt == 2)
    {
        ret = spi_read_halfword_regs(dev, (uint8_t)*off, data, cnt);
        err = copy_to_user(buf, data, cnt);
        read_byte = cnt;
    }
    return read_byte;
}
ssize_t fpga_cmd_write(struct file* filp, const char __user* buf, size_t cnt, loff_t* off)
{
    int ret;
    int write_byte= 0;
    long err = 0;
    uint8_t data[16];
    fpga_cmd_dev_t* dev = (fpga_cmd_dev_t*)filp->private_data;
    if (cnt == 2)
    {
        err = copy_from_user(data, buf, cnt);
        ret = spi_write_halfword_regs(dev, (uint8_t)*off, data, cnt);
        write_byte = cnt;
    }
    return write_byte;
}

static int fpga_cmd_release(struct inode* inode, struct file* filp)
{
    return 0;
}

loff_t fpga_cmd_llseek(struct file* filp, loff_t off, int whence)
{
    int err = 0;
    loff_t new_offset;
    switch (whence)
    {
    case SEEK_SET:
    case SEEK_CUR:
    case SEEK_END:
        new_offset = off;
        break;
    default:
        err = -EINVAL;
        return err;
    }
    /* 更新文件指针 */
    filp->f_pos = new_offset;
    /* 如果成功，返回当前位置 */
    return new_offset;
}

/* file操作函数 */
static const struct file_operations fpga_cmd_ops = {
    .owner = THIS_MODULE,
    .llseek = fpga_cmd_llseek,
    .open = fpga_cmd_open,
    .read = fpga_cmd_read,
    .write = fpga_cmd_write,
    .release = fpga_cmd_release,
};

static int fpga_cmd_probe(struct spi_device* spi)
{
    int ret;
    /* 1、构建设备号 */
    if (fpga_cmd_dev.major) {
        fpga_cmd_dev.devid = MKDEV(fpga_cmd_dev.major, 0);
        register_chrdev_region(fpga_cmd_dev.devid, FPAG_CMD_CNT, FPAG_CMD_NAME);
    }
    else {
        alloc_chrdev_region(&fpga_cmd_dev.devid, 1, FPAG_CMD_CNT, FPAG_CMD_NAME);
        fpga_cmd_dev.major = MAJOR(fpga_cmd_dev.devid);
    }

    /* 2、注册设备 */
    fpga_cmd_dev.cdev.owner = THIS_MODULE;
    cdev_init(&fpga_cmd_dev.cdev, &fpga_cmd_ops);
    ret = cdev_add(&fpga_cmd_dev.cdev, fpga_cmd_dev.devid, FPAG_CMD_CNT);
    if (ret) {
        return -EBUSY;
    }
    /* 3、创建类 */
    fpga_cmd_dev.class = class_create(THIS_MODULE, FPAG_CMD_NAME);
    if (IS_ERR(fpga_cmd_dev.class)) {
        ret = PTR_ERR(fpga_cmd_dev.class);
        goto err1;
    }
    
    /* 4、创建设备 */
    fpga_cmd_dev.device = device_create(fpga_cmd_dev.class, NULL, fpga_cmd_dev.devid, NULL, FPAG_CMD_NAME);
    if (IS_ERR(fpga_cmd_dev.device)) {
        ret = PTR_ERR(fpga_cmd_dev.device);
        goto err2;
    }

    /*初始化spi_device */
    spi->mode = SPI_MODE_3;	/* MODE3，CPOL=1，CPHA=1 */
    ret = spi_setup(spi);
    if (ret < 0)
    {
        printk("FPGA CMD spi_setup err\n");
        goto err3;
    }
    fpga_cmd_dev.private_data = spi; /* 设置私有数据 */

    printk("FPGA CMD probe successful !\r\n");
    return 0;
err3:
    device_destroy(fpga_cmd_dev.class, fpga_cmd_dev.devid);
err2:
    class_destroy(fpga_cmd_dev.class);
err1:
    cdev_del(&fpga_cmd_dev.cdev);
    unregister_chrdev_region(fpga_cmd_dev.devid, FPAG_CMD_CNT);
    return ret;
}

static int fpga_cmd_remove(struct spi_device* spi)
{
    printk("FPGA CMD fpga_cmd_remove\n");
    /* 注销掉类和设备 */
    device_destroy(fpga_cmd_dev.class, fpga_cmd_dev.devid);
    class_destroy(fpga_cmd_dev.class);
    
    /* 删除设备 */
    cdev_del(&fpga_cmd_dev.cdev);
    unregister_chrdev_region(fpga_cmd_dev.devid, FPAG_CMD_CNT);
    return 0;
}

/* 传统匹配方式ID列表 */
static const struct spi_device_id fpga_cmd_id[] = {
    {"heygears,fpga_cmd", 0},
    {}
};

/* 设备树匹配列表 */
static const struct of_device_id fpga_cmd_of_match[] = {
    {.compatible = "heygears,fpga_cmd" },
    { /* Sentinel */ }
};

/* SPI驱动结构体 */
static struct spi_driver fpga_cmd_driver = {
    .probe = fpga_cmd_probe,
    .remove = fpga_cmd_remove,
    .driver = {
            .owner = THIS_MODULE,
            .name = "fpga_cmd",
            .of_match_table = fpga_cmd_of_match,
           },
    .id_table = fpga_cmd_id,
};


static int __init fpga_cmd_init(void)
{
    printk("FPGA CMD fpga_cmd_init\n");
    return spi_register_driver(&fpga_cmd_driver);
}
static void __exit fpga_cmd_exit(void)
{
    printk("FPGA CMD fpga_cmd_exit\n");

    spi_unregister_driver(&fpga_cmd_driver);
}


module_init(fpga_cmd_init);
module_exit(fpga_cmd_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zuozhongkai");

