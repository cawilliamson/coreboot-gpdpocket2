/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Vladimir Serbinenko
 * Copyright (C) 2018 Intel Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 or (at your option)
 * any later version of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <arch/acpi_device.h>
#include <arch/acpigen.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <elog.h>
#include <sar.h>
#include <smbios.h>
#include <string.h>
#include <wrdd.h>
#include "chip.h"

#define PMCS_DR 0xcc
#define PME_STS (1 << 15)

#if IS_ENABLED(CONFIG_GENERATE_SMBIOS_TABLES)
static int smbios_write_wifi(struct device *dev, int *handle,
			     unsigned long *current)
{
	struct smbios_type_intel_wifi {
		u8 type;
		u8 length;
		u16 handle;
		u8 str;
		u8 eos[2];
	} __packed;

	struct smbios_type_intel_wifi *t =
		(struct smbios_type_intel_wifi *)*current;
	int len = sizeof(struct smbios_type_intel_wifi);

	memset(t, 0, sizeof(struct smbios_type_intel_wifi));
	t->type = 0x85;
	t->length = len - 2;
	t->handle = *handle;
	/*
	 * Intel wifi driver expects this string to be in the table 0x85
	 * with PCI IDs enumerated below.
	 */
	t->str = smbios_add_string(t->eos, "KHOIHGIUCCHHII");

	len = t->length + smbios_string_table_len(t->eos);
	*current += len;
	*handle += 1;
	return len;
}
#endif

__weak
int get_wifi_sar_limits(struct wifi_sar_limits *sar_limits)
{
	return -1;
}

