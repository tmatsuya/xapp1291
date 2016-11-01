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
* @file main.c
*
* This is header file for output subsystem of Xilinx Video Processing Reference
* Design.
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

#ifndef XOUTSS_H		 /* prevent circular inclusions */
#define XOUTSS_H		 /* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

#include "xvidc.h"
#include "xv_hdmitxss.h"
#include "xvphy.h"
#include "xv_mix_l2.h"

#include "../res/periph.h"

#define XVMIX_DDR_MAX_FRAMES       (600)

/************************** Constant Definitions *****************************/
typedef enum {
  XOUTSS_SINK_HDMI_TX = 0
}XOutput_Sel;

/************************** Structure Definitions *****************************/
typedef struct {
  u16 FrameWidth;
  u16 FrameHeight;
  XVidC_VideoWindow Win;
  u16 NumFrames;
  u32 MemAddr[XVMIX_DDR_MAX_FRAMES];
  XVidC_ColorFormat CFmt;
  volatile u16 HeadPos;
}FrameBuf;

typedef struct
{
  XV_HdmiTxSs *HdmiTxSsPtr; /* handle for hdmi tx subsystem driver */
  XVphy       *VphyPtr;     /* handle for video phy subsystem driver */
} HdmiSink;

typedef struct
{
  HdmiSink   Hdmi;
  void       *SysIntcPtr;  /* pointer to system interrupt controller */
  XV_Mix_l2  *MixerPtr;
  XGpio      *MixerResetPtr;

  FrameBuf MixerFrameBuf[7];
  XOutput_Sel OutputSinkSel;
  u32 vsyncEvntPend;
}XOutss;

/***************** Macros (Inline Functions) Definitions *********************/

/*****************************************************************************/
/*****************************************************************************/
/**
 * This macro sets the default output source
 *
 * @param  pOutss is pointer to the output subsystem
 * @param  val is the active sink to be set
 * @return none
 *
 *****************************************************************************/
#define XOutss_SetOutputSinkSel(pOutss, val)   ((pOutss)->OutputSinkSel = val)

/*****************************************************************************/
/**
 * This macro returns the current selected output sink
 *
 * @param  pOutss is pointer to the output subsystem
 * @param
 * @return none
 *
 *****************************************************************************/
#define XOutss_GetActiveSink(pOutss)                 ((pOutss)->OutputSinkSel)

/*****************************************************************************/
/**
 * This macro sets the VSync Event Pending state (T/F)
 *
 * @param  pOutss is pointer to the output subsystem
 * @param  val is mode (T/F)
 * @return none
 *
 *****************************************************************************/
#define XOutss_SetVSyncEvntPend(pOutss, val)   ((pOutss)->vsyncEvntPend = val)

/*****************************************************************************/
/**
 * This macro check if VSync Event has been scheduled
 *
 * @param  pOutss is pointer to the output subsystem
 * @return none
 *
 *****************************************************************************/
#define XOutss_IsVsyncEvntPend(pOutss)              ((pOutss)->vsyncEvntPend)


/************************** Exported APIs ************************************/
int  XOutss_PowerOnInit(XOutss *InstancePtr);
void XOutss_Start(XOutss *InstancePtr);
void XOutss_Stop(XOutss *InstancePtr);
void XOutss_Reset(XOutss *InstancePtr);
void XOutss_SetActiveSink(XOutss *InstancePtr, XOutput_Sel Sink);
int XOutss_SetOutputStream(XOutss *InstancePtr, XVidC_VideoStream *strmOut);
void XOutss_VideoMute(XOutss *InstancePtr, u32 Enable);
void XOutss_SetupMixer(XOutss *InstancePtr, XVidC_VideoStream *StrmOut);
void XOutss_UpdateMixerWindow(XOutss *InstancePtr,
		                      XVidC_VideoStream *StrmOut,
					          u32 PipModeOn);
void XOutss_SetUserTimerHandler(XOutss *InstancePtr,
		                        XVidC_DelayHandler CallbackFunc,
		                        void *CallbackRef);

void XOutss_RegisterSysIntc(XOutss *InstancePtr, void *IntcPtr);

#ifdef __cplusplus
}
#endif

#endif /* end of protection macro */


