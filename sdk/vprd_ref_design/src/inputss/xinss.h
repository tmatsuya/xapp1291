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
* This is header file for input subsystem of Xilinx Video Processing Reference
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

#ifndef XINSS_H		 /* prevent circular inclusions */
#define XINSS_H		 /* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

#include "xvidc.h"
#include "xv_hdmirxss.h"
#include "xvphy.h"

/************************** Constant Definitions *****************************/


/************************** Structure Definitions *****************************/
typedef enum {
  XINSS_SOURCE_TPG = 0,
  XINSS_SOURCE_HDMI_RX
}XInput_Sel;

typedef struct
{
  XV_HdmiRxSs *HdmiRxSsPtr;  /* handle for hdmi rx subsystem driver */
  XVphy       *VphyPtr;      /* handle for video phy subsystem driver */
} HdmiSource;

typedef struct
{
  HdmiSource  Hdmi;
  void        *SysIntcPtr;   /* pointer to system interrupt controller */

  XInput_Sel InputSrcSel;
  u8 InputLocked;
}XInss;

/***************** Macros (Inline Functions) Definitions *********************/

/*****************************************************************************/
/**
 * This macro check if Input Subsystem has locked to a valid stream
 *
 * @param  pointer to the input subsystem Input
 * @return none
 *
 *****************************************************************************/
#define XInss_IsStreamValid(pInss)              ((pInss)->InputLocked)

/*****************************************************************************/
/**
 * This macro sets Stream Mode Status (lock/!Lock)
 *
 * @param  pointer to the input subsystem
 * @param  val is mode (T/F)
 * @return none
 *
 *****************************************************************************/
#define XInss_SetStreamLockMode(pInss, val)     ((pInss)->InputLocked = val)

/*****************************************************************************/
/**
 * This macro returns the default input source
 *
 * @param  pointer to the input subsystem
 * @param
 * @return none
 *
 *****************************************************************************/
#define XInss_GetInputSourceSel(pInss)     ((pInss)->InputSrcSel)

/************************** Exported APIs ************************************/
int  XInss_PowerOnInit(XInss *InstancePtr);
void XInss_Start(XInss *InstancePtr);
void XInss_Stop(XInss *InstancePtr);
void XInss_Reset(XInss *InstancePtr);
void XInss_SetActiveSource(XInss *InstancePtr, XInput_Sel Source);
u32 XInss_GetStreamInfo(XInss *InstancePtr, XVidC_VideoStream *Stream);
void XInss_StartStream(XInss *InstancePtr);

void XInss_SetUserTimerHandler(XInss *InstancePtr,
		                       XVidC_DelayHandler CallbackFunc,
		                       void  *CallbackRef);

void XInss_RegisterSysIntc(XInss *InstancePtr, void *IntcPtr);


#ifdef __cplusplus
}
#endif

#endif /* end of protection macro */
