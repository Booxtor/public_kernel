/*
 * Copyright (C) 2010-2011 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/nodemask.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/fsl_devices.h>
#include <linux/spi/spi.h>
#include <linux/android_pmem.h>
#include <linux/usb/android_composite.h>
#include <linux/i2c.h>
#include <linux/ata.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/machine.h>
#include <linux/pmic_external.h>
#include <linux/pmic_status.h>
#include <linux/videodev2.h>
#include <linux/mxcfb.h>
#include <linux/fec.h>
#include <linux/gpmi-nfc.h>
#include <linux/gpio_keys.h>
#include <linux/input/zforce_ts.h>
#include <asm/mach/keypad.h>
#include <asm/irq.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/mach/flash.h>
#include <mach/common.h>
#include <mach/hardware.h>
#include <mach/arc_otg.h>
#include <mach/memory.h>
#include <mach/gpio.h>
#include <mach/mmc.h>
#include <mach/mxc_dvfs.h>
#include <mach/iomux-mx50.h>
#include <mach/i2c.h>
#include <mach/check_fuse.h>

#include "devices.h"
#include "crm_regs.h"
#include "usb.h"
#include "dma-apbh.h"

#define SD1_WP	(3*32 + 19)	/*GPIO_4_19 */
#define SD1_CD	(0*32 + 27)	/*GPIO_1_27 */
#define HP_DETECT	(3*32 + 16)	/*GPIO_4_16 */
#define PWR_INT		(3*32 + 18)	/*GPIO_4_18 */

#define EPDC_D0		(2*32 + 0)	/*GPIO_3_0 */
#define EPDC_D1		(2*32 + 1)	/*GPIO_3_1 */
#define EPDC_D2		(2*32 + 2)	/*GPIO_3_2 */
#define EPDC_D3		(2*32 + 3)	/*GPIO_3_3 */
#define EPDC_D4		(2*32 + 4)	/*GPIO_3_4 */
#define EPDC_D5		(2*32 + 5)	/*GPIO_3_5 */
#define EPDC_D6		(2*32 + 6)	/*GPIO_3_6 */
#define EPDC_D7		(2*32 + 7)	/*GPIO_3_7 */
#define EPDC_GDCLK	(2*32 + 16)	/*GPIO_3_16 */
#define EPDC_GDSP	(2*32 + 17)	/*GPIO_3_17 */
#define EPDC_GDOE	(2*32 + 18)	/*GPIO_3_18 */
#define EPDC_GDRL	(2*32 + 19)	/*GPIO_3_19 */
#define EPDC_SDCLK	(2*32 + 20)	/*GPIO_3_20 */
#define EPDC_SDOE	(2*32 + 23)	/*GPIO_3_23 */
#define EPDC_SDLE	(2*32 + 24)	/*GPIO_3_24 */
#define EPDC_SDSHR	(2*32 + 26)	/*GPIO_3_26 */
#define EPDC_BDR0	(3*32 + 23)	/*GPIO_4_23 */
#define EPDC_SDCE0	(3*32 + 25)	/*GPIO_4_25 */
#define EPDC_SDCE1	(3*32 + 26)	/*GPIO_4_26 */
#define EPDC_SDCE2	(3*32 + 27)	/*GPIO_4_27 */

#define EPDC_PMIC_WAKE		(5*32 + 16)	/*GPIO_6_16 */
#define EPDC_PMIC_INT		(5*32 + 17)	/*GPIO_6_17 */
#define EPDC_VCOM	(2*32 + 27)	/*GPIO_3_27 */
#define EPDC_PWRSTAT	(2*32 + 28)	/*GPIO_3_28 */
#define EPDC_ELCDIF_BACKLIGHT	(1*32 + 18)	/*GPIO_2_18 */
#define CSPI_CS1	(3*32 + 13)	/*GPIO_4_13 */
#define CSPI_CS2	(3*32 + 11) /*GPIO_4_11*/
#define TP_DETECT	(3*32 + 12) /*GPIO_4_12*/
#define USB_OTG_PWR	(5*32 + 25) /*GPIO_6_25*/
#define MODEM_PWR	(0*32 + 20) /*GPIO_1_20*/
#define MODEM_RST	(0*32 + 18) /*GPIO_1_18*/
#define AP_WKUP_3G	(0*32 + 17) /*GPIO_1_17*/
#define IR_RESET_TP_SLP		(5*32 + 14) /*GPIO_6_14*/
#define IR_TEST_TP_RST		(5*32 + 15) /*GPIO_6_15*/

#define VOLUME_UP	(0*32 + 26) /*GPIO_1_26*/
#define VOLUME_DOWN	(0*32 + 25) /*GPIO_1_25*/
#define OFN_DOME	(3*32 + 5) /*GPIO_4_5*/
#define BACK_KEY	(3*32 + 0)  /*GPIO_4_0*/
#define PAGE_DOWN	(3*32 + 2)  /*GPIO_4_2*/
#define PAGE_UP		(3*32 + 4)  /*GPIO_4_4*/
#define MENU_KEY	(3*32 + 6)  /*GPIO_4_6*/
#define OR_UP		(3*32 + 1) /*GPIO_4_1*/
#define OR_DOWN		(3*32 + 3) /*GPIO_4_3*/
#define OR_LEFT		(3*32 + 14) /*GPIO_4_14*/
#define OR_RIGHT	(3*32 + 15) /*GPIO_4_15*/
#define IR_MCU_PWR	(1*32 + 17) /*GPIO_2_17*/
#define UART2_TXD_GPIO_6_10		(5*32 + 10)
#define UART2_RXD_GPIO_6_11		(5*32 + 11)
#define I2C3_SCL_GPIO_6_22		(5*32 + 22)
#define I2C3_SDA_GPIO_6_23		(5*32 + 23)

#define GPIO_GDIR 4
extern int __init mx50_arm2_init_mc13892(void);
extern struct cpu_wp *(*get_cpu_wp)(int *wp);
extern void (*set_num_cpu_wp)(int num);
extern struct dvfs_wp *(*get_dvfs_core_wp)(int *wp);
static void mx50_suspend_enter(void);
static void mx50_suspend_exit(void);
static int num_cpu_wp;

