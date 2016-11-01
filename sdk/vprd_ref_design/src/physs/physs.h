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
* This is header file for phy layer
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

#ifndef XPHYSS_H		 /* prevent circular inclusions */
#define XPHYSS_H		 /* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

#include "xvphy.h"
#include "../inputss/xinss.h"
#include "../outputss/xoutss.h"

/************************** Constant Definitions *****************************/


/************************** Structure Definitions *****************************/
typedef struct
{
  HdmiSource *HdmiRx;
  HdmiSink   *HdmiTx;
  void       *SysIntcPtr;   /* pointer to system interrupt controller */

  u32 outputLocked;
}XVphyss;

/***************** Macros (Inline Functions) Definitions *********************/
/*****************************************************************************/
/**
 * This macro check if Output Subsystem is connected to a display
 *
 * @param  pointer to the output subsystem
 * @return none
 *
 *****************************************************************************/
#define XVphyss_IsPanelConnected(pVphyss)              ((pVphyss)->outputLocked)

/*****************************************************************************/
/**
 * This macro sets the panel connection mode (lock/!Lock)
 *
 * @param  pointer to the output subsystem
 * @param  val is mode (T/F)
 * @return none
 *
 *****************************************************************************/
#define XVphyss_SetSPanelLockMode(pVphyss, val) ((pVphyss)->outputLocked = val)


/************************** Exported APIs ************************************/
int  XVPhyss_PowerOnInit(XVphyss *InstancePtr);
void XVphyss_RegisterSysIntc(XVphyss *InstancePtr, void *IntcPtr);

#ifdef __cplusplus
}
#endif

#endif /* end of protection macro */
