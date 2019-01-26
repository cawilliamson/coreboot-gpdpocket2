/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2016-2017 Intel Corporation.
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

#include <chip.h>
#include <bootmode.h>
#include <bootstate.h>
#include <device/pci.h>
#include <fsp/api.h>
#include <arch/acpi.h>
#include <arch/io.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci_ids.h>
#include <fsp/util.h>
#include <intelblocks/chip.h>
#include <intelblocks/itss.h>
#include <intelblocks/xdci.h>
#include <intelpch/lockdown.h>
#include <romstage_handoff.h>
#include <soc/acpi.h>
#include <soc/intel/common/vbt.h>
#include <soc/interrupt.h>
#include <soc/iomap.h>
#include <soc/irq.h>
#include <soc/itss.h>
#include <soc/pci_devs.h>
#include <soc/ramstage.h>
#include <soc/systemagent.h>
#include <string.h>

struct pcie_entry {
	unsigned int devfn;
	unsigned int func_count;
};

/*
 * According to table 2-2 in doc#546717:
 * PCI bus[function]	ID
 * D28:[F0 - F7]		0xA110 - 0xA117
 * D29:[F0 - F7]		0xA118 - 0xA11F
 * D27:[F0 - F3]		0xA167 - 0xA16A
 */
static const struct pcie_entry pcie_table_skl_pch_h[] = {
	{PCH_DEVFN_PCIE1, 8},
	{PCH_DEVFN_PCIE9, 8},
	{PCH_DEVFN_PCIE17, 4},
};

/*
 * According to table 2-2 in doc#564464:
 * PCI bus[function]	ID
 * D28:[F0 - F7]		0xA290 - 0xA297
 * D29:[F0 - F7]		0xA298 - 0xA29F
 * D27:[F0 - F7]		0xA2E7 - 0xA2EE
 */
static const struct pcie_entry pcie_table_kbl_pch_h[] = {
	{PCH_DEVFN_PCIE1, 8},
	{PCH_DEVFN_PCIE9, 8},
	{PCH_DEVFN_PCIE17, 8},
};

/*
 * According to table 2-2 in doc#567995/545659:
 * PCI bus[function]	ID
 * D28:[F0 - F7]		0x9D10 - 0x9D17
 * D29:[F0 - F3]		0x9D18 - 0x9D1B
 */
static const struct pcie_entry pcie_table_skl_pch_lp[] = {
	{PCH_DEVFN_PCIE1, 8},
	{PCH_DEVFN_PCIE9, 4},
};

/*
 * If the PCIe root port at function 0 is disabled,
 * the PCIe root ports might be coalesced after FSP silicon init.
 * The below function will swap the devfn of the first enabled device
 * in devicetree and function 0 resides a pci device
 * so that it won't confuse coreboot.
 */
static void pcie_update_device_tree(const struct pcie_entry *pcie_rp_group,
		size_t pci_groups)
{
	struct device *func0;
	unsigned int devfn, devfn0;
	int i, group;
	unsigned int inc = PCI_DEVFN(0, 1);

	for (group = 0; group < pci_groups; group++) {
		devfn0 = pcie_rp_group[group].devfn;
		func0 = dev_find_slot(0, devfn0);
		if (func0 == NULL)
			continue;

		/* No more functions if function 0 is disabled. */
		if (pci_read_config32(func0, PCI_VENDOR_ID) == 0xffffffff)
			continue;

		devfn = devfn0 + inc;

		/*
		 * Increase function by 1.
		 * Then find first enabled device to replace func0
		 * as that port was move to func0.
		 */
		for (i = 1; i < pcie_rp_group[group].func_count;
				i++, devfn += inc) {
			struct device *dev = dev_find_slot(0, devfn);
			if (dev == NULL || !dev->enabled)
				continue;

			/*
			 * Found the first enabled device in
			 * a given dev number.
			 */
			printk(BIOS_INFO, "PCI func %d was swapped"
				" to func 0.\n", i);
			func0->path.pci.devfn = dev->path.pci.devfn;
			dev->path.pci.devfn = devfn0;
			break;
		}
	}
}