static iomux_v3_cfg_t  mx50_armadillo2[] = {
	/* SD1 */
	MX50_PAD_ECSPI2_SS0__GPIO_4_19,
	MX50_PAD_EIM_CRE__GPIO_1_27,
	MX50_PAD_SD1_CMD__SD1_CMD,

	MX50_PAD_SD1_CLK__SD1_CLK,
	MX50_PAD_SD1_D0__SD1_D0,
	MX50_PAD_SD1_D1__SD1_D1,
	MX50_PAD_SD1_D2__SD1_D2,
	MX50_PAD_SD1_D3__SD1_D3,

	/* SD2 */
	MX50_PAD_SD2_CD__GPIO_5_17,
	MX50_PAD_SD2_WP__GPIO_5_16,
	MX50_PAD_SD2_CMD__SD2_CMD,
	MX50_PAD_SD2_CLK__SD2_CLK,
	MX50_PAD_SD2_D0__SD2_D0,
	MX50_PAD_SD2_D1__SD2_D1,
	MX50_PAD_SD2_D2__SD2_D2,
	MX50_PAD_SD2_D3__SD2_D3,
	MX50_PAD_SD2_D4__SD2_D4,
	MX50_PAD_SD2_D5__SD2_D5,
	MX50_PAD_SD2_D6__SD2_D6,
	MX50_PAD_SD2_D7__SD2_D7,

	/* SD3 */
	MX50_PAD_SD3_CMD__SD3_CMD,
	MX50_PAD_SD3_CLK__SD3_CLK,
	MX50_PAD_SD3_D0__SD3_D0,
	MX50_PAD_SD3_D1__SD3_D1,
	MX50_PAD_SD3_D2__SD3_D2,
	MX50_PAD_SD3_D3__SD3_D3,
	MX50_PAD_SD3_D4__SD3_D4,
	MX50_PAD_SD3_D5__SD3_D5,
	MX50_PAD_SD3_D6__SD3_D6,
	MX50_PAD_SD3_D7__SD3_D7,

	MX50_PAD_SSI_RXD__SSI_RXD,
	MX50_PAD_SSI_TXD__SSI_TXD,
	MX50_PAD_SSI_TXC__SSI_TXC,
	MX50_PAD_SSI_TXFS__SSI_TXFS,

	/* Neonode zForce touchscreen data ready interrupt */
	MX50_PAD_ECSPI1_SCLK__GPIO_4_12,

	/* Use SSI_EXT1_CLK for audio MCLK */
	MX50_PAD_SSI_EXT1_CLK,

	/* LINE1_DETECT (headphone detect) */
	MX50_PAD_ECSPI2_SCLK__GPIO_4_16,

	/* PWR_INT */
	MX50_PAD_ECSPI2_MISO__GPIO_4_18,

	/* UART pad setting */
	MX50_PAD_UART1_TXD__UART1_TXD,
	MX50_PAD_UART1_RXD__UART1_RXD,
	MX50_PAD_UART1_CTS__UART1_CTS,
	MX50_PAD_UART1_RTS__UART1_RTS,
	MX50_PAD_UART2_TXD__UART2_TXD,
	MX50_PAD_UART2_RXD__UART2_RXD,
	MX50_PAD_UART2_CTS__UART2_CTS,
	MX50_PAD_UART2_RTS__UART2_RTS,

	MX50_PAD_I2C1_SCL__I2C1_SCL,
	MX50_PAD_I2C1_SDA__I2C1_SDA,
	MX50_PAD_I2C2_SCL__I2C2_SCL,
	MX50_PAD_I2C2_SDA__I2C2_SDA,
	MX50_PAD_I2C3_SCL__I2C3_SCL,
	MX50_PAD_I2C3_SDA__I2C3_SDA,

	/* EPDC pins */
	MX50_PAD_EPDC_D0__EPDC_D0,
	MX50_PAD_EPDC_D1__EPDC_D1,
	MX50_PAD_EPDC_D2__EPDC_D2,
	MX50_PAD_EPDC_D3__EPDC_D3,
	MX50_PAD_EPDC_D4__EPDC_D4,
	MX50_PAD_EPDC_D5__EPDC_D5,
	MX50_PAD_EPDC_D6__EPDC_D6,
	MX50_PAD_EPDC_D7__EPDC_D7,
	MX50_PAD_EPDC_GDCLK__EPDC_GDCLK,
	MX50_PAD_EPDC_GDSP__EPDC_GDSP,
	MX50_PAD_EPDC_GDOE__EPDC_GDOE	,
	MX50_PAD_EPDC_GDRL__EPDC_GDRL,
	MX50_PAD_EPDC_SDCLK__EPDC_SDCLK,
	MX50_PAD_EPDC_SDOE__EPDC_SDOE,
	MX50_PAD_EPDC_SDLE__EPDC_SDLE,
	MX50_PAD_EPDC_SDSHR__EPDC_SDSHR,
	MX50_PAD_EPDC_BDR0__EPDC_BDR0,
	MX50_PAD_EPDC_SDCE0__EPDC_SDCE0,
	MX50_PAD_EPDC_SDCE1__EPDC_SDCE1,
	MX50_PAD_EPDC_SDCE2__EPDC_SDCE2,

	MX50_PAD_EPDC_PWRSTAT__GPIO_3_28,
	MX50_PAD_EPDC_VCOM0__GPIO_4_21,

	MX50_PAD_DISP_D8__DISP_D8,
	MX50_PAD_DISP_D9__DISP_D9,
	MX50_PAD_DISP_D10__DISP_D10,
	MX50_PAD_DISP_D11__DISP_D11,
	MX50_PAD_DISP_D12__DISP_D12,
	MX50_PAD_DISP_D13__DISP_D13,
	MX50_PAD_DISP_D14__DISP_D14,
	MX50_PAD_DISP_D15__DISP_D15,
	MX50_PAD_DISP_RS__ELCDIF_VSYNC,

	/* ELCDIF contrast */
	MX50_PAD_DISP_BUSY__GPIO_2_18,

	MX50_PAD_DISP_CS__ELCDIF_HSYNC,
	MX50_PAD_DISP_RD__ELCDIF_EN,
	MX50_PAD_DISP_WR__ELCDIF_PIXCLK,

	/* EPD PMIC WAKEUP */
	MX50_PAD_UART4_TXD__GPIO_6_16,

	/* EPD PMIC intr */
	MX50_PAD_UART4_RXD__GPIO_6_17,

	/* using gpio to control otg pwr */
	MX50_PAD_PWM2__GPIO_6_25,
	MX50_PAD_PWM1__USBOTG_OC,

	MX50_PAD_SSI_RXC__FEC_MDIO,
	MX50_PAD_SSI_RXC__FEC_MDIO,
	MX50_PAD_DISP_D0__FEC_TXCLK,
	MX50_PAD_DISP_D1__FEC_RX_ER,
	MX50_PAD_DISP_D2__FEC_RX_DV,
	MX50_PAD_DISP_D3__FEC_RXD1,
	MX50_PAD_DISP_D4__FEC_RXD0,
	MX50_PAD_DISP_D5__FEC_TX_EN,
	MX50_PAD_DISP_D6__FEC_TXD1,
	MX50_PAD_DISP_D7__FEC_TXD0,
	MX50_PAD_SSI_RXFS__FEC_MDC,

	MX50_PAD_CSPI_SS0__CSPI_SS0,
	MX50_PAD_ECSPI1_MOSI__CSPI_SS1,
	MX50_PAD_CSPI_MOSI__CSPI_MOSI,
	MX50_PAD_CSPI_MISO__CSPI_MISO,

	/* GPIO keys */
	MX50_PAD_EIM_LBA__GPIO_1_26,
	MX50_PAD_EIM_RW__GPIO_1_25,
	MX50_PAD_KEY_COL0__GPIO_4_0,
	MX50_PAD_KEY_COL1__GPIO_4_2,
	MX50_PAD_KEY_COL2__GPIO_4_4,
	MX50_PAD_KEY_COL3__GPIO_4_6,
	MX50_PAD_KEY_ROW0__GPIO_4_1,

	MX50_PAD_KEY_ROW1__GPIO_4_3,
	MX50_PAD_KEY_ROW2__GPIO_4_5,
	MX50_PAD_ECSPI1_MISO__GPIO_4_14,
	MX50_PAD_ECSPI1_SS0__GPIO_4_15,

	MX50_PAD_EIM_EB1__GPIO_1_20,
	MX50_PAD_EIM_CS0__GPIO_1_18,
	MX50_PAD_EIM_CS1__GPIO_1_17,

	MX50_PAD_UART3_TXD__GPIO_6_14,
	MX50_PAD_EPITO__GPIO_6_27,
	MX50_PAD_UART3_RXD__GPIO_6_15,
	/*Audio pan select*/
	MX50_PAD_DISP_D9__GPIO_2_9,
	MX50_PAD_DISP_D14__GPIO_2_14,
	MX50_PAD_DISP_RS__GPIO_2_17,
	MX50_PAD_EIM_OE__GPIO_1_24
};

