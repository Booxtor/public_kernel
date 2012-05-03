/*
 * Onyx hanvon touchscreen driver
 *
 * Copyright (C) 2009-2011 Onyx international, Inc. All Rights Reserved.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

/*!
 * @file hanvon_ts.c
 *
 * @brief Driver for the Onyx hanvon touchscreen with calibration support.
 *
 * The touchscreen driver is designed as a standard input driver which is a
 * wrapper over serial ttymxc1. During initialization, this driver creates
 * a kernel thread. This thread then calls serial to obtain touchscreen values
 * continously. These values are then passed to the input susbsystem.
 *
 * @ingroup touchscreen
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/freezer.h>
#include <linux/kthread.h>
#include <asm/termios.h>
#include <asm/ioctls.h>
#include <linux/serial.h>
#include <linux/slab.h>
#include <linux/poll.h>

#define MXC_TS_NAME	"hanvon_ts"
#define PACKAGE_SIZE	7
#define INSTATUS_SYNC	0x80       //A0 or 80 
#define HANVON_MIN_XC	0
#define HANVON_MAX_XC	1200
#define HANVON_MIN_YC	0
#define HANVON_MAX_YC	825


struct hanvon_coord{
	int x;
	int y;
	int pressure;
};

struct hanvon_ts{
	struct task_struct *tstask;
	struct input_dev *inputdev;
	struct file  *tty_filp;
};

struct hanvon_ts  *hanvon_ts;

static long tty_ioctl(struct file *fp, unsigned op, unsigned long param)
{
	if (fp->f_op->unlocked_ioctl)
		return fp->f_op->unlocked_ioctl(fp, op, param);

	return -ENOSYS;
}

static unsigned char  tty_read(struct file *fp, int timeout)
{
	unsigned char result;

	result = -1;
	if (!IS_ERR(fp)) {
		mm_segment_t oldfs;

		oldfs = get_fs();
		set_fs(KERNEL_DS);
		if (fp->f_op->poll) {
			struct poll_wqueues table;
			struct timeval start, now;

			do_gettimeofday(&start);
			poll_initwait(&table);
			while (1) {
				long elapsed;
				int mask;
				
				mask = fp->f_op->poll(fp, &table.pt);
				if (mask & (POLLRDNORM | POLLRDBAND | POLLIN |
							POLLHUP | POLLERR)) {
					break;
				}
				do_gettimeofday(&now);
				elapsed =
					(1000000 * (now.tv_sec - start.tv_sec) +
					 now.tv_usec - start.tv_usec);
				if (elapsed > timeout) {
					break;
				}
				set_current_state(TASK_INTERRUPTIBLE);
				schedule_timeout(((timeout -
								elapsed) * HZ) / 10000);
			}
			poll_freewait(&table);
			{
				unsigned char ch;
				
				fp->f_pos = 0;
				if (fp->f_op->read(fp, &ch, 1, &fp->f_pos) == 1) {
					result = ch;
				}
			}
		} else {
			/* Device does not support poll, busy wait */
			int retries = 0;
			while (1) {
				unsigned char ch;
				
				retries++;
				if (retries >= timeout) {
					break;
				}
				
				fp->f_pos = 0;
				if (fp->f_op->read(fp, &ch, 1, &fp->f_pos) == 1) {
					result = ch;
					break;
				}
				udelay(100);
			}
		}
		set_fs(oldfs);
	}
	return result;
}


static void hanvon_parse_data(unsigned char *data, struct hanvon_coord *coord)
{
	int sync_pos = 0;
	int temp;

	coord->y = ((data[sync_pos + 6]>> 3) & 0x3) | ((data[sync_pos + 4] << 2) | (data[sync_pos + 3] << 9));
	coord->x = ((data[sync_pos + 6] & 0x60)>> 5) | ((data[sync_pos + 2] << 2) | (data[sync_pos + 1] << 9));
	coord->pressure = ((data[sync_pos + 6]&0x07) << 7) | data[sync_pos + 5];

	/*for freescale imx508 9.7 inch panel*/
	coord->y = coord->y * 1200 / 0x2000;
	coord->x = 825 - coord->x * 825 / 0x1800;
}

