/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015  Damien Zammit <damien@zamaudio.com>
 * Copyright (C) 2018  Arthur Heymans <arthur@aheymans.xyz>
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
 */

#include <arch/io.h>
#include <device/pnp_def.h>
#include <southbridge/intel/i82801gx/i82801gx.h>
#include <northbridge/intel/pineview/pineview.h>
#include <superio/ite/common/ite.h>
#include <superio/ite/it8721f/it8721f.h>

#define SERIAL_DEV PNP_DEV(0x2e, IT8721F_SP1)

void mb_enable_lpc(void)
{
	/* Disable Serial IRQ */
	pci_write_config8(PCI_DEV(0, 0x1f, 0), SERIRQ_CNTL, 0xd0);
	/* Decode range */
	pci_write_config16(PCI_DEV(0, 0x1f, 0), LPC_IO_DEC,
		pci_read_config16(PCI_DEV(0, 0x1f, 0), LPC_IO_DEC) | 0x0010);
	pci_write_config16(PCI_DEV(0, 0x1f, 0), LPC_EN, CNF1_LPC_EN | KBC_LPC_EN
			   | FDD_LPC_EN | LPT_LPC_EN | COMB_LPC_EN
			   | COMA_LPC_EN);

	/* Environment Controller */
	pci_write_config32(PCI_DEV(0, 0x1f, 0), GEN1_DEC, 0x00fc0a01);

	ite_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);
}

void get_mb_spd_addrmap(u8 *spd_addrmap)
{
	spd_addrmap[0] = 0x50;
	spd_addrmap[1] = 0x51;
}
