/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Google Inc.
 * Copyright (C) 2015-2018 Intel Corporation.
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

#include <arch/cpu.h>
#include <arch/io.h>
#include <console/console.h>
#include <cpu/x86/msr.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <intelblocks/mp_init.h>
#include <soc/bootblock.h>
#include <soc/pch.h>
#include <soc/pci_devs.h>
#include <string.h>

#define BIOS_SIGN_ID	0x8B

static struct {
	u32 cpuid;
	const char *name;
} cpu_table[] = {
	{ CPUID_CANNONLAKE_A0, "Cannonlake A0" },
	{ CPUID_CANNONLAKE_B0, "Cannonlake B0" },
	{ CPUID_CANNONLAKE_C0, "Cannonlake C0" },
	{ CPUID_CANNONLAKE_D0, "Cannonlake D0" },
	{ CPUID_COFFEELAKE_D0, "Coffeelake D0" },
	{ CPUID_WHISKEYLAKE_W0, "Whiskeylake W0"},
	{ CPUID_COFFEELAKE_U0, "Coffeelake U0 (6+2)" },
};

static struct {
	u16 mchid;
	const char *name;
} mch_table[] = {
	{ PCI_DEVICE_ID_INTEL_CNL_ID_U, "Cannonlake-U" },
	{ PCI_DEVICE_ID_INTEL_CNL_ID_Y, "Cannonlake-Y" },
	{ PCI_DEVICE_ID_INTEL_CFL_ID_U, "Coffeelake U (4+3e)"},
	{ PCI_DEVICE_ID_INTEL_WHL_ID_Wx4, "Whiskeylake W (4+2)"},
	{ PCI_DEVICE_ID_INTEL_WHL_ID_Wx2, "Whiskeylake W (2+2)"},
	{ PCI_DEVICE_ID_INTEL_CFL_ID_H, "Coffeelake-H" },
	{ PCI_DEVICE_ID_INTEL_CFL_ID_S, "Coffeelake-S" },
};

static struct {
	u16 lpcid;
	const char *name;
} pch_table[] = {
	{ PCI_DEVICE_ID_INTEL_CNL_BASE_U_LPC, "Cannonlake-U Base" },
	{ PCI_DEVICE_ID_INTEL_CNL_U_PREMIUM_LPC, "Cannonlake-U Premium" },
	{ PCI_DEVICE_ID_INTEL_CNL_Y_PREMIUM_LPC, "Cannonlake-Y Premium" },
	{ PCI_DEVICE_ID_INTEL_CNP_H_LPC_Q370, "Cannonlake-H Q370" },
	{ PCI_DEVICE_ID_INTEL_CNP_H_LPC_QM370, "Cannonlake-H QM370" },
};

static struct {
	u16 igdid;
	const char *name;
} igd_table[] = {
	{ PCI_DEVICE_ID_INTEL_CNL_GT2_ULX_1, "Cannonlake ULX GT2" },
	{ PCI_DEVICE_ID_INTEL_CNL_GT2_ULX_2, "Cannonlake ULX GT1.5" },
	{ PCI_DEVICE_ID_INTEL_CNL_GT2_ULX_3, "Cannonlake ULX GT1" },
	{ PCI_DEVICE_ID_INTEL_CNL_GT2_ULX_4, "Cannonlake ULX GT0.5" },
	{ PCI_DEVICE_ID_INTEL_CNL_GT2_ULT_1, "Cannonlake ULT GT2" },
	{ PCI_DEVICE_ID_INTEL_CNL_GT2_ULT_2, "Cannonlake ULT GT1.5" },
	{ PCI_DEVICE_ID_INTEL_CNL_GT2_ULT_3, "Cannonlake ULT GT1" },
	{ PCI_DEVICE_ID_INTEL_CNL_GT2_ULT_4, "Cannonlake ULT GT0.5" },
	{ PCI_DEVICE_ID_INTEL_CFL_GT2_ULT, "Coffeelake ULT GT2"},
	{ PCI_DEVICE_ID_INTEL_WHL_GT2_ULT_1, "Whiskeylake ULT GT1"},
	{ PCI_DEVICE_ID_INTEL_CFL_H_GT2, "Coffeelake-H GT2" },
	{ PCI_DEVICE_ID_INTEL_CFL_S_GT2, "Coffeelake-S GT2" },
};