static int ts_thread(void *arg)
{
	struct  hanvon_ts *hanvon = (struct hanvon_ts *)arg;
	struct file *fp = hanvon->tty_filp;
	unsigned char event_buffer[PACKAGE_SIZE];
	struct hanvon_coord coord;
	unsigned char data;
	int i =0;
	int idx = 0;

	do {
		data = tty_read(fp, 1000);
		event_buffer[idx] = data;
		switch(idx++){
		case 0:
			if (!(event_buffer[0] & INSTATUS_SYNC))
			{
				idx = 0;
			}
			if (event_buffer[0] & 0x10)
			{
				idx = 0;
			}
		break;
		case 6:
			for (i = 1; i < PACKAGE_SIZE; ++i)
			{
				if (event_buffer[i] & INSTATUS_SYNC)
				{
					idx = 0;
					goto out;
				}
			}

			hanvon_parse_data(&event_buffer[0], &coord);
			if (coord.pressure){
				input_report_abs(hanvon->inputdev, ABS_X, coord.y);
				input_report_abs(hanvon->inputdev, ABS_Y, coord.x);
				input_report_abs(hanvon->inputdev, ABS_PRESSURE, coord.pressure);
				input_report_key(hanvon->inputdev, BTN_TOUCH, 1);
			}
			else
			{
				input_report_abs(hanvon->inputdev, ABS_X, coord.y);
				input_report_abs(hanvon->inputdev, ABS_Y, coord.x);
				input_report_abs(hanvon->inputdev, ABS_PRESSURE, coord.pressure);
				input_report_key(hanvon->inputdev, BTN_TOUCH, 0);
			}

			input_sync(hanvon->inputdev);
			idx = 0;
		out:
			break;
		}

	} while (!kthread_should_stop());

	return 0;
}

static void tty_set_termios(struct file *fp)
{
	mm_segment_t oldfs;
	struct termios settings;
	
	oldfs = get_fs();
	set_fs(KERNEL_DS);

	tty_ioctl(fp, TCGETS, (unsigned long)&settings);

	settings.c_iflag = 0;
	settings.c_oflag = 0;
	settings.c_lflag = 0;
	settings.c_cflag = CLOCAL | CS8 | CREAD;
	settings.c_cc[VMIN] = 0;
	settings.c_cc[VTIME] = 0;

	settings.c_cflag |= B19200;

	tty_ioctl(fp, TCSETS, (unsigned long)&settings);

	set_fs(oldfs);

}

static int __init hanvon_ts_init(void)
{
	int err = 0;

	hanvon_ts = kmalloc(sizeof(struct hanvon_ts), GFP_KERNEL);
	if (!hanvon_ts) {
		pr_err("Failed to allocate for hanvon touchscreen device\n");
		err = -ENOMEM;
		goto fail1;
	}

	hanvon_ts->tty_filp = filp_open("/dev/ttymxc1", O_RDONLY, 0);
	if (IS_ERR(hanvon_ts->tty_filp)){
		err = -ENODEV;
		goto fail2;
	}

	tty_set_termios(hanvon_ts->tty_filp);

	hanvon_ts->inputdev = input_allocate_device();
	if (!hanvon_ts->inputdev) {
		printk(KERN_ERR"hanvon_ts_init: not enough memory\n");
		err = -ENOMEM;
		goto fail2;
	}

	hanvon_ts->inputdev->name = MXC_TS_NAME;
	hanvon_ts->inputdev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) |BIT_MASK(EV_SYN);
	hanvon_ts->inputdev->keybit[BIT_WORD(BTN_TOUCH)] |= BIT_MASK(BTN_TOUCH);
	
	input_set_abs_params(hanvon_ts->inputdev, ABS_X, HANVON_MIN_XC, HANVON_MAX_XC, 0, 0);
	input_set_abs_params(hanvon_ts->inputdev, ABS_Y, HANVON_MIN_YC, HANVON_MAX_YC, 0, 0);
	input_set_abs_params(hanvon_ts->inputdev, ABS_PRESSURE, 0, 1024, 0, 0);
	
	err = input_register_device(hanvon_ts->inputdev);
	if (err < 0) {
		goto fail3;
	}

	hanvon_ts->tstask = kthread_run(ts_thread, hanvon_ts, "hanvon_ts");
	if (IS_ERR(hanvon_ts->tstask)) {
		printk(KERN_ERR"hanvon_ts_init: failed to create kthread");
		hanvon_ts->tstask = NULL;
		err = -1;
		goto fail3;
	}

	printk("hanvon input touchscreen loaded\n");

	return err;

fail3:
	input_free_device(hanvon_ts->inputdev);
fail2:
	kfree(hanvon_ts);
	hanvon_ts = NULL;
fail1:

	return err;
}

static void __exit hanvon_ts_exit(void)
{
	if (hanvon_ts->tstask)
		kthread_stop(hanvon_ts->tstask);

	input_unregister_device(hanvon_ts->inputdev);

	if (hanvon_ts->inputdev) {
		input_free_device(hanvon_ts->inputdev);
		hanvon_ts->inputdev = NULL;
	}

	filp_close(hanvon_ts->tty_filp, 0);

	kfree(hanvon_ts);
	hanvon_ts = NULL;
}

late_initcall(hanvon_ts_init);
module_exit(hanvon_ts_exit);

MODULE_DESCRIPTION("Hanvon touchscreen driver with calibration");
MODULE_AUTHOR("Jeach (Onyx International Ltd.)");
MODULE_LICENSE("GPL");
