/*
 * This file is part of the coreboot project.
 *
 * Copyright 2018 Google LLC
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

#include <baseboard/gpio.h>
#include <baseboard/variants.h>
#include <commonlib/helpers.h>
#include <device/device.h>
#include <soc/pci_devs.h>

/* Pad configuration in ramstage */
/* Leave eSPI pins untouched from default settings */
static const struct pad_config gpio_table[] = {
	/* A0  : RCIN# ==> NC(TP763) */
	PAD_CFG_NC(GPP_A0),
	/* A1  : ESPI_IO0 */
	/* A2  : ESPI_IO1 */
	/* A3  : ESPI_IO2 */
	/* A4  : ESPI_IO3 */
	/* A5  : ESPI_CS# */
	/* A6  : SERIRQ ==> NC(TP764) */
	PAD_CFG_NC(GPP_A6),
	/* A7  : PIRQA# ==> NC(TP703) */
	PAD_CFG_NC(GPP_A7),
	/* A8  : CLKRUN# ==> NC(TP758)) */
	PAD_CFG_NC(GPP_A8),
	/* A9  : ESPI_CLK */
	/* A10 : CLKOUT_LPC1 ==> NC */
	PAD_CFG_NC(GPP_A10),
	/* A11 : PME# ==> NC(TP726) */
	PAD_CFG_NC(GPP_A11),
	/* A12 : BM_BUSY# ==> NC */
	PAD_CFG_NC(GPP_A12),
	/* A13 : SUSWARN# ==> SUSWARN_L */
	PAD_CFG_NF(GPP_A13, NONE, DEEP, NF1),
	/* A14 : ESPI_RESET# */
	/* A15 : SUSACK# ==> SUSACK_L */
	PAD_CFG_NF(GPP_A15, NONE, DEEP, NF1),
	/* A16 : SD_1P8_SEL ==> NC */
	PAD_CFG_NC(GPP_A16),
	/* A17 : SD_PWR_EN# ==> NC */
	PAD_CFG_NC(GPP_A17),
	/* A18 : ISH_GP0 ==> ISH_GP0 */
	PAD_CFG_NC(GPP_A18),
	/* A21 : ISH_GP3 */
	PAD_CFG_NC(GPP_A21),
	/* A22 : ISH_GP4 */
	PAD_CFG_NC(GPP_A22),
	/* A23 : ISH_GP5 ==> TRACKPAD_INT_L */
	PAD_CFG_GPI_ACPI_SCI(GPP_A23, NONE, DEEP, INVERT),

	/* B0  : CORE_VID0 ==> NC(TP42) */
	PAD_CFG_NC(GPP_B0),
	/* B1  : CORE_VID1 ==> NC(TP43) */
	PAD_CFG_NC(GPP_B1),
	/* B2  : VRALERT# ==> NC */
	PAD_CFG_NC(GPP_B2),
	/* B3  : CPU_GP2 ==> NC */
	PAD_CFG_NC(GPP_B3),
	/* B4  : CPU_GP3 ==> NC */
	PAD_CFG_NC(GPP_B4),
	/* B5  : SRCCLKREQ0# ==> NC */
	PAD_CFG_NC(GPP_B5),
	/* B6  : SRCCLKREQ1# ==> WLAN_PCIE_CLKREQ_L */
	PAD_CFG_NF(GPP_B6, NONE, DEEP, NF1),
	/* B7  : SRCCLKREQ2# ==> NC */
	PAD_CFG_NC(GPP_B7),
	/* B8  : SRCCLKREQ3# ==> WLAN_PE_RST */
	PAD_CFG_GPO(GPP_B8, 0, RSMRST),
	/* B9  : SRCCLKREQ4# ==> NVME_PCIE_CLKREQ_L */
	PAD_CFG_NF(GPP_B9, NONE, DEEP, NF1),
	/* B10 : SRCCLKREQ5# ==> NC */
	PAD_CFG_NC(GPP_B10),
	/* B11 : EXT_PWR_GATE# ==> NC */
	PAD_CFG_NC(GPP_B11),
	/* B12 : SLP_S0# ==> SLP_S0_L_G */
	PAD_CFG_NF(GPP_B12, NONE, DEEP, NF1),
	/* B13 : PLTRST# ==> PLT_RST_L */
	PAD_CFG_NF(GPP_B13, NONE, DEEP, NF1),
	/* B14 : SPKR ==> NC */
	PAD_CFG_NC(GPP_B14),
	/* B15 : GSPI0_CS# ==> H1_SLAVE_SPI_CS_L */
	PAD_CFG_NF(GPP_B15, NONE, DEEP, NF1),
	/* B16 : GSPI0_CLK ==> H1_SLAVE_SPI_CLK */
	PAD_CFG_NF(GPP_B16, NONE, DEEP, NF1),
	/* B17 : GSPI0_MISO ==> H1_SLAVE_SPI_MISO */
	PAD_CFG_NF(GPP_B17, NONE, DEEP, NF1),
	/* B18 : GSPI0_MOSI ==> H1_SLAVE_SPI_MOSI */
	PAD_CFG_NF(GPP_B18, NONE, DEEP, NF1),
	/* B19 : GSPI1_CS# ==> NC */
	PAD_CFG_NC(GPP_B19),
	/* B20 : GSPI1_CLK ==> NC */
	PAD_CFG_NC(GPP_B20),
	/* B21 : GSPI1_MISO ==> NC */
	PAD_CFG_NC(GPP_B21),
	/* B22 : GSPI1_MOSI ==> NC */
	PAD_CFG_NC(GPP_B22),
	/* B23 : SM1ALERT# ==> NC */
	PAD_CFG_NC(GPP_B23),

	/* C0  : SMBCLK ==> NC */
	PAD_CFG_NC(GPP_C0),
	/* C1  : SMBDATA ==> NC */
	PAD_CFG_NC(GPP_C1),
	/* C2  : SMBALERT# ==> NC */
	PAD_CFG_NC(GPP_C2),
	/* C3  : SML0CLK ==> NC */
	PAD_CFG_NC(GPP_C3),
	/* C4  : SML0DATA ==> NC */
	PAD_CFG_NC(GPP_C4),
	/* C5  : SML0ALERT# ==> NC */
	PAD_CFG_NC(GPP_C5),
	/* C6  : SM1CLK ==> EC_IN_RW_OD */
	PAD_CFG_GPI_GPIO_DRIVER(GPP_C6, 20K_PU, DEEP),
	/* C7  : SM1DATA ==> NC */
	PAD_CFG_NC(GPP_C7),
	/* C8  : UART0_RXD ==> NC */
	PAD_CFG_NC(GPP_C8),
	/* C9  : UART0_TXD ==> NC */
	PAD_CFG_NC(GPP_C9),
	/* C10 : UART0_RTS# ==> NC */
	PAD_CFG_NC(GPP_C10),
	/* C11 : UART0_CTS# ==> NC */
	PAD_CFG_NC(GPP_C11),
	/* C12 : UART1_RXD ==> PCH_MEM_CONFIG[0] */
	PAD_CFG_GPI_GPIO_DRIVER(GPP_C12, NONE, DEEP),
	/* C13 : UART1_TXD ==> PCH_MEM_CONFIG[1] */
	PAD_CFG_GPI_GPIO_DRIVER(GPP_C13, NONE, DEEP),
	/* C14 : UART1_RTS# ==> PCH_MEM_CONFIG[2] */
	PAD_CFG_GPI_GPIO_DRIVER(GPP_C14, NONE, DEEP),
	/* C15 : UART1_CTS# ==> PCH_MEM_CONFIG[3] */
	PAD_CFG_GPI_GPIO_DRIVER(GPP_C15, NONE, DEEP),
	/* C16 : I2C0_SDA ==> PCH_I2C0_TOUCHSCREEN_SDA */
	PAD_CFG_NF(GPP_C16, NONE, DEEP, NF1),
	/* C17 : I2C0_SCL ==> PCH_I2C0_TOUCHSCREEN_SCL */
	PAD_CFG_NF(GPP_C17, NONE, DEEP, NF1),
	/* C18 : I2C1_SDA ==> NC */
	PAD_CFG_NC(GPP_C18),
	/* C19 : I2C1_SCL ==> NC */
	PAD_CFG_NC(GPP_C19),
	/* C20 : UART2_RXD ==> PCHRX_SERVOTX_UART */
	PAD_CFG_NF(GPP_C20, NONE, DEEP, NF1),
	/* C21 : UART2_TXD ==> PCHTX_SERVORX_UART */
	PAD_CFG_NF(GPP_C21, NONE, DEEP, NF1),
	/* C22 : UART2_RTS# ==> EN_PP3300_DX_TOUCHSCREEN */
	PAD_CFG_GPO(GPP_C22, 0, DEEP),
	/* C23 : UART2_CTS# ==> PCH_WP */
	PAD_CFG_GPI_GPIO_DRIVER(GPP_C23, 20K_PU, DEEP),

	/* D0  : SPI1_CS# ==> NC */
	PAD_CFG_NC(GPP_D0),
	/* D1  : SPI1_CLK ==> TRACKPAD_RESET_1V8_ODL */
	PAD_CFG_GPO(GPP_D1, 0, DEEP),
	/* D2  : SPI1_MISO ==> NC */
	PAD_CFG_NC(GPP_D2),
	/* D3  : SPI1_MOSI ==> NC */
	PAD_CFG_NC(GPP_D3),
	/* D4  : FASHTRIG ==> NC */
	PAD_CFG_NC(GPP_D4),
	/* D5  : ISH_I2C0_SDA ==> ISH_I2C0_SDA */
	PAD_CFG_NF_1V8(GPP_D5, NONE, DEEP, NF1),
	/* D6  : ISH_I2C0_SCL ==> ISH_I2C0_SCL */
	PAD_CFG_NF_1V8(GPP_D6, NONE, DEEP, NF1),
	/* D7  : ISH_I2C1_SDA ==> SPKR_IRQ_L */
	PAD_CFG_GPI_APIC(GPP_D7, NONE, PLTRST),
	/* D8  : ISH_I2C1_SCL ==> EN_CAMERA_PWR */
	PAD_CFG_GPO(GPP_D8, 0, DEEP),
	/* D9  : ISH_SPI_CS# ==> ISH_SPI_CS_L */
	PAD_CFG_NF_1V8(GPP_D9, NONE, DEEP, NF1),
	/* D10 : ISH_SPI_CLK ==> ISH_SPI_CLK */
	PAD_CFG_NF_1V8(GPP_D10, NONE, DEEP, NF1),
	/* D11 : ISH_SPI_MISO ==> ISH_SPI_MISO */
	PAD_CFG_NF_1V8(GPP_D11, NONE, DEEP, NF1),
	/* D12 : ISH_SPI_MOSI ==> ISH_SPI_MOSI */
	PAD_CFG_NF_1V8(GPP_D12, NONE, DEEP, NF1),
	/* D15 : ISH_UART0_RTS# ==> NC */
	PAD_CFG_NC(GPP_D15),
	/* D16 : ISH_UART0_CTS# ==> NC */
	PAD_CFG_NC(GPP_D16),
	/* D17 : DMIC_CLK1 ==> PCH_CAMERA_RESET */
	PAD_CFG_GPO(GPP_D17, 0, DEEP),
	/* D18 : DMIC_DATA1 ==> PCH_CAMERA_CLOCK_ENABLE */
	PAD_CFG_GPO(GPP_D18, 0, DEEP),
	/* D19 : DMIC_CLK0 ==> PCH_DMIC_CLK_OUT */
	PAD_CFG_NF(GPP_D19, NONE, DEEP, NF1),
	/* D20 : DMIC_DATA0 ==> PCH_DMIC_DATA_IN */
	PAD_CFG_NF(GPP_D20, NONE, DEEP, NF1),
	/* D21 : SPI1_IO2 ==> NC */
	PAD_CFG_NC(GPP_D21),
	/* D22 : SPI1_IO3 ==> NC */
	PAD_CFG_NC(GPP_D22),
	/* D23 : I2S_MCLK ==> I2S_MCLK_R */
	PAD_CFG_NF(GPP_D23, NONE, DEEP, NF1),

	/* E0  : SATAXPCI0 ==> CHP3_HAVEN_INT_L */
	PAD_CFG_GPI_APIC_INVERT(GPP_E0, NONE, PLTRST),
	/* E1  : SATAXPCIE1 ==> NC */
	PAD_CFG_NC(GPP_E1),
	/* E2  : SATAXPCIE2 ==> BT_DISABLE_L */
	PAD_CFG_GPO(GPP_E2, 1, DEEP),
	/* E3  : CPU_GP0 ==> NC */
	PAD_CFG_NC(GPP_E3),
	/* E4  : SATA_DEVSLP0 ==> NC */
	PAD_CFG_NC(GPP_E4),
	/* E5  : SATA_DEVSLP1 ==> NC */
	PAD_CFG_NC(GPP_E5),
	/* E6  : SATA_DEVSLP2 ==> DISPLAY_DCR_EN */
	PAD_CFG_GPO(GPP_E6, 1, DEEP),
	/* E7  : CPU_GP1 ==> TOUCHSCREEN_INT_L */
	PAD_CFG_GPI_APIC(GPP_E7, NONE, PLTRST),
	/* E8  : SATALED# ==> NC */
	PAD_CFG_NC(GPP_E8),
	/* E9  : USB2_OCO# ==> USB_C0_OC_ODL */
	PAD_CFG_NF(GPP_E9, NONE, DEEP, NF1),
	/* E10 : USB2_OC1# ==> USB_C1_OC_ODL */
	PAD_CFG_NF(GPP_E10, NONE, DEEP, NF1),
	/* E11 : USB2_OC2# ==> TOUCHSCREEN_RESET_L */
	PAD_CFG_GPO(GPP_E11, 0, DEEP),
	/* E12 : USB2_OC3# ==> NC */
	PAD_CFG_NC(GPP_E12),
	/* E13 : DDPB_HPD0 ==> USB_C1_DP_HPD */
	PAD_CFG_NF(GPP_E13, 20K_PD, DEEP, NF1),
	/* E14 : DDPC_HPD1 ==> USB_C0_DP_HPD */
	PAD_CFG_NF(GPP_E14, 20K_PD, DEEP, NF1),
	/* E15 : DDPD_HPD2 ==> EN_PP3300_DX_WLAN */
	PAD_CFG_GPO(GPP_E15, 1, DEEP),
	/* E16 : DDPE_HPD3 ==> NC */
	PAD_CFG_NC(GPP_E16),
	/* E17 : EDP_HPD */
	PAD_CFG_NF(GPP_E17, NONE, DEEP, NF1),
	/* E18 : DDPB_CTRLCLK ==> NC */
	PAD_CFG_NC(GPP_E18),
	/* E19 : DDPB_CTRLDATA ==> NC */
	PAD_CFG_NC(GPP_E19),
	/* E20 : DDPC_CTRLCLK ==> NC */
	PAD_CFG_NC(GPP_E20),
	/* E21 : DDPC_CTRLDATA ==> NC */
	PAD_CFG_NC(GPP_E21),
	/* E22 : DDPD_CTRLCLK ==> TRACKPAD_SHDN_L */
	PAD_CFG_GPO(GPP_E22, 1, DEEP),
	/* E23 : DDPD_CTRLDATA ==> NC */
	PAD_CFG_NC(GPP_E23),

	/* F0  : I2S2_SCLK ==> BOOT_BEEP_BCLK */
	PAD_CFG_GPI_GPIO_DRIVER(GPP_F0, NONE, DEEP),
	/* F1  : I2S2_SFRM ==> BOOT_BEEP_BUFFER_OE */
	PAD_CFG_GPO(GPP_F1, 1, DEEP),
	/* F2  : I2S2_TXD ==> BOOT_BEEP_LRCLK */
	PAD_CFG_GPI_GPIO_DRIVER(GPP_F2, NONE, DEEP),
	/* F3  : I2S2_RXD ==> NC */
	PAD_CFG_NC(GPP_F3),
	/* F4  : I2C2_SDA ==> PCH_I2C2_TRACKPAD_1V8_SDA */
	PAD_CFG_NF_1V8(GPP_F4, NONE, DEEP, NF1),
	/* F5  : I2C2_SCL ==> PCH_I2C2_TRACKPAD_1V8_SCL */
	PAD_CFG_NF_1V8(GPP_F5, NONE, DEEP, NF1),
	/* F6  : I2C3_SDA ==> PCH_I2C3_CAMERA_1V8_SDA */
	PAD_CFG_NF_1V8(GPP_F6, NONE, DEEP, NF1),
	/* F7  : I2C3_SCL ==> PCH_I2C3_CAMERA_1V8_SCL */
	PAD_CFG_NF_1V8(GPP_F7, NONE, DEEP, NF1),
	/* F8  : I2C4_SDA ==> PCH_I2C4_AUDIO_1V8_SDA */
	PAD_CFG_NF_1V8(GPP_F8, NONE, DEEP, NF1),
	/* F9  : I2C4_SCL ==> PCH_I2C4_AUDIO_1V8_SCL */
	PAD_CFG_NF_1V8(GPP_F9, NONE, DEEP, NF1),
	/* F10 : I2C5_SDA ==> HP_IRQ_GPIO */
	PAD_CFG_GPI_APIC(GPP_F10, 20K_PU, PLTRST),
	/* F11 : I2C5_SCL ==> SPKR_RST_L */
	PAD_CFG_GPO(GPP_F11, 1, PLTRST),
	/* F12 : EMMC_CMD */
	PAD_CFG_NF(GPP_F12, NONE, DEEP, NF1),
	/* F13 : EMMC_DATA0 */
	PAD_CFG_NF(GPP_F13, NONE, DEEP, NF1),
	/* F14 : EMMC_DATA1 */
	PAD_CFG_NF(GPP_F14, NONE, DEEP, NF1),
	/* F15 : EMMC_DATA2 */
	PAD_CFG_NF(GPP_F15, NONE, DEEP, NF1),
	/* F16 : EMMC_DATA3 */
	PAD_CFG_NF(GPP_F16, NONE, DEEP, NF1),
	/* F17 : EMMC_DATA4 */
	PAD_CFG_NF(GPP_F17, NONE, DEEP, NF1),
	/* F18 : EMMC_DATA5 */
	PAD_CFG_NF(GPP_F18, NONE, DEEP, NF1),
	/* F19 : EMMC_DATA6 */
	PAD_CFG_NF(GPP_F19, NONE, DEEP, NF1),
	/* F20 : EMMC_DATA7 */
	PAD_CFG_NF(GPP_F20, NONE, DEEP, NF1),
	/* F21 : EMMC_RCLK */
	PAD_CFG_NF(GPP_F21, NONE, DEEP, NF1),
	/* F22 : EMMC_CLK */
	PAD_CFG_NF(GPP_F22, NONE, DEEP, NF1),
	/* F23 : RSVD ==> NC */
	PAD_CFG_NC(GPP_F23),

	/* G0  : SD_CMD */
	PAD_CFG_NF(GPP_G0, NONE, DEEP, NF1),
	/* G1  : SD_DATA0 */
	PAD_CFG_NF(GPP_G1, NONE, DEEP, NF1),
	/* G2  : SD_DATA1 */
	PAD_CFG_NF(GPP_G2, NONE, DEEP, NF1),
	/* G3  : SD_DATA2 */
	PAD_CFG_NF(GPP_G3, NONE, DEEP, NF1),
	/* G4  : SD_DATA3 */
	PAD_CFG_NF(GPP_G4, NONE, DEEP, NF1),
	/* G5  : SD_CD# */
	PAD_CFG_NF(GPP_G5, NONE, DEEP, NF1),
	/* G6  : SD_CLK */
	PAD_CFG_NF(GPP_G6, NONE, DEEP, NF1),
	/* G7  : SD_WP */
	PAD_CFG_NF(GPP_G7, NONE, DEEP, NF1),

	/* GPD0: BATLOW# ==> PCH_BATLOW_L */
	PAD_CFG_NF(GPD0, NONE, DEEP, NF1),
	/* GPD1: ACPRESENT ==> EC_PCH_ACPRESENT */
	PAD_CFG_NF(GPD1, NONE, DEEP, NF1),
	/* GPD2: LAN_WAKE# ==> EC_PCH_WAKE_R_L */
	PAD_CFG_NF(GPD2, NONE, DEEP, NF1),
	/* GPD3: PWRBTN# ==> PCH_PWR_BTN_L */
	PAD_CFG_NF(GPD3, 20K_PU, DEEP, NF1),
	/* GPD4: SLP_S3# ==> SLP_S3_L */
	PAD_CFG_NF(GPD4, NONE, DEEP, NF1),
	/* GPD5: SLP_S4# ==> SLP_S4_L */
	PAD_CFG_NF(GPD5, NONE, DEEP, NF1),
	/* GPD6: SLP_A# ==> NC */
	PAD_CFG_NC(GPD6),
	/* GPD7: RSVD ==> NC */
	PAD_CFG_NC(GPD7),
	/* GPD8: SUSCLK ==> PCH_SUSCLK */
	PAD_CFG_NF(GPD8, NONE, DEEP, NF1),
	/* GPD9: SLP_WLAN# ==> NC */
	PAD_CFG_NC(GPD9),
	/* GPD10: SLP_S5# ==> NC */
	PAD_CFG_NC(GPD10),
	/* GPD11: LANPHYC ==> NC */
	PAD_CFG_NC(GPD11),
};

