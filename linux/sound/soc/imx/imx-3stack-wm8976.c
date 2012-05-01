/*
 * imx-3stack-wm8976.c  --  i.MX 3Stack Driver for Freescale WM8976 Codec
 *
 * Copyright (C) 2008-2011 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  Revision history
 *    21th Oct 2008   Initial version.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/bitops.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/fsl_devices.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>

#include <mach/dma.h>
#include <mach/clock.h>

#include "../codecs/wm8976.h"
#include "imx-ssi.h"
#include "imx-pcm.h"

static int imx_3stack_startup(struct snd_pcm_substream *substream)
{
	/* Enable SSI_EXT1_CLK for WM8976 MCLK, 27MHz */
	struct clk* ssi_ext1_clk = clk_get(NULL, "ssi_ext1_clk");
	clk_enable(ssi_ext1_clk);
	clk_put(ssi_ext1_clk);
	return 0;
}

static int imx_3stack_audio_hw_params(struct snd_pcm_substream *substream,
				      struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai_link *machine = rtd->dai;
	struct snd_soc_dai *cpu_dai = machine->cpu_dai;
	struct snd_soc_dai *codec_dai = machine->codec_dai;
	unsigned int rate = params_rate(params);
	unsigned int channels = params_channels(params);
	struct imx_ssi *ssi_mode = (struct imx_ssi *)cpu_dai->private_data;
	int ret = 0;
	u32 dai_format;

	ret = snd_soc_dai_set_pll(codec_dai, 0, 32768, 27000000, ((rate % 8000) == 0) ? 12288000 : 11289600);
	if (ret < 0) {
			printk(KERN_ERR "can't set codec system clock. ret = %d\n", ret);
			return ret;
	}

	ret = snd_soc_dai_set_clkdiv(codec_dai, WM8976_MCLKSEL, WM8976_MCLK_PLL); /* enable the pll */
	if (ret < 0) {
			printk(KERN_ERR "can't set codec clksel. ret = %d\n", ret);
			return ret;
	}

	dai_format = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
	    SND_SOC_DAIFMT_CBM_CFM;

	ssi_mode->sync_mode = 1;
	if (channels == 1)
		ssi_mode->network_mode = 0;
	else
		ssi_mode->network_mode = 1;

	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, dai_format);
	if (ret < 0)
		return ret;

	/* set i.MX active slot mask */
	snd_soc_dai_set_tdm_slot(cpu_dai,
				 channels == 1 ? 0xfffffffe : 0xfffffffc,
				 channels == 1 ? 0xfffffffe : 0xfffffffc,
				 2, 32);

	/* set cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, dai_format);
	if (ret < 0)
		return ret;

	/* set the SSI system clock as input (unused) */
	snd_soc_dai_set_sysclk(cpu_dai, IMX_SSP_SYS_CLK, 0, SND_SOC_CLOCK_IN);

	return 0;
}

static void imx_3stack_shutdown(struct snd_pcm_substream *substream)
{
	struct clk* ssi_ext1_clk = clk_get(NULL, "ssi_ext1_clk");
	clk_disable(ssi_ext1_clk);
	clk_put(ssi_ext1_clk);
}

/*
 * imx_3stack WM8976 audio DAI opserations.
 */
static struct snd_soc_ops imx_3stack_ops = {
	.startup = imx_3stack_startup,
	.shutdown = imx_3stack_shutdown,
	.hw_params = imx_3stack_audio_hw_params,
};

