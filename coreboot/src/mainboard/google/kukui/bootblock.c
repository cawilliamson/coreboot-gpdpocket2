/*
 * This file is part of the coreboot project.
 *
 * Copyright 2018 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <bootblock_common.h>
#include <gpio.h>
#include <soc/gpio.h>
#include <soc/mt8183.h>
#include <soc/spi.h>

#include "gpio.h"

#define BOOTBLOCK_EN_L (GPIO(KPROW0))
#define AP_IN_SLEEP_L (GPIO(SRCLKENA0))

void bootblock_mainboard_init(void)
{
	mt8183_early_init();

	setup_chromeos_gpios();

	/* Turn on real eMMC. */
	gpio_output(BOOTBLOCK_EN_L, 1);

	/* Declare we are in S0 */
	gpio_output(AP_IN_SLEEP_L, 1);

	mtk_spi_init(CONFIG_EC_GOOGLE_CHROMEEC_SPI_BUS, SPI_PAD0_MASK, 6 * MHz);
	mtk_spi_init(CONFIG_BOOT_DEVICE_SPI_FLASH_BUS, SPI_PAD0_MASK, 26 * MHz);
}
