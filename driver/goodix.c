/*
 *  Driver for Goodix Touchscreens
 *
 *  Copyright (c) 2014 Red Hat Inc.
 *  Copyright (c) 2015 K. Merker <merker@debian.org>
 *
 *  This code is based on gt9xx.c authored by andrew@goodix.com:
 *
 *  2010 - 2012 Goodix Technology.
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; version 2 of the License.
 */

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
#define DEBUG

struct goodix_ts_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	int abs_x_max;
	int abs_y_max;
	bool swapped_x_y;
	bool inverted_x;
	bool inverted_y;
	unsigned int max_touch_num;
	unsigned int int_trigger_type;
	int cfg_len;
	struct gpio_desc *gpiod_int;
	struct gpio_desc *gpiod_rst;
	u16 id;
	u16 version;
	const char *cfg_name;
	struct completion firmware_loading_complete;
	unsigned long irq_flags;
};

#define GOODIX_GPIO_INT_NAME		"goodix_int"
#define GOODIX_GPIO_RST_NAME		"goodix_rst"

#define GOODIX_MAX_HEIGHT		4096
#define GOODIX_MAX_WIDTH		4096
#define GOODIX_INT_TRIGGER		1
#define GOODIX_CONTACT_SIZE		8
#define GOODIX_MAX_CONTACTS		10

#define GOODIX_CONFIG_MAX_LENGTH	240
#define GOODIX_CONFIG_911_LENGTH	186
#define GOODIX_CONFIG_967_LENGTH	228

/* Register defines */
#define GOODIX_REG_COMMAND		0x8040
#define GOODIX_CMD_SCREEN_OFF		0x05

#define GOODIX_READ_COOR_ADDR		0x814E
#define GOODIX_REG_CONFIG_DATA		0x8047
#define GOODIX_REG_ID			0x8140

#define RESOLUTION_LOC		1
#define MAX_CONTACTS_LOC	5
#define TRIGGER_LOC		6

static const unsigned long goodix_irq_flags[] = {
	IRQ_TYPE_EDGE_RISING,
	IRQ_TYPE_EDGE_FALLING,
	IRQ_TYPE_LEVEL_LOW,
	IRQ_TYPE_LEVEL_HIGH,
};

/*
 * Those tablets have their coordinates origin at the bottom right
 * of the tablet, as if rotated 180 degrees
 */
static const struct dmi_system_id rotated_screen[] = {
#if defined(CONFIG_DMI) && defined(CONFIG_X86)
	{
		.ident = "Teclast X89",
		.matches = {
			/* tPAD is too generic, also match on bios date */
			DMI_MATCH(DMI_BOARD_VENDOR, "TECLAST"),
			DMI_MATCH(DMI_BOARD_NAME, "tPAD"),
			DMI_MATCH(DMI_BIOS_DATE, "12/19/2014"),
		},
	},
	{
		.ident = "WinBook TW100",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "WinBook"),
			DMI_MATCH(DMI_PRODUCT_NAME, "TW100")
		}
	},
	{
		.ident = "WinBook TW700",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "WinBook"),
			DMI_MATCH(DMI_PRODUCT_NAME, "TW700")
		},
	},
#endif
	{}
};

/**
 * goodix_i2c_read - read data from a register of the i2c slave device.
 *
 * @client: i2c device.
 * @reg: the register to read from.
 * @buf: raw write data buffer.
 * @len: length of the buffer to write
 */
static int goodix_i2c_read(struct i2c_client *client,
			   u16 reg, u8 *buf, int len)
{
	struct i2c_msg msgs[2];
	u16 wbuf = cpu_to_be16(reg);
	int ret;
	msgs[0].flags = 0;
	msgs[0].addr  = client->addr;
	msgs[0].len   = 2;
	msgs[0].buf   = (u8 *)&wbuf;

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = client->addr;
	msgs[1].len   = len;
	msgs[1].buf   = buf;
	ret = i2c_transfer(client->adapter, msgs, 2);
	return ret < 0 ? ret : (ret != ARRAY_SIZE(msgs) ? -EIO : 0);
}

/**
 * goodix_i2c_write - write data to a register of the i2c slave device.
 *
 * @client: i2c device.
 * @reg: the register to write to.
 * @buf: raw data buffer to write.
 * @len: length of the buffer to write
 */
