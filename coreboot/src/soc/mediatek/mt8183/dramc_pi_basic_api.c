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

#include <arch/io.h>
#include <delay.h>
#include <soc/emi.h>
#include <soc/spm.h>
#include <soc/dramc_register.h>
#include <soc/dramc_pi_api.h>

static void sw_imp_cal_vref_sel(u8 term_option, u8 impcal_stage)
{
	u8 vref_sel = 0;

	if (term_option == 1)
		vref_sel = IMP_LP4X_TERM_VREF_SEL;
	else {
		switch (impcal_stage) {
		case IMPCAL_STAGE_DRVP:
			vref_sel = IMP_DRVP_LP4X_UNTERM_VREF_SEL;
			break;
		case IMPCAL_STAGE_DRVN:
			vref_sel = IMP_DRVN_LP4X_UNTERM_VREF_SEL;
			break;
		default:
			vref_sel = IMP_TRACK_LP4X_UNTERM_VREF_SEL;
			break;
		}
	}

	clrsetbits_le32(&ch[0].phy.shu[0].ca_cmd[11], 0x3f << 8, vref_sel << 8);
}

void dramc_sw_impedance(const struct sdram_params *params)
{
	u8 term = 0, ca_term = ODT_OFF, dq_term = ODT_ON;
	u32 sw_impedance[2][4] = {0};

	for (term = 0; term < 2; term++)
		for (u8 i = 0; i < 4; i++)
			sw_impedance[term][i] = params->impedance[term][i];

	sw_impedance[ODT_OFF][2] = sw_impedance[ODT_ON][2];
	sw_impedance[ODT_OFF][3] = sw_impedance[ODT_ON][3];

	clrsetbits_le32(&ch[0].phy.shu[0].ca_cmd[11], 0xff, 0x3);
	sw_imp_cal_vref_sel(dq_term, IMPCAL_STAGE_DRVP);

	/* DQ */
	clrsetbits_le32(&ch[0].ao.shu[0].drving[0], (0x1f << 5) | (0x1f << 0),
		(sw_impedance[dq_term][0] << 5) |
		(sw_impedance[dq_term][1] << 0));
	clrsetbits_le32(&ch[0].ao.shu[0].drving[1],
		(0x1f << 25)|(0x1f << 20) | (1 << 31),
		(sw_impedance[dq_term][0] << 25) |
		(sw_impedance[dq_term][1] << 20) | (!dq_term << 31));
	clrsetbits_le32(&ch[0].ao.shu[0].drving[2], (0x1f << 5) | (0x1f << 0),
		(sw_impedance[dq_term][2] << 5) |
		(sw_impedance[dq_term][3] << 0));
	clrsetbits_le32(&ch[0].ao.shu[0].drving[3], (0x1f << 25) | (0x1f << 20),
		(sw_impedance[dq_term][2] << 25) |
		(sw_impedance[dq_term][3] << 20));

	/* DQS */
	for (u8 i = 0; i <= 2; i += 2) {
		clrsetbits_le32(&ch[0].ao.shu[0].drving[i],
			(0x1f << 25) | (0x1f << 20),
			(sw_impedance[dq_term][i] << 25) |
			(sw_impedance[dq_term][i + 1] << 20));
		clrsetbits_le32(&ch[0].ao.shu[0].drving[i],
			(0x1f << 15) | (0x1f << 10),
			(sw_impedance[dq_term][i] << 15) |
			(sw_impedance[dq_term][i + 1] << 10));
	}

	/* CMD & CLK */
	for (u8 i = 1; i <= 3; i += 2) {
		clrsetbits_le32(&ch[0].ao.shu[0].drving[i],
			(0x1f << 15) | (0x1f << 10),
			(sw_impedance[ca_term][i - 1] << 15) |
			(sw_impedance[ca_term][i] << 10));
		clrsetbits_le32(&ch[0].ao.shu[0].drving[i],
			(0x1f << 5) | (0x1f << 0),
			(sw_impedance[ca_term][i - 1] << 5) |
			(sw_impedance[ca_term][i] << 0));
	}

	clrsetbits_le32(&ch[0].phy.shu[0].ca_cmd[11], 0x1f << 17,
		sw_impedance[ca_term][0] << 17);
	clrsetbits_le32(&ch[0].phy.shu[0].ca_cmd[11], 0x1f << 22,
		sw_impedance[ca_term][1] << 22);

	clrsetbits_le32(&ch[0].phy.shu[0].ca_cmd[3],
		SHU1_CA_CMD3_RG_TX_ARCMD_PU_PRE_MASK,
		1 << SHU1_CA_CMD3_RG_TX_ARCMD_PU_PRE_SHIFT);
	clrbits_le32(&ch[0].phy.shu[0].ca_cmd[0],
		SHU1_CA_CMD0_RG_TX_ARCLK_DRVN_PRE_MASK);

	clrsetbits_le32(&ch[0].phy.shu[0].ca_dll[1], 0x1f << 16, 0x9 << 16);
}

