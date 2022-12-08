/*
* SPI master driver using generic GPIOs for FPGA load program
*
* Copyright (C) 2022  Shunfan Wang @Heygears
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

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
#include <linux/spi/spi.h>
#include "heygears_spi_fpga.h"


#ifndef DRIVER_NAME
#define DRIVER_NAME         "spi_fpga"
#endif

#define FPGA_FIRMWARE_PATH  "fw_spi_fpga.bin"
#define RETRY_COUNT  3 //升级失败后重试次数
DECLARE_COMPLETION(spi_update_completion); //静态创建一个和初始化一个完成量。
EXPORT_SYMBOL_GPL(spi_update_completion); //导出完成量符号表给sdio_fpga使用
static struct  mutex status_lock;

struct fasync_struct *fpga_fasync; //异步信号结构体变量



struct spi_fpga_dev {
    struct spi_fpga_gpio *spi_gpios;
    struct cdev *cdev;
    struct platform_device *pdev;
    struct work_struct work;
    bool   work_func_flags;
    uint8_t *fw_data;
    dev_t devid;
    int major;
    struct class *fpga_dev_class;
};
//升级状态标志
typedef enum {
    UPDATE_SUCCESS = 0, //升级完成
    UPDATE_FAILED,      //升级失败
    UPDATEING,          //升级中
    UPDATEING_RETRY,    //升级失败重试中
}fpga_update_status;

fpga_update_status status;

struct spi_fpga_dev *spi_fpga = NULL;

static int fpga_update_fasync(int fd,struct file *file,int on)
{
    if(fasync_helper(fd,file,on,&fpga_fasync)>=0){
        return 0;
    }else{
        return -1;
    }
        
}

static int fpga_open(struct inode* inode,struct file * file)
{
    printk("device open \n");
    return 0;
}


ssize_t fpga_status_read(struct file *file,char __user *buf,size_t size,loff_t *ppos)
{
    int fd;
    char data;
    mutex_lock(&status_lock);
    data =(char )status;
    mutex_unlock(&status_lock);
    fd = copy_to_user(buf,(char *)&status,sizeof(status));
    if(fd < 0){
        return -1;
    }
    return 0;
}


static struct file_operations fpga_fops = {
    .owner = THIS_MODULE,
    .open = fpga_open,
    .read = fpga_status_read,
    .fasync = fpga_update_fasync,
};






static inline void setsck(struct spi_fpga_gpio *spi_gpios, int is_on)
{
    gpio_set_value_cansleep(spi_gpios->sck, is_on);
}

static inline void setmosi(struct spi_fpga_gpio *spi_gpios, int is_on)
{
    gpio_set_value_cansleep(spi_gpios->mosi, is_on);
}

static inline void spi_chipselect(struct spi_fpga_gpio *spi_gpios, int is_select)
{
    gpio_set_value_cansleep(spi_gpios->ncs, is_select);
}

static inline int getmiso(struct spi_fpga_gpio *spi_gpios)
{
    return !!gpio_get_value_cansleep(spi_gpios->miso);
}


// MSB
static int spi_fpga_write_cmd(struct spi_fpga_gpio *spi_gpios, uint8_t data_out)
{
    int i = 0;
    uint8_t data_in = 0;

    for(i = 0; i < 8; i++)
    {
        data_in <<= 1;

		//setsck(spi_gpios, 0);
		if (data_out & 0x80)
			setmosi(spi_gpios, 1);
		else
			setmosi(spi_gpios, 0);
		setsck(spi_gpios, 0);
		setsck(spi_gpios, 1);

		if (getmiso(spi_gpios) != 0)
			data_in |= 1;

        data_out <<= 1;
    }

    return data_in;
}


//data:LSB or MSB
static int spi_fpga_write_data(struct spi_fpga_gpio *spi_gpios, uint8_t data_out)
{
#if 1   // MSB
    return spi_fpga_write_cmd(spi_gpios, data_out);
#else   // data LSB
    int i = 0;
    uint8_t data_in = 0;

    for(i = 0; i < 8; i++)
    {
        data_in >>= 1;

		//setsck(spi_gpios, 0);
		if (data_out & 0x01)
			setmosi(spi_gpios, 1);
		else
			setmosi(spi_gpios, 0);
		setsck(spi_gpios, 0);
		setsck(spi_gpios, 1);

		if (getmiso(spi_gpios) != 0)
			data_in |= 0x80;
        
        data_out >>= 1;
    }

    return data_in;
#endif
}


static int spi_fpga_write_operand(struct spi_fpga_gpio *spi_gpios)
{
    int i = 0;

    for(i = 0; i < 24; i++)
    {
        setmosi(spi_gpios, 0);
        setsck(spi_gpios,  0);
        setsck(spi_gpios,  1);
    }
    
    setsck(spi_gpios, 0);

    return 0;
}


int spi_fpga_trans_active(struct spi_fpga_dev *spi_fpga, u8 *trans_data, unsigned long len)
{
    unsigned int i = 0;

    // cs high level and sck low level
	spi_chipselect(spi_fpga->spi_gpios, SPI_CS_NOSELECT);
	setsck(spi_fpga->spi_gpios, 0);
	
	// cs low level
	spi_chipselect(spi_fpga->spi_gpios, SPI_CS_SELECT);
	
	// send active data
	for(i = 0; i < len; i++)
	{
		spi_fpga_write_cmd(spi_fpga->spi_gpios, trans_data[i]);
	}
	
	// sck pull low level
	setsck(spi_fpga->spi_gpios, 0);
	
	// cs pull high level
	spi_chipselect(spi_fpga->spi_gpios, SPI_CS_NOSELECT);
	
	return 0;

}


int spi_fpga_trans(struct spi_fpga_dev *spi_fpga, u8 cmd, u8 *trans_data, unsigned long len, uint32_t delay_ms)
{
    unsigned int i = 0;

    // CS High
    spi_chipselect(spi_fpga->spi_gpios, SPI_CS_NOSELECT);
    setsck(spi_fpga->spi_gpios, 0);

    // CS Low
    spi_chipselect(spi_fpga->spi_gpios, SPI_CS_SELECT);

    // send cmd
    spi_fpga_write_cmd(spi_fpga->spi_gpios, cmd);

    // send operand
    spi_fpga_write_operand(spi_fpga->spi_gpios);

    // special cmd need sleep between operand and data
    if (delay_ms != 0)
         msleep(delay_ms);

    // send data
    for(i = 0; i < len; i++)
    {
        spi_fpga_write_data(spi_fpga->spi_gpios, trans_data[i]);
    }

    // sck pull low level
    setsck(spi_fpga->spi_gpios, 0);

    // cs pull high level
    spi_chipselect(spi_fpga->spi_gpios, SPI_CS_NOSELECT);

    return 0;
}


static int spi_fpga_alloc(unsigned pin, const char *label, bool is_in)
{
    int value;

    value = gpio_request(pin, label);
    if (value == 0) 
    {
        if (is_in)
            value = gpio_direction_input(pin);
        else
            value = gpio_direction_output(pin, 0);
    }

    return value;
}


static int spi_fpga_request(struct spi_fpga_gpio *spi_gpios,
                const char *label, u16 *res_flags)
{
    int value;

    if (spi_gpios->mosi != SPI_GPIO_NO_MOSI) {
        value = spi_fpga_alloc(spi_gpios->mosi, label, false);
        if (value)
            goto done;
    } else {
        /* HW configuration without MOSI pin */
        *res_flags |= SPI_MASTER_NO_TX;
    }

    if (spi_gpios->miso != SPI_GPIO_NO_MISO) {
        value = spi_fpga_alloc(spi_gpios->miso, label, true);
        if (value)
            goto free_mosi;
    } else {
        /* HW configuration without MISO pin */
        *res_flags |= SPI_MASTER_NO_RX;
    }

    value = spi_fpga_alloc(spi_gpios->sck, label, false);
    if (value)
        goto free_miso;

    value = spi_fpga_alloc(spi_gpios->ncs, label, false);
    if (value)
        goto free_sck;

    value = spi_fpga_alloc(spi_gpios->prog, label, false);
    if (value)
        goto free_ncs;

    value = spi_fpga_alloc(spi_gpios->init, label, true);
    if (value)
        goto free_prog;
    
    value = spi_fpga_alloc(spi_gpios->done, label, true);
    if (value)
        goto free_init;
    
    value = spi_fpga_alloc(spi_gpios->power, label, false);
    if (value)
        goto free_done;
    else
        gpio_set_value(spi_gpios->power, 1);
  
    goto done;