static int goodix_i2c_write(struct i2c_client *client, u16 reg, const u8 *buf,
			    unsigned len)
{
	u8 *addr_buf;
	struct i2c_msg msg;
	int ret;

	addr_buf = kmalloc(len + 2, GFP_KERNEL);
	if (!addr_buf)
		return -ENOMEM;

	addr_buf[0] = reg >> 8;
	addr_buf[1] = reg & 0xFF;
	memcpy(&addr_buf[2], buf, len);

	msg.flags = 0;
	msg.addr = client->addr;
	msg.buf = addr_buf;
	msg.len = len + 2;

	ret = i2c_transfer(client->adapter, &msg, 1);
	kfree(addr_buf);
	return ret < 0 ? ret : (ret != 1 ? -EIO : 0);
}

static int goodix_i2c_write_u8(struct i2c_client *client, u16 reg, u8 value)
{
	return goodix_i2c_write(client, reg, &value, sizeof(value));
}

static int goodix_get_cfg_len(u16 id)
{
	switch (id) {
	case 911:
	case 9271:
	case 9110:
	case 927:
	case 928:
		return GOODIX_CONFIG_911_LENGTH;

	case 912:
	case 967:
		return GOODIX_CONFIG_967_LENGTH;

	default:
		return GOODIX_CONFIG_MAX_LENGTH;
	}
}

static int goodix_ts_read_input_report(struct goodix_ts_data *ts, u8 *data)
{
	int touch_num;
	int error;

	error = goodix_i2c_read(ts->client, GOODIX_READ_COOR_ADDR, data,
				GOODIX_CONTACT_SIZE + 1);
	if (error) {
		dev_err(&ts->client->dev, "I2C transfer error: %d\n", error);
		return error;
	}

	if (!(data[0] & 0x80))
		return -EAGAIN;

	touch_num = data[0] & 0x0f;
	if (touch_num > ts->max_touch_num)
		return -EPROTO;

	if (touch_num > 1) {
		data += 1 + GOODIX_CONTACT_SIZE;
		error = goodix_i2c_read(ts->client,
					GOODIX_READ_COOR_ADDR +
						1 + GOODIX_CONTACT_SIZE,
					data,
					GOODIX_CONTACT_SIZE * (touch_num - 1));
		if (error)
			return error;
	}

	return touch_num;
}

static void goodix_ts_report_touch(struct goodix_ts_data *ts, u8 *coor_data)
{
	int id = coor_data[0] & 0x0F;
	int input_x = get_unaligned_le16(&coor_data[1]);

	int input_y = get_unaligned_le16(&coor_data[3]);
	int input_w = get_unaligned_le16(&coor_data[5]);

//	input_x = ts->abs_x_max - input_x;
	/* Inversions have to happen before axis swapping */
	if (ts->inverted_x)
		input_x = ts->abs_x_max - input_x;
	if (ts->inverted_y)
		input_y = ts->abs_y_max - input_y;
	if (ts->swapped_x_y)
		swap(input_x, input_y);

	input_mt_slot(ts->input_dev, id);
	input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, true);
	input_report_abs(ts->input_dev, ABS_MT_POSITION_X, input_x);
	input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, input_y);
	input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, input_w);
	input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, input_w);
}

/**
 * goodix_process_events - Process incoming events
 *
 * @ts: our goodix_ts_data pointer
 *
 * Called when the IRQ is triggered. Read the current device state, and push
 * the input events to the user space.
 */
static void goodix_process_events(struct goodix_ts_data *ts)
{
	u8  point_data[1 + GOODIX_CONTACT_SIZE * GOODIX_MAX_CONTACTS];
	int touch_num;
	int i;

	touch_num = goodix_ts_read_input_report(ts, point_data);
	if (touch_num < 0)
		return;

	for (i = 0; i < touch_num; i++)
		goodix_ts_report_touch(ts,
				&point_data[1 + GOODIX_CONTACT_SIZE * i]);

	input_mt_sync_frame(ts->input_dev);
	input_sync(ts->input_dev);
}

/**
 * goodix_ts_irq_handler - The IRQ handler
 *
 * @irq: interrupt number.
 * @dev_id: private data pointer.
 */
static irqreturn_t goodix_ts_irq_handler(int irq, void *dev_id)
{
	struct goodix_ts_data *ts = dev_id;
	goodix_process_events(ts);
	if (goodix_i2c_write_u8(ts->client, GOODIX_READ_COOR_ADDR, 0) < 0)
		dev_err(&ts->client->dev, "I2C write end_cmd error\n");

	return IRQ_HANDLED;
}

static void goodix_free_irq(struct goodix_ts_data *ts)
{
	devm_free_irq(&ts->client->dev, ts->client->irq, ts);
}

static int goodix_request_irq(struct goodix_ts_data *ts)
{
	return devm_request_threaded_irq(&ts->client->dev, ts->client->irq,
					 NULL, goodix_ts_irq_handler,
					 ts->irq_flags, ts->client->name, ts);
}