#if IS_ENABLED(CONFIG_HAVE_ACPI_TABLES)
static void emit_sar_acpi_structures(void)
{
	int i, j, package_size;
	struct wifi_sar_limits sar_limits;
	struct wifi_sar_delta_table *wgds;

	/* Retrieve the sar limits data */
	if (get_wifi_sar_limits(&sar_limits) < 0) {
		printk(BIOS_ERR, "Error: failed from getting SAR limits!\n");
		return;
	}

	/*
	 * Name ("WRDS", Package () {
	 *   Revision,
	 *   Package () {
	 *     Domain Type,	// 0x7:WiFi
	 *     WiFi SAR BIOS,	// BIOS SAR Enable/disable
	 *     SAR Table Set	// Set#1 of SAR Table (10 bytes)
	 *   }
	 * })
	 */
	acpigen_write_name("WRDS");
	acpigen_write_package(2);
	acpigen_write_dword(WRDS_REVISION);
	/* Emit 'Domain Type' + 'WiFi SAR BIOS' + 10 bytes for Set#1 */
	package_size = 1 + 1 + BYTES_PER_SAR_LIMIT;
	acpigen_write_package(package_size);
	acpigen_write_dword(WRDS_DOMAIN_TYPE_WIFI);
	acpigen_write_dword(CONFIG_SAR_ENABLE);
	for (i = 0; i < BYTES_PER_SAR_LIMIT; i++)
		acpigen_write_byte(sar_limits.sar_limit[0][i]);
	acpigen_pop_len();
	acpigen_pop_len();

	/*
	 * Name ("EWRD", Package () {
	 *   Revision,
	 *   Package () {
	 *     Domain Type,		// 0x7:WiFi
	 *     Dynamic SAR Enable,	// Dynamic SAR Enable/disable
	 *     Extended SAR sets,	// Number of optional SAR table sets
	 *     SAR Table Set,		// Set#2 of SAR Table (10 bytes)
	 *     SAR Table Set,		// Set#3 of SAR Table (10 bytes)
	 *     SAR Table Set		// Set#4 of SAR Table (10 bytes)
	 *   }
	 * })
	 */
	acpigen_write_name("EWRD");
	acpigen_write_package(2);
	acpigen_write_dword(EWRD_REVISION);
	/*
	 * Emit 'Domain Type' + "Dynamic SAR Enable' + 'Extended SAR sets'
	 * + number of bytes for Set#2 & 3 & 4
	 */
	package_size = 1 + 1 + 1 + (NUM_SAR_LIMITS - 1) * BYTES_PER_SAR_LIMIT;
	acpigen_write_package(package_size);
	acpigen_write_dword(EWRD_DOMAIN_TYPE_WIFI);
	acpigen_write_dword(CONFIG_DSAR_ENABLE);
	acpigen_write_dword(CONFIG_DSAR_SET_NUM);
	for (i = 1; i < NUM_SAR_LIMITS; i++)
		for (j = 0; j < BYTES_PER_SAR_LIMIT; j++)
			acpigen_write_byte(sar_limits.sar_limit[i][j]);
	acpigen_pop_len();
	acpigen_pop_len();


	if (!IS_ENABLED(CONFIG_GEO_SAR_ENABLE))
		return;

	/*
	 * Name ("WGDS", Package() {
	 *  Revision,
	 *  Package() {
	 *   DomainType,                         // 0x7:WiFi
	 *   WgdsWiFiSarDeltaGroup1PowerMax1,    // Group 1 FCC 2400 Max
	 *   WgdsWiFiSarDeltaGroup1PowerChainA1, // Group 1 FCC 2400 A Offset
	 *   WgdsWiFiSarDeltaGroup1PowerChainB1, // Group 1 FCC 2400 B Offset
	 *   WgdsWiFiSarDeltaGroup1PowerMax2,    // Group 1 FCC 5200 Max
	 *   WgdsWiFiSarDeltaGroup1PowerChainA2, // Group 1 FCC 5200 A Offset
	 *   WgdsWiFiSarDeltaGroup1PowerChainB2, // Group 1 FCC 5200 B Offset
	 *   WgdsWiFiSarDeltaGroup2PowerMax1,    // Group 2 EC Jap 2400 Max
	 *   WgdsWiFiSarDeltaGroup2PowerChainA1, // Group 2 EC Jap 2400 A Offset
	 *   WgdsWiFiSarDeltaGroup2PowerChainB1, // Group 2 EC Jap 2400 B Offset
	 *   WgdsWiFiSarDeltaGroup2PowerMax2,    // Group 2 EC Jap 5200 Max
	 *   WgdsWiFiSarDeltaGroup2PowerChainA2, // Group 2 EC Jap 5200 A Offset
	 *   WgdsWiFiSarDeltaGroup2PowerChainB2, // Group 2 EC Jap 5200 B Offset
	 *   WgdsWiFiSarDeltaGroup3PowerMax1,    // Group 3 ROW 2400 Max
	 *   WgdsWiFiSarDeltaGroup3PowerChainA1, // Group 3 ROW 2400 A Offset
	 *   WgdsWiFiSarDeltaGroup3PowerChainB1, // Group 3 ROW 2400 B Offset
	 *   WgdsWiFiSarDeltaGroup3PowerMax2,    // Group 3 ROW 5200 Max
	 *   WgdsWiFiSarDeltaGroup3PowerChainA2, // Group 3 ROW 5200 A Offset
	 *   WgdsWiFiSarDeltaGroup3PowerChainB2, // Group 3 ROW 5200 B Offset
	 *  }
	 * })
	 */

	wgds = &sar_limits.wgds;
	acpigen_write_name("WGDS");
	acpigen_write_package(2);
	acpigen_write_dword(wgds->version);
	/* Emit 'Domain Type' +
	 * Group specific delta of power (6 bytes * NUM_WGDS_SAR_GROUPS)
	 */
	package_size = sizeof(sar_limits.wgds.group) + 1;
	acpigen_write_package(package_size);
	acpigen_write_dword(WGDS_DOMAIN_TYPE_WIFI);
	for (i = 0; i < SAR_NUM_WGDS_GROUPS; i++) {
		acpigen_write_byte(wgds->group[i].power_max_2400mhz);
		acpigen_write_byte(wgds->group[i].power_chain_a_2400mhz);
		acpigen_write_byte(wgds->group[i].power_chain_b_2400mhz);
		acpigen_write_byte(wgds->group[i].power_max_5200mhz);
		acpigen_write_byte(wgds->group[i].power_chain_a_5200mhz);
		acpigen_write_byte(wgds->group[i].power_chain_b_5200mhz);
	}

	acpigen_pop_len();
	acpigen_pop_len();
}

