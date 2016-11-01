/******************************************************************************
*
* Copyright (C) 2016 Xilinx, Inc.  All rights reserved.
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
* @file system.h
*
* This is header for top level resource file that will initialize all system
* level peripherals
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who    Date     Changes
* ----- ---- -------- -------------------------------------------------------
* 1.00  rco   09/11/15   Initial Release
* 2.00  rco   01/28/16   Added APIs to set active input/output
*
* </pre>
*
******************************************************************************/
#ifndef XSYSTEM_H		 /* prevent circular inclusions */
#define XSYSTEM_H		 /* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

#include "res/periph.h"
#include "res/sleep.h"
#include "physs/physs.h"
#include "inputss/xinss.h"
#include "xvprocss.h"
#include "outputss/xoutss.h"

/************************** Constant Definitions *****************************/

/************************** Structure Definitions *****************************/

/************************** Exported APIs ************************************/
int XSys_Init(XPeriph  *pPeriph,
	   	      XInss    *pInpss,
		      XVprocSs *pVprocss,
		      XOutss   *pOutss,
		      XVphyss  *pVphyss);

int XSys_Start(XPeriph  *pPeriph,
	   	       XInss    *pInpss,
		       XVprocSs *pVprocss,
		       XOutss   *pOutss);

int XSys_Stop(XPeriph  *pPeriph,
	   	      XInss    *pInpss,
		      XVprocSs *pVprocss,
		      XOutss   *pOutss);

int XSys_Reset(XPeriph  *pPeriph,
	   	       XInss    *pInpss,
		       XVprocSs *pVprocss,
		       XOutss   *pOutss);

void XSys_SetActiveInput(XPeriph    *pPeriph,
		                 XInss      *pInpss,
						 XVprocSs   *pVprocss,
						 XOutss     *pOutss,
		                 XInput_Sel Source);

void XSys_SetActiveOutput(XPeriph     *pPeriph,
		                  XVprocSs    *pVprocss,
		                  XOutss      *pOutss,
		                  XOutput_Sel Sink);

void XSys_SetVpssToTpgResolution(XPeriph *pPeriph, XVprocSs *pVprocss);

void XSys_SetOutputResolution(XVprocSs *pVprocss,
		                      XOutss   *pOutss,
		                      XVidC_VideoMode NewVmId);

void XSys_ProcessDipSiwtch(XVprocSs *pVprocss,
		                   XOutss   *pOutss,
		                   u32 keypress);

void XSys_ReportSystemInfo(XPeriph  *pPeriph,
	   	                   XInss    *pInpss,
		                   XVprocSs *pVprocss,
		                   XOutss   *pOutss);

void XSys_ReportSystemStatus(XPeriph  *pPeriph,
		                     XInss    *pInss,
		                     XVprocSs *pVprocss,
		                     XVphyss   *pVphyss);

void XSys_ReportLinkStatus(XInss   *pInpss,
		                   XOutss  *pOutss,
		                   XVphyss *pVphyss);

void XSys_CriticalError(XPeriph  *pPeriph,
	   	                XInss    *pInpss,
		                XVprocSs *pVprocss,
		                XOutss   *pOutss);


#ifdef __cplusplus
}
#endif

#endif /* End of protection macro */