free_done:
    gpio_free(spi_gpios->done);
free_init:
    gpio_free(spi_gpios->init);
free_prog:
    gpio_free(spi_gpios->prog);
free_ncs:
    if (spi_gpios->ncs != SPI_GPIO_NO_NCS)
        gpio_free(spi_gpios->ncs);
free_sck:
    if (spi_gpios->sck != SPI_GPIO_NO_SCK)
        gpio_free(spi_gpios->sck);
free_miso:
    if (spi_gpios->miso != SPI_GPIO_NO_MISO)
        gpio_free(spi_gpios->miso);
free_mosi:
    if (spi_gpios->mosi != SPI_GPIO_NO_MOSI)
        gpio_free(spi_gpios->mosi);
done:
    return value;
}


static int spi_fpga_free(struct spi_fpga_gpio *spi_gpios)
{
    gpio_set_value_cansleep(spi_gpios->power, 0);

    gpio_free(spi_gpios->power);
    gpio_free(spi_gpios->done);
    gpio_free(spi_gpios->init);
    gpio_free(spi_gpios->prog);

    if (spi_gpios->ncs != SPI_GPIO_NO_NCS)
         gpio_free(spi_gpios->ncs);

    if (spi_gpios->sck != SPI_GPIO_NO_SCK)
         gpio_free(spi_gpios->sck);

    if (spi_gpios->miso != SPI_GPIO_NO_MISO)
         gpio_free(spi_gpios->miso);

    if (spi_gpios->mosi != SPI_GPIO_NO_MOSI)
         gpio_free(spi_gpios->mosi);

    return 0;
}


