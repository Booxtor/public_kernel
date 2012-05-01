/*
* Neonode zForce optical touchscreen driver
*
* Copyright (c) 2011 Onyx International, Inc
*/

/*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License version 2 as published by
* the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/input/zforce_ts.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>

#define MAX_RESP_DATA_LEN 256
#define MAX_LEDS          32

#define CMD_DEACTIVATE         0x0
#define CMD_ACTIVATE           0x1
#define CMD_RESOLUTION         0x02
#define CMD_CONFIGURATION      0x03
#define CMD_TOUCHDATA          0x04
#define CMD_SCANNINGFREQUENCY  0x08
#define CMD_VERSION            0x0a
#define CMD_LOWSIGNALALERT     0x0d
#define CMD_FIXEDPULSESTRENGTH 0x0f
#define CMD_LEDLEVEL           0x1c
#define CMD_SETACTIVELEDS      0x1d
#define CMD_STATUSCOMMAND      0x1e
#define CMD_INVALIDCOMMAND     0xfe

/* By default, enable multi touch. */
static unsigned int mcu_config = 1;
static int zforce_ts_set_res(struct i2c_client *client, u16 x, u16 y);
extern void touchpanel_suspend_pads(void);
extern void touchpanel_resume_pads(void);
extern void pull_down_touchpanel_pins(void);
extern void mcu_power_enable(int enable);

struct zforce_ts_priv
{
    struct i2c_client *client;
    struct input_dev *input;
    struct work_struct work;
    unsigned int dr_gpio;
    unsigned int reset_gpio;
    unsigned int test_gpio;
    unsigned int x_res;
    unsigned int y_res;
};


/* Should be 7 bytes long */
struct zforce_point
{
    u16 x;
    u16 y;
    u16 info;
    u8  probability;
} __attribute__((__packed__));

struct version_request
{
    u16 major;
    u16 minor;
    u16 build;
    u16 revision;
} __attribute__((__packed__));

struct settings_data
{
    u16 major;
    u16 minor;
    u16 build;
    u16 revision;

    u8 active;

    u16 scanning_counter;

    u32 configuration;

    u8 idle_frequency;
    u8 full_scanning_frequency;

    u8 settings_reserved;

    u16 width;
    u16 height;

    u8 led_x_start;
    u8 led_x_End;
    u8 led_y_start;
    u8 led_y_end;

    u8 additional_reserved[38];
} __attribute__((__packed__));

struct ledsignalinfo{
        unsigned char ledstrength1:4;
        unsigned char ledstrength2:4;
        unsigned char pd_signal1;
        unsigned char pd_signal2;
} __attribute__((__packed__)) ;

struct ledlevelresponse{
        unsigned char xcount;
        unsigned char ycount;
        struct ledsignalinfo    xleds[MAX_LEDS];
        struct ledsignalinfo    yleds[MAX_LEDS];
};


static struct zforce_ts_priv g_priv;
static int zforce_ts_reset(struct zforce_ts_priv *priv);
static int retry_count = 0;

static void report_activate_exception(void)
{
    printk(KERN_INFO"reset muc as activation failed.\n");
    if (retry_count++ <= 2)
    {
        zforce_ts_reset(&g_priv);
    }
}


/* send request, but don't wait for resp. When resp is available,
we will read it from interrupt handler
*/
static int zforce_ts_send_request(struct i2c_client *client,
                                  u8 *cmd,
                                  u8 cmd_len)
{
    int err;
    //printk("%s: %d\n", __func__, cmd[0]);
    if ((err = i2c_master_send(client, cmd, cmd_len)) != cmd_len)
    {
        dev_err(&client->dev, "Error in sending request %d.\n", *cmd);
        return err;
    }
    return 0;
}

static int activate_ts(struct i2c_client *client, int activate)
{
    u8 command_id = activate > 0 ? CMD_ACTIVATE : CMD_DEACTIVATE;
    int ret = 0;
    ret = zforce_ts_send_request(client, &command_id, 1);
    if (ret != 0 && activate > 0)
    {
        report_activate_exception();
    }
    return ret;
}

static int zforce_ts_get_ledlevel(struct i2c_client *client)
{
    u8 command_id = CMD_LEDLEVEL;
    return zforce_ts_send_request(client, &command_id, 1);
}

