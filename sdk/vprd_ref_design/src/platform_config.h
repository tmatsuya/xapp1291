/******************************************************************************
*
* Copyright (C) 2015 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/
/*****************************************************************************/
/**
*
* @file platform_config.h
*
* This is header for configuring software to specified hw platform
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who    Date     Changes
* ----- ---- -------- -------------------------------------------------------
* 1.00  rco   09/21/15   Initial Release
*
* </pre>
*
******************************************************************************/

#ifndef __PLATFORM_CONFIG_H_
#define __PLATFORM_CONFIG_H_

/* Xilinx device series Supported */
#define XILINX_DEVICE_ARTIX 		0
#define XILINX_DEVICE_KINTEX		1
#define XILINX_DEVICE_ULTRASCALE 	2
#define XILINX_DEVICE_ZYNQ			3

#define XIL_SIZE_ONE_MB             (1024*1024UL)
#define XIL_VPRD_APP_SIZE           (10  * XIL_SIZE_ONE_MB)
#define XIL_VIDEO_BUF_SIZE          (304 * XIL_SIZE_ONE_MB)
#define XIL_MIXER_LAYER_SIZE        (4   * XIL_SIZE_ONE_MB)

#define XILINX_DEVICE_SELECTED      XILINX_DEVICE_KINTEX

#if (XILINX_DEVICE_SELECTED == XILINX_DEVICE_ZYNQ)
#define USR_FRAME_BUF_BASEADDR     (0x80000000 + XIL_VPRD_APP_SIZE)
#elif (XILINX_DEVICE_SELECTED == XILINX_DEVICE_ULTRASCALE)
#define USR_FRAME_BUF_BASEADDR     (XPAR_MEMORY_SS_DDR4_0_BASEADDR + XIL_VPRD_APP_SIZE)
#else
#define USR_FRAME_BUF_BASEADDR     (XPAR_MIG7SERIES_0_BASEADDR + XIL_VPRD_APP_SIZE)
#endif

/* Memory Layers for Mixer */
#define XVMIX_LAYER1_BASEADDR      (USR_FRAME_BUF_BASEADDR + XIL_VIDEO_BUF_SIZE)
#define XVMIX_LAYER_ADDR_OFFSET    (XIL_MIXER_LAYER_SIZE)
#endif