static void pcie_override_devicetree_after_silicon_init(void)
{
	uint16_t id, id_mask;

	id = pci_read_config16(PCH_DEV_PCIE1, PCI_DEVICE_ID);
	/*
	 * We may read an ID other than func 0 after FSP-S.
	 * Strip out 4 least significant bits.
	 */
	id_mask = id & ~0xf;
	printk(BIOS_INFO, "Override DT after FSP-S, PCH is ");
	if (id_mask == (PCI_DEVICE_ID_INTEL_SPT_LP_PCIE_RP1 & ~0xf)) {
		printk(BIOS_INFO, "KBL/SKL PCH-LP SKU\n");
		pcie_update_device_tree(&pcie_table_skl_pch_lp[0],
			ARRAY_SIZE(pcie_table_skl_pch_lp));
	} else if (id_mask == (PCI_DEVICE_ID_INTEL_KBP_H_PCIE_RP1 & ~0xf)) {
		printk(BIOS_INFO, "KBL PCH-H SKU\n");
		pcie_update_device_tree(&pcie_table_kbl_pch_h[0],
			ARRAY_SIZE(pcie_table_kbl_pch_h));
	} else if (id_mask == (PCI_DEVICE_ID_INTEL_SPT_H_PCIE_RP1 & ~0xf)) {
		printk(BIOS_INFO, "SKL PCH-H SKU\n");
		pcie_update_device_tree(&pcie_table_skl_pch_h[0],
			ARRAY_SIZE(pcie_table_skl_pch_h));
	} else {
		printk(BIOS_ERR, "[BUG] PCIE Root Port id 0x%x"
			" is not found\n", id);
		return;
	}
}

void soc_init_pre_device(void *chip_info)
{
	/* Snapshot the current GPIO IRQ polarities. FSP is setting a
	 * default policy that doesn't honor boards' requirements. */
	itss_snapshot_irq_polarities(GPIO_IRQ_START, GPIO_IRQ_END);

	/* Perform silicon specific init. */
	fsp_silicon_init(romstage_handoff_is_resume());

	/* Restore GPIO IRQ polarities back to previous settings. */
	itss_restore_irq_polarities(GPIO_IRQ_START, GPIO_IRQ_END);

	/* swap enabled PCI ports in device tree if needed */
	pcie_override_devicetree_after_silicon_init();
}

void soc_fsp_load(void)
{
	fsps_load(romstage_handoff_is_resume());
}

static void pci_domain_set_resources(struct device *dev)
{
	assign_resources(dev->link_list);
}

static struct device_operations pci_domain_ops = {
	.read_resources   = &pci_domain_read_resources,
	.set_resources    = &pci_domain_set_resources,
	.scan_bus         = &pci_domain_scan_bus,
#if IS_ENABLED(CONFIG_HAVE_ACPI_TABLES)
	.write_acpi_tables	= &northbridge_write_acpi_tables,
	.acpi_name		= &soc_acpi_name,
#endif
};

static struct device_operations cpu_bus_ops = {
	.read_resources   = DEVICE_NOOP,
	.set_resources    = DEVICE_NOOP,
	.enable_resources = DEVICE_NOOP,
	.init             = DEVICE_NOOP,
#if IS_ENABLED(CONFIG_HAVE_ACPI_TABLES)
	.acpi_fill_ssdt_generator = generate_cpu_entries,
#endif
};

static void soc_enable(struct device *dev)
{
	/* Set the operations if it is a special bus type */
	if (dev->path.type == DEVICE_PATH_DOMAIN)
		dev->ops = &pci_domain_ops;
	else if (dev->path.type == DEVICE_PATH_CPU_CLUSTER)
		dev->ops = &cpu_bus_ops;
}

struct chip_operations soc_intel_skylake_ops = {
	CHIP_NAME("Intel 6th Gen")
	.enable_dev	= &soc_enable,
	.init		= &soc_init_pre_device,
};