/**
 * goodix_check_cfg - Checks if config fw is valid
 *
 * @ts: goodix_ts_data pointer
 * @cfg: firmware config data
 */
static int goodix_check_cfg(struct goodix_ts_data *ts,
			    const struct firmware *cfg)
{
	int i, raw_cfg_len;
	u8 check_sum = 0;

	if (cfg->size > GOODIX_CONFIG_MAX_LENGTH) {
		dev_err(&ts->client->dev,
			"The length of the config fw is not correct");
		return -EINVAL;
	}

	raw_cfg_len = cfg->size - 2;
	for (i = 0; i < raw_cfg_len; i++)
		check_sum += cfg->data[i];
	check_sum = (~check_sum) + 1;
	if (check_sum != cfg->data[raw_cfg_len]) {
        printk("raw_cfg_len =%d  checksum = %x cfg->data[raw_cfg_len] = %x \n",raw_cfg_len,check_sum,cfg->data[raw_cfg_len]);
		dev_err(&ts->client->dev,
			"The checksum of the config fw is not correct");
		return -EINVAL;
	}

	if (cfg->data[raw_cfg_len + 1] != 1) {
		dev_err(&ts->client->dev,
			"Config fw must have Config_Fresh register set");
		return -EINVAL;
	}

	return 0;
}

/**
 * goodix_send_cfg - Write fw config to device
 *
 * @ts: goodix_ts_data pointer
 * @cfg: config firmware to write to device
 */
static int goodix_send_cfg(struct goodix_ts_data *ts,
			   const struct firmware *cfg)
{
	int error;
    unsigned char rdata[186];
    int i;

 /*
    unsigned char data[186]= {0x60,0x00,0x04,0x58,0x02,0x0A,0xCD,0x00,0x01,0xC2,0x1F,0x05,0x50,0x32,0x03,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x19,0x17,0x20,0x14,0x8C,0x2E,0x0E,0x57,0x59,0xEE,0x06,0x00,0x00,0x00,0x9A,0x42,0x1C,0x17,0x00,0x00,0x00,0x00,0x02,0xFF,0xFF,0x15,0x80,0x0F,0x3C,0x6E,0x94,0xC0,0x52,0x07,0x00,0x00,0x04,0xCA,0x3F,0x00,0xAB,0x48,0x00,0x92,0x51,0x00,0x7B,0x5B,0x00,0x6A,0x67,0x00,0x6A,0x00,0x00,0x00,0x00,0xF0,0x50,0x32,0xFF,0xFF,0x07,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x04,0x06,0x08,0x0A,0x0C,0x0E,0x10,0x12,0x14,0x16,0x18,0x1A,0x1C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x04,0x06,0x08,0x0A,0x0C,0x0F,0x10,0x12,0x13,0x14,0x16,0x18,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x24,0x26,0x28,0x29,0x2A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xAD,0x01};
    */
//    goodix_i2c_read(ts->client,GOODIX_REG_CONFIG_DATA,rdata,186); 
//    for(i= 0 ;i < 186;i++){
//        printk("%x, ",cfg->data[i]);
//    }
	error = goodix_check_cfg(ts, cfg);
     
	if (error)
		return error;
    printk("%x %d  \n",cfg->data[185],cfg->size);
	error = goodix_i2c_write(ts->client, GOODIX_REG_CONFIG_DATA, cfg->data,
				 cfg->size);
//	error = goodix_i2c_write(ts->client, GOODIX_REG_CONFIG_DATA, data,
//				 cfg->size);
	if (error) {
		dev_err(&ts->client->dev, "Failed to write config data: %d",
			error);
		return error;
	}

	dev_dbg(&ts->client->dev, "Config sent successfully.");
//	printk( "Config sent successfully.\n");

	/* Let the firmware reconfigure itself, so sleep for 10ms */
	usleep_range(10000, 11000);

	return 0;
}

static int goodix_int_sync(struct goodix_ts_data *ts)
{
	int error;

	error = gpiod_direction_output(ts->gpiod_int, 0);
	if (error)
		return error;

	msleep(50);				/* T5: 50ms */

	error = gpiod_direction_input(ts->gpiod_int);
	if (error)
		return error;

	return 0;
}

/**
 * goodix_reset - Reset device during power on
 *
 * @ts: goodix_ts_data pointer
 */
