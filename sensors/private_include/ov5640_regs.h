/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2011-2013 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2014-2017 Mentor Graphics Inc.
 * 
 * OV5640 register definitions.
 */
#ifndef __REG_REGS_H__
#define __REG_REGS_H__

/* min/typical/max system clock (xclk) frequencies */
#define OV5640_XCLK_MIN  6000000
#define OV5640_XCLK_MAX 54000000

#define OV5640_DEFAULT_SLAVE_ID 0x3c

#define SYS_RESET02		0x3002
#define SYS_CLOCK_ENABLE02	0x3006
#define SYSTEM_CTROL0		0x3008
#define CHIP_ID		0x300a
#define IO_MIPI_CTRL00	0x300e
#define PAD_OUTPUT_ENABLE01	0x3017
#define PAD_OUTPUT_ENABLE02	0x3018
#define PAD_OUTPUT00		0x3019
#define SYSTEM_CONTROL1	0x302e
#define SC_PLL_CTRL0		0x3034
#define SC_PLL_CTRL1		0x3035
#define SC_PLL_CTRL2		0x3036
#define SC_PLL_CTRL3		0x3037
#define SLAVE_ID		0x3100
#define SCCB_SYS_CTRL1	0x3103
#define SYS_ROOT_DIVIDER	0x3108
#define AWB_R_GAIN		0x3400
#define AWB_G_GAIN		0x3402
#define AWB_B_GAIN		0x3404
#define AWB_MANUAL_CTRL	0x3406
#define AEC_PK_EXPOSURE_HI	0x3500
#define AEC_PK_EXPOSURE_MED	0x3501
#define AEC_PK_EXPOSURE_LO	0x3502
#define AEC_PK_MANUAL	0x3503
#define AEC_PK_REAL_GAIN	0x350a
#define AEC_PK_VTS		0x350c
#define TIMING_DVPHO		0x3808
#define TIMING_DVPVO		0x380a
#define TIMING_HTS		0x380c
#define TIMING_VTS		0x380e
#define TIMING_TC_REG20	0x3820
#define TIMING_TC_REG21	0x3821
#define AEC_CTRL00		0x3a00
#define AEC_B50_STEP		0x3a08
#define AEC_B60_STEP		0x3a0a
#define AEC_CTRL0D		0x3a0d
#define AEC_CTRL0E		0x3a0e
#define AEC_CTRL0F		0x3a0f
#define AEC_CTRL10		0x3a10
#define AEC_CTRL11		0x3a11
#define AEC_CTRL1B		0x3a1b
#define AEC_CTRL1E		0x3a1e
#define AEC_CTRL1F		0x3a1f
#define HZ5060_CTRL00	0x3c00
#define HZ5060_CTRL01	0x3c01
#define SIGMADELTA_CTRL0C	0x3c0c
#define FRAME_CTRL01		0x4202
#define FORMAT_CTRL00	0x4300
#define VFIFO_HSIZE		0x4602
#define VFIFO_VSIZE		0x4604
#define JPG_MODE_SELECT	0x4713
#define CLOCK_POL_CONTROL	0x4740
#define MIPI_CTRL00		0x4800
#define DEBUG_MODE		0x4814
#define FORMAT_CTRL	0x501f
#define PRE_ISP_TEST_SETTING_1	0x503d
#define SDE_CTRL0		0x5580
#define SDE_CTRL1		0x5581
#define SDE_CTRL3		0x5583
#define SDE_CTRL4		0x5584
#define SDE_CTRL5		0x5585
#define AVG_READOUT		0x56a1

#endif //__REG_REGS_H__