static void intel_wifi_fill_ssdt(struct device *dev)
{
	struct drivers_intel_wifi_config *config = dev->chip_info;
	const char *path = acpi_device_path(dev->bus->dev);
	u32 address;

	if (!path || !dev->enabled)
		return;

	/* Device */
	acpigen_write_scope(path);
	acpigen_write_device(acpi_device_name(dev));
	acpigen_write_name_integer("_UID", 0);
	if (dev->chip_ops)
		acpigen_write_name_string("_DDN", dev->chip_ops->name);

	/* Address */
	address = PCI_SLOT(dev->path.pci.devfn) & 0xffff;
	address <<= 16;
	address |= PCI_FUNC(dev->path.pci.devfn) & 0xffff;
	acpigen_write_name_dword("_ADR", address);

	/* Wake capabilities */
	if (config && config->wake)
		acpigen_write_PRW(config->wake, 3);

	/* Fill regulatory domain structure */
	if (IS_ENABLED(CONFIG_HAVE_REGULATORY_DOMAIN)) {
		/*
		 * Name ("WRDD", Package () {
		 *   WRDD_REVISION, // Revision
		 *   Package () {
		 *     WRDD_DOMAIN_TYPE_WIFI,   // Domain Type, 7:WiFi
		 *     wifi_regulatory_domain() // Country Identifier
		 *   }
		 * })
		 */
		acpigen_write_name("WRDD");
		acpigen_write_package(2);
		acpigen_write_integer(WRDD_REVISION);
		acpigen_write_package(2);
		acpigen_write_dword(WRDD_DOMAIN_TYPE_WIFI);
		acpigen_write_dword(wifi_regulatory_domain());
		acpigen_pop_len();
		acpigen_pop_len();
	}

	/* Fill Wifi sar related ACPI structures */
	if (IS_ENABLED(CONFIG_USE_SAR))
		emit_sar_acpi_structures();

	acpigen_pop_len(); /* Device */
	acpigen_pop_len(); /* Scope */

	printk(BIOS_INFO, "%s.%s: %s %s\n", path, acpi_device_name(dev),
	       dev->chip_ops ? dev->chip_ops->name : "", dev_path(dev));
}

static const char *intel_wifi_acpi_name(const struct device *dev)
{
	return "WIFI";
}
#endif

static void pci_dev_apply_quirks(struct device *dev)
{
	unsigned int cap;
	uint16_t val;
	struct device *root = dev->bus->dev;

	switch (dev->device) {
	case PCI_DEVICE_ID_TP_9260_SERIES_WIFI:
		cap = pci_find_capability(root, PCI_CAP_ID_PCIE);
		/* Check the LTR for root port and enable it */
		if (cap) {
			val = pci_read_config16(root, cap +
				PCI_EXP_DEV_CAP2_OFFSET);
			if (val & LTR_MECHANISM_SUPPORT) {
				val = pci_read_config16(root, cap +
					PCI_EXP_DEV_CTL_STS2_CAP_OFFSET);
				val |= LTR_MECHANISM_EN;
				pci_write_config16(root, cap +
					PCI_EXP_DEV_CTL_STS2_CAP_OFFSET, val);
			}
		}
	}
}

static void wifi_pci_dev_init(struct device *dev)
{
	pci_dev_init(dev);
	pci_dev_apply_quirks(dev);

	if (IS_ENABLED(CONFIG_ELOG)) {
		uint32_t val;
		val = pci_read_config16(dev, PMCS_DR);
		if (val & PME_STS)
			elog_add_event_wake(ELOG_WAKE_SOURCE_PME_WIFI, 0);
	}
}

