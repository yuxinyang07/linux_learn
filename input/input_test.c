#include <linux/kernel.h>
#include <linux/dmi.h>
#include <linux/firmware.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/acpi.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/fs.h>
#include <asm/unaligned.h>
#include <asm/uaccess.h>
#include <linux/namei.h>
#include <linux/platform_device.h>



static int  input_test_probe(struct platform_device * dev)
{
    
    return 0;
}



static int input_test_remove(struct platform_device *dev)
{
    return 0;
}





static const  struct of_device_id  input_test_match[] ={
    {"key_input"},
    {}
};








static struct platform_driver key_input = {
    .probe = input_test_probe,
    .remove = input_test_remove,
    .driver = {
        .name = "key_input",
        .of_match_table = of_match_ptr(input_test_match),
    },
};












module_platform_driver(key_input)

MODULE_LICENSE("GPL");