static int zforce_ts_get_version(struct i2c_client *client, struct zforce_fw_version *ver)
{
    u8 command_id = CMD_VERSION;
    return zforce_ts_send_request(client, &command_id, 1);
}

static int zforce_ts_set_scanfreq(struct i2c_client *client, unsigned char idle_sf, unsigned char full_sf)
{
    u8 cmd[3] = {CMD_SCANNINGFREQUENCY, 0, 0};
    cmd[1] = idle_sf;
    cmd[2] = full_sf;
    return zforce_ts_send_request(client, cmd, 3);
}

static int zforce_ts_set_config(struct i2c_client *client, unsigned int flags)
{
    u8 cmd[5] = {CMD_CONFIGURATION, 0, 0, 0, 0};
    cmd[1] = flags & 0xFF;
    cmd[2] = (flags >> 8) & 0xFF;
    cmd[3] = (flags >> 16) & 0xFF;
    cmd[4] = flags >> 24;
    return zforce_ts_send_request(client, cmd, 5);
}

static int zforce_ts_set_res(struct i2c_client *client, u16 x, u16 y)
{
    u8 cmd[5] = {CMD_RESOLUTION, 0, 0, 0, 0};

    cmd[1] = x & 0xFF;
    cmd[2] = x >> 8;
    cmd[3] = y & 0xFF;
    cmd[4] = y >> 8;
    return zforce_ts_send_request(client, cmd, 5);
}

static int zforce_ts_report_status(struct i2c_client *client)
{
    u8 cmd = CMD_STATUSCOMMAND;
    return zforce_ts_send_request(client, &cmd, 1);
}

static int zforce_ts_request_touch_data(struct i2c_client *client)
{
    u8 command_id = CMD_TOUCHDATA;
    return zforce_ts_send_request(client, &command_id, 1);
}

/* the data without FrameStart 0xEE (BYTE) and DataSize, you can use it directly. */
static int on_recv_firmware(u8 * data, const int len)
{
    struct version_request *v = (struct version_request *)(&data[1]);
    printk("%s: %d.%d.%d.%d\n", __func__, v->major, v->minor, v->build, v->revision);
    return 0;
}

static int on_recv_activate(u8 * data, const int len)
{
    printk("%s: %d\n", __func__, data[1]);
    if (data[1] != 0)
    {
        report_activate_exception();
    }
    return 0;
}

static int on_recv_resolution(u8 * data, const int len)
{
     printk("%s: %d\n", __func__, data[1]);
     return 0;
}

static int on_recv_config(u8 * data, const int len)
{
    printk("%s: %d\n", __func__, data[1]);
    return 0;
}

static int on_recv_status(u8 * data, const int len)
{
    struct settings_data *settings = (struct settings_data *)(&data[1]);
    printk("%s: version: %d.%d.%d.%d\n", __func__, settings->major, settings->minor, settings->build, settings->revision);
    printk("%s: active: %d\n", __func__, settings->active);
    printk("%s: scanning_counter: %d\n", __func__, settings->scanning_counter);
    printk("%s: configuration: %d\n", __func__, settings->configuration);
    printk("%s: idle_frequency: %d\n", __func__, settings->idle_frequency);
    printk("%s: full_scanning_frequency: %d\n", __func__, settings->full_scanning_frequency);
    printk("%s: settings_reserved: %d\n", __func__, settings->settings_reserved);

    printk("%s: width: %d\n", __func__, settings->width);
    printk("%s: height: %d\n", __func__, settings->height);
    printk("%s: led_x_start: %d\n", __func__, settings->led_x_start);
    printk("%s: led_x_End: %d\n", __func__, settings->led_x_End);

    printk("%s: led_y_start: %d\n", __func__,  settings->led_y_start);
    printk("%s: led_y_end: %d\n", __func__, settings->led_y_end);
    return 0;
}

