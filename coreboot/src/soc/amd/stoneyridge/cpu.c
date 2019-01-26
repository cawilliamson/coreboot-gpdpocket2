/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015-2016 Intel Corp.
 * Copyright (C) 2017 Advanced Micro Devices, Inc.
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

#include <cpu/cpu.h>
#include <cpu/x86/mp.h>
#include <cpu/x86/mtrr.h>
#include <cpu/x86/msr.h>
#include <cpu/amd/msr.h>
#include <cpu/x86/lapic.h>
#include <device/device.h>
#include <device/pci_ops.h>
#include <soc/pci_devs.h>
#include <soc/cpu.h>
#include <soc/northbridge.h>
#include <soc/smi.h>
#include <soc/iomap.h>
#include <console/console.h>

/*
 * MP and SMM loading initialization.
 */
struct smm_relocation_attrs {
	uint32_t smbase;
	uint32_t tseg_base;
	uint32_t tseg_mask;
};

static struct smm_relocation_attrs relo_attrs;

/*
 * Do essential initialization tasks before APs can be fired up -
 *
 *  1. Prevent race condition in MTRR solution. Enable MTRRs on the BSP. This
 *  creates the MTRR solution that the APs will use. Otherwise APs will try to
 *  apply the incomplete solution as the BSP is calculating it.
 */
static void pre_mp_init(void)
{
	x86_setup_mtrrs_with_detect();
	x86_mtrr_check();
}

static int get_cpu_count(void)
{
	return (pci_read_config16(SOC_HT_DEV, D18F0_CPU_CNT) & CPU_CNT_MASK)
									+ 1;
}

static void get_smm_info(uintptr_t *perm_smbase, size_t *perm_smsize,
				size_t *smm_save_state_size)
{
	void *smm_base;
	size_t smm_size;
	void *handler_base;
	size_t handler_size;

	/* Initialize global tracking state. */
	smm_region_info(&smm_base, &smm_size);
	smm_subregion(SMM_SUBREGION_HANDLER, &handler_base, &handler_size);

	relo_attrs.smbase = (uint32_t)smm_base;
	relo_attrs.tseg_base = relo_attrs.smbase;
	relo_attrs.tseg_mask = ALIGN_DOWN(~(smm_size - 1), 128 * KiB);
	relo_attrs.tseg_mask |= SMM_TSEG_WB;

	*perm_smbase = (uintptr_t)handler_base;
	*perm_smsize = handler_size;
	*smm_save_state_size = sizeof(amd64_smm_state_save_area_t);
}

static void relocation_handler(int cpu, uintptr_t curr_smbase,
				uintptr_t staggered_smbase)
{
	msr_t tseg_base, tseg_mask;
	amd64_smm_state_save_area_t *smm_state;

	tseg_base.lo = relo_attrs.tseg_base;
	tseg_base.hi = 0;
	wrmsr(SMM_ADDR_MSR, tseg_base);
	tseg_mask.lo = relo_attrs.tseg_mask;
	tseg_mask.hi = ((1 << (cpu_phys_address_size() - 32)) - 1);
	wrmsr(SMM_MASK_MSR, tseg_mask);
	smm_state = (void *)(SMM_AMD64_SAVE_STATE_OFFSET + curr_smbase);
	smm_state->smbase = staggered_smbase;
}

static const struct mp_ops mp_ops = {
	.pre_mp_init = pre_mp_init,
	.get_cpu_count = get_cpu_count,
	.get_smm_info = get_smm_info,
	.relocation_handler = relocation_handler,
	.post_mp_init = enable_smi_generation,
};

void stoney_init_cpus(struct device *dev)
{
	/* Clear for take-off */
	if (mp_init_with_smm(dev->link_list, &mp_ops) < 0)
		printk(BIOS_ERR, "MP initialization failure.\n");

	/* The flash is now no longer cacheable. Reset to WP for performance. */
	mtrr_use_temp_range(FLASH_BASE_ADDR, CONFIG_ROM_SIZE, MTRR_TYPE_WRPROT);

	set_warm_reset_flag();
}

static void model_15_init(struct device *dev)
{
	check_mca();
	setup_lapic();

	/*
	 * Per AMD, sync an undocumented MSR with the PSP base address.
	 * Experiments showed that if you write to the MSR after it has
	 * been previously programmed, it causes a general protection fault.
	 * Also, the MSR survives warm reset and S3 cycles, so we need to
	 * test if it was previously written before writing to it.
	 */
	msr_t psp_msr;
	uint32_t psp_bar; /* Note: NDA BKDG names this 32-bit register BAR3 */
	psp_bar = pci_read_config32(SOC_PSP_DEV, PCI_BASE_ADDRESS_4);
	psp_bar &= ~PCI_BASE_ADDRESS_MEM_ATTR_MASK;
	psp_msr = rdmsr(0xc00110a2);
	if (psp_msr.lo == 0) {
		psp_msr.lo = psp_bar;
		wrmsr(0xc00110a2, psp_msr);
	}
}

static struct device_operations cpu_dev_ops = {
	.init = model_15_init,
};

static struct cpu_device_id cpu_table[] = {
	{ X86_VENDOR_AMD, 0x670f00 },
	{ 0, 0 },
};

static const struct cpu_driver model_15 __cpu_driver = {
	.ops      = &cpu_dev_ops,
	.id_table = cpu_table,
};
