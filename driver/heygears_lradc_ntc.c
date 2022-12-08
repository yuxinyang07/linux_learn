/*
 * Based on drivers/input/keyboard/sunxi-keyboard.c
 *
 * Copyright (C) 2015 Allwinnertech Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/keyboard.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/timer.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include <linux/iio/iio.h>
#include <linux/iio/machine.h>
#include <linux/iio/driver.h>

#if defined(CONFIG_PM)
#include <linux/pm.h>
#endif


#include "heygears_lradc.h"

struct sunxi_ntc_data {
    dev_t devid;            		/* 设备号 */
    struct cdev cdev;       		/* cdev   */
    struct class *class;    		/* 类     */
    struct device *device;  		/* 设备   */

	struct platform_device	*pdev;
	struct clk *mclk;
	struct clk *pclk;
	struct sunxi_adc_disc *disc;
	void __iomem *reg_base;
	int irq_num;

    uint16_t adc_val;
};

static struct sunxi_adc_disc disc_1350 = {
	.measure = 1350,
	.resol = 21,
};
static struct sunxi_adc_disc disc_1200 = {
	.measure = 1200,
	.resol = 19,
};
static struct sunxi_adc_disc disc_2000 = {
	.measure = 2000,
	.resol = 31,
};

#ifdef CONFIG_OF
/*
 * Translate OpenFirmware node properties into platform_data
 */
static struct of_device_id const sunxi_ntc_of_match[] = {
	{ .compatible = "allwinner,lradc_1350mv",
		.data = &disc_1350 },
	{ .compatible = "allwinner,lradc_1200mv",
		.data = &disc_1200 },
	{ .compatible = "allwinner,lradc_2000mv",
		.data = &disc_2000 },
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_ntc_of_match);
#else /* !CONFIG_OF */
#endif




static int hg_ntc_open(struct inode *inode, struct file *filp)
{
    struct cdev *cdev = filp->f_path.dentry->d_inode->i_cdev;
    struct sunxi_ntc_data *ntc_data = container_of(cdev, struct sunxi_ntc_data, cdev);

    return 0;
}


static ssize_t hg_ntc_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
    uint16_t adc_val;
    long err = 0;

    struct cdev *cdev = filp->f_path.dentry->d_inode->i_cdev;
    struct sunxi_ntc_data *ntc_data = container_of(cdev, struct sunxi_ntc_data, cdev);
    
    adc_val = ntc_data->adc_val;
    err = copy_to_user(buf, &adc_val, sizeof(adc_val));

    return 0;
}

static int hg_ntc_release(struct inode *inode, struct file *filp)
{
    return 0;
}
static void ntc_ctrl_set(void __iomem *reg_base,
				enum key_mode key_mode, u32 para)
{
	u32 ctrl_reg = 0;

	if (para != 0)
		ctrl_reg = readl(reg_base + LRADC_CTRL);
	if (CONCERT_DLY_SET & key_mode)
		ctrl_reg |= (FIRST_CONCERT_DLY & para);
	if (ADC_CHAN_SET & key_mode)
		ctrl_reg |= (ADC_CHAN_SELECT & para);
	if (KEY_MODE_SET & key_mode)
		ctrl_reg |= (KEY_MODE_SELECT & para);
	if (LRADC_HOLD_SET & key_mode)
		ctrl_reg |= (LRADC_HOLD_EN & para);
	if (LEVELB_VOL_SET & key_mode) {
		ctrl_reg |= (LEVELB_VOL & para);
#if defined(CONFIG_ARCH_SUN8IW18)
		ctrl_reg &= ~(u32)(3 << 4);
#endif
	}
	if (LRADC_SAMPLE_SET & key_mode)
		ctrl_reg |= (LRADC_SAMPLE_250HZ & para);
	if (LRADC_EN_SET & key_mode)
		ctrl_reg |= (LRADC_EN & para);

	writel(ctrl_reg, reg_base + LRADC_CTRL);
}

static void sunxi_ntc_int_set(void __iomem *reg_base,
                    enum int_mode int_mode, u32 para)
{
    u32 ctrl_reg = 0;