static int on_recv_touch_data(u8 *data, const int len)
{
    int points = 0;
    int i;
    struct zforce_point pt;

    // Not necessary now.
    // Send new data request as soon as possible for maximum performace.
    // zforce_ts_request_touch_data(g_priv.client);

    points = data[1];
    //printk("%s: touch data %d\n", __func__, points);

    if (points == 1)
    {
        memcpy(&pt, data + 2, 7);
        // dump_point(pt);
        input_report_abs(g_priv.input, ABS_X, pt.x);
        input_report_abs(g_priv.input, ABS_Y, pt.y);
        input_report_abs(g_priv.input, ABS_PRESSURE, pt.info);
    }
    else if (points >= 2)
    {
        for (i = 0; i < points; i++)
        {
            memcpy(&pt, data + 2 + 7 * i, 7);
            // dump_point(pt);
            input_event(g_priv.input, EV_ABS, ABS_MT_POSITION_X, pt.x);
            input_event(g_priv.input, EV_ABS, ABS_MT_POSITION_Y, pt.y);
            input_event(g_priv.input, EV_ABS, ABS_MT_PRESSURE, pt.info);
            input_mt_sync(g_priv.input);
        }
    }
    input_sync(g_priv.input);
    return 0;
}

static void on_recv_ledlevel(u8 *data, const int len)
{
	int x_len, y_len;
	int i = 0;
	int value;
	struct ledlevelresponse  ledlevel;

	ledlevel.xcount = data[1];
	ledlevel.ycount = data[2];
	
	x_len = (ledlevel.xcount) * sizeof(struct ledsignalinfo);
	y_len = (ledlevel.ycount) * sizeof(struct ledsignalinfo);

	memcpy(&ledlevel.xleds, data + 3, x_len);
	memcpy(&ledlevel.yleds, data + 3 + x_len, y_len);

	// printk("ledlevel.xcount=%d, ledlevel.ycount=%d\n", ledlevel.xcount, ledlevel.ycount);

	input_event(g_priv.input, EV_ABS, ABS_MISC, ledlevel.xcount);
	for (i = 0; i < ledlevel.xcount; i++) {
		// printk("x[%d] ledstrength1=%d, ledstrength2=%d, pd_singal1=%d, pd_signal2=%d\n", i,
		//		ledlevel.xleds[i].ledstrength1, ledlevel.xleds[i].ledstrength2,
		//		ledlevel.xleds[i].pd_signal1, ledlevel.xleds[i].pd_signal2);
		memcpy(&value, &ledlevel.xleds[i], sizeof(struct ledsignalinfo));
		input_event(g_priv.input, EV_ABS, ABS_MISC, -1);
		input_event(g_priv.input, EV_ABS, ABS_MISC, value);
	}
        input_event(g_priv.input, EV_ABS, ABS_MISC, ledlevel.ycount);
	for (i = 0; i < ledlevel.ycount; i++) {
		//printk("y[%d] ledstrength1=%d, ledstrength2=%d, pd_singal1=%d, pd_signal2=%d\n", i,
		//		ledlevel.yleds[i].ledstrength1, ledlevel.yleds[i].ledstrength2,
		//		ledlevel.yleds[i].pd_signal1, ledlevel.yleds[i].pd_signal2);
		memcpy(&value, &ledlevel.yleds[i], sizeof(struct ledsignalinfo));
		input_event(g_priv.input, EV_ABS, ABS_MISC, -1);
		input_event(g_priv.input, EV_ABS, ABS_MISC, value);
	}
        input_sync(g_priv.input);
}