#ifdef CONFIG_OF
static const struct of_device_id spi_fpga_dt_ids[] = {
    { .compatible = "spi-fpga" },
    {}
};
MODULE_DEVICE_TABLE(of, spi_fpga_dt_ids);


static int spi_fpga_probe_dt(struct platform_device *pdev)
{
    int ret;
    struct spi_fpga_gpio   *pdata;
    struct device_node *np = pdev->dev.of_node;
    const struct of_device_id *of_id =
            of_match_device(spi_fpga_dt_ids, &pdev->dev);

    if (!of_id)
        return 1;

    pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
    if (!pdata)
        return -ENOMEM;

    ret = of_get_named_gpio(np, "gpio-sck", 0);
    if (ret < 0) {
        dev_err(&pdev->dev, "gpio-sck property not found\n");
        goto error_free;
    }
    pdata->sck = ret;

    ret = of_get_named_gpio(np, "gpio-miso", 0);
    if (ret < 0) {
        dev_info(&pdev->dev, "gpio-miso property not found, switching to no-rx mode\n");
        pdata->miso = SPI_GPIO_NO_MISO;
    } else
        pdata->miso = ret;

    ret = of_get_named_gpio(np, "gpio-mosi", 0);
    if (ret < 0) {
        dev_info(&pdev->dev, "gpio-mosi property not found, switching to no-tx mode\n");
        pdata->mosi = SPI_GPIO_NO_MOSI;
    } else
        pdata->mosi = ret;

    ret = of_get_named_gpio(np, "cs-gpios", 0);
    if (ret < 0) {
        dev_info(&pdev->dev, "cs-gpios property not found\n");
        goto error_free;
    } else
        pdata->ncs = ret;

    ret = of_get_named_gpio(np, "gpio-prog", 0);
    if (ret < 0) {
        dev_info(&pdev->dev, "gpio-prog property not found\n");
        goto error_free;
    } else
        pdata->prog = ret;

    ret = of_get_named_gpio(np, "gpio-init", 0);
    if (ret < 0) {
        dev_info(&pdev->dev, "gpio-init property not found\n");
        goto error_free;
    } else
        pdata->init = ret;

    ret = of_get_named_gpio(np, "gpio-done", 0);
    if (ret < 0) {
        dev_info(&pdev->dev, "gpio-done property not found\n");
        goto error_free;
    } else
        pdata->done = ret;

    ret = of_get_named_gpio(np, "gpio-power", 0);
    if (ret < 0) {
        dev_info(&pdev->dev, "gpio-power property not found\n");
        goto error_free;
    } else
        pdata->power = ret;
    
    pdev->dev.platform_data = pdata;
    
    return 0;

error_free:
    devm_kfree(&pdev->dev, pdata);
    return ret;
}
#else
static inline int spi_fpga_probe_dt(struct platform_device *pdev)
{
    return 0;
}
#endif
//通过检测fpga GPIO口判断fpga是否升级成功
static int check_fpga_update_sccess(struct spi_fpga_dev *spi_fpga)
{
    int ret,i;
    for(i = 0;i < 500;i++){
        ret = gpio_get_value(spi_fpga->spi_gpios->done);
        if(ret){
            printk("fpga_update sccess ret = %d \n",ret);
            break;
        }else{
          //  printk("fpga_update failed  ret = %d \n",ret);
        }
    }
    return ret;

}