/* UPD parameters to be initialized before SiliconInit */
void platform_fsp_silicon_init_params_cb(FSPS_UPD *supd)
{
	FSP_S_CONFIG *params = &supd->FspsConfig;
	FSP_S_TEST_CONFIG *tconfig = &supd->FspsTestConfig;
	static struct soc_intel_skylake_config *config;
	uintptr_t vbt_data = (uintptr_t)vbt_get();
	int i;

	struct device *dev = SA_DEV_ROOT;
	if (!dev || !dev->chip_info) {
		printk(BIOS_ERR, "BUG! Could not find SOC devicetree config\n");
		return;
	}
	config = dev->chip_info;

	mainboard_silicon_init_params(params);
	/* Set PsysPmax if it is available from DT */
	if (config->psys_pmax) {
		/* PsysPmax is in unit of 1/8 Watt */
		tconfig->PsysPmax = config->psys_pmax * 8;
		printk(BIOS_DEBUG, "psys_pmax = %d\n", tconfig->PsysPmax);
	}

	params->GraphicsConfigPtr = (u32) vbt_data;

	for (i = 0; i < ARRAY_SIZE(config->usb2_ports); i++) {
		params->PortUsb20Enable[i] =
				config->usb2_ports[i].enable;
		params->Usb2OverCurrentPin[i] =
				config->usb2_ports[i].ocpin;
		params->Usb2AfePetxiset[i] =
				config->usb2_ports[i].pre_emp_bias;
		params->Usb2AfeTxiset[i] =
				config->usb2_ports[i].tx_bias;
		params->Usb2AfePredeemp[i] =
				config->usb2_ports[i].tx_emp_enable;
		params->Usb2AfePehalfbit[i] =
				config->usb2_ports[i].pre_emp_bit;
	}

	for (i = 0; i < ARRAY_SIZE(config->usb3_ports); i++) {
		params->PortUsb30Enable[i] = config->usb3_ports[i].enable;
		params->Usb3OverCurrentPin[i] = config->usb3_ports[i].ocpin;
		if (config->usb3_ports[i].tx_de_emp) {
			params->Usb3HsioTxDeEmphEnable[i] = 1;
			params->Usb3HsioTxDeEmph[i] =
				config->usb3_ports[i].tx_de_emp;
		}
		if (config->usb3_ports[i].tx_downscale_amp) {
			params->Usb3HsioTxDownscaleAmpEnable[i] = 1;
			params->Usb3HsioTxDownscaleAmp[i] =
				config->usb3_ports[i].tx_downscale_amp;
		}
	}

	memcpy(params->SataPortsEnable, config->SataPortsEnable,
	       sizeof(params->SataPortsEnable));
	memcpy(params->SataPortsDevSlp, config->SataPortsDevSlp,
	       sizeof(params->SataPortsDevSlp));
	memcpy(params->PcieRpClkReqSupport, config->PcieRpClkReqSupport,
	       sizeof(params->PcieRpClkReqSupport));
	memcpy(params->PcieRpClkReqNumber, config->PcieRpClkReqNumber,
	       sizeof(params->PcieRpClkReqNumber));
	memcpy(params->PcieRpAdvancedErrorReporting,
		config->PcieRpAdvancedErrorReporting,
			sizeof(params->PcieRpAdvancedErrorReporting));
	memcpy(params->PcieRpLtrEnable, config->PcieRpLtrEnable,
	       sizeof(params->PcieRpLtrEnable));
	memcpy(params->PcieRpHotPlug, config->PcieRpHotPlug,
	       sizeof(params->PcieRpHotPlug));

	/*
	 * PcieRpClkSrcNumber UPD is set to clock source number(0-6) for
	 * all the enabled PCIe root ports, invalid(0x1F) is set for
	 * disabled PCIe root ports.
	 */
	for (i = 0; i < CONFIG_MAX_ROOT_PORTS; i++) {
		if (config->PcieRpClkReqSupport[i])
			params->PcieRpClkSrcNumber[i] =
				config->PcieRpClkSrcNumber[i];
		else
			params->PcieRpClkSrcNumber[i] = 0x1F;
	}

	/* disable Legacy PME */
	memset(params->PcieRpPmSci, 0, sizeof(params->PcieRpPmSci));

	memcpy(params->SerialIoDevMode, config->SerialIoDevMode,
	       sizeof(params->SerialIoDevMode));

	params->PchCio2Enable = config->Cio2Enable;
	params->SaImguEnable = config->SaImguEnable;
	params->Heci3Enabled = config->Heci3Enabled;

	params->LogoPtr = config->LogoPtr;
	params->LogoSize = config->LogoSize;

	params->CpuConfig.Bits.VmxEnable = config->VmxEnable;

	params->PchPmWoWlanEnable = config->PchPmWoWlanEnable;
	params->PchPmWoWlanDeepSxEnable = config->PchPmWoWlanDeepSxEnable;
	params->PchPmLanWakeFromDeepSx = config->WakeConfigPcieWakeFromDeepSx;

	params->PchLanEnable = config->EnableLan;
	if (config->EnableLan) {
		params->PchLanLtrEnable = config->EnableLanLtr;
		params->PchLanK1OffEnable = config->EnableLanK1Off;
		params->PchLanClkReqSupported = config->LanClkReqSupported;
		params->PchLanClkReqNumber = config->LanClkReqNumber;
	}
	params->SataSalpSupport = config->SataSalpSupport;
	params->SsicPortEnable = config->SsicPortEnable;
	params->ScsEmmcEnabled = config->ScsEmmcEnabled;
	params->ScsEmmcHs400Enabled = config->ScsEmmcHs400Enabled;
	params->ScsSdCardEnabled = config->ScsSdCardEnabled;

	if (!!params->ScsEmmcHs400Enabled && !!config->EmmcHs400DllNeed) {
		params->PchScsEmmcHs400DllDataValid =
			!!config->EmmcHs400DllNeed;
		params->PchScsEmmcHs400RxStrobeDll1 =
			config->ScsEmmcHs400RxStrobeDll1;
		params->PchScsEmmcHs400TxDataDll =
			config->ScsEmmcHs400TxDataDll;
	}

	/* If ISH is enabled, enable ISH elements */
	dev = dev_find_slot(0, PCH_DEVFN_ISH);
	if (dev)
		params->PchIshEnable = dev->enabled;
	else
		params->PchIshEnable = 0;

	params->PchHdaEnable = config->EnableAzalia;
	params->PchHdaIoBufferOwnership = config->IoBufferOwnership;
	params->PchHdaDspEnable = config->DspEnable;
	params->Device4Enable = config->Device4Enable;
	params->SataEnable = config->EnableSata;
	params->SataMode = config->SataMode;
	params->SataSpeedLimit = config->SataSpeedLimit;
	params->SataPwrOptEnable = config->SataPwrOptEnable;
	params->EnableTcoTimer = !config->PmTimerDisabled;

	tconfig->PchLockDownGlobalSmi = config->LockDownConfigGlobalSmi;
	tconfig->PchLockDownRtcLock = config->LockDownConfigRtcLock;
	tconfig->PowerLimit4 = config->PowerLimit4;
	/*
	 * To disable HECI, the Psf needs to be left unlocked
	 * by FSP till end of post sequence. Based on the devicetree
	 * setting, we set the appropriate PsfUnlock policy in FSP,
	 * do the changes and then lock it back in coreboot during finalize.
	 */
	tconfig->PchSbAccessUnlock = (config->HeciEnabled == 0) ? 1 : 0;
	if (get_lockdown_config() == CHIPSET_LOCKDOWN_COREBOOT) {
		tconfig->PchLockDownBiosInterface = 0;
		params->PchLockDownBiosLock = 0;
		params->PchLockDownSpiEiss = 0;
		/*
		 * Skip Spi Flash Lockdown from inside FSP.
		 * Making this config "0" means FSP won't set the FLOCKDN bit
		 * of SPIBAR + 0x04 (i.e., Bit 15 of BIOS_HSFSTS_CTL).
		 * So, it becomes coreboot's responsibility to set this bit
		 * before end of POST for security concerns.
		 */
		params->SpiFlashCfgLockDown = 0;
	}
	/* only replacing preexisting subsys ID defaults when non-zero */
#if defined(CONFIG_SUBSYSTEM_VENDOR_ID) && CONFIG_SUBSYSTEM_VENDOR_ID
	params->DefaultSvid = CONFIG_SUBSYSTEM_VENDOR_ID;
	params->PchSubSystemVendorId = CONFIG_SUBSYSTEM_VENDOR_ID;
#endif
#if defined(CONFIG_SUBSYSTEM_DEVICE_ID) && CONFIG_SUBSYSTEM_DEVICE_ID
	params->DefaultSid = CONFIG_SUBSYSTEM_DEVICE_ID;
	params->PchSubSystemId = CONFIG_SUBSYSTEM_DEVICE_ID;
#endif
	params->PchPmWolEnableOverride = config->WakeConfigWolEnableOverride;
	params->PchPmPcieWakeFromDeepSx = config->WakeConfigPcieWakeFromDeepSx;
	params->PchPmDeepSxPol = config->PmConfigDeepSxPol;
	params->PchPmSlpS0Enable = config->s0ix_enable;
	params->PchPmSlpS3MinAssert = config->PmConfigSlpS3MinAssert;
	params->PchPmSlpS4MinAssert = config->PmConfigSlpS4MinAssert;
	params->PchPmSlpSusMinAssert = config->PmConfigSlpSusMinAssert;
	params->PchPmSlpAMinAssert = config->PmConfigSlpAMinAssert;
	params->PchPmLpcClockRun = config->PmConfigPciClockRun;
	params->PchPmSlpStrchSusUp = config->PmConfigSlpStrchSusUp;
	params->PchPmPwrBtnOverridePeriod =
				config->PmConfigPwrBtnOverridePeriod;
	params->PchPmPwrCycDur = config->PmConfigPwrCycDur;

	/* Indicate whether platform supports Voltage Margining */
	params->PchPmSlpS0VmEnable = config->PchPmSlpS0VmEnable;

	params->PchSirqEnable = config->SerialIrqConfigSirqEnable;
	params->PchSirqMode = config->SerialIrqConfigSirqMode;

	params->CpuConfig.Bits.SkipMpInit = !chip_get_fsp_mp_init();

	for (i = 0; i < ARRAY_SIZE(config->i2c_voltage); i++)
		params->SerialIoI2cVoltage[i] = config->i2c_voltage[i];

	for (i = 0; i < ARRAY_SIZE(config->domain_vr_config); i++)
		fill_vr_domain_config(params, i, &config->domain_vr_config[i]);

	/* Show SPI controller if enabled in devicetree.cb */
	dev = dev_find_slot(0, PCH_DEVFN_SPI);
	params->ShowSpiController = dev->enabled;

	/* Enable xDCI controller if enabled in devicetree and allowed */
	dev = dev_find_slot(0, PCH_DEVFN_USBOTG);
	if (!xdci_can_enable())
		dev->enabled = 0;
	params->XdciEnable = dev->enabled;

	/*
	 * Send VR specific mailbox commands:
	 * 000b - no VR specific command sent
	 * 001b - VR mailbox command specifically for the MPS IMPV8 VR
	 *	  will be sent
	 * 010b - VR specific command sent for PS4 exit issue
	 * 100b - VR specific command sent for MPS VR decay issue
	 */
	params->SendVrMbxCmd1 = config->SendVrMbxCmd;

	/*
	 * Activates VR mailbox command for Intersil VR C-state issues.
	 * 0 - no mailbox command sent.
	 * 1 - VR mailbox command sent for IA/GT rails only.
	 * 2 - VR mailbox command sent for IA/GT/SA rails.
	 */
	params->IslVrCmd = config->IslVrCmd;

	/* Acoustic Noise Mitigation */
	params->AcousticNoiseMitigation = config->AcousticNoiseMitigation;
	params->SlowSlewRateForIa = config->SlowSlewRateForIa;
	params->SlowSlewRateForGt = config->SlowSlewRateForGt;
	params->SlowSlewRateForSa = config->SlowSlewRateForSa;
	params->FastPkgCRampDisableIa = config->FastPkgCRampDisableIa;
	params->FastPkgCRampDisableGt = config->FastPkgCRampDisableGt;
	params->FastPkgCRampDisableSa = config->FastPkgCRampDisableSa;

	/* Enable PMC XRAM read */
	tconfig->PchPmPmcReadDisable = config->PchPmPmcReadDisable;

	/* Enable/Disable EIST */
	tconfig->Eist = config->eist_enable;

	/* Set TccActivationOffset */
	tconfig->TccActivationOffset = config->tcc_offset;

	/* Enable VT-d and X2APIC */
	if (!config->ignore_vtd && soc_is_vtd_capable()) {
		params->VtdBaseAddress[0] = GFXVT_BASE_ADDRESS;
		params->VtdBaseAddress[1] = VTVC0_BASE_ADDRESS;
		params->X2ApicOptOut = 0;
		tconfig->VtdDisable = 0;

		params->PchIoApicBdfValid = 1;
		params->PchIoApicBusNumber = 250;
		params->PchIoApicDeviceNumber = 31;
		params->PchIoApicFunctionNumber = 0;
	}

	soc_irq_settings(params);
}

/* Mainboard GPIO Configuration */
__weak void mainboard_silicon_init_params(FSP_S_CONFIG *params)
{
	printk(BIOS_DEBUG, "WEAK: %s/%s called\n", __FILE__, __func__);
}