/* read resp and invoke correspoding handler.*/
static int zforce_ts_recv_resp(struct i2c_client *client)
{
    int err = 0;
    u8 data_size;
    u8 resp_hdr[2];
    u8 resp_data[MAX_RESP_DATA_LEN];
    u8 cmd = 0;

    if ((err = i2c_master_recv(client, resp_hdr, 2)) != 2)
    {
        dev_err(&client->dev, "Error receing frame start!\n");
        return err;
    }

    if (resp_hdr[0] != 0xEE)
    {
        dev_err(&client->dev, "%s: unexpected frame start(0x%02x) received, "
            "should be 0xEE!\n", __FUNCTION__, resp_hdr[0]);
        return -EIO;
    }

    data_size = resp_hdr[1];
    if ((err = i2c_master_recv(client, resp_data, data_size)) != data_size)
    {
        dev_err(&client->dev, "Error receving data payload!\n");
        return err;
    }

    cmd = resp_data[0];
    //dev_err(&client->dev, "Command response %d\n", cmd);

    switch (cmd)
    {
    case CMD_ACTIVATE:
        on_recv_activate(resp_data, data_size);
        break;
    case CMD_RESOLUTION:
        on_recv_resolution(resp_data, data_size);
        break;
    case CMD_CONFIGURATION:
        on_recv_config(resp_data, data_size);
        break;
    case CMD_TOUCHDATA:
        on_recv_touch_data(resp_data, data_size);
        break;
    case CMD_SCANNINGFREQUENCY:
        break;
    case CMD_VERSION:
        on_recv_firmware(resp_data, data_size);
        break;
    case CMD_LOWSIGNALALERT:
        break;
    case CMD_FIXEDPULSESTRENGTH:
        break;
    case CMD_LEDLEVEL:
        on_recv_ledlevel(resp_data, data_size);
        break;
    case CMD_SETACTIVELEDS:
        break;
    case CMD_STATUSCOMMAND:
        on_recv_status(resp_data, data_size);
        break;
    case CMD_INVALIDCOMMAND:
        break;
    default:
        break;
    }
    return 0;
}

/* Interrupt handler */
static void zforce_ts_interrupt_handler(struct work_struct *work)
{
    struct zforce_ts_priv *priv =
        container_of(work, struct zforce_ts_priv, work);
    struct i2c_client* client = priv->client;
    zforce_ts_recv_resp(client);
}

static irqreturn_t data_ready_irq(int irq, void *dev_id)
{
    struct zforce_ts_priv *priv = dev_id;

    /* postpone I2C transactions as we are atomic */
    schedule_work(&priv->work);

    return IRQ_HANDLED;
}

static int zforce_ts_open(struct input_dev *dev)
{
    struct zforce_ts_priv* priv = input_get_drvdata(dev);
    enable_irq(gpio_to_irq(priv->dr_gpio));
    if (!activate_ts(priv->client, 1)){
	    zforce_ts_set_res(priv->client, priv->x_res, priv->y_res);
	    zforce_ts_set_config(priv->client, mcu_config);
	    zforce_ts_set_scanfreq(priv->client, 33, 100);
	    zforce_ts_report_status(priv->client);
	    zforce_ts_request_touch_data(priv->client);
	}

    return 0;
}

static void zforce_ts_close(struct input_dev *dev)
{
    struct zforce_ts_priv* priv = input_get_drvdata(dev);
    disable_irq(gpio_to_irq(priv->dr_gpio));
}

/*
Not necessary now, it's self power initialized.
static void zforce_init(struct zforce_ts_priv* priv)
{
    gpio_request(priv->reset_gpio, "reset_gpio");
    gpio_request(priv->test_gpio, "test_gpio");
    gpio_direction_output(priv->reset_gpio, 0);
    gpio_direction_output(priv->test_gpio, 0);
    msleep(10);
    gpio_direction_output(priv->reset_gpio, 1);
    gpio_free(priv->reset_gpio);
    gpio_free(priv->test_gpio);
    msleep(100);
}
*/
static int zforce_ts_reset(struct zforce_ts_priv *priv)
{
	touchpanel_suspend_pads();
	pull_down_touchpanel_pins();
	mcu_power_enable(0);
	msleep(1000);
	mcu_power_enable(1);
	touchpanel_resume_pads();
	msleep(1000);


	/*Reactivate the IR MCU*/
    if (!activate_ts(priv->client, 1)){
	    zforce_ts_set_res(priv->client, priv->x_res, priv->y_res);
	    zforce_ts_set_config(priv->client, mcu_config);
	    zforce_ts_set_scanfreq(priv->client, 33, 100);
	    zforce_ts_report_status(priv->client);
	    zforce_ts_request_touch_data(priv->client);
	}

    return 0;
}