static int update_fpga_fw(struct spi_fpga_dev *spi_fpga, const struct firmware *fw)
{   
    int i = 0;
    size_t len = fw->size;
    uint8_t *trans_data = (uint8_t*)fw->data;
    uint8_t sdr_data[8] = {0x00};
    uint8_t active_data[4] = {0xA4, 0xC6, 0xF4, 0x8A};      // config fpga sspi key

    // enter config
    gpio_set_value(spi_fpga->spi_gpios->prog, 1);
    udelay(10);
    gpio_set_value(spi_fpga->spi_gpios->prog, 0);
    udelay(1);
    spi_fpga_trans_active(spi_fpga, active_data, sizeof(active_data));
    gpio_set_value(spi_fpga->spi_gpios->prog, 1);

    // check init
    for(i = 0; i < 500; i++)
    {
        udelay(1);
        if (gpio_get_value(spi_fpga->spi_gpios->init))
        {
            printk("[spi-foga] fpga sspi init success!\r\n");
            break;
        }
        else
        {
            //printk("[spi-foga] fpga sspi init failed!\r\n");
        }
    }
    
    msleep(50);
    // check the idcode
    spi_fpga_trans(spi_fpga, LSC_READ_IDCODE, sdr_data, sizeof(sdr_data) - 4, 0); 

    // check the password
    spi_fpga_trans(spi_fpga, LSC_READ_STATUS, sdr_data, sizeof(sdr_data), 0); 
    
    // enable sram transparent mode
    spi_fpga_trans(spi_fpga, LSC_ENABLE_X, NULL, 0, 0); 
    msleep(1);

    // check the sram erase lock
    spi_fpga_trans(spi_fpga, LSC_READ_STATUS, sdr_data, sizeof(sdr_data), 0); 
    
    // check the sram program lock
    spi_fpga_trans(spi_fpga, LSC_READ_STATUS, sdr_data, sizeof(sdr_data), 0); 
    
    // enable programing  mode
    spi_fpga_trans(spi_fpga, ISC_ENABLE, NULL, 0, 0); 
    msleep(1);
    
    // erase the device
    spi_fpga_trans(spi_fpga, ISC_ERASE, NULL, 0, 0); 
    msleep(1000);
    spi_fpga_trans(spi_fpga, LSC_READ_STATUS, sdr_data, sizeof(sdr_data), 0); 
    
    // program fuse map
    spi_fpga_trans(spi_fpga, LSC_INIT_ADDR, NULL, 0, 0); 
    msleep(1000);
    spi_fpga_trans(spi_fpga, LSC_BITSTREAM_BURST, trans_data, len, 0); 

    // verify usercode
    spi_fpga_trans(spi_fpga, LSC_READ_USERCODE, sdr_data, sizeof(sdr_data) - 4, 1); 
    
    // verify status register
    msleep(2000);
    spi_fpga_trans(spi_fpga, LSC_READ_STATUS, sdr_data, sizeof(sdr_data), 0); 

    // exit the programming mode
    spi_fpga_trans(spi_fpga, ISC_DISABLE, NULL, 0, 0); 
    msleep(1000);

    // check done
#if 0
    for(i = 0; i < 500; i++)
    {
        udelay(1);
        if (gpio_get_value(spi_fpga->spi_gpios->done))
        {
            printk("[spi-foga] fpga load %s done!\r\n", FPGA_FIRMWARE_PATH);
            break;
        }
        else
        {
            //printk("[spi-foga] fpga load %s failed!\r\n", FPGA_FIRMWARE_PATH);
        }
    }
#endif

   // sdio_uart_init();

    return 0;
}


