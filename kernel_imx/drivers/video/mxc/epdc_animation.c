#include <linux/fb.h>
#include <linux/mxcfb.h>
#include <linux/mxcfb_epdc_kernel.h>
#include <linux/platform_device.h>

#define ANIMATION_TIMER  40
#define NUM_RECTS        10

struct onyx_animation
{
	struct delayed_work progress_work;
	struct fb_info *info;
};

static unsigned int    progress = 0;
static int    anim_height = 25;
static __u32  marker_val = 9573476;

static int update_to_display(struct fb_info *info, int left, int top, int width, int height,
	int wave_mode, uint flags)
{
	int ret = 0;
	struct mxcfb_update_data update;

	update.update_region.left = left;
	update.update_region.width = width;
	update.update_region.top = top;
	update.update_region.height = height;
	update.update_mode = UPDATE_MODE_FULL;
	update.waveform_mode = wave_mode;
	update.update_marker = marker_val++;
	update.temp = TEMP_USE_AMBIENT;
	update.flags = flags;

	mxc_epdc_fb_send_update(&update, info);
	ret = mxc_epdc_fb_wait_update_complete(update.update_marker, info);
	if (ret < 0)
		printk(KERN_ERR
			"Wait for update complete failed.  Error = 0x%x", ret);
	return ret;
}

void draw_progress_bar(struct work_struct *work)
{
	int i;
	u32 x = 0, y = 0, w = 0, h = 0;
	struct onyx_animation* anim = container_of(work, struct onyx_animation, progress_work.work);
	struct fb_info *info = anim->info;
	int bytes_per_pixel = info->var.bits_per_pixel / 8;
	int row_stride = ALIGN(info->var.xres, 32) * bytes_per_pixel;

	// Draw internal rectange and fill with black.
	x = 3;
	w = anim_height - 6;
	h = (info->var.yres - 6) / NUM_RECTS - 2;
	y = 4 + (2 + h) * progress + min(progress, (info->var.yres - 6) % NUM_RECTS);

	if (progress < (info->var.yres - 6) % NUM_RECTS)
	{
		h++;
	}

	if (info->var.xres == 1200)
	{
		x = info->var.xres - (anim_height - 3);
		y = info->var.yres - y - h;
	}

	for (i = 0; i < h; i++)
	{
		memset(info->screen_base + (y + i) * row_stride + x * bytes_per_pixel,
			0, w * bytes_per_pixel);
	}

	update_to_display(info, x - 3, 0, anim_height, info->var.yres, WAVEFORM_MODE_AUTO, 0);

	if (++progress < NUM_RECTS)
	{
		// Reschedule work
		schedule_delayed_work(&anim->progress_work, ANIMATION_TIMER);
	}
	else
	{
		kfree(anim);
	}
}

extern struct platform_device epdc_device;

int epdc_blank_screen(void)
{
	struct fb_info *info = dev_get_drvdata(&epdc_device.dev);

	return update_to_display(info, 0, 0, info->var.xres, info->var.yres, WAVEFORM_MODE_GC16, 0);
}

static int draw_outline(struct fb_info *info)
{
	int i, x = 0;
	int bytes_per_pixel = info->var.bits_per_pixel / 8;
	int row_stride = ALIGN(info->var.xres, 32) * bytes_per_pixel;

	for (i = 0; i < info->var.yres; i++)
	{
		void* s = info->screen_base + i * row_stride;
		if (info->var.xres == 1200)
		{
			s += (info->var.xres - anim_height) * bytes_per_pixel;
		}

		if (i >= 2 && i < info->var.yres - 2)
		{
			memset(s, 0xFF, anim_height * bytes_per_pixel);
			memset(s, 0, 2 * bytes_per_pixel);
			memset(s + (anim_height - 2) * bytes_per_pixel, 0, 2 * bytes_per_pixel);
		}
		else
		{
			memset(s, 0, anim_height * bytes_per_pixel);
		}
	}

	if (info->var.xres == 1200)
	{
		x = info->var.xres - anim_height;
	}

	return update_to_display(info, x, 0, anim_height, info->var.yres, WAVEFORM_MODE_GC16, 0);
}

int mxc_start_animation(struct fb_info *info)
{
	struct onyx_animation* anim = NULL;

	printk("Onyx animation splash loaded.\n");
	anim = kzalloc(sizeof(struct onyx_animation), GFP_KERNEL);
	if (!anim)
		return -ENOMEM;

	anim->info = info;
	if (info->var.yres != 600)
	{
		anim_height = 25 * info->var.yres / 600;
	}

	INIT_DELAYED_WORK(&anim->progress_work, draw_progress_bar);
	schedule_delayed_work(&anim->progress_work, ANIMATION_TIMER);
	return draw_outline(info);
}