static int goodix_reset(struct goodix_ts_data *ts)
{
	int error;
    unsigned gpio_rst;
    unsigned gpio_int;
    gpio_rst = desc_to_gpio(ts->gpiod_rst);
    gpio_int = desc_to_gpio(ts->gpiod_int);

    gpio_request(gpio_rst, GOODIX_GPIO_INT_NAME);
    gpio_request(gpio_int, GOODIX_GPIO_RST_NAME);

	/* begin select I2C slave addr */
	error = gpio_direction_output(gpio_rst, 0);
	if (error)
		return error;

	msleep(20);				/* T2: > 10ms */

	/* HIGH: 0x28/0x29, LOW: 0xBA/0xBB */
	error = gpio_direction_output(gpio_int, ts->client->addr == 0x14);
//	error = gpio_direction_output(gpio_int, 1);
	if (error)
		return error;

	usleep_range(100, 2000);		/* T3: > 100us */

	error = gpio_direction_output(gpio_rst, 1);
	if (error)
		return error;

	usleep_range(6000, 10000);		/* T4: > 5ms */

	/* end select I2C slave addr */
	error = gpio_direction_input(gpio_rst);
	if (error)
		return error;

	error = goodix_int_sync(ts);
	if (error)
		return error;

	return 0;
}

/**
 * goodix_get_gpio_config - Get GPIO config from ACPI/DT
 *
 * @ts: goodix_ts_data pointer
 */
/*
static int goodix_get_gpio_config(struct goodix_ts_data *ts)
{
	int error;
	struct device *dev;
	struct gpio_desc *gpiod;

	if (!ts->client)
		return -EINVAL;
	dev = &ts->client->dev;

	//Get the interrupt GPIO pin number
	gpiod = devm_gpiod_get_optional(dev, GOODIX_GPIO_INT_NAME, GPIOD_IN);
	if (IS_ERR(gpiod)) {
		error = PTR_ERR(gpiod);
		if (error != -EPROBE_DEFER)
			dev_dbg(dev, "Failed to get %s GPIO: %d\n",
				GOODIX_GPIO_INT_NAME, error);
		return error;
	}
	ts->gpiod_int = gpiod;

	//Get the reset line GPIO pin number
	gpiod = devm_gpiod_get_optional(dev, GOODIX_GPIO_RST_NAME, GPIOD_IN);
	if (IS_ERR(gpiod)) {
		error = PTR_ERR(gpiod);
		if (error != -EPROBE_DEFER)
			dev_dbg(dev, "Failed to get %s GPIO: %d\n",
				GOODIX_GPIO_RST_NAME, error);
		return error;
	}

	ts->gpiod_rst = gpiod;

	return 0;
}
*/
/**
 * goodix_read_config - Read the embedded configuration of the panel
 *
 * @ts: our goodix_ts_data pointer
 *
 * Must be called during probe
 */
static void goodix_read_config(struct goodix_ts_data *ts)
{
	u8 config[GOODIX_CONFIG_MAX_LENGTH];
	int error;
char x_low,x_high;
	error = goodix_i2c_read(ts->client, GOODIX_REG_CONFIG_DATA,
				config, ts->cfg_len);
	if (error) {
		dev_warn(&ts->client->dev,
			 "Error reading config (%d), using defaults\n",
			 error);
		ts->abs_x_max = GOODIX_MAX_WIDTH;
		ts->abs_y_max = GOODIX_MAX_HEIGHT;
		if (ts->swapped_x_y)
			swap(ts->abs_x_max, ts->abs_y_max);
		ts->int_trigger_type = GOODIX_INT_TRIGGER;
		ts->max_touch_num = GOODIX_MAX_CONTACTS;
		return;
	}

//    x_low = 0xe0;
//    x_high = 0x01;
//   goodix_i2c_write(ts->client,0x8146,&x_low,1); 
//   goodix_i2c_write(ts->client,0x8147,&x_high,1); 
//   printk("0x%x 0x%x ",x_low,x_high);
//   x_low = 0x10;
//   x_high = 0x01;
//   goodix_i2c_write(ts->client,0x8148,&x_low,1); 
//   goodix_i2c_write(ts->client,0x8149,&x_high,1); 
//   printk("0x%x 0x%x ",x_low,x_high);
   goodix_i2c_read(ts->client,0x8146,&x_low,1); 
   goodix_i2c_read(ts->client,0x8147,&x_high,1); 
   printk("X 0x%x 0x%x ",x_low,x_high);
   goodix_i2c_read(ts->client,0x8148,&x_low,1); 
   goodix_i2c_read(ts->client,0x8149,&x_high,1); 
   printk("Y 0x%x 0x%x ",x_low,x_high);
	ts->abs_x_max = get_unaligned_le16(&config[RESOLUTION_LOC]);
    ts->abs_y_max = get_unaligned_le16(&config[RESOLUTION_LOC + 2]);
//    ts->abs_x_max = 1024;
//    ts->abs_y_max = 600;
    printk("%d %s abs_x_max = %d abs_y_max = %d \n,",__LINE__,__func__,ts->abs_x_max,ts->abs_y_max);

	if (ts->swapped_x_y)
		swap(ts->abs_x_max, ts->abs_y_max);
	ts->int_trigger_type = config[TRIGGER_LOC] & 0x03;
	ts->max_touch_num = config[MAX_CONTACTS_LOC] & 0x0f;
	if (!ts->abs_x_max || !ts->abs_y_max || !ts->max_touch_num) {
		dev_err(&ts->client->dev,
			"Invalid config, using defaults\n");
		ts->abs_x_max = GOODIX_MAX_WIDTH;
		ts->abs_y_max = GOODIX_MAX_HEIGHT;
		if (ts->swapped_x_y)
			swap(ts->abs_x_max, ts->abs_y_max);
		ts->max_touch_num = GOODIX_MAX_CONTACTS;
	}

	if (dmi_check_system(rotated_screen)) {
		ts->inverted_x = true;
		ts->inverted_y = true;
		dev_dbg(&ts->client->dev,
			 "Applying '180 degrees rotated screen' quirk\n");
	}
}

