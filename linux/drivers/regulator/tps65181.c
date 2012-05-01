#include <linux/module.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <mach/gpio.h>

struct tps65181_private
{
	int vcom;
	struct i2c_client *client;
};

extern void enable_eink_pmic(int enable);
static struct tps65181_private* tps65181_priv = NULL;

int tps65181_get_temperature(char* temp)
{
	int ret;
	if (!tps65181_priv)
		return -ENODEV;

	/* Start temperature sampling */
	ret = i2c_smbus_write_byte_data(tps65181_priv->client, 0xC, 0x80);
	if (ret < 0)
	{
		printk(KERN_ERR "Write 0x%02x[0xC] failed.\n", tps65181_priv->client->addr);
		return ret;
	}

	/* Wait until conversion result is ready */
	do
	{
		ret = i2c_smbus_read_byte_data(tps65181_priv->client, 0xC);
		if (ret < 0)
		{
			printk(KERN_ERR "Read 0x%02x[0xC] failed.\n", tps65181_priv->client->addr);
			return ret;
		}
	} while ((ret & 0x20) == 0);

	ret = i2c_smbus_read_byte_data(tps65181_priv->client, 0x0);
	if (ret < 0)
	{
		printk(KERN_ERR "Read 0x%02x[0x0] failed.\n", tps65181_priv->client->addr);
		return ret;
	}

	*(u8 *)temp = ret & 0xFF;
	return 0;
}
EXPORT_SYMBOL(tps65181_get_temperature);

int enable_vdd(int enable)
{
	int err;
	if (!tps65181_priv)
		return -ENODEV;

	if (1 == enable)
	{
		err = i2c_smbus_write_byte_data(tps65181_priv->client, 0x1, 0xAF);
		mdelay(8);
		err = i2c_smbus_write_byte_data(tps65181_priv->client, 0x1, 0xBF);
	}
	else
	{
		mdelay(5);
		err = i2c_smbus_write_byte_data(tps65181_priv->client, 0x1, 0x6F);
		mdelay(20);
		err = i2c_smbus_write_byte_data(tps65181_priv->client, 0x1, 0x60);
		mdelay(5);
		err = i2c_smbus_write_byte_data(tps65181_priv->client, 0x1, 0x40);
	}

	return err;
}
EXPORT_SYMBOL(enable_vdd);

int set_vcom_voltage(void)
{
	if (!tps65181_priv)
		return -ENODEV;

	i2c_smbus_write_byte_data(tps65181_priv->client, 0x3, 0xA3);
	return i2c_smbus_write_byte_data(tps65181_priv->client, 0x4, tps65181_priv->vcom * 0xFF / 2750);

}
EXPORT_SYMBOL(set_vcom_voltage);

static ssize_t vcom_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int val;
	if (sscanf(buf, "%d", &val) == 1)
	{
		tps65181_priv->vcom = val;
		return count;
	}

	return -EINVAL;
}

static ssize_t vcom_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", tps65181_priv->vcom);
}

static struct device_attribute vcom_attr =
	__ATTR(vcom, S_IRUSR | S_IWUSR, vcom_show, vcom_store);

static int __devinit tps65181_i2c_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{

	int ret;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;

	ret = i2c_smbus_read_byte_data(client, 0x10);
	if (ret < 0)
	{
		printk(KERN_ERR "Read 0x%02x[0x10] failed.\n", client->addr);
		return ret;
	}

	tps65181_priv = kzalloc(sizeof(struct tps65181_private), GFP_KERNEL);
	tps65181_priv->client = client;
	tps65181_priv->vcom = 1300;

	printk("Tps65181 found, i2c addrss is 0x%x\n",client->addr);
	ret = i2c_smbus_write_byte_data(client, 0x3, 0xA3);
	if (ret < 0)
	{
		printk(KERN_ERR "Failed writing register 0x3.\n");
		return ret;
	}

	device_create_file(&client->dev, &vcom_attr);
	return 0;
}

static int tps65181_i2c_suspend(struct i2c_client *client, pm_message_t mesg)
{
	enable_eink_pmic(0);
	return 0;
}

static int tps65181_i2c_resume(struct i2c_client *client)
{
	enable_eink_pmic(1);
	return 0;
}

static int __devexit tps65181_i2c_remove(struct i2c_client *client)
{

	if (tps65181_priv)
		kfree(tps65181_priv);
	return 0;
}

static const struct i2c_device_id tps65181_id[] = {
       { "tps65181", 0 },
       { }
};

static struct i2c_driver tps65181_i2c_driver = {
	.driver = {
		.name = "TPS65181",
		.owner = THIS_MODULE,
	},
	.probe    = tps65181_i2c_probe,
	.suspend  = tps65181_i2c_suspend,
	.resume   = tps65181_i2c_resume,
	.remove   = __devexit_p(tps65181_i2c_remove),
	.id_table = tps65181_id,
};

static int __init tps65181_init(void)
{
	return i2c_add_driver(&tps65181_i2c_driver);
}

static void __exit tps65181_exit(void)
{
	i2c_del_driver(&tps65181_i2c_driver);
}

module_init(tps65181_init);
module_exit(tps65181_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivan Li <ivan@onyx-international.com>");
MODULE_DESCRIPTION("E-ink display PMIC driver");
