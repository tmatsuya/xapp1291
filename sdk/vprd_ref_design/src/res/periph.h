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
* @file periph.h
*
* This is header for resource file that will initialize all system
* level peripherals
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who    Date     Changes
* ----- ---- -------- -------------------------------------------------------
* 1.00  rco   09/11/15   Initial Release
*
* </pre>
*
******************************************************************************/
#ifndef XPERIPH_H		 /* prevent circular inclusions */
#define XPERIPH_H		 /* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

#include "xvidc.h"
#include "xgpio.h"
#include "xtmrctr.h"
#include "xv_tpg.h"
#include "xaxis_switch.h"

#if defined(__arm__)
#include "xscugic.h"
#include "xuartps_hw.h"
#else
#include "xintc.h"
#include "xuartlite_l.h"
#endif

/************************** Constant Definitions *****************************/

/* DIP Switch ReadBack */
typedef enum
{
   MOVE_WIN_RIGHT = 0x08,
   MOVE_WIN_LEFT  = 0x02,
   MOVE_WIN_UP    = 0x10,
   MOVE_WIN_DN    = 0x04,
   ZP_MODE_OFF    = 0x01,
}XPeriphUserDIPSel;

/* Active Input */
typedef enum
{
  XPERIPH_INPUT_MUX_TPG = 0,
  XPERIPH_INPUT_MUX_HDMI
}XPeriphInputMux;

/* Active Output */
typedef enum
{
  XPERIPH_OUTPUT_MUX_HDMI = 0
}XPeriphOutputMux;

/************************** Structure Definitions *****************************/
typedef struct
{
  XVidC_VideoMode TpgVidMode;
  XVidC_ColorFormat TpgColorFmt;
  u8 TpgSelPattrn;
  u8 ActiveOutput;
}XPeriph_IData;

/**
 * System Peripheral configuration structure.
 * Each device should have a configuration structure associated
 */
typedef struct
{
  void    *IntcPtr;       //Interrupt Controller
  XGpio   *GpioPtr;       //DIP Switch support
  XV_tpg  *TpgPtr;
  XTmrCtr *TimerPtr;    //Timer for delay
  XAxis_Switch *InputMuxPtr;
  u32 UartBaseAddr;
  XPeriph_IData UsrSet;
}XPeriph;

/************************** Macros Definitions *******************************/


/*****************************************************************************/
/**
 * This macro returns the system active output flag
 *
 * @param  pPeriph is pointer to the peripheral Instance
 * @return none
 *
 *****************************************************************************/
#define XPeriph_GetActiveOutput(pPeriph)      ((pPeriph)->UsrSet.ActiveOutput)

/*****************************************************************************/
/**
 * This macro sets the TPG color format
 *
 * @param  pPeriph is pointer to the peripheral Instance
 * @param  ColorFormat  is the new color format
 * @return none
 *
 *****************************************************************************/
#define XPeriph_SetTPGColorFormat(pPeriph, ColorFormat) \
	                      ((pPeriph)->UsrSet.TpgColorFmt = ColorFormat)


/************************** Exported APIs ************************************/
int XPeriph_PowerOnInit(XPeriph *InstancePtr);
void XPeriph_Start(XPeriph *InstancePtr);
void XPeriph_Stop(XPeriph *InstancePtr);
void XPeriph_Reset(XPeriph *InstancePtr);
void XPeriph_ReportDeviceInfo(XPeriph *InstancePtr);
void XPeriph_ConfigTpg(XPeriph *InstancePtr);
void XPeriph_DisableTpg(XPeriph *InstancePtr);
void XPeriph_TpgDbgReportStatus(XPeriph *InstancePtr);
void XPeriph_SetInputMux(XPeriph *InstancePtr, XPeriphInputMux MuxSel);
void XPeriph_SetOutputMux(XPeriph *InstancePtr, XPeriphOutputMux MuxSel);
void XPeriph_EnableAllInterrupts(void *IntcPtr);
void XPeriph_DisableAllInterrupts(void *IntcPtr);

//User Interface APIs
u32 XPeriph_ReadDIPSwitch(XPeriph *InstancePtr);
u8  XPeriph_ReadUART(XPeriph *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif /* End of protection macro */