/**
 * goodix_read_version - Read goodix touchscreen version
 *
 * @ts: our goodix_ts_data pointer
 */
static int goodix_read_version(struct goodix_ts_data *ts)
{
	int error;
	u8 buf[6];
	char id_str[5];

	error = goodix_i2c_read(ts->client, GOODIX_REG_ID, buf, sizeof(buf));
	if (error) {
		dev_err(&ts->client->dev, "read version failed: %d\n", error);
		return error;
	}

	memcpy(id_str, buf, 4);
	id_str[4] = 0;
	if (kstrtou16(id_str, 10, &ts->id))
		ts->id = 0x1001;

	ts->version = get_unaligned_le16(&buf[4]);

	dev_info(&ts->client->dev, "ID %d, version: %04x\n", ts->id,
		 ts->version);

	printk( "ID %d, version\n", ts->id);
	return 0;
}

/**
 * goodix_i2c_test - I2C test function to check if the device answers.
 *
 * @client: the i2c client
 */
static int goodix_i2c_test(struct i2c_client *client)
{
	int retry = 0;
	int error;
	u8 test;

	while (retry++ < 2) {
		error = goodix_i2c_read(client, GOODIX_REG_CONFIG_DATA,
					&test, 1);
		if (!error)
			return 0;

		dev_err(&client->dev, "i2c test failed attempt %d: %d\n",
			retry, error);
		msleep(20);
	}

	return error;
}

/**
 * goodix_request_input_dev - Allocate, populate and register the input device
 *
 * @ts: our goodix_ts_data pointer
 *
 * Must be called during probe
 */
static int goodix_request_input_dev(struct goodix_ts_data *ts)
{
	int error;

	ts->input_dev = devm_input_allocate_device(&ts->client->dev);
	if (!ts->input_dev) {
		dev_err(&ts->client->dev, "Failed to allocate input device.");
		return -ENOMEM;
	}
    printk("abs_x = %d ,abs_y =%d ",ts->abs_x_max,ts->abs_y_max);
//    ts->abs_x_max = 1024;
//    ts->abs_y_max = 600;
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X,
			     0, ts->abs_x_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y,
			     0, ts->abs_y_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);

	input_mt_init_slots(ts->input_dev, ts->max_touch_num,
			    INPUT_MT_DIRECT | INPUT_MT_DROP_UNUSED);

	ts->input_dev->name = "Goodix Capacitive TouchScreen";
	ts->input_dev->phys = "input/ts";
	ts->input_dev->id.bustype = BUS_I2C;
	ts->input_dev->id.vendor = 0x0416;
	ts->input_dev->id.product = ts->id;
	ts->input_dev->id.version = ts->version;

	error = input_register_device(ts->input_dev);
	if (error) {
		dev_err(&ts->client->dev,
			"Failed to register input device: %d", error);
		return error;
	}

	return 0;
}

/**
 * goodix_configure_dev - Finish device initialization
 *
 * @ts: our goodix_ts_data pointer
 *
 * Must be called from probe to finish initialization of the device.
 * Contains the common initialization code for both devices that
 * declare gpio pins and devices that do not. It is either called
 * directly from probe or from request_firmware_wait callback.
 */