static void work_func(struct work_struct *work)
{
    int ret = 0;
    int count = 0;
    const struct firmware *fw = NULL;
    struct spi_fpga_dev *spi_fpga = container_of(work, struct spi_fpga_dev, work);
    // 申请用户空间固件
    ret = request_firmware(&fw, FPGA_FIRMWARE_PATH, &spi_fpga->pdev->dev);
    if (ret) 
    {
        goto out;
    }
    else 
    {
        mutex_lock(&status_lock);
        status=UPDATEING;
        mutex_unlock(&status_lock);
        printk("[spi-fpga]:fpga firmware size is %lu bytes.\r\n", fw->size);
retry:
        count++;
        // 模拟spi接口升级fpga固件
        update_fpga_fw(spi_fpga, fw);
        
        if(!check_fpga_update_sccess(spi_fpga)){
            if(count <= RETRY_COUNT)
            {
                mutex_lock(&status_lock);
                status=UPDATEING_RETRY;
                mutex_unlock(&status_lock);
                goto retry;
            }else{
                mutex_lock(&status_lock);
                status = UPDATE_FAILED;
                mutex_unlock(&status_lock);
                printk("fpga update failed \n");
                kill_fasync(&fpga_fasync,SIGIO,POLL_IN);
                goto out;

            }
        }else{
            printk("fpga update success \n");
            mutex_lock(&status_lock);
            status = UPDATE_SUCCESS;
            mutex_unlock(&status_lock);
            kill_fasync(&fpga_fasync,SIGIO,POLL_IN);
            complete(&spi_update_completion);

        }


    }

out:
    release_firmware(fw);
    fw = NULL;
}


static void update_start_up(struct spi_fpga_dev *spi_fpga)
{
    if (!work_pending(&spi_fpga->work))
        schedule_work(&spi_fpga->work);
}


static ssize_t update_fw_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
    unsigned long value;

    if (kstrtoul(buf, 10, &value))
        return -EINVAL;

    if (value == 1)
    {
        update_start_up(spi_fpga);
    }

    return count;
}


static DEVICE_ATTR(update_fw, S_IWUSR, NULL, update_fw_store);


