#ifndef LINUX_INPUT_ZFORCE_TS_H
#define LINUX_INPUT_ZFORCE_TS_H

#define IOCTL_ZFORCE_MAGIC		'J'

struct zforce_abs{
	unsigned short x;
	unsigned short y;
};

struct zforce_scanfreq{
	unsigned char idle_scanfreq;
	unsigned char full_scanfreq;
};

struct zforce_fw_version{
	unsigned short major;
	unsigned short minor;
	unsigned short build;
	unsigned short revision;
};

#define ZFORCE_SET_ABS		_IOW(IOCTL_ZFORCE_MAGIC, 0, struct zforce_abs *)
#define ZFORCE_SET_CONFIG	_IOW(IOCTL_ZFORCE_MAGIC, 1, int *)
#define ZFORCE_SET_SCANFREQ	_IOW(IOCTL_ZFORCE_MAGIC, 2, struct zforce_scanfreq *)
#define ZFORCE_GET_VERSION	_IOR(IOCTL_ZFORCE_MAGIC, 3, struct zforce_fw_version)
#define ZFORCE_GET_LEDLEVEL	_IOR(IOCTL_ZFORCE_MAGIC, 4, struct ledlevelresponse)
#define ZFORCE_RESET            _IOW(IOCTL_ZFORCE_MAGIC, 5, int *)

struct zforce_ts_platform_data {
	unsigned int dr_gpio;
	unsigned int reset_gpio;
	unsigned int test_gpio;
	unsigned int mcu_pwr_gpio;
};

#endif /* LINUX_INPUT_ZFORCE_TS_H */