static int goodix_configure_dev(struct goodix_ts_data *ts)
{
	int error;

	ts->swapped_x_y = device_property_read_bool(&ts->client->dev,
						    "touchscreen-swapped-x-y");
	ts->inverted_x = device_property_read_bool(&ts->client->dev,
						   "touchscreen-inverted-x");
	ts->inverted_y = device_property_read_bool(&ts->client->dev,
						   "touchscreen-inverted-y");

	goodix_read_config(ts);

	error = goodix_request_input_dev(ts);
	if (error)
		return error;

	ts->irq_flags = goodix_irq_flags[ts->int_trigger_type] | IRQF_ONESHOT;
	error = goodix_request_irq(ts);
	if (error) {
		dev_err(&ts->client->dev, "request IRQ failed: %d\n", error);
		return error;
	}

	return 0;
}

/**
 * goodix_config_cb - Callback to finish device init
 *
 * @ts: our goodix_ts_data pointer
 *
 * request_firmware_wait callback that finishes
 * initialization of the device.
 */
static void goodix_config_cb(const struct firmware *cfg, void *ctx)
{
	struct goodix_ts_data *ts = ctx;
	int error;
    printk("%d  %s \n",__LINE__,__func__);
	if (cfg) {
		/* send device configuration to the firmware */
		error = goodix_send_cfg(ts, cfg);
		if (error)
			goto err_release_cfg;
	}
	goodix_configure_dev(ts);

err_release_cfg:
	release_firmware(cfg);
	complete_all(&ts->firmware_loading_complete);
}

ssize_t gtcfg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int ret = 0;
    char *srcbuf = NULL;
    char tmpbuf[256] = {0};
    struct goodix_ts_data *ts;

    ts = (struct goodix_ts_data*)dev_get_drvdata(dev);
    if(!ts){
        dev_err(dev,"no driver data\n");
        return -EINVAL;
    }
    srcbuf = (char*)kzalloc(256*5,GFP_KERNEL);
    if(!srcbuf){
        dev_err(dev,"kzalloc tmpbuf failed\n");
        return -ENOMEM;
    }
    sprintf(tmpbuf,"ts_irqnum:%d\n",ts->client->irq);
    strcat(srcbuf,tmpbuf);
    sprintf(tmpbuf,"ts_intpin:%d\n",desc_to_gpio(ts->gpiod_int));
    strcat(srcbuf,tmpbuf);
    sprintf(tmpbuf,"ts_rstpin:%d\n",desc_to_gpio(ts->gpiod_rst));
    strcat(srcbuf,tmpbuf);

    ret = sprintf(buf,"%s",srcbuf);
    kfree(srcbuf);

    return ret;
}

ssize_t gtcfg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    long ret = 0;
    char filepath[256];
    char *str = NULL;
    char *tmpbuf = NULL;
    struct file* filp = NULL;
    struct path p;
    struct kstat ks;
    unsigned long filesize = 0;
    unsigned long long offset = 0;
    mm_segment_t oldfs;
    struct firmware cfg;
    struct goodix_ts_data *ts;

    ts = (struct goodix_ts_data*)dev_get_drvdata(dev);
    if(!ts){
        dev_err(dev,"no driver data\n");
        return -EINVAL;
    }
    if(buf){
        strcpy(filepath,buf);
        str = filepath;
        while((*str != ' ') && (*str != '\r') && (*str != '\n'))
            str++;
        *str = '\0';

        oldfs = get_fs();
        set_fs(get_ds());

        filp = filp_open(filepath, O_RDONLY, 0);
        if(IS_ERR(filp))
        {
            dev_err(dev,"Open File fail [%s]\n",filepath);
            ret = PTR_ERR(filp);
            set_fs(oldfs);
            return ret;
        }
        kern_path(filepath, 0, &p);
        vfs_getattr(&p, &ks);
        filesize = ks.size;
        if(filesize != GOODIX_CONFIG_911_LENGTH){
            dev_err(dev,"file size Invalid [%s]\n",filepath);
            filp_close(filp,NULL);
            set_fs(oldfs);
            return -EINVAL;
        }
        set_fs(oldfs);

        tmpbuf = (char*)kzalloc(filesize,GFP_KERNEL);
        if(!tmpbuf){
            dev_err(dev,"alloc failed\n");
            filp_close(filp,NULL);
            set_fs(oldfs);
            return -ENOMEM;
        }
        else{
            cfg.data = tmpbuf;
            cfg.size = filesize;
        }

        oldfs = get_fs();
        set_fs(get_ds());
        ret = vfs_read(filp, tmpbuf, filesize, &offset);
        if(ret == cfg.size){
            goodix_send_cfg(ts,&cfg);
        }
        filp_close(filp,NULL);
        set_fs(oldfs);

        kfree(tmpbuf);
        printk("config done\n");

        return count;
    }
    return 0;
}