static void transfer_pll_to_spm_control(void)
{
	u8 shu_lev = (read32(&ch[0].ao.shustatus) & 0x00000006) >> 1;

	clrsetbits_le32(&mtk_spm->poweron_config_set,
		(0xffff << 16) | (0x1 << 0),
		(0xb16 << 16) | (0x1 << 0));

	/* Set SPM pinmux */
	clrbits_le32(&mtk_spm->pcm_pwr_io_en, (0xff << 0) | (0xff << 16));
	setbits_le32(&mtk_spm->dramc_dpy_clk_sw_con_sel, 0xffffffff);
	setbits_le32(&mtk_spm->dramc_dpy_clk_sw_con_sel2, 0xffffffff);

	setbits_le32(&mtk_spm->spm_power_on_val0, (0x1 << 8) | (0xf << 12));
	setbits_le32(&mtk_spm->spm_s1_mode_ch, 0x3 << 0);

	shu_lev = (shu_lev == 1) ? 2 : 1;
	clrsetbits_le32(&mtk_spm->spm_power_on_val0, 0x3 << 28, shu_lev << 28);
	clrsetbits_le32(&mtk_spm->dramc_dpy_clk_sw_con2,
		0x3 << 2, shu_lev << 2);

	udelay(1);
	for (size_t chn = CHANNEL_A; chn < CHANNEL_MAX; chn++) {
		clrbits_le32(&ch[chn].phy.pll1, 0x1 << 31);
		clrbits_le32(&ch[chn].phy.pll2, 0x1 << 31);
	}
}