/* Early pad configuration in bootblock */
static const struct pad_config early_gpio_table[] = {
	/* B8 : SRCCLKREQ3# ==> WLAN_PE_RST */
	PAD_CFG_GPO(GPP_B8, 0, RSMRST),

	/* B15 : GSPI0_CS# ==> H1_SLAVE_SPI_CS_L */
	PAD_CFG_NF(GPP_B15, NONE, DEEP, NF1),
	/* B16 : GSPI0_CLK ==> H1_SLAVE_SPI_CLK */
	PAD_CFG_NF(GPP_B16, NONE, DEEP, NF1),
	/* B17 : GSPI0_MISO ==> H1_SLAVE_SPI_MISO */
	PAD_CFG_NF(GPP_B17, NONE, DEEP, NF1),
	/* B18 : GSPI0_MOSI ==> H1_SLAVE_SPI_MOSI */
	PAD_CFG_NF(GPP_B18, NONE, DEEP, NF1),

	/* Ensure UART pins are in native mode for H1. */
	/* C20 : UART2_RXD ==> PCHRX_SERVOTX_UART */
	PAD_CFG_NF(GPP_C20, NONE, DEEP, NF1),
	/* C21 : UART2_TXD ==> PCHTX_SERVORX_UART */
	PAD_CFG_NF(GPP_C21, NONE, DEEP, NF1),

	/* C23 : UART2_CTS# ==> PCH_WP */
	PAD_CFG_GPI_GPIO_DRIVER(GPP_C23, 20K_PU, DEEP),

	/* E0 : SATAXPCI0 ==> H1_PCH_INT_ODL */
	PAD_CFG_GPI_APIC_INVERT(GPP_E0, NONE, PLTRST),
};