static iomux_v3_cfg_t suspend_enter_pads[] = {
	MX50_PAD_EIM_DA0__GPIO_1_0,
	MX50_PAD_EIM_DA1__GPIO_1_1,
	MX50_PAD_EIM_DA2__GPIO_1_2,
	MX50_PAD_EIM_DA3__GPIO_1_3,
	MX50_PAD_EIM_DA4__GPIO_1_4,
	MX50_PAD_EIM_DA5__GPIO_1_5,
	MX50_PAD_EIM_DA6__GPIO_1_6,
	MX50_PAD_EIM_DA7__GPIO_1_7,

	MX50_PAD_EIM_DA8__GPIO_1_8,
	MX50_PAD_EIM_DA9__GPIO_1_9,
	MX50_PAD_EIM_DA10__GPIO_1_10,
	MX50_PAD_EIM_DA11__GPIO_1_11,
	MX50_PAD_EIM_DA12__GPIO_1_12,
	MX50_PAD_EIM_DA13__GPIO_1_13,
	MX50_PAD_EIM_DA15__GPIO_1_15,
	MX50_PAD_EIM_CS2__GPIO_1_16,
	MX50_PAD_EIM_CS1__GPIO_1_17,
	MX50_PAD_EIM_CS0__GPIO_1_18,
	MX50_PAD_EIM_EB0__GPIO_1_19,
	MX50_PAD_EIM_EB1__GPIO_1_20,
	MX50_PAD_EIM_WAIT__GPIO_1_21,
	MX50_PAD_EIM_BCLK__GPIO_1_22,
	MX50_PAD_EIM_RDY__GPIO_1_23,
	MX50_PAD_EIM_OE__GPIO_1_24,
	MX50_PAD_EIM_RW__GPIO_1_25,
	MX50_PAD_EIM_LBA__GPIO_1_26,
	MX50_PAD_EIM_CRE__GPIO_1_27,

	/* NVCC_NANDF pads */
	MX50_PAD_DISP_D8__GPIO_2_8,
	MX50_PAD_DISP_D10__GPIO_2_10,
	MX50_PAD_DISP_D11__GPIO_2_11,
	MX50_PAD_DISP_D12__GPIO_2_12,
	MX50_PAD_DISP_D13__GPIO_2_13,
	MX50_PAD_DISP_D14__GPIO_2_14,
	MX50_PAD_DISP_D15__GPIO_2_15,
	MX50_PAD_SD3_CMD__GPIO_5_18,
	MX50_PAD_SD3_CLK__GPIO_5_19,
	MX50_PAD_SD3_D0__GPIO_5_20,
	MX50_PAD_SD3_D1__GPIO_5_21,
	MX50_PAD_SD3_D2__GPIO_5_22,
	MX50_PAD_SD3_D3__GPIO_5_23,
	MX50_PAD_SD3_D4__GPIO_5_24,
	MX50_PAD_SD3_D5__GPIO_5_25,
	MX50_PAD_SD3_D6__GPIO_5_26,
	MX50_PAD_SD3_D7__GPIO_5_27,
	MX50_PAD_SD3_WP__GPIO_5_28,

	/* NVCC_LCD pads */
	MX50_PAD_DISP_D0__GPIO_2_0,
	MX50_PAD_DISP_D1__GPIO_2_1,
	MX50_PAD_DISP_D2__GPIO_2_2,
	MX50_PAD_DISP_D3__GPIO_2_3,
	MX50_PAD_DISP_D4__GPIO_2_4,
	MX50_PAD_DISP_D5__GPIO_2_5,
	MX50_PAD_DISP_D6__GPIO_2_6,
	MX50_PAD_DISP_D7__GPIO_2_7,
	MX50_PAD_DISP_WR__GPIO_2_16,
	MX50_PAD_DISP_BUSY__GPIO_2_18,
	MX50_PAD_DISP_RD__GPIO_2_19,
	MX50_PAD_DISP_RESET__GPIO_2_20,
	MX50_PAD_DISP_CS__GPIO_2_21,

	/* CSPI pads */
	MX50_PAD_CSPI_SCLK__GPIO_4_8,
	MX50_PAD_CSPI_MOSI__GPIO_4_9,
	MX50_PAD_CSPI_MISO__GPIO_4_10,
	MX50_PAD_CSPI_SS0__GPIO_4_11,

	/*NVCC_MISC pins as GPIO */
	MX50_PAD_I2C1_SCL__GPIO_6_18,
	MX50_PAD_I2C1_SDA__GPIO_6_19,
	MX50_PAD_I2C2_SCL__GPIO_6_20,
	MX50_PAD_I2C2_SDA__GPIO_6_21,
	MX50_PAD_I2C3_SCL__GPIO_6_22,
	MX50_PAD_I2C3_SDA__GPIO_6_23,

	/* NVCC_MISC_PWM_USB_OTG pins */
	MX50_PAD_PWM1__GPIO_6_24,
	MX50_PAD_PWM2__GPIO_6_25,
	MX50_PAD_WDOG__GPIO_6_28,
};

static iomux_v3_cfg_t suspend_exit_pads[ARRAY_SIZE(suspend_enter_pads)];

static struct mxc_dvfs_platform_data dvfs_core_data = {
	.reg_id = "SW1",
	.clk1_id = "cpu_clk",
	.clk2_id = "gpc_dvfs_clk",
	.gpc_cntr_offset = MXC_GPC_CNTR_OFFSET,
	.gpc_vcr_offset = MXC_GPC_VCR_OFFSET,
	.ccm_cdcr_offset = MXC_CCM_CDCR_OFFSET,
	.ccm_cacrr_offset = MXC_CCM_CACRR_OFFSET,
	.ccm_cdhipr_offset = MXC_CCM_CDHIPR_OFFSET,
	.prediv_mask = 0x1F800,
	.prediv_offset = 11,
	.prediv_val = 3,
	.div3ck_mask = 0xE0000000,
	.div3ck_offset = 29,
	.div3ck_val = 2,
	.emac_val = 0x08,
	.upthr_val = 25,
	.dnthr_val = 9,
	.pncthr_val = 33,
	.upcnt_val = 10,
	.dncnt_val = 10,
	.delay_time = 30,
};

static struct mxc_bus_freq_platform_data bus_freq_data = {
	.gp_reg_id = "SW1",
	.lp_reg_id = "SW2",
};

struct dvfs_wp dvfs_core_setpoint[] = {
	{33, 13, 33, 10, 10, 0x08}, /* 800MHz*/
	{28, 8, 33, 10, 10, 0x08},   /* 400MHz */
	{20, 0, 33, 20, 10, 0x08},   /* 160MHz*/
	{28, 8, 33, 20, 30, 0x08},   /*160MHz, AHB 133MHz, LPAPM mode*/
	{29, 0, 33, 20, 10, 0x08},}; /* 160MHz, AHB 24MHz */

/* working point(wp): 0 - 800MHz; 1 - 400MHz, 2 - 160MHz; */
static struct cpu_wp cpu_wp_auto[] = {
	{
	 .pll_rate = 800000000,
	 .cpu_rate = 800000000,
	 .pdf = 0,
	 .mfi = 8,
	 .mfd = 2,
	 .mfn = 1,
	 .cpu_podf = 0,
	 .cpu_voltage = 1050000,},
	{
	 .pll_rate = 800000000,
	 .cpu_rate = 400000000,
	 .cpu_podf = 1,
	 .cpu_voltage = 1050000,},
	{
	 .pll_rate = 800000000,
	 .cpu_rate = 160000000,
	 .cpu_podf = 4,
	 .cpu_voltage = 850000,},
};

static struct dvfs_wp *mx50_arm2_get_dvfs_core_table(int *wp)
{
	*wp = ARRAY_SIZE(dvfs_core_setpoint);
	return dvfs_core_setpoint;
}

static struct cpu_wp *mx50_arm2_get_cpu_wp(int *wp)
{
	*wp = num_cpu_wp;
	return cpu_wp_auto;
}

static void mx50_arm2_set_num_cpu_wp(int num)
{
	num_cpu_wp = num;
	return;
}

static struct fec_platform_data fec_data = {
	.phy = PHY_INTERFACE_MODE_RMII,
};

/* workaround for cspi chipselect pin may not keep correct level when idle */
static void mx50_arm2_gpio_spi_chipselect_active(int cspi_mode, int status,
					     int chipselect)
{
	switch (cspi_mode) {
	case 1:
		break;
	case 2:
		break;
	case 3:
		switch (chipselect) {
		case 0x1:
			{
			iomux_v3_cfg_t cspi_ss0 = MX50_PAD_CSPI_SS0__CSPI_SS0;
			iomux_v3_cfg_t cspi_cs1 = MX50_PAD_ECSPI1_MOSI__GPIO_4_13;

			/* pull up/down deassert it */
			mxc_iomux_v3_setup_pad(cspi_ss0);
			mxc_iomux_v3_setup_pad(cspi_cs1);

			gpio_request(CSPI_CS1, "cspi-cs1");
			gpio_direction_input(CSPI_CS1);
			}
			break;
		case 0x2:
			{
			iomux_v3_cfg_t cspi_ss1 = MX50_PAD_ECSPI1_MOSI__CSPI_SS1;
			iomux_v3_cfg_t cspi_ss0 = MX50_PAD_CSPI_SS0__GPIO_4_11;

			/*disable other ss */
			mxc_iomux_v3_setup_pad(cspi_ss1);
			mxc_iomux_v3_setup_pad(cspi_ss0);

			/* pull up/down deassert it */
			gpio_request(CSPI_CS2, "cspi-cs2");
			gpio_direction_input(CSPI_CS2);
			}
			break;
		default:
			break;
		}
		break;

	default:
		break;
	}
}