static void dramc_rx_input_delay_tracking(u8 chn)
{
	/* Enable RX_FIFO macro DIV4 clock CG */
	write32(&ch[chn].phy.misc_cg_ctrl1, 0xffffffff);

	/* DVS mode to RG mode */
	for (size_t r = 0; r < 2; r++)
		for (size_t b = 0; b < 2; b++)
			clrbits_le32(&ch[chn].phy.r[r].b[b].rxdvs[2], 3 << 30);

	clrsetbits_le32(&ch[chn].phy.b0_rxdvs[0], 0x1 << 19, 0x1 << 9);
	clrsetbits_le32(&ch[chn].phy.b1_rxdvs[0], 0x1 << 19, 0x1 << 9);

	for (size_t r = 0; r < 2; r++)
		for (size_t b = 0; b < 2; b++) {
			/* Track rising and update rising/falling together */
			clrbits_le32(&ch[chn].phy.r[r].b[b].rxdvs[2],
				0x1 << 29);
			clrsetbits_le32(&ch[chn].phy.r[r].b[b].rxdvs[7],
				(0x3f << 0) | (0x3f << 8) |
				(0x7f << 16) | (0x7f << 24),
				(0x0 << 0) | (0x3f << 8) |
				(0x0 << 16) | (0x7f << 24));
			clrsetbits_le32(&ch[chn].phy.r[r].b[b].rxdvs[1],
				(0xffff << 16) | (0xffff << 0),
				(0x2 << 16) | (0x2 << 0));

			/* DQ/DQS Rx DLY adjustment for tracking mode */
			clrbits_le32(&ch[chn].phy.r[r].b[b].rxdvs[2],
				(0x3 << 26) | (0x3 << 24) |
				(0x3 << 18) | (0x3 << 16));
		}

	clrbits_le32(&ch[chn].phy.ca_cmd[10], (0x7 << 28) | (0x7 << 24));

	/* Rx DLY tracking setting (Static) */
	clrsetbits_le32(&ch[chn].phy.b0_rxdvs[0],
		(0x1 << 29) | (0xf << 4) | (0x1 << 0),
		(0x1 << 29) | (0x0 << 4) | (0x1 << 0));
	clrsetbits_le32(&ch[chn].phy.b1_rxdvs[0],
		(0x1 << 29) | (0xf << 4) | (0x1 << 0),
		(0x1 << 29) | (0x0 << 4) | (0x1 << 0));

	for (u8 b = 0; b < 2; b++) {
		clrsetbits_le32(&ch[chn].phy.b[b].dq[9],
			(0x7 << 28) | (0x7 << 24),
			(0x1 << 28) | (0x0 << 24));
		setbits_le32(&ch[chn].phy.b[b].dq[5], 0x1 << 31);
	}

	setbits_le32(&ch[chn].phy.b0_rxdvs[0], (0x1 << 28) | (0x1 << 31));
	setbits_le32(&ch[chn].phy.b1_rxdvs[0], (0x1 << 28) | (0x1 << 31));
	for (u8 rank = RANK_0; rank < RANK_MAX; rank++)
		for (u8 b = 0; b < 2; b++)
			clrsetbits_le32(&ch[chn].phy.r[rank].b[b].rxdvs[2],
				(0x3 << 30) | (0x1 << 28) | (0x1 << 23),
				(0x2 << 30) | (0x1 << 28) | (0x1 << 23));

}

static void dramc_hw_dqs_gating_tracking(u8 chn)
{
	setbits_le32(&ch[chn].ao.stbcal, (0x3 << 26) | (0x1 << 0));
	clrsetbits_le32(&ch[chn].ao.stbcal1,
		(0xffff << 16) | (0x1 << 8) | (0x1 << 6),
		(0x1 << 16) | (0x1 << 8) | (0x0 << 6));

	clrsetbits_le32(&ch[chn].phy.misc_ctrl0,
		(0x1 << 24) | (0x1f << 11) | (0xf << 0),
		(0x1 << 24) | (0x0 << 11) | (0x0 << 0));

	clrbits_le32(&ch[chn].phy.b[0].dq[6], 0x1 << 31);
	clrbits_le32(&ch[chn].phy.b[1].dq[6], 0x1 << 31);
	clrbits_le32(&ch[chn].phy.ca_cmd[6], 0x1 << 31);
}

static void dramc_hw_gating_init(void)
{
	for (size_t chn = 0; chn < CHANNEL_MAX; chn++) {
		clrbits_le32(&ch[chn].ao.stbcal,
			(0x7 << 22) | (0x3 << 14) | (0x1 << 19) | (0x1 << 21));
		setbits_le32(&ch[chn].ao.stbcal, (0x1 << 20) | (0x3 << 28));
		setbits_le32(&ch[chn].phy.misc_ctrl1, 0x1 << 24);

		dramc_hw_dqs_gating_tracking(chn);
	}
}