static int spi_fpga_probe(struct platform_device *pdev)
{
    int status = 0;
    int err;
    u16 master_flags = 0;
    struct device *device;
    mutex_init(&status_lock);
    printk("[spi_fpga] %s(%d)\r\n", __FUNCTION__, __LINE__);
      
    // 申请内存
    spi_fpga = devm_kzalloc(&pdev->dev, sizeof(struct spi_fpga_dev), GFP_KERNEL);
    err = alloc_chrdev_region(&spi_fpga->devid, 0, 1, "spi_fpga");
    if(err < 0){
        printk("alloc_chrdev_degion error \n");
        return -1;
    }
    spi_fpga->major = MAJOR(spi_fpga->devid);
    spi_fpga->cdev = cdev_alloc();
    cdev_init(spi_fpga->cdev,&fpga_fops);
    err = cdev_add(spi_fpga->cdev,spi_fpga->devid,1);
    if(err){
        printk("add cdev failed \n");
        status = -1;
        goto err1;
    }
    //创建类,类名fpga_class
    spi_fpga->fpga_dev_class = class_create(THIS_MODULE,"fpga_class");
    if(IS_ERR(spi_fpga->fpga_dev_class)){
        printk("class create error \n");
        status = -1;
        goto err2;
    }
    //创建设备
    device = device_create(spi_fpga->fpga_dev_class,NULL,MKDEV(spi_fpga->major,0),NULL,"dev_fpga");
    if(device == NULL){
        printk("deivce create failed \n");
        status = -1;
        goto err3;

    }
    // 设备树gpio口解析
    status = spi_fpga_probe_dt(pdev);
    if (status < 0)
        return status;

    spi_fpga->spi_gpios = dev_get_platdata(&pdev->dev);
    if (!(spi_fpga->spi_gpios))
        return -ENODEV;
    
    // 为模拟spi接口申请gpio
    status = spi_fpga_request(spi_fpga->spi_gpios, dev_name(&pdev->dev), &master_flags);
    if (status < 0)
        return status;

#if 1
    printk("[spi-fpga]:gpio-miso=%ld, gpio-mosi=%ld, gpio-sck=%ld, gpio-cs=%ld gpio-prog=%ld gpio-init=%ld gpio-done=%ld\n",
            spi_fpga->spi_gpios->miso, spi_fpga->spi_gpios->mosi, 
			spi_fpga->spi_gpios->sck, spi_fpga->spi_gpios->ncs, 
            spi_fpga->spi_gpios->prog, spi_fpga->spi_gpios->init, spi_fpga->spi_gpios->done);
#endif

    INIT_WORK(&spi_fpga->work, work_func);
	spi_fpga->work_func_flags = false;

    // 设置平台数据
    platform_set_drvdata(pdev, spi_fpga);
    spi_fpga->pdev = pdev;
    
    // 创建sysfs节点
    if (device_create_file(&pdev->dev, &dev_attr_update_fw)) {
        spi_fpga_free(spi_fpga->spi_gpios);
        return -EINVAL;
    }
    update_start_up(spi_fpga);
    return status;
err3:
    class_destroy(spi_fpga->fpga_dev_class);
    spi_fpga->fpga_dev_class= 0;
err2:
    cdev_del(spi_fpga->cdev);
err1:
    unregister_chrdev_region(spi_fpga->devid,1);


    return status;

}


static int spi_fpga_remove(struct platform_device *pdev)
{
    device_destroy(spi_fpga->fpga_dev_class,MKDEV(spi_fpga->major,0));
    class_destroy(spi_fpga->fpga_dev_class);
    cdev_del(spi_fpga->cdev);
    unregister_chrdev_region(spi_fpga->devid,1);

    if (spi_fpga->work_func_flags)
        cancel_work_sync(&spi_fpga->work);
    
    spi_fpga_free(spi_fpga->spi_gpios);

    device_remove_file(&pdev->dev, &dev_attr_update_fw);

    return 0;
}


static struct platform_driver spi_fpga_driver = {
    .driver = {
        .name   = DRIVER_NAME,
        .of_match_table = of_match_ptr(spi_fpga_dt_ids),
    },
    .probe      = spi_fpga_probe,
    .remove     = spi_fpga_remove,
};

#if 0
module_platform_driver(spi_fpga_driver);
#else
static int __init spi_fpga_driver_init(void)
{
    return platform_driver_register(&spi_fpga_driver);
}
module_init(spi_fpga_driver_init);
 
static void __exit spi_fpga_driver_exit(void)
{
    platform_driver_unregister(&spi_fpga_driver);
}
module_exit(spi_fpga_driver_exit);
#endif

MODULE_DESCRIPTION("SPI master driver using generic GPIOs for fpga");
MODULE_AUTHOR("Shunfan Wang");
MODULE_LICENSE("GPL");                                                     