static void mx50_arm2_gpio_spi_chipselect_inactive(int cspi_mode, int status,
					       int chipselect)
{
	switch (cspi_mode) {
	case 1:
		break;
	case 2:
		break;
	case 3:
		switch (chipselect) {
		case 0x1:
			gpio_free(CSPI_CS1);
			break;
		case 0x2:
			gpio_free(CSPI_CS2);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

}

static struct mxc_spi_master mxcspi1_data = {
	.maxchipselect = 4,
	.spi_version = 23,
	.chipselect_active = mx50_arm2_gpio_spi_chipselect_active,
	.chipselect_inactive = mx50_arm2_gpio_spi_chipselect_inactive,
};

static struct mxc_spi_master mxcspi3_data = {
	.maxchipselect = 4,
	.spi_version = 7,
	.chipselect_active = mx50_arm2_gpio_spi_chipselect_active,
	.chipselect_inactive = mx50_arm2_gpio_spi_chipselect_inactive,
};

static struct imxi2c_platform_data mxci2c_data = {
	.bitrate = 100000,
};

#define mV_to_uV(mV) (mV * 1000)
#define uV_to_mV(uV) (uV / 1000)
#define V_to_uV(V) (mV_to_uV(V * 1000))
#define uV_to_V(uV) (uV_to_mV(uV) / 1000)

static int epdc_get_pins(void)
{
	/* Claim GPIOs for EPDC pins - used during power up/down */
	gpio_request(EPDC_D0, "epdc_d0");
	gpio_request(EPDC_D1, "epdc_d1");
	gpio_request(EPDC_D2, "epdc_d2");
	gpio_request(EPDC_D3, "epdc_d3");
	gpio_request(EPDC_D4, "epdc_d4");
	gpio_request(EPDC_D5, "epdc_d5");
	gpio_request(EPDC_D6, "epdc_d6");
	gpio_request(EPDC_D7, "epdc_d7");
	gpio_request(EPDC_GDCLK, "epdc_gdclk");
	gpio_request(EPDC_GDSP, "epdc_gdsp");
	gpio_request(EPDC_GDOE, "epdc_gdoe");
	gpio_request(EPDC_GDRL, "epdc_gdrl");
	gpio_request(EPDC_SDCLK, "epdc_sdclk");
	gpio_request(EPDC_SDOE, "epdc_sdoe");
	gpio_request(EPDC_SDLE, "epdc_sdle");
	gpio_request(EPDC_SDSHR, "epdc_sdshr");
	gpio_request(EPDC_BDR0, "epdc_bdr0");
	gpio_request(EPDC_SDCE0, "epdc_sdce0");
	gpio_request(EPDC_SDCE1, "epdc_sdce1");
	gpio_request(EPDC_SDCE2, "epdc_sdce2");

	return 0;
}

static void epdc_put_pins(void)
{
	gpio_free(EPDC_D0);
	gpio_free(EPDC_D1);
	gpio_free(EPDC_D2);
	gpio_free(EPDC_D3);
	gpio_free(EPDC_D4);
	gpio_free(EPDC_D5);
	gpio_free(EPDC_D6);
	gpio_free(EPDC_D7);
	gpio_free(EPDC_GDCLK);
	gpio_free(EPDC_GDSP);
	gpio_free(EPDC_GDOE);
	gpio_free(EPDC_GDRL);
	gpio_free(EPDC_SDCLK);
	gpio_free(EPDC_SDOE);
	gpio_free(EPDC_SDLE);
	gpio_free(EPDC_SDSHR);
	gpio_free(EPDC_BDR0);
	gpio_free(EPDC_SDCE0);
	gpio_free(EPDC_SDCE1);
	gpio_free(EPDC_SDCE2);
}

static iomux_v3_cfg_t mx50_epdc_pads_enabled[] = {
	MX50_PAD_EPDC_D0__EPDC_D0,
	MX50_PAD_EPDC_D1__EPDC_D1,
	MX50_PAD_EPDC_D2__EPDC_D2,
	MX50_PAD_EPDC_D3__EPDC_D3,
	MX50_PAD_EPDC_D4__EPDC_D4,
	MX50_PAD_EPDC_D5__EPDC_D5,
	MX50_PAD_EPDC_D6__EPDC_D6,
	MX50_PAD_EPDC_D7__EPDC_D7,
	MX50_PAD_EPDC_GDCLK__EPDC_GDCLK,
	MX50_PAD_EPDC_GDSP__EPDC_GDSP,
	MX50_PAD_EPDC_GDOE__EPDC_GDOE,
	MX50_PAD_EPDC_GDRL__EPDC_GDRL,
	MX50_PAD_EPDC_SDCLK__EPDC_SDCLK,
	MX50_PAD_EPDC_SDOE__EPDC_SDOE,
	MX50_PAD_EPDC_SDLE__EPDC_SDLE,
	MX50_PAD_EPDC_SDSHR__EPDC_SDSHR,
	MX50_PAD_EPDC_BDR0__EPDC_BDR0,
	MX50_PAD_EPDC_SDCE0__EPDC_SDCE0,
	MX50_PAD_EPDC_SDCE1__EPDC_SDCE1,
	MX50_PAD_EPDC_SDCE2__EPDC_SDCE2,
};

static iomux_v3_cfg_t mx50_epdc_pads_disabled[] = {
	MX50_PAD_EPDC_D0__GPIO_3_0,
	MX50_PAD_EPDC_D1__GPIO_3_1,
	MX50_PAD_EPDC_D2__GPIO_3_2,
	MX50_PAD_EPDC_D3__GPIO_3_3,
	MX50_PAD_EPDC_D4__GPIO_3_4,
	MX50_PAD_EPDC_D5__GPIO_3_5,
	MX50_PAD_EPDC_D6__GPIO_3_6,
	MX50_PAD_EPDC_D7__GPIO_3_7,
	MX50_PAD_EPDC_GDCLK__GPIO_3_16,
	MX50_PAD_EPDC_GDSP__GPIO_3_17,
	MX50_PAD_EPDC_GDOE__GPIO_3_18,
	MX50_PAD_EPDC_GDRL__GPIO_3_19,
	MX50_PAD_EPDC_SDCLK__GPIO_3_20,
	MX50_PAD_EPDC_SDOE__GPIO_3_23,
	MX50_PAD_EPDC_SDLE__GPIO_3_24,
	MX50_PAD_EPDC_SDSHR__GPIO_3_26,
	MX50_PAD_EPDC_BDR0__GPIO_4_23,
	MX50_PAD_EPDC_SDCE0__GPIO_4_25,
	MX50_PAD_EPDC_SDCE1__GPIO_4_26,
	MX50_PAD_EPDC_SDCE2__GPIO_4_27,
};

static void epdc_enable_pins(void)
{
	/* Configure MUX settings to enable EPDC use */
	mxc_iomux_v3_setup_multiple_pads(mx50_epdc_pads_enabled, \
				ARRAY_SIZE(mx50_epdc_pads_enabled));

	gpio_direction_input(EPDC_D0);
	gpio_direction_input(EPDC_D1);
	gpio_direction_input(EPDC_D2);
	gpio_direction_input(EPDC_D3);
	gpio_direction_input(EPDC_D4);
	gpio_direction_input(EPDC_D5);
	gpio_direction_input(EPDC_D6);
	gpio_direction_input(EPDC_D7);
	gpio_direction_input(EPDC_GDCLK);
	gpio_direction_input(EPDC_GDSP);
	gpio_direction_input(EPDC_GDOE);
	gpio_direction_input(EPDC_GDRL);
	gpio_direction_input(EPDC_SDCLK);
	gpio_direction_input(EPDC_SDOE);
	gpio_direction_input(EPDC_SDLE);
	gpio_direction_input(EPDC_SDSHR);
	gpio_direction_input(EPDC_BDR0);
	gpio_direction_input(EPDC_SDCE0);
	gpio_direction_input(EPDC_SDCE1);
	gpio_direction_input(EPDC_SDCE2);
}

static void epdc_disable_pins(void)
{
	/* Configure MUX settings for EPDC pins to
	 * GPIO and drive to 0. */
	mxc_iomux_v3_setup_multiple_pads(mx50_epdc_pads_disabled, \
				ARRAY_SIZE(mx50_epdc_pads_disabled));

	gpio_direction_output(EPDC_D0, 0);
	gpio_direction_output(EPDC_D1, 0);
	gpio_direction_output(EPDC_D2, 0);
	gpio_direction_output(EPDC_D3, 0);
	gpio_direction_output(EPDC_D4, 0);
	gpio_direction_output(EPDC_D5, 0);
	gpio_direction_output(EPDC_D6, 0);
	gpio_direction_output(EPDC_D7, 0);
	gpio_direction_output(EPDC_GDCLK, 0);
	gpio_direction_output(EPDC_GDSP, 0);
	gpio_direction_output(EPDC_GDOE, 0);
	gpio_direction_output(EPDC_GDRL, 0);
	gpio_direction_output(EPDC_SDCLK, 0);
	gpio_direction_output(EPDC_SDOE, 0);
	gpio_direction_output(EPDC_SDLE, 0);
	gpio_direction_output(EPDC_SDSHR, 0);
	gpio_direction_output(EPDC_BDR0, 0);
	gpio_direction_output(EPDC_SDCE0, 0);
	gpio_direction_output(EPDC_SDCE1, 0);
	gpio_direction_output(EPDC_SDCE2, 0);
}

static struct fb_videomode e60_v110_mode = {
	.name = "E60_V110",
	.refresh = 50,
	.xres = 800,
	.yres = 600,
	.pixclock = 18604700,
	.left_margin = 8,
	.right_margin = 178,
	.upper_margin = 4,
	.lower_margin = 10,
	.hsync_len = 20,
	.vsync_len = 4,
	.sync = 0,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
};

static struct fb_videomode e60_v220_hd_pvi_mode = {
	.name = "E60_V220_HD_PVI",
	.refresh = 85,
	.xres = 1024,
	.yres = 758,
	.pixclock = 36910060,
	.left_margin = 8,
	.right_margin = 32,
	.upper_margin = 4,
	.lower_margin = 8,
	.hsync_len = 4,
	.vsync_len = 1,
	.sync = 0,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
};
static struct fb_videomode e60_v220_hd_mode = {
	.name = "E60_V220_HD",
	.refresh = 85,
	.xres = 1024,
	.yres = 768,
	.pixclock = 36910060,
	.left_margin = 8,
	.right_margin = 32,
	.upper_margin = 4,
	.lower_margin = 8,
	.hsync_len = 4,
	.vsync_len = 1,
	.sync = 0,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
};

static struct fb_videomode e60_v220_mode = {
	.name = "E60_V220",
	.refresh = 85,
	.xres = 800,
	.yres = 600,
	.pixclock = 32010660,
	.left_margin = 8,
	.right_margin = 166,
	.upper_margin = 4,
	.lower_margin = 26,
	.hsync_len = 20,
	.vsync_len = 4,
	.sync = 0,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
};

static struct fb_videomode e97_v110_mode = {
	.name = "E97_V110",
	.refresh = 50,
	.xres = 1200,
	.yres = 825,
	.pixclock = 32034000,
	.left_margin = 12,
	.right_margin = 128,
	.upper_margin = 4,
	.lower_margin = 10,
	.hsync_len = 20,
	.vsync_len = 4,
	.sync = 0,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
};

static struct mxc_epdc_fb_mode panel_modes[] = {
	{
		&e60_v220_hd_pvi_mode,
		4,	/* vscan_holdoff */
		10,	/* sdoed_width */
		20,	/* sdoed_delay */
		10,	/* sdoez_width */
		20,	/* sdoez_delay */
		520,/* gdclk_hp_offs */
		20,	/* gdsp_offs */
		0,	/* gdoe_offs */
		1,	/* gdclk_offs */
		1,	/* num_ce */
	},
	{
		&e60_v220_hd_mode,
		4,	/* vscan_holdoff */
		10,	/* sdoed_width */
		20,	/* sdoed_delay */
		10,	/* sdoez_width */
		20,	/* sdoez_delay */
		520,/* gdclk_hp_offs */
		20,	/* gdsp_offs */
		0,	/* gdoe_offs */
		1,	/* gdclk_offs */
		1,	/* num_ce */
	},
	{
		&e60_v110_mode,
		4,	/* vscan_holdoff */
		10,	/* sdoed_width */
		20,	/* sdoed_delay */
		10,	/* sdoez_width */
		20,	/* sdoez_delay */
		428,	/* gdclk_hp_offs */
		20,	/* gdsp_offs */
		0,	/* gdoe_offs */
		1,	/* gdclk_offs */
		1,	/* num_ce */
	},
	{
		&e60_v220_mode,
		4,	/* vscan_holdoff */
		10,	/* sdoed_width */
		20,	/* sdoed_delay */
		10,	/* sdoez_width */
		20,	/* sdoez_delay */
		428,	/* gdclk_hp_offs */
		20,	/* gdsp_offs */
		0,	/* gdoe_offs */
		9,	/* gdclk_offs */
		1,	/* num_ce */
	},
	{
		&e97_v110_mode,
		8,	/* vscan_holdoff */
		10,	/* sdoed_width */
		20,	/* sdoed_delay */
		10,	/* sdoez_width */
		20,	/* sdoez_delay */
		632,	/* gdclk_hp_offs */
		20,	/* gdsp_offs */
		0,	/* gdoe_offs */
		1,	/* gdclk_offs */
		3,	/* num_ce */
	}
};

static struct mxc_epdc_fb_platform_data epdc_data = {
	.epdc_mode = panel_modes,
	.num_modes = ARRAY_SIZE(panel_modes),
	.get_pins = epdc_get_pins,
	.put_pins = epdc_put_pins,
	.enable_pins = epdc_enable_pins,
	.disable_pins = epdc_disable_pins,
};

/* GPIO-keys input device */
static struct gpio_keys_button mxc_gpio_keys[] = {
	{
		.type	= EV_KEY,
		.code	= KEY_ESC,
		.gpio	= BACK_KEY,
		.desc	= "Back",
		.active_low = 1,
		.wakeup	= 1,
		.debounce_interval = 30,
	}, 
	{
		.type	= EV_KEY,
		.code	= KEY_PAGEDOWN,
		.gpio	= PAGE_DOWN,
		.desc	= "Page Down",
		.active_low = 1,
		.wakeup	= 1,
		.debounce_interval = 30,
	}, 
	{
		.type	= EV_KEY,
		.code	= KEY_PAGEUP,
		.gpio	= PAGE_UP,
		.desc	= "Page Up",
		.active_low = 1,
		.wakeup	= 1,
		.debounce_interval = 30,
	}, 
#if !defined(CONFIG_TOUCHSCREEN_ZFORCE)
	{
		.type	= EV_KEY,
		.code	= KEY_VOLUMEUP,
		.gpio	= VOLUME_UP,
		.desc	= "Volume Up",
		.active_low = 1,
		.wakeup	= 1,
		.debounce_interval = 30,
	}, 
	{
		.type	= EV_KEY,
		.code	= KEY_VOLUMEDOWN,
		.gpio	= VOLUME_DOWN,
		.desc	= "Volume Down",
		.active_low = 0,
		.wakeup	= 1,
		.debounce_interval = 30,
	}, 
	{
		.type	= EV_KEY,
		.code	= KEY_ENTER,
		.gpio	= OFN_DOME,
		.desc	= "Enter",
		.active_low = 1,
		.wakeup	= 1,
		.debounce_interval = 30,
	}, 
	{
		.type	= EV_KEY,
		.code	= KEY_MENU,
		.gpio	= MENU_KEY,
		.desc	= "Menu",
		.active_low = 1,
		.wakeup	= 1,
		.debounce_interval = 30,
	},
	{
		.type	= EV_KEY,
		.code	= KEY_LEFT,
		.gpio	= OR_LEFT,
		.desc	= "KEY LEFT",
		.active_low = 1,
		.wakeup	= 1,
		.debounce_interval = 30,
	}, 
	{
		.type	= EV_KEY,
		.code	= KEY_RIGHT,
		.gpio	= OR_RIGHT,
		.desc	= "KEY RIGHT",
		.active_low = 1,
		.wakeup	= 1,
		.debounce_interval = 30,
	}, 
	{
		.type	= EV_KEY,
		.code	= KEY_UP,
		.gpio	= OR_UP,
		.desc	= "KEY UP",
		.active_low = 1,
		.wakeup	= 1,
		.debounce_interval = 30,
	}, 
	{
		.type	= EV_KEY,
		.code	= KEY_DOWN,
		.gpio	= OR_DOWN,
		.desc	= "KEY DOWN",
		.active_low = 1,
		.wakeup	= 1,
		.debounce_interval = 30,
	}, 
#ifdef CONFIG_ONYX_A62_DEV
	{
		.type	= EV_KEY,
		.code	= KEY_F22,
		.gpio	= HP_DETECT,
		.desc	= "Headphone detect",
		.active_low = 0,
		.wakeup	= 1,
		.debounce_interval = 45,
	},
#else
	{
		.type	= EV_KEY,
		.code	= KEY_F22,
		.gpio	= HP_DETECT,
		.desc	= "Headphone detect",
		.active_low = 1,
		.wakeup	= 1,
		.debounce_interval = 45,
	},
#endif
	{
		.type	= EV_KEY,
		.code	= KEY_F23,
		.gpio	= TP_DETECT,
		.desc	= "Touch Pen Detect",
		.wakeup	= 0,
	}
#endif
};

struct gpio_keys_platform_data mxc_gpio_keys_platform_data = {
	.buttons	= mxc_gpio_keys,
	.nbuttons	= ARRAY_SIZE(mxc_gpio_keys),
	.rep		= 0, /* No auto-repeat */
};
EXPORT_SYMBOL(mxc_gpio_keys_platform_data);

/* mxc keypad driver */
static struct platform_device mxc_keypad_device = {
	.name	= "gpio-keys",
	.id	= -1,
	.dev	= {
		.platform_data	= &mxc_gpio_keys_platform_data,
	},
};

static void mxc_init_keypad(void)
{
	(void)platform_device_register(&mxc_keypad_device);
}

static struct i2c_board_info mxc_i2c0_board_info[] __initdata = {
	{
	 .type = "tps65181",
	 .addr = 0x48,
	 .platform_data = NULL,
	},
};

static struct i2c_board_info mxc_i2c1_board_info[] __initdata = {
	{
	 .type = "wm8xxx",
	 .addr = 0x1A,
	 .platform_data = NULL,
	}
};

#if defined(CONFIG_TOUCHSCREEN_ZFORCE)
static struct zforce_ts_platform_data zforce_data = {
	.dr_gpio = TP_DETECT,
	.reset_gpio = IR_RESET_TP_SLP,
	.test_gpio = IR_TEST_TP_RST,
};

static struct i2c_board_info mxc_i2c2_board_info[] __initdata = {
	{
	 .type = "zforce_ts",
	 .addr = 0x50,
	 .platform_data = &zforce_data,
	}
};
#endif

static struct mtd_partition mxc_dataflash_partitions[] = {
	{
	 .name = "bootloader",
	 .offset = 0,
	 .size = 0x000100000,},
	{
	 .name = "kernel",
	 .offset = MTDPART_OFS_APPEND,
	 .size = MTDPART_SIZ_FULL,},
};

static struct flash_platform_data mxc_spi_flash_data[] = {
	{
	 .name = "mxc_dataflash",
	 .parts = mxc_dataflash_partitions,
	 .nr_parts = ARRAY_SIZE(mxc_dataflash_partitions),
	 .type = "at45db321d",}
};


static struct spi_board_info mxc_dataflash_device[] __initdata = {
	{
	 .modalias = "mxc_dataflash",
	 .max_speed_hz = 25000000,	/* max spi clock (SCK) speed in HZ */
	 .bus_num = 3,
	 .chip_select = 1,
	 .platform_data = &mxc_spi_flash_data[0],},
};

static void mx50_arm2_usb_set_vbus(bool enable)
{
	gpio_set_value(USB_OTG_PWR, enable);
}


static int sdhc_write_protect(struct device *dev)
{
	unsigned short rc = 0;

	if (to_platform_device(dev)->id == 0)
		rc = gpio_get_value(SD1_WP);

	return rc;
}

static unsigned int sdhc_get_card_det_status(struct device *dev)
{
	int ret = 0;
	if (to_platform_device(dev)->id == 0)
		ret = gpio_get_value(SD1_CD);

	return ret;
}

static struct mxc_mmc_platform_data mmc1_data = {
	.ocr_mask = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30
		| MMC_VDD_31_32,
	.caps = MMC_CAP_4_BIT_DATA,
	.min_clk = 400000,
	.max_clk = 50000000,
	.card_inserted_state = 0,
	.status = sdhc_get_card_det_status,
#ifdef CONFIG_TOUCHSCREEN_ZFORCE 
	.wp_status = 0,
#else
	.wp_status = sdhc_write_protect,
#endif
	.clock_mmc = "esdhc_clk",
	.power_mmc = "VGEN1",
};

static struct mxc_mmc_platform_data mmc2_data = {
	.ocr_mask = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30
		| MMC_VDD_31_32,
	.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_SDIO_IRQ,
	.min_clk = 400000,
	.max_clk = 26000000,
	.card_inserted_state = 1,
	.status = sdhc_get_card_det_status,
	.wp_status = sdhc_write_protect,
	.clock_mmc = "esdhc_clk",
};

static struct mxc_mmc_platform_data mmc3_data = {
#ifdef CONFIG_FSL_UTP
	.ocr_mask = MMC_VDD_165_195,
	.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA,
	.min_clk = 400000,
	.max_clk = 26000000,
#else
	.ocr_mask = MMC_VDD_165_195 | MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32
		| MMC_VDD_32_33,
	.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA
		| MMC_CAP_DATA_DDR | MMC_CAP_NONREMOVABLE,
	.min_clk = 400000,
	.max_clk = 52000000,
#endif
	.card_inserted_state = 1,
	.status = sdhc_get_card_det_status,
	.wp_status = sdhc_write_protect,
	.clock_mmc = "esdhc_clk",
};

static struct mxc_audio_platform_data wm8xxx_data = {
	.ssi_num = 1,
	.src_port = 2,
	.ext_port = 3,
	.sysclk = 12288000,
};

static struct platform_device mxc_wm8xxx_device = {
	.name = "imx-3stack-wm8xxx",
};

#ifdef CONFIG_ANDROID_PMEM
static struct android_pmem_platform_data android_pmem_data = {
	.name = "pmem_adsp",
	.size = SZ_16M,
};
static char *usb_functions_ums[] = {
	"usb_mass_storage",
};

static char *usb_functions_ums_adb[] = {
	"usb_mass_storage",
	"adb",
};

static char *usb_functions_rndis[] = {
	"rndis",
};

static char *usb_functions_all[] = {
	"rndis",
	"usb_mass_storage",
	"adb"
};

static struct android_usb_product usb_products[] = {
	{
		.product_id	= 0x0c01,
		.num_functions	= ARRAY_SIZE(usb_functions_ums),
		.functions	= usb_functions_ums,
	},
	{
		.product_id	= 0x0c02,
		.num_functions	= ARRAY_SIZE(usb_functions_ums_adb),
		.functions	= usb_functions_ums_adb,
	},
	{
		.product_id	= 0x0c10,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis),
		.functions	= usb_functions_rndis,
	},
};

static struct usb_mass_storage_platform_data mass_storage_data = {
	.nluns		= 3,
	.vendor		= "Freescale",
	.product	= "MX50 ARM2 Android",
	.release	= 0x0100,
};

static struct usb_ether_platform_data rndis_data = {
	.vendorID	= 0x15A2,
	.vendorDescr	= "Freescale",
};

static struct android_usb_platform_data android_usb_data = {
	.vendor_id      = 0x15A2,
	.product_id     = 0x0c01,
	.version        = 0x0100,
	.product_name   = "MX50 ARM2 Android",
	.manufacturer_name = "Freescale",
	.num_products = ARRAY_SIZE(usb_products),
	.products = usb_products,
	.num_functions = ARRAY_SIZE(usb_functions_all),
	.functions = usb_functions_all,
};

static int __initdata enable_keypad = {0};
static int __init keypad_setup(char *__unused)
{
	enable_keypad = 1;
	return cpu_is_mx50();
}

__setup("keypad", keypad_setup);
#endif

static struct mxs_dma_plat_data dma_apbh_data = {
	.chan_base = MXS_DMA_CHANNEL_AHB_APBH,
	.chan_num = MXS_MAX_DMA_CHANNELS,
};

/* OTP data */
/* Building up eight registers's names of a bank */
/*
#define BANK(a, b, c, d, e, f, g, h)	\
	{\
	("HW_OCOTP_"#a), ("HW_OCOTP_"#b), ("HW_OCOTP_"#c), ("HW_OCOTP_"#d), \
	("HW_OCOTP_"#e), ("HW_OCOTP_"#f), ("HW_OCOTP_"#g), ("HW_OCOTP_"#h) \
	}

#define BANKS		(5)
#define BANK_ITEMS	(8)
static const char *bank_reg_desc[BANKS][BANK_ITEMS] = {
	BANK(LOCK, CFG0, CFG1, CFG2, CFG3, CFG4, CFG5, CFG6),
	BANK(MEM0, MEM1, MEM2, MEM3, MEM4, MEM5, GP0, GP1),
	BANK(SCC0, SCC1, SCC2, SCC3, SCC4, SCC5, SCC6, SCC7),
	BANK(SRK0, SRK1, SRK2, SRK3, SRK4, SRK5, SRK6, SRK7),
	BANK(SJC0, SJC1, MAC0, MAC1, HWCAP0, HWCAP1, HWCAP2, SWCAP),
};

static struct fsl_otp_data otp_data = {
	.fuse_name	= (char **)bank_reg_desc,
	.fuse_num	= BANKS * BANK_ITEMS,
};
#undef BANK
#undef BANKS
#undef BANK_ITEMS
*/


extern int keypad_wakeup_enable;

void touchpanel_suspend_pads(void)
{
	mxc_iomux_v3_setup_pad(MX50_PAD_UART2_TXD__GPIO_6_10);
	mxc_iomux_v3_setup_pad(MX50_PAD_UART2_RXD__GPIO_6_11);

#ifdef CONFIG_TOUCHSCREEN_ZFORCE
	mxc_iomux_v3_setup_pad(MX50_PAD_I2C3_SCL__GPIO_6_22);
	mxc_iomux_v3_setup_pad(MX50_PAD_I2C3_SDA__GPIO_6_23);
#endif
}

void touchpanel_resume_pads(void)
{
	mxc_iomux_v3_setup_pad(MX50_PAD_UART2_RXD__UART2_RXD);
	mxc_iomux_v3_setup_pad(MX50_PAD_UART2_TXD__UART2_TXD);

#ifdef CONFIG_TOUCHSCREEN_ZFORCE
	mxc_iomux_v3_setup_pad(MX50_PAD_I2C3_SCL__I2C3_SCL);
	mxc_iomux_v3_setup_pad(MX50_PAD_I2C3_SDA__I2C3_SDA);
#endif
}

void pull_down_touchpanel_pins(void){
	/*Pull down UART pads*/
	gpio_set_value(UART2_TXD_GPIO_6_10, 0);
	gpio_set_value(UART2_RXD_GPIO_6_11, 0);
	/*Pull down TP_PDCT*/
	gpio_set_value(TP_DETECT, 0);

	/*Pull down I2C pads*/
	gpio_request(I2C3_SCL_GPIO_6_22, "gpio_6_22");
	gpio_request(I2C3_SDA_GPIO_6_23, "gpio_6_23");
	gpio_direction_output(I2C3_SCL_GPIO_6_22, 0);
	gpio_direction_output(I2C3_SDA_GPIO_6_23, 0);
	gpio_free(I2C3_SCL_GPIO_6_22);
	gpio_free(I2C3_SDA_GPIO_6_23);

}

void mcu_power_enable(int enable)
{
	gpio_request(IR_MCU_PWR, "mcu_pwr");
	gpio_direction_output(IR_MCU_PWR, enable);
	gpio_free(IR_MCU_PWR);
}

EXPORT_SYMBOL(pull_down_touchpanel_pins);
EXPORT_SYMBOL(touchpanel_suspend_pads);
EXPORT_SYMBOL(touchpanel_resume_pads);
EXPORT_SYMBOL(mcu_power_enable);


static u32 gpio1_ir;
static u32 gpio2_ir;
static u32 gpio3_ir;
static u32 gpio4_ir;
static u32 gpio5_ir;
static u32 gpio6_ir;
static void mx50_suspend_enter()
{
	iomux_v3_cfg_t *p = suspend_enter_pads;
	int i;
	/* Set PADCTRL to 0 for all IOMUX. */
	for (i = 0; i < ARRAY_SIZE(suspend_enter_pads); i++) {
		suspend_exit_pads[i] = *p;
		*p &= ~MUX_PAD_CTRL_MASK;
		p++;
	}
	mxc_iomux_v3_get_multiple_pads(suspend_exit_pads,
			ARRAY_SIZE(suspend_exit_pads));
	mxc_iomux_v3_setup_multiple_pads(suspend_enter_pads,
			ARRAY_SIZE(suspend_enter_pads));

	if (!keypad_wakeup_enable){
		touchpanel_suspend_pads();

#ifdef CONFIG_TOUCHSCREEN_ZFORCE
		mcu_power_enable(0);
		pull_down_touchpanel_pins();
#else
		/*Pull down UART pads*/
		gpio_set_value(UART2_TXD_GPIO_6_10, 0);
		gpio_set_value(UART2_RXD_GPIO_6_11, 0);
		/*Pull down TP_PDCT*/
		gpio_set_value(TP_DETECT, 0);
		gpio_set_value(IR_TEST_TP_RST, 0);
#endif
    gpio1_ir = __raw_readl(IO_ADDRESS(GPIO1_BASE_ADDR) + GPIO_GDIR);
    gpio2_ir = __raw_readl(IO_ADDRESS(GPIO2_BASE_ADDR) + GPIO_GDIR);
    gpio3_ir = __raw_readl(IO_ADDRESS(GPIO3_BASE_ADDR) + GPIO_GDIR);
    gpio4_ir = __raw_readl(IO_ADDRESS(GPIO4_BASE_ADDR) + GPIO_GDIR);
    gpio5_ir = __raw_readl(IO_ADDRESS(GPIO5_BASE_ADDR) + GPIO_GDIR);
    gpio6_ir = __raw_readl(IO_ADDRESS(GPIO6_BASE_ADDR) + GPIO_GDIR);
    __raw_writel(0, IO_ADDRESS(GPIO1_BASE_ADDR) + GPIO_GDIR);
    __raw_writel(0, IO_ADDRESS(GPIO2_BASE_ADDR) + GPIO_GDIR);
    __raw_writel(0, IO_ADDRESS(GPIO3_BASE_ADDR) + GPIO_GDIR);
    __raw_writel(0, IO_ADDRESS(GPIO4_BASE_ADDR) + GPIO_GDIR);
    __raw_writel(0, IO_ADDRESS(GPIO5_BASE_ADDR) + GPIO_GDIR);
    __raw_writel(0, IO_ADDRESS(GPIO6_BASE_ADDR) + GPIO_GDIR);
	}
}

static void mx50_suspend_exit()
{
	mxc_iomux_v3_setup_multiple_pads(suspend_exit_pads,
			ARRAY_SIZE(suspend_exit_pads));

	if (!keypad_wakeup_enable){
		touchpanel_resume_pads();

#ifdef CONFIG_TOUCHSCREEN_ZFORCE
		mcu_power_enable(1);
#else
		gpio_set_value(IR_RESET_TP_SLP, 0);
#endif 
    __raw_writel(gpio1_ir, IO_ADDRESS(GPIO1_BASE_ADDR) + GPIO_GDIR);
    __raw_writel(gpio2_ir, IO_ADDRESS(GPIO2_BASE_ADDR) + GPIO_GDIR);
    __raw_writel(gpio3_ir, IO_ADDRESS(GPIO3_BASE_ADDR) + GPIO_GDIR);
    __raw_writel(gpio4_ir, IO_ADDRESS(GPIO4_BASE_ADDR) + GPIO_GDIR);
    __raw_writel(gpio5_ir, IO_ADDRESS(GPIO5_BASE_ADDR) + GPIO_GDIR);
    __raw_writel(gpio6_ir, IO_ADDRESS(GPIO6_BASE_ADDR) + GPIO_GDIR);
	}
}

static struct mxc_pm_platform_data mx50_pm_data = {
	.suspend_enter = mx50_suspend_enter,
	.suspend_exit = mx50_suspend_exit,
};


#ifdef CONFIG_ANDROID_PMEM
static void __init fixup_android_board(struct machine_desc *desc, struct tag *tags,
				   char **cmdline, struct meminfo *mi)
{
	int pmem_size = android_pmem_data.size;
	struct tag *mem_tag = NULL;
	int total_mem = 0;

	mxc_set_cpu_type(MXC_CPU_MX50);
	get_cpu_wp = mx50_arm2_get_cpu_wp;
	set_num_cpu_wp = mx50_arm2_set_num_cpu_wp;
	get_dvfs_core_wp = mx50_arm2_get_dvfs_core_table;
	num_cpu_wp = ARRAY_SIZE(cpu_wp_auto);

	for_each_tag(mem_tag, tags) {
		if (mem_tag->hdr.tag == ATAG_MEM) {
			total_mem = mem_tag->u.mem.size;
			break;
		}
	}
	if (mem_tag){
		android_pmem_data.start = mem_tag->u.mem.start
				+ total_mem - pmem_size;
		tags->u.mem.size = android_pmem_data.start - mem_tag->u.mem.start;
	}
}
#else
/*!
 * Board specific fixup function. It is called by \b setup_arch() in
 * setup.c file very early on during kernel starts. It allows the user to
 * statically fill in the proper values for the passed-in parameters. None of
 * the parameters is used currently.
 *
 * @param  desc         pointer to \b struct \b machine_desc
 * @param  tags         pointer to \b struct \b tag
 * @param  cmdline      pointer to the command line
 * @param  mi           pointer to \b struct \b meminfo
 */
static void __init fixup_mxc_board(struct machine_desc *desc, struct tag *tags,
				   char **cmdline, struct meminfo *mi)
{
	mxc_set_cpu_type(MXC_CPU_MX50);

	get_cpu_wp = mx50_arm2_get_cpu_wp;
	set_num_cpu_wp = mx50_arm2_set_num_cpu_wp;
	get_dvfs_core_wp = mx50_arm2_get_dvfs_core_table;
	num_cpu_wp = ARRAY_SIZE(cpu_wp_auto);
}
#endif
void enable_pwrcom(int enable)
{
	gpio_request(EPDC_VCOM, "epdc-vcom");
	gpio_direction_output(EPDC_VCOM, enable);
	gpio_free(EPDC_VCOM);
}
EXPORT_SYMBOL(enable_pwrcom);

void enable_eink_pmic(int enable)
{
	gpio_request(EPDC_PMIC_WAKE, "epdc-pmic-wake");
	gpio_direction_output(EPDC_PMIC_WAKE, enable);
	gpio_free(EPDC_PMIC_WAKE);
}
EXPORT_SYMBOL(enable_eink_pmic);

static void __init mx50_arm2_io_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(mx50_armadillo2, \
			ARRAY_SIZE(mx50_armadillo2));

	gpio_request(SD1_WP, "sdhc1-wp");
	gpio_direction_input(SD1_WP);

	gpio_request(SD1_CD, "sdhc1-cd");
	gpio_direction_input(SD1_CD);

	gpio_request(PWR_INT, "pwr-int");
	gpio_direction_input(PWR_INT);

	enable_eink_pmic(1);
	gpio_request(EPDC_PMIC_INT, "epdc-pmic-int");
	gpio_direction_input(EPDC_PMIC_INT);

	gpio_request(EPDC_PWRSTAT, "epdc-pwrstat");
	gpio_direction_input(EPDC_PWRSTAT);

	/* ELCDIF backlight */
	gpio_request(EPDC_ELCDIF_BACKLIGHT, "elcdif-backlight");
	gpio_direction_output(EPDC_ELCDIF_BACKLIGHT, 1);

	/* USB OTG PWR */
	gpio_request(USB_OTG_PWR, "usb otg power");
	gpio_direction_output(USB_OTG_PWR, 1);
	gpio_set_value(USB_OTG_PWR, 0);

	/* Enable 3G */
	gpio_request(AP_WKUP_3G, "AP_WKUP_3G");
	gpio_direction_output(AP_WKUP_3G, 1);
	gpio_free(AP_WKUP_3G);

	gpio_request(MODEM_PWR, "3G POWER_ON_OFF");
	gpio_direction_output(MODEM_PWR, 0);
	msleep(200);
	gpio_direction_output(MODEM_PWR, 1);
	gpio_free(MODEM_PWR);

	gpio_request(MODEM_RST, "3G RST");
	gpio_direction_output(MODEM_RST, 0);
	msleep(10);
	gpio_direction_output(MODEM_RST, 1);
	gpio_free(MODEM_RST);
#ifndef CONFIG_TOUCHSCREEN_ZFORCE
	gpio_request(IR_TEST_TP_RST, "TP_RST");
	gpio_direction_output(IR_TEST_TP_RST, 0);
	gpio_free(IR_TEST_TP_RST);
	msleep(200);
	gpio_request(IR_TEST_TP_RST, "TP_RST");
	gpio_direction_output(IR_TEST_TP_RST, 1);
	gpio_free(IR_TEST_TP_RST);

	gpio_request(IR_RESET_TP_SLP, "TP_SLP");
	gpio_direction_output(IR_RESET_TP_SLP, 0);
	gpio_free(IR_RESET_TP_SLP);
#endif
}