static void dramc_impedance_tracking_enable(void)
{
	setbits_le32(&ch[0].phy.misc_ctrl0, 0x1 << 10);
	for (size_t chn = 0; chn < CHANNEL_MAX; chn++) {
		setbits_le32(&ch[chn].ao.impcal, (0x1 << 31) | (0x1 << 29) |
			(0x1 << 26) | (0x1 << 17) | (0x7 << 11));
		clrbits_le32(&ch[chn].ao.impcal, 0x1 << 30);
		setbits_le32(&ch[chn].phy.misc_ctrl0, 0x1 << 18);
		setbits_le32(&ch[chn].ao.impcal, 0x1 << 19);
	}
	setbits_le32(&ch[0].ao.impcal, 0x1 << 14);
	setbits_le32(&ch[1].ao.refctrl0, 0x1 << 2);
	for (size_t chn = 0; chn < CHANNEL_MAX; chn++)
		setbits_le32(&ch[chn].ao.refctrl0, 0x1 << 3);
}

static void dramc_phy_low_power_enable(void)
{
	u32 broadcast_bak = dramc_get_broadcast();
	dramc_set_broadcast(DRAMC_BROADCAST_OFF);

	for (size_t chn = 0; chn < CHANNEL_MAX; chn++) {
		for (size_t b = 0; b < 2; b++) {
			clrbits_le32(&ch[chn].phy.b[b].dll_fine_tune[2],
				0x3fffff << 10);
			write32(&ch[chn].phy.b[b].dll_fine_tune[3], 0x2e800);
		}
		clrsetbits_le32(&ch[chn].phy.ca_dll_fine_tune[2],
			0x3fffff << 10, 0x2 << 10);
	}
	write32(&ch[0].phy.ca_dll_fine_tune[3], 0xba000);
	write32(&ch[1].phy.ca_dll_fine_tune[3], 0x3a000);

	dramc_set_broadcast(broadcast_bak);
}
void dramc_runtime_config(void)
{
	clrbits_le32(&ch[0].ao.refctrl0, 0x1 << 29);
	clrbits_le32(&ch[1].ao.refctrl0, 0x1 << 29);

	transfer_pll_to_spm_control();
	setbits_le32(&mtk_spm->spm_power_on_val0, 0x3 << 25);

	for (u8 chn = 0; chn < CHANNEL_MAX; chn++)
		dramc_rx_input_delay_tracking(chn);

	dramc_hw_gating_init();
	dramc_hw_gating_onoff(CHANNEL_A, true);

	for (size_t chn = 0; chn < CHANNEL_MAX; chn++)
		clrbits_le32(&ch[chn].ao.stbcal2,
			(0x3 << 4) | (0x3 << 8) | (0x1 << 28));

	clrbits_le32(&ch[0].ao.spcmdctrl, 0x1 << 30);
	clrbits_le32(&ch[1].ao.spcmdctrl, 0x1 << 30);

	dramc_phy_low_power_enable();
	dramc_enable_phy_dcm(true);

	for (size_t chn = 0; chn < CHANNEL_MAX; chn++)
		for (size_t shu = 0; shu < DRAM_DFS_SHUFFLE_MAX; shu++)
			clrbits_le32(&ch[chn].ao.shu[shu].dqsg_retry,
				(0x1 << 1) | (0x3 << 13));

	write32(&ch[0].phy.misc_spm_ctrl0, 0xfbffefff);
	write32(&ch[1].phy.misc_spm_ctrl0, 0xfbffefff);
	write32(&ch[0].phy.misc_spm_ctrl2, 0xffffffef);
	write32(&ch[1].phy.misc_spm_ctrl2, 0x7fffffef);

	dramc_impedance_tracking_enable();

	for (size_t chn = 0; chn < CHANNEL_MAX; chn++) {
		clrbits_le32(&ch[chn].ao.spcmdctrl, 0x3 << 28);
		setbits_le32(&ch[chn].ao.hw_mrr_fun, (0x1 << 0) | (0x1 << 11));
		clrbits_le32(&ch[0].ao.refctrl0, 0x1 << 18);
		setbits_le32(&ch[chn].phy.dvfs_emi_clk, 0x1 << 24);
		setbits_le32(&ch[chn].ao.dvfsdll, 0x1 << 7);
	}
}