static int32_t zforce_io_ioctl(struct inode *inode, struct file *filp,
                               u32 cmd, unsigned long arg)
{
    struct zforce_ts_priv *priv = filp->private_data;
    struct zforce_abs abs;
    struct zforce_scanfreq scanfreq;
    struct zforce_fw_version ver;
    unsigned int	flags;
    int ret = 0;

    switch (cmd)
    {
    case ZFORCE_SET_ABS:
        if (copy_from_user(&abs, (void __user *)arg, sizeof(abs)))
        {
            ret = -EFAULT;
        }
        zforce_ts_set_res(priv->client, abs.x, abs.y);

        break;
    case ZFORCE_SET_CONFIG:
        if (copy_from_user(&flags, (void __user *)arg, sizeof(flags)))
        {
            ret = -EFAULT;
        }
        mcu_config = flags;
        zforce_ts_set_config(priv->client, mcu_config);
        break;
    case ZFORCE_SET_SCANFREQ:
        if (copy_from_user(&scanfreq, (void __user *)arg, sizeof(scanfreq)))
        {
            ret = -EFAULT;
        }
        zforce_ts_set_scanfreq(priv->client, scanfreq.idle_scanfreq, scanfreq.full_scanfreq);

        break;
    case ZFORCE_GET_VERSION:
        zforce_ts_get_version(priv->client, &ver);
        if (copy_to_user((void __user *)arg, &ver, sizeof(ver)))
        {
            ret = -EFAULT;
        }
        break;
    case ZFORCE_GET_LEDLEVEL:
        zforce_ts_get_ledlevel(priv->client);
        break;
    case ZFORCE_RESET:
        zforce_ts_reset(priv);
        break;

    default:
        printk(KERN_ERR"Not a valid zforce command\n");
        ret = -EINVAL;
        break;
    }

    return ret;
}

static int32_t zforce_io_open (struct inode *inode, struct file *file)
{
    struct zforce_ts_priv *priv =&g_priv;

    file->private_data = priv;
    return 0; //success
}

static int32_t zforce_io_release (struct inode *inode, struct file *file)
{
    file->private_data = NULL;
    return 0; //success
}

static struct file_operations zforce_fops = {
    .owner = THIS_MODULE,
    .ioctl = zforce_io_ioctl,
    .open = zforce_io_open,
    .release = zforce_io_release,
};

static struct miscdevice zforce_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "zforce_io",
    .fops = &zforce_fops,
    .this_device = NULL,
};

static int __devinit zforce_ts_probe(struct i2c_client *client,
                                     const struct i2c_device_id *idp)
{
    int err = 0;
    struct input_dev *input;
    char *options = NULL;
    char name[] = "mxcepdcfb";
    struct zforce_ts_platform_data *pdata;
    struct zforce_ts_priv* priv;

    priv = &g_priv;

    if (fb_get_options(name, &options)) {
        err = -ENODEV;
        return err;
    }
    
    if (strstr(options, "E97_V110") != NULL){
        priv->x_res = 825;
        priv->y_res = 1200;
    }
    else if (strstr(options, "PVI") != NULL){
        priv->x_res = 758;
        priv->y_res = 1024;
    }
    else if(strstr(options, "HD") != NULL){
        priv->x_res = 768;
        priv->y_res = 1024;
    }
    else {
        priv->x_res = 600;
        priv->y_res = 800;
    }
    printk(KERN_INFO"%s:the screen is :x_res = %d, y_res = %d\n",__func__, priv->x_res, priv->y_res);
    input = input_allocate_device();
    if (!input) {
        dev_err(&client->dev, "Failed to allocate input device.\n");
        goto err0;
    }

    input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
    input->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

    input_set_abs_params(input, ABS_X, 0, 0x7FF, 0, 0);
    input_set_abs_params(input, ABS_Y, 0, 0x7FF, 0, 0);
    input_set_abs_params(input, ABS_PRESSURE, 0, 0xffff, 0, 0);

    input_set_abs_params(input, ABS_MT_POSITION_X, 0, 0x7ff, 0, 0);
    input_set_abs_params(input, ABS_MT_POSITION_Y, 0, 0x7ff, 0, 0);
    input_set_abs_params(input, ABS_MT_PRESSURE, 0, 0xffff, 0, 0);

    input_set_abs_params(input, ABS_BRAKE, 0, 0xffff, 0, 0);
    input_set_abs_params(input, ABS_MISC, 0, 0xffff, 0, 0);

    input->name = client->name;
    input->id.bustype = BUS_I2C;
    input->dev.parent = &client->dev;
    input->open = zforce_ts_open;
    input->close = zforce_ts_close;

    input_set_drvdata(input, priv);
    err = input_register_device(input);
    if (err)
        goto err1;

    pdata = client->dev.platform_data;
    priv->dr_gpio = pdata->dr_gpio;
    priv->reset_gpio = pdata->reset_gpio;
    priv->test_gpio = pdata->test_gpio;

    /* Power on, reset */
    // zforce_init(priv);

    err = gpio_request(priv->dr_gpio, "zforce_data_ready");
    if (err < 0) {
        dev_err(&client->dev, "failed to request GPIO %d, error %d\n",
            priv->dr_gpio, err);
        goto err2;
    }

    priv->client = client;
    priv->input = input;

    INIT_WORK(&priv->work, zforce_ts_interrupt_handler);

    gpio_direction_input(priv->dr_gpio);
    err = request_irq(gpio_to_irq(priv->dr_gpio), data_ready_irq, IRQF_TRIGGER_FALLING, client->name, priv);
    if (err)
    {
        dev_err(&client->dev, "Request irq for zForce touchscreen failed.\n");
        goto err3;
    }
     
    disable_irq(gpio_to_irq(priv->dr_gpio));

    i2c_set_clientdata(client, priv);

    if (misc_register(&zforce_miscdev) < 0 )
    {
        printk(KERN_ALERT"Unable to register divice node /dev/zforce_io\n");
        err = -ENODEV;
        goto err3;
    }
    return 0;

err3:
    gpio_free(priv->dr_gpio);
err2:
    input_unregister_device(input);
err1:
    input_free_device(input);
err0:
    return err;
}