    if (para != 0)
        ctrl_reg = readl(reg_base + LRADC_INTC);

    if (ADC0_DOWN_INT_SET & int_mode)
        ctrl_reg |= (LRADC_ADC0_DOWN_EN & para);
    if (ADC0_UP_INT_SET & int_mode)
        ctrl_reg |= (LRADC_ADC0_UP_EN & para);
    if (ADC0_DATA_INT_SET & int_mode)
        ctrl_reg |= (LRADC_ADC0_DATA_EN & para);

    writel(ctrl_reg, reg_base + LRADC_INTC);
}



static u32 sunxi_ntc_read_ints(void __iomem *reg_base)
{
	u32 reg_val;

	reg_val  = readl(reg_base + LRADC_INT_STA);

	return reg_val;
}

static void sunxi_ntc_clr_ints(void __iomem *reg_base, u32 reg_val)
{
	writel(reg_val, reg_base + LRADC_INT_STA);
}

static u32 sunxi_ntc_read_data(void __iomem *reg_base)
{
	u32 reg_val;

	reg_val = readl(reg_base + LRADC_DATA0);

	return reg_val;
}

#ifdef CONFIG_PM
static int sunxi_ntc_suspend(struct device *dev)
{
    return 0;
}

static int sunxi_ntc_resume(struct device *dev)
{
    return 0;
}
#endif


static irqreturn_t sunxi_isr_ntc(int irq, void *dummy)
{
	struct sunxi_ntc_data *ntc_data = (struct sunxi_ntc_data *)dummy;
	u32 reg_val = 0;

	reg_val = sunxi_ntc_read_ints(ntc_data->reg_base);
	if (reg_val & LRADC_ADC0_DATAPEND) {

        ntc_data->adc_val = sunxi_ntc_read_data(ntc_data->reg_base) & 0x3f;
    }

	sunxi_ntc_clr_ints(ntc_data->reg_base, reg_val);

	return IRQ_HANDLED;
}


static int sunxi_ntc_probe_dt(struct sunxi_ntc_data *ntc_data,
				struct platform_device *pdev)
{
	struct device_node *np = NULL;
	int ret = 0;

	np = pdev->dev.of_node;
	if (!of_device_is_available(np)) {
		pr_err("%s: sunxi ntc is disable\n", __func__);
		return -EPERM;
	}
	ntc_data->reg_base = of_iomap(np, 0);
	if (ntc_data->reg_base == 0) {
		pr_err("%s:Failed to ioremap() io memory region.\n", __func__);
		ret = -EBUSY;
	} else
		printk("ntc base: %p !\n", ntc_data->reg_base);

	ntc_data->irq_num = irq_of_parse_and_map(np, 0);
	if (ntc_data->irq_num == 0) {
		pr_err("%s:Failed to map irq.\n", __func__);
		ret = -EBUSY;
	} else
		printk("ir irq num: %d !\n", ntc_data->irq_num);

	/* some IC will use clock gating while others HW use 24MHZ, So just try
	 * to get the clock, if it doesn't exist, give warning instead of error
	 */
	ntc_data->mclk = of_clk_get(np, 0);
	if (IS_ERR_OR_NULL(ntc_data->mclk)) {
		printk("%s: ntc has no clk.\n", __func__);
	} else{
		if (clk_prepare_enable(ntc_data->mclk)) {
			pr_err("%s enable apb1_keyadc clock failed!\n",
							__func__);
			return -EINVAL;
		}
	}
	return ret;
}

static const struct file_operations hg_ntc_ops = {
    .owner = THIS_MODULE,
    .open = hg_ntc_open,
    .read = hg_ntc_read,
    .release = hg_ntc_release,
};