static uint8_t get_dev_revision(pci_devfn_t dev)
{
	return pci_read_config8(dev, PCI_REVISION_ID);
}

static uint16_t get_dev_id(pci_devfn_t dev)
{
	return pci_read_config16(dev, PCI_DEVICE_ID);
}

static void report_cpu_info(void)
{
	struct cpuid_result cpuidr;
	u32 i, index, cpu_id, cpu_feature_flag;
	char cpu_string[50], *cpu_name = cpu_string; /* 48 bytes are reported */
	int vt, txt, aes;
	msr_t microcode_ver;
	static const char *const mode[] = {"NOT ", ""};
	const char *cpu_type = "Unknown";
	u32 p[13];

	index = 0x80000000;
	cpuidr = cpuid(index);
	if (cpuidr.eax < 0x80000004) {
		strcpy(cpu_string, "Platform info not available");
	} else {
		int j=0;

		for (i = 2; i <= 4; i++) {
			cpuidr = cpuid(index + i);
			p[j++] = cpuidr.eax;
			p[j++] = cpuidr.ebx;
			p[j++] = cpuidr.ecx;
			p[j++] = cpuidr.edx;
		}
		p[12]=0;
		cpu_name = (char *)p;
	}
	/* Skip leading spaces in CPU name string */
	while (cpu_name[0] == ' ')
		cpu_name++;

	microcode_ver.lo = 0;
	microcode_ver.hi = 0;
	wrmsr(BIOS_SIGN_ID, microcode_ver);
	cpu_id = cpu_get_cpuid();
	microcode_ver = rdmsr(BIOS_SIGN_ID);

	/* Look for string to match the name */
	for (i = 0; i < ARRAY_SIZE(cpu_table); i++) {
		if (cpu_table[i].cpuid == cpu_id) {
			cpu_type = cpu_table[i].name;
			break;
		}
	}

	printk(BIOS_DEBUG, "CPU: %s\n", cpu_name);
	printk(BIOS_DEBUG, "CPU: ID %x, %s, ucode: %08x\n",
	       cpu_id, cpu_type, microcode_ver.hi);

	cpu_feature_flag = cpu_get_feature_flags_ecx();
	aes = (cpu_feature_flag & CPUID_AES) ? 1 : 0;
	txt = (cpu_feature_flag & CPUID_SMX) ? 1 : 0;
	vt = (cpu_feature_flag & CPUID_VMX) ? 1 : 0;
	printk(BIOS_DEBUG,
		"CPU: AES %ssupported, TXT %ssupported, VT %ssupported\n",
		mode[aes], mode[txt], mode[vt]);
}

static void report_mch_info(void)
{
	int i;
	pci_devfn_t dev = SA_DEV_ROOT;
	uint16_t mchid = get_dev_id(dev);
	uint8_t mch_revision = get_dev_revision(dev);
	const char *mch_type = "Unknown";

	for (i = 0; i < ARRAY_SIZE(mch_table); i++) {
		if (mch_table[i].mchid == mchid) {
			mch_type = mch_table[i].name;
			break;
		}
	}

	printk(BIOS_DEBUG, "MCH: device id %04x (rev %02x) is %s\n",
		mchid, mch_revision, mch_type);
}

static void report_pch_info(void)
{
	int i;
	pci_devfn_t dev = PCH_DEV_LPC;
	uint16_t lpcid = get_dev_id(dev);
	const char *pch_type = "Unknown";

	for (i = 0; i < ARRAY_SIZE(pch_table); i++) {
		if (pch_table[i].lpcid == lpcid) {
			pch_type = pch_table[i].name;
			break;
		}
	}
	printk(BIOS_DEBUG, "PCH: device id %04x (rev %02x) is %s\n",
		lpcid, get_dev_revision(dev), pch_type);
}

static void report_igd_info(void)
{
	int i;
	pci_devfn_t dev = SA_DEV_IGD;
	uint16_t igdid = get_dev_id(dev);
	const char *igd_type = "Unknown";

	for (i = 0; i < ARRAY_SIZE(igd_table); i++) {
		if (igd_table[i].igdid == igdid) {
			igd_type = igd_table[i].name;
			break;
		}
	}
	printk(BIOS_DEBUG, "IGD: device id %04x (rev %02x) is %s\n",
		igdid, get_dev_revision(dev), igd_type);
}

void report_platform_info(void)
{
	report_cpu_info();
	report_mch_info();
	report_pch_info();
	report_igd_info();
}