const struct pad_config *variant_gpio_table(size_t *num)
{
	*num = ARRAY_SIZE(gpio_table);
	return gpio_table;
}

const struct pad_config *variant_early_gpio_table(size_t *num)
{
	*num = ARRAY_SIZE(early_gpio_table);
	return early_gpio_table;
}

static const struct pad_config ish_enabled_gpio_table[] = {
	/* A19 : ISH_GP1 ==> TRACKPAD_INT_L
	 * trackpad interrupt to ISH
	 */
	PAD_CFG_NF(GPP_A19, NONE, DEEP, NF1),
	/* A20 : ISH_GP2 ==> ISH_UART0_RXD
	 * ISH_UART0_RXD signal goes to this ISH GPIO pin.
	 * It is used as wake up source in ISH firmware.
	 * Implementation is in ISH firmware also.
	 */
	PAD_CFG_NF(GPP_A20, NONE, DEEP, NF1),

	/* D13 : ISH_UART0_RXD ==> ISH_UART0_RXD */
	PAD_CFG_NF_1V8(GPP_D13, NONE, DEEP, NF1),
	/* D14 : ISH_UART0_TXD ==> ISH_UART0_TXD */
	PAD_CFG_NF_1V8(GPP_D14, NONE, DEEP, NF1),
};


static const struct pad_config ish_disabled_gpio_table[] = {
	/* A19 : GPP_A19 ==> TRACKPAD_INT_L
	 * trackpad interrupt to PCH
	 */
	PAD_CFG_GPI_APIC(GPP_A19, NONE, PLTRST),
	/* A20 : ISH_GP2 ==> NC */
	PAD_CFG_NC(GPP_A20),

	/* D13 : ISH_UART0_RXD ==> NC */
	PAD_CFG_NC(GPP_D13),
	/* D14 : ISH_UART0_TXD ==> NC */
	PAD_CFG_NC(GPP_D14),
};

const struct pad_config *variant_sku_gpio_table(size_t *num)
{
	const struct pad_config *board_gpio_tables;
	const struct device *dev = pcidev_path_on_root(PCH_DEVFN_ISH);
	if (dev && dev->enabled) {
		*num = ARRAY_SIZE(ish_enabled_gpio_table);
		board_gpio_tables = ish_enabled_gpio_table;
	} else {
		*num = ARRAY_SIZE(ish_disabled_gpio_table);
		board_gpio_tables = ish_disabled_gpio_table;
	}
	return board_gpio_tables;
}