static int __devexit zforce_ts_remove(struct i2c_client *client)
{
    struct zforce_ts_priv *priv = i2c_get_clientdata(client);

    activate_ts(priv->client, 0);
    misc_deregister(&zforce_miscdev);
    free_irq(gpio_to_irq(priv->dr_gpio), priv);
    gpio_free(priv->dr_gpio);
    input_unregister_device(priv->input);
    input_free_device(priv->input);

    return 0;
}

#ifdef CONFIG_PM

extern int keypad_wakeup_enable; 

static int zforce_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
    struct zforce_ts_priv *priv = i2c_get_clientdata(client);
    if (!keypad_wakeup_enable)
    {
        activate_ts(priv->client, 0);
        retry_count = 0;
        disable_irq(gpio_to_irq(priv->dr_gpio));
    }
    else
    {
        enable_irq_wake(gpio_to_irq(priv->dr_gpio));
    }
    return 0;
}

static int zforce_ts_resume(struct i2c_client *client)
{
    struct zforce_ts_priv *priv = i2c_get_clientdata(client);
    if (!keypad_wakeup_enable)
    {
        enable_irq(gpio_to_irq(priv->dr_gpio));

    	if (!activate_ts(priv->client, 1)){
	        zforce_ts_set_res(priv->client, priv->x_res, priv->y_res);
	        zforce_ts_set_config(priv->client, mcu_config);
	        zforce_ts_set_scanfreq(priv->client, 33, 100);
	        zforce_ts_report_status(priv->client);
	        zforce_ts_request_touch_data(priv->client);
		}
    }
    else
    {
        disable_irq_wake(gpio_to_irq(priv->dr_gpio));
    }
    return 0;
}
#else
#define zforce_ts_suspend NULL
#define zforce_ts_resume NULL
#endif

static const struct i2c_device_id zforce_ts_id[] = {
    { "zforce_ts", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, zforce_ts_id);

static struct i2c_driver zforce_ts_driver = {
    .driver = {
        .name = "zforce_ts",
    },
    .probe = zforce_ts_probe,
    .remove = __devexit_p(zforce_ts_remove),
    .suspend = zforce_ts_suspend,
    .resume = zforce_ts_resume,
    .id_table = zforce_ts_id,
};

static int __init zforce_ts_init(void)
{
    return i2c_add_driver(&zforce_ts_driver);
}

static void __exit zforce_ts_exit(void)
{
    i2c_del_driver(&zforce_ts_driver);
}

MODULE_DESCRIPTION("Onyx IR Touchscreen driver");
MODULE_AUTHOR("Ivan Li <ivan@onyx-international.com>");
MODULE_LICENSE("GPL");

module_init(zforce_ts_init);
module_exit(zforce_ts_exit);