DEVICE_ATTR(gtcfg,0664,gtcfg_show,gtcfg_store);

static int goodix_ts_sysfs_init(struct goodix_ts_data *ts)
{
    int ret = 0;

    ret = device_create_file(&ts->client->dev, &dev_attr_gtcfg);
    if(ret){
        dev_err(&ts->client->dev,"gt911 device create file failed\n");
        return ret;
    }
    return ret;
}

static void goodix_ts_sysfs_deinit(struct goodix_ts_data *ts)
{
    device_remove_file(&ts->client->dev, &dev_attr_gtcfg);
}

static int goodix_ts_config(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    struct goodix_ts_data *ts;
    int error;
    u32 gpio_rst;
    u32 gpio_int;
    int i;
   u8  x_low,x_high;
   u8 rdata[186];
    dev_dbg(&client->dev, "I2C Address: 0x%02x\n", client->addr);

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        dev_err(&client->dev, "I2C check functionality failed.\n");
        return -ENXIO;
    }

    ts = devm_kzalloc(&client->dev, sizeof(*ts), GFP_KERNEL);
    if (!ts)
        return -ENOMEM;

    ts->client = client;
    i2c_set_clientdata(client, ts);
    init_completion(&ts->firmware_loading_complete);
/*
    if(0 != of_property_read_u32(client->dev.of_node, GOODIX_GPIO_RST_NAME, &gpio_rst))
        return -EINVAL;
    ts->gpiod_rst = gpio_to_desc(gpio_rst);
    if(0 != of_property_read_u32(client->dev.of_node, GOODIX_GPIO_INT_NAME, &gpio_int))
        return -EINVAL;
    ts->gpiod_int = gpio_to_desc(gpio_int);
*/
    gpio_rst = of_get_named_gpio(client->dev.of_node, GOODIX_GPIO_RST_NAME,0);
    gpio_int =  of_get_named_gpio(client->dev.of_node, GOODIX_GPIO_INT_NAME,0);
    ts->gpiod_rst = gpio_to_desc(gpio_rst);
    ts->gpiod_int = gpio_to_desc(gpio_int);
   
  //  ts->client->irq = of_irq_get_byname(client->dev.of_node, GOODIX_GPIO_INT_NAME);
    ts->client->irq  = gpio_to_irq(gpio_int);
    dev_dbg(&client->dev, "goodix_irq_num:%d\n",ts->client->irq);
    printk( "goodix_irq_num:%d\n",ts->client->irq);
    /*
       error = goodix_get_gpio_config(ts);
       if (error)
       return error;
       */
    if (ts->gpiod_int && ts->gpiod_rst) {
        /* reset the controller */
        error = goodix_reset(ts);
        if (error) {
            dev_err(&client->dev, "Controller reset failed.\n");
            return error;
        }
    }

    error = goodix_i2c_test(client);
    if (error) {
        dev_err(&client->dev, "I2C communication failure: %d\n", error);
        return error;
    }

    error = goodix_read_version(ts);
    if (error) {
        dev_err(&client->dev, "Read version failed.\n");
        return error;
    }

    ts->cfg_len = goodix_get_cfg_len(ts->id);
//    goodix_i2c_read(ts->client,GOODIX_REG_CONFIG_DATA,rdata,186); 
//    for(i= 0 ;i < 186;i++){
//        printk("%x, ",rdata[i]);
//    }

    if (ts->gpiod_int && ts->gpiod_rst) {
        /* update device config */
        ts->cfg_name = devm_kasprintf(&client->dev, GFP_KERNEL,
                "goodix_%d_cfg.bin", ts->id);
        if (!ts->cfg_name)
            return -ENOMEM;
        printk(" cfg_name = %s \n ",ts->cfg_name);

        error = request_firmware_nowait(THIS_MODULE, true, ts->cfg_name,
                &client->dev, GFP_KERNEL, ts,
                goodix_config_cb);
        if (error) {
            dev_err(&client->dev,
                    "Failed to invoke firmware loader: %d\n",
                    error);
            return error;
        }
    } else {
        printk("%d goodix_config_dev \n",__LINE__);
        error = goodix_configure_dev(ts);
        if (error)
            return error;
    }
    if(goodix_ts_sysfs_init(ts)){
        dev_err(&client->dev,"gt911 sysfs init failed\n");
    }

    return 0;
}

struct goodix_ts_work{
    struct i2c_client *client;
    const struct i2c_device_id *id;
    struct work_struct work;
};
struct goodix_ts_work tswork;