static void imx_3stack_init_dam(int ssi_port, int dai_port)
{
	unsigned int ssi_ptcr = 0;
	unsigned int dai_ptcr = 0;
	unsigned int ssi_pdcr = 0;
	unsigned int dai_pdcr = 0;

	/* WM8976 uses SSI2 via AUDMUX port dai_port for audio */
	/* reset port ssi_port & dai_port */
	__raw_writel(0, DAM_PTCR(ssi_port));
	__raw_writel(0, DAM_PTCR(dai_port));
	__raw_writel(0, DAM_PDCR(ssi_port));
	__raw_writel(0, DAM_PDCR(dai_port));

	/* set to synchronous */
	ssi_ptcr |= AUDMUX_PTCR_SYN;
	dai_ptcr |= AUDMUX_PTCR_SYN;

	/* set Rx sources ssi_port <--> dai_port */
	ssi_pdcr |= AUDMUX_PDCR_RXDSEL(dai_port);
	dai_pdcr |= AUDMUX_PDCR_RXDSEL(ssi_port);

	/* set Tx frame direction and source  dai_port--> ssi_port output */
	ssi_ptcr |= AUDMUX_PTCR_TFSDIR;
	ssi_ptcr |= AUDMUX_PTCR_TFSSEL(AUDMUX_FROM_TXFS, dai_port);

	/* set Tx Clock direction and source dai_port--> ssi_port output */
	ssi_ptcr |= AUDMUX_PTCR_TCLKDIR;
	ssi_ptcr |= AUDMUX_PTCR_TCSEL(AUDMUX_FROM_TXFS, dai_port);

	__raw_writel(ssi_ptcr, DAM_PTCR(ssi_port));
	__raw_writel(dai_ptcr, DAM_PTCR(dai_port));
	__raw_writel(ssi_pdcr, DAM_PDCR(ssi_port));
	__raw_writel(dai_pdcr, DAM_PDCR(dai_port));
}

static int imx_3stack_wm8976_init(struct snd_soc_codec *codec)
{
	return 0;
}

/* imx_3stack digital audio interface glue - connects codec <--> CPU */
static struct snd_soc_dai_link imx_3stack_dai = {
	.name = "WM8976",
	.stream_name = "WM8976",
	.codec_dai = &wm8976_dai,
	.init = imx_3stack_wm8976_init,
	.ops = &imx_3stack_ops,
};

static int imx_3stack_card_remove(struct platform_device *pdev)
{
	return 0;
}

static struct snd_soc_card snd_soc_card_imx_3stack = {
	.name = "imx-3stack",
	.platform = &imx_soc_platform,
	.dai_link = &imx_3stack_dai,
	.num_links = 1,
	.remove = imx_3stack_card_remove,
};

static struct snd_soc_device imx_3stack_snd_devdata = {
	.card = &snd_soc_card_imx_3stack,
	.codec_dev = &soc_codec_dev_wm8976,
};

static int __devinit imx_3stack_wm8976_probe(struct platform_device *pdev)
{
	struct mxc_audio_platform_data *plat = pdev->dev.platform_data;
	struct snd_soc_dai *wm8976_cpu_dai = NULL;

	gpio_activate_audio_ports();
	imx_3stack_init_dam(plat->src_port, plat->ext_port);

	if (plat->src_port == 2)
		wm8976_cpu_dai = imx_ssi_dai[2];
	else if (plat->src_port == 1)
		wm8976_cpu_dai = imx_ssi_dai[0];
	else if (plat->src_port == 7)
		wm8976_cpu_dai = imx_ssi_dai[4];

	imx_3stack_dai.cpu_dai = wm8976_cpu_dai;
	return 0;
}

static int imx_3stack_wm8976_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver imx_3stack_wm8976_audio_driver = {
	.probe = imx_3stack_wm8976_probe,
	.remove = imx_3stack_wm8976_remove,
	.driver = {
		   .name = "imx-3stack-wm8xxx",
		   },
};

static struct platform_device *imx_3stack_snd_device;

static int __init imx_3stack_init(void)
{
	int ret;

	ret = platform_driver_register(&imx_3stack_wm8976_audio_driver);
	if (ret)
		return -ENOMEM;

	imx_3stack_snd_device = platform_device_alloc("soc-audio", 2);
	if (!imx_3stack_snd_device)
		return -ENOMEM;

	platform_set_drvdata(imx_3stack_snd_device, &imx_3stack_snd_devdata);
	imx_3stack_snd_devdata.dev = &imx_3stack_snd_device->dev;
	ret = platform_device_add(imx_3stack_snd_device);

	if (ret)
		platform_device_put(imx_3stack_snd_device);

	return ret;
}

static void __exit imx_3stack_exit(void)
{
	platform_driver_unregister(&imx_3stack_wm8976_audio_driver);
	platform_device_unregister(imx_3stack_snd_device);
}

module_init(imx_3stack_init);
module_exit(imx_3stack_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("WM8976 Driver for i.MX 3STACK");
MODULE_LICENSE("GPL");