static int sunxi_ntc_probe(struct platform_device *pdev)
{
	int err;
	struct sunxi_ntc_data *ntc_data = NULL;
    unsigned long mode, para;

	printk("sunxi ntc probed\n");
	
	/* 申请内存 */
	ntc_data = devm_kzalloc(&pdev->dev, sizeof(*ntc_data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(ntc_data)) {
		pr_err("ntc_data: not enough memory for ntc data\n");
		return -ENOMEM;
	}

	/* 设备树解析 */
	if (pdev->dev.of_node) {
		/* get dt and sysconfig */
		err = sunxi_ntc_probe_dt(ntc_data, pdev);
	} else {
		pr_err("sunxi ntc device tree err!\n");
		return -EBUSY;
	}

	if (err < 0)
		return err;

    /* 创建设备号 */
    err = alloc_chrdev_region(&ntc_data->devid, 0, 1, "hg_ntc");
    if (err < 0) {
        pr_err("%s Couldn't alloc_chrdev_region, ret=%d\r\n", "hg_ntc", err);
        return -ENOMEM;
    }

    /* 初始化cdev */
    ntc_data->cdev.owner = THIS_MODULE;
    cdev_init(&ntc_data->cdev, &hg_ntc_ops);

    /* 添加cdev */
    err = cdev_add(&ntc_data->cdev, ntc_data->devid, 1);
    if (err < 0)
        goto del_unregister;

    /* 创建类 */
    ntc_data->class = class_create(THIS_MODULE, "hg_ntc");
    if (IS_ERR(ntc_data->class))
        goto del_cdev;

    /* 创建设备 */
    ntc_data->device = device_create(ntc_data->class, NULL, 
            ntc_data->devid, NULL, "hg_ntc");
    if (IS_ERR(ntc_data->device))
        goto destroy_class;

    
    /* lradc初始化 */
    mode = ADC0_DATA_INT_SET;
	para = LRADC_ADC0_DATA_EN;
	sunxi_ntc_int_set(ntc_data->reg_base, mode, para);

	mode = CONCERT_DLY_SET | ADC_CHAN_SET | KEY_MODE_SET
		| LRADC_SAMPLE_SET | LRADC_EN_SET;
	para = FIRST_CONCERT_DLY | ADC_CHAN_SELECT | KEY_MODE_SELECT 
		|LRADC_SAMPLE_250HZ| LRADC_EN;
	ntc_ctrl_set(ntc_data->reg_base, mode, para);

	/* 申请lradc中断 */
	if (request_irq(ntc_data->irq_num, sunxi_isr_ntc, 0,
					"sunxintc", ntc_data)) {
		err = -EBUSY;
		pr_err("request irq failure.\n");
		goto req_irq_error;
	}

    platform_set_drvdata(pdev, ntc_data);

	printk("sunxi ntc probe end\n");
	
	return 0;
	
req_irq_error:  
    device_destroy(ntc_data->class, ntc_data->devid);
destroy_class:
    class_destroy(ntc_data->class); 
del_cdev:
    cdev_del(&ntc_data->cdev);
del_unregister:
    unregister_chrdev_region(ntc_data->devid, 1);
	return err;
}

static int sunxi_ntc_remove(struct platform_device *pdev)
{
	struct sunxi_ntc_data *ntc_data = platform_get_drvdata(pdev);

    printk("ntc driver removed\n");

	free_irq(ntc_data->irq_num, ntc_data);
    device_destroy(ntc_data->class, ntc_data->devid);
    class_destroy(ntc_data->class);
    cdev_del(&ntc_data->cdev);
    unregister_chrdev_region(ntc_data->devid, 1);
	
	return 0;
}

#ifdef CONFIG_PM
static const struct dev_pm_ops sunxi_ntc_pm_ops = {
	.suspend = sunxi_ntc_suspend,
	.resume = sunxi_ntc_resume,
};

#define SUNXI_NTC_PM_OPS (&sunxi_ntc_pm_ops)
#endif

static struct platform_driver sunxi_ntc_driver = {
	.probe  = sunxi_ntc_probe,
	.remove = sunxi_ntc_remove,
	.driver = {
		.name   = "allwinner,lradc-ntc",
		.owner  = THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= SUNXI_NTC_PM_OPS,
#endif
		.of_match_table = of_match_ptr(sunxi_ntc_of_match),
	},
};

module_platform_driver(sunxi_ntc_driver);

MODULE_AUTHOR("Suvan");
MODULE_DESCRIPTION("sunxi-lradc-ntc driver");
MODULE_LICENSE("GPL");