static struct pci_operations pci_ops = {
	.set_subsystem = pci_dev_set_subsystem,
};

struct device_operations device_ops = {
	.read_resources           = pci_dev_read_resources,
	.set_resources            = pci_dev_set_resources,
	.enable_resources         = pci_dev_enable_resources,
	.init                     = wifi_pci_dev_init,
#if IS_ENABLED(CONFIG_GENERATE_SMBIOS_TABLES)
	.get_smbios_data          = smbios_write_wifi,
#endif
	.ops_pci                  = &pci_ops,
#if IS_ENABLED(CONFIG_HAVE_ACPI_TABLES)
	.acpi_name                = intel_wifi_acpi_name,
	.acpi_fill_ssdt_generator = intel_wifi_fill_ssdt,
#endif
};

static const unsigned short pci_device_ids[] = {
	PCI_DEVICE_ID_1000_SERIES_WIFI,
	PCI_DEVICE_ID_6005_SERIES_WIFI,
	PCI_DEVICE_ID_6005_I_SERIES_WIFI,
	PCI_DEVICE_ID_1030_SERIES_WIFI,
	PCI_DEVICE_ID_6030_I_SERIES_WIFI,
	PCI_DEVICE_ID_6030_SERIES_WIFI,
	PCI_DEVICE_ID_6150_SERIES_WIFI,
	PCI_DEVICE_ID_2030_SERIES_WIFI,
	PCI_DEVICE_ID_2000_SERIES_WIFI,
	PCI_DEVICE_ID_0135_SERIES_WIFI,
	PCI_DEVICE_ID_0105_SERIES_WIFI,
	PCI_DEVICE_ID_6035_SERIES_WIFI,
	PCI_DEVICE_ID_5300_SERIES_WIFI,
	PCI_DEVICE_ID_5100_SERIES_WIFI,
	PCI_DEVICE_ID_6000_SERIES_WIFI,
	PCI_DEVICE_ID_6000_I_SERIES_WIFI,
	PCI_DEVICE_ID_5350_SERIES_WIFI,
	PCI_DEVICE_ID_5150_SERIES_WIFI,
	/* Wilkins Peak 2 */
	PCI_DEVICE_ID_WP_7260_SERIES_1_WIFI,
	PCI_DEVICE_ID_WP_7260_SERIES_2_WIFI,
	/* Stone Peak 2 */
	PCI_DEVICE_ID_SP_7265_SERIES_1_WIFI,
	PCI_DEVICE_ID_SP_7265_SERIES_2_WIFI,
	/* Stone Field Peak */
	PCI_DEVICE_ID_SFP_8260_SERIES_1_WIFI,
	PCI_DEVICE_ID_SFP_8260_SERIES_2_WIFI,
	/* Windstorm Peak */
	PCI_DEVICE_ID_WSP_8275_SERIES_1_WIFI,
	/* Jefferson Peak */
	PCI_DEVICE_ID_JP_9000_SERIES_1_WIFI,
	PCI_DEVICE_ID_JP_9000_SERIES_2_WIFI,
	PCI_DEVICE_ID_JP_9000_SERIES_3_WIFI,
	/* Thunder Peak 2 */
	PCI_DEVICE_ID_TP_9260_SERIES_WIFI,
	/* Harrison Peak */
	PCI_DEVICE_ID_HrP_9560_SERIES_1_WIFI,
	PCI_DEVICE_ID_HrP_9560_SERIES_2_WIFI,
	0
};

static const struct pci_driver pch_intel_wifi __pci_driver = {
	.ops	 = &device_ops,
	.vendor	 = PCI_VENDOR_ID_INTEL,
	.devices = pci_device_ids,
};

static void intel_wifi_enable(struct device *dev)
{
	dev->ops = &device_ops;
}

struct chip_operations drivers_intel_wifi_ops = {
	CHIP_NAME("Intel WiFi")
	.enable_dev = intel_wifi_enable
};