/*!
 * Board specific initialization.
 */
static void __init mxc_board_init(void)
{
	/* SD card detect irqs */
	mxcsdhc1_device.resource[2].start = gpio_to_irq(SD1_CD);
	mxcsdhc1_device.resource[2].end = gpio_to_irq(SD1_CD);
	mxcsdhc2_device.resource[2].start = 0;
	mxcsdhc2_device.resource[2].end = 0;
	mxcsdhc3_device.resource[2].start = 0;
	mxcsdhc3_device.resource[2].end = 0;

	mxc_cpu_common_init();
	mx50_arm2_io_init();

	mxc_register_device(&mxc_dma_device, NULL);
	mxc_register_device(&mxs_dma_apbh_device, &dma_apbh_data);
	mxc_register_device(&mxc_wdt_device, NULL);
	mxc_register_device(&mxcspi1_device, &mxcspi1_data);
	mxc_register_device(&mxcspi3_device, &mxcspi3_data);
	mxc_register_device(&mxci2c_devices[0], &mxci2c_data);
	mxc_register_device(&mxci2c_devices[1], &mxci2c_data);
	mxc_register_device(&mxci2c_devices[2], &mxci2c_data);

	mxc_register_device(&mxc_rtc_device, NULL);
	mxc_register_device(&pm_device, &mx50_pm_data);
	mxc_register_device(&mxc_dvfs_core_device, &dvfs_core_data);
	mxc_register_device(&busfreq_device, &bus_freq_data);

	mxc_init_keypad();

	mxc_register_device(&mxcsdhc3_device, &mmc3_data);
	mxc_register_device(&mxcsdhc1_device, &mmc1_data);
	mxc_register_device(&mxcsdhc2_device, &mmc2_data);
	mxc_register_device(&mxc_ssi1_device, NULL);
	mxc_register_device(&mxc_ssi2_device, NULL);
	mxc_register_device(&mxc_fec_device, &fec_data);
	spi_register_board_info(mxc_dataflash_device,
				ARRAY_SIZE(mxc_dataflash_device));
	i2c_register_board_info(0, mxc_i2c0_board_info,
				ARRAY_SIZE(mxc_i2c0_board_info));
	i2c_register_board_info(1, mxc_i2c1_board_info,
				ARRAY_SIZE(mxc_i2c1_board_info));
#if defined(CONFIG_TOUCHSCREEN_ZFORCE)
	i2c_register_board_info(2, mxc_i2c2_board_info,
				ARRAY_SIZE(mxc_i2c2_board_info));
#endif

	mxc_register_device(&mxc_pxp_device, NULL);
	mxc_register_device(&epdc_device, &epdc_data);

#ifdef CONFIG_ANDROID_PMEM
	mxc_register_device(&mxc_android_pmem_device, &android_pmem_data);
	mxc_register_device(&usb_mass_storage_device, &mass_storage_data);
	mxc_register_device(&usb_rndis_device, &rndis_data);
	mxc_register_device(&android_usb_device, &android_usb_data);
#endif

	mx50_arm2_init_mc13892();
/*
	pm_power_off = mxc_power_off;
	*/
	mxc_register_device(&mxc_wm8xxx_device, &wm8xxx_data);

	mx5_set_otghost_vbus_func(mx50_arm2_usb_set_vbus);
	mx5_usb_dr_init();
	mx5_usbh1_init();

	if (mx50_revision() >= IMX_CHIP_REVISION_1_1)
		mxc_register_device(&mxc_zq_calib_device, NULL);
}

static void __init mx50_arm2_timer_init(void)
{
	struct clk *uart_clk;

	mx50_clocks_init(32768, 24000000, 22579200);

	uart_clk = clk_get_sys("mxcintuart.0", NULL);
	early_console_setup(MX53_BASE_ADDR(UART1_BASE_ADDR), uart_clk);
}

static struct sys_timer mxc_timer = {
	.init	= mx50_arm2_timer_init,
};

/*
 * The following uses standard kernel macros define in arch.h in order to
 * initialize __mach_desc_MX50_ARM2 data structure.
 */
MACHINE_START(MX50_ARM2, "Freescale MX50 ARM2 Board")
	/* Maintainer: Freescale Semiconductor, Inc. */
#ifdef CONFIG_ANDROID_PMEM
	.fixup = fixup_android_board,
#else
	.fixup = fixup_mxc_board,
#endif
	.map_io = mx5_map_io,
	.init_irq = mx5_init_irq,
	.init_machine = mxc_board_init,
	.timer = &mxc_timer,
MACHINE_END