static void goodix_ts_work_handler(struct work_struct *pwork)
{
    struct goodix_ts_work *ptswork;

    ptswork = container_of(pwork, struct goodix_ts_work, work);
    if(goodix_ts_config(ptswork->client, ptswork->id))
    {
        dev_err(&ptswork->client->dev,"touchscreen config failed!!!\n");
    }
}

static int goodix_ts_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    tswork.client = client;
    tswork.id = id;
    INIT_WORK(&tswork.work, goodix_ts_work_handler);
    schedule_work(&tswork.work);

    return 0;
}

static int goodix_ts_remove(struct i2c_client *client)
{
	struct goodix_ts_data *ts = i2c_get_clientdata(client);

    goodix_ts_sysfs_deinit(ts);
	if (ts->gpiod_int && ts->gpiod_rst)
		wait_for_completion(&ts->firmware_loading_complete);

	return 0;
}

static int __maybe_unused goodix_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct goodix_ts_data *ts = i2c_get_clientdata(client);
	int error;

	/* We need gpio pins to suspend/resume */
	if (!ts->gpiod_int || !ts->gpiod_rst) {
		disable_irq(client->irq);
		return 0;
	}

	wait_for_completion(&ts->firmware_loading_complete);

	/* Free IRQ as IRQ pin is used as output in the suspend sequence */
	goodix_free_irq(ts);

	/* Output LOW on the INT pin for 5 ms */
	error = gpiod_direction_output(ts->gpiod_int, 0);
	if (error) {
		goodix_request_irq(ts);
		return error;
	}

	usleep_range(5000, 6000);

	error = goodix_i2c_write_u8(ts->client, GOODIX_REG_COMMAND,
				    GOODIX_CMD_SCREEN_OFF);
	if (error) {
		dev_err(&ts->client->dev, "Screen off command failed\n");
		gpiod_direction_input(ts->gpiod_int);
		goodix_request_irq(ts);
		return -EAGAIN;
	}

	/*
	 * The datasheet specifies that the interval between sending screen-off
	 * command and wake-up should be longer than 58 ms. To avoid waking up
	 * sooner, delay 58ms here.
	 */
	msleep(58);
	return 0;
}

static int __maybe_unused goodix_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct goodix_ts_data *ts = i2c_get_clientdata(client);
	int error;

	if (!ts->gpiod_int || !ts->gpiod_rst) {
		enable_irq(client->irq);
		return 0;
	}

	/*
	 * Exit sleep mode by outputting HIGH level to INT pin
	 * for 2ms~5ms.
	 */
	error = gpiod_direction_output(ts->gpiod_int, 1);
	if (error)
		return error;

	usleep_range(2000, 5000);

	error = goodix_int_sync(ts);
	if (error)
		return error;

	error = goodix_request_irq(ts);
	if (error)
		return error;

	return 0;
}

static SIMPLE_DEV_PM_OPS(goodix_pm_ops, goodix_suspend, goodix_resume);

static const struct i2c_device_id goodix_ts_id[] = {
	{ "GDIX1001:00", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, goodix_ts_id);

#ifdef CONFIG_ACPI
static const struct acpi_device_id goodix_acpi_match[] = {
	{ "GDIX1001", 0 },
	{ "GDIX1002", 0 },
	{ }
};
MODULE_DEVICE_TABLE(acpi, goodix_acpi_match);
#endif

#ifdef CONFIG_OF
static const struct of_device_id goodix_of_match[] = {
	{ .compatible = "goodix,gt911" },
	{ .compatible = "goodix,gt9110" },
	{ .compatible = "goodix,gt912" },
	{ .compatible = "goodix,gt927" },
	{ .compatible = "goodix,gt9271" },
	{ .compatible = "goodix,gt928" },
	{ .compatible = "goodix,gt967" },
	{ }
};
MODULE_DEVICE_TABLE(of, goodix_of_match);
#endif

static struct i2c_driver goodix_ts_driver = {
	.probe = goodix_ts_probe,
	.remove = goodix_ts_remove,
	.id_table = goodix_ts_id,
	.driver = {
		.name = "Goodix-TS",
		.acpi_match_table = ACPI_PTR(goodix_acpi_match),
		.of_match_table = of_match_ptr(goodix_of_match),
		.pm = &goodix_pm_ops,
	},
};
module_i2c_driver(goodix_ts_driver);

MODULE_AUTHOR("Benjamin Tissoires <benjamin.tissoires@gmail.com>");
MODULE_AUTHOR("Bastien Nocera <hadess@hadess.net>");
MODULE_DESCRIPTION("Goodix touchscreen driver");
MODULE_LICENSE("GPL v2");
