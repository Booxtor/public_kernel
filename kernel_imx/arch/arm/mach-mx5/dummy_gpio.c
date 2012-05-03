/*
 * Copyright 2007-2010 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/errno.h>
#include <linux/module.h>
#include <mach/iomux-mx50.h>

/* Keypad */
static iomux_v3_cfg_t mx50_keypad_enable[] = {
	MX50_PAD_KEY_COL0__KEY_COL0,
	MX50_PAD_KEY_ROW0__KEY_ROW0,
	MX50_PAD_KEY_COL1__KEY_COL1,
	MX50_PAD_KEY_ROW1__KEY_ROW1,
	MX50_PAD_KEY_COL2__KEY_COL2,
	MX50_PAD_KEY_ROW2__KEY_ROW2,
	MX50_PAD_KEY_COL3__KEY_COL3,
	MX50_PAD_KEY_ROW3__KEY_ROW3,
	MX50_PAD_EIM_DA0__KEY_COL4,
	MX50_PAD_EIM_DA1__KEY_ROW4,
	MX50_PAD_EIM_DA2__KEY_COL5,
	MX50_PAD_EIM_DA3__KEY_ROW5,
	MX50_PAD_EIM_DA4__KEY_COL6,
	MX50_PAD_EIM_DA5__KEY_ROW6,
	MX50_PAD_EIM_DA6__KEY_COL7,
	MX50_PAD_EIM_DA7__KEY_ROW7,
};

static iomux_v3_cfg_t mx50_keypad_disable[] = {
	MX50_PAD_KEY_COL0__GPIO_4_0,
	MX50_PAD_KEY_ROW0__GPIO_4_1,
	MX50_PAD_KEY_COL1__GPIO_4_2,
	MX50_PAD_KEY_ROW1__GPIO_4_3,
	MX50_PAD_KEY_COL2__GPIO_4_4,			
	MX50_PAD_KEY_ROW2__GPIO_4_5,
	MX50_PAD_KEY_COL3__GPIO_4_6,
	MX50_PAD_KEY_ROW3__GPIO_4_7,
	MX50_PAD_EIM_DA0__GPIO_1_0,
	MX50_PAD_EIM_DA1__GPIO_1_1,
	MX50_PAD_EIM_DA2__GPIO_1_2,
	MX50_PAD_EIM_DA3__GPIO_1_3,
	MX50_PAD_EIM_DA4__GPIO_1_4,
	MX50_PAD_EIM_DA5__GPIO_1_5,
	MX50_PAD_EIM_DA6__GPIO_1_6,
	MX50_PAD_EIM_DA7__GPIO_1_7,
};

void gpio_uart_active(int port, int no_irda) {}
EXPORT_SYMBOL(gpio_uart_active);

void gpio_uart_inactive(int port, int no_irda) {}
EXPORT_SYMBOL(gpio_uart_inactive);

void gpio_gps_active(void) {}
EXPORT_SYMBOL(gpio_gps_active);

void gpio_gps_inactive(void) {}
EXPORT_SYMBOL(gpio_gps_inactive);

void config_uartdma_event(int port) {}
EXPORT_SYMBOL(config_uartdma_event);

void gpio_spi_active(int cspi_mod) {}
EXPORT_SYMBOL(gpio_spi_active);

void gpio_spi_inactive(int cspi_mod) {}
EXPORT_SYMBOL(gpio_spi_inactive);

void gpio_owire_active(void) {}
EXPORT_SYMBOL(gpio_owire_active);

void gpio_owire_inactive(void) {}
EXPORT_SYMBOL(gpio_owire_inactive);

void gpio_i2c_active(int i2c_num) {}
EXPORT_SYMBOL(gpio_i2c_active);

void gpio_i2c_inactive(int i2c_num) {}
EXPORT_SYMBOL(gpio_i2c_inactive);

void gpio_i2c_hs_active(void) {}
EXPORT_SYMBOL(gpio_i2c_hs_active);

void gpio_i2c_hs_inactive(void) {}
EXPORT_SYMBOL(gpio_i2c_hs_inactive);

void gpio_pmic_active(void) {}
EXPORT_SYMBOL(gpio_pmic_active);

void gpio_activate_audio_ports(void) {}
EXPORT_SYMBOL(gpio_activate_audio_ports);

void gpio_sdhc_active(int module) {}
EXPORT_SYMBOL(gpio_sdhc_active);

void gpio_sdhc_inactive(int module) {}
EXPORT_SYMBOL(gpio_sdhc_inactive);

void gpio_sensor_select(int sensor) {}

void gpio_sensor_active(unsigned int csi) {}
EXPORT_SYMBOL(gpio_sensor_active);

void gpio_sensor_inactive(unsigned int csi) {}
EXPORT_SYMBOL(gpio_sensor_inactive);

void gpio_ata_active(void) {}
EXPORT_SYMBOL(gpio_ata_active);

void gpio_ata_inactive(void) {}
EXPORT_SYMBOL(gpio_ata_inactive);

void gpio_nand_active(void) {}
EXPORT_SYMBOL(gpio_nand_active);

void gpio_nand_inactive(void) {}
EXPORT_SYMBOL(gpio_nand_inactive);

void gpio_keypad_active(void) {
	mxc_iomux_v3_setup_multiple_pads(mx50_keypad_enable, \
			ARRAY_SIZE(mx50_keypad_enable));
}

EXPORT_SYMBOL(gpio_keypad_active);

void gpio_keypad_inactive(void) {
	mxc_iomux_v3_setup_multiple_pads(mx50_keypad_disable, \
			ARRAY_SIZE(mx50_keypad_disable));
}
EXPORT_SYMBOL(gpio_keypad_inactive);

int gpio_usbotg_hs_active(void)
{
	return 0;
}
EXPORT_SYMBOL(gpio_usbotg_hs_active);

void gpio_usbotg_hs_inactive(void) {}
EXPORT_SYMBOL(gpio_usbotg_hs_inactive);

void gpio_fec_active(void) {}
EXPORT_SYMBOL(gpio_fec_active);

void gpio_fec_inactive(void) {}
EXPORT_SYMBOL(gpio_fec_inactive);

void gpio_spdif_active(void) {}
EXPORT_SYMBOL(gpio_spdif_active);

void gpio_spdif_inactive(void) {}
EXPORT_SYMBOL(gpio_spdif_inactive);

void gpio_mlb_active(void) {}
EXPORT_SYMBOL(gpio_mlb_active);

void gpio_mlb_inactive(void) {}
EXPORT_SYMBOL(gpio_mlb_inactive);
