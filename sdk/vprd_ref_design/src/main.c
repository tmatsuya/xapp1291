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
* @file main.c
*
* This is main loop of Xilinx Video Processing Reference Design.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who    Date     Changes
* ----- ---- -------- -------------------------------------------------------
* 1.00  rco   09/11/15   Initial Release
* 2.00  rco   01/28/16   Integrated Subsystems
*             02/05/16   App extended to use multiple input/output sources
*             02/24/16   Added Mixer IP to the design
*             04/06/16   Updated design to use mixer to generate BLUE frame
* </pre>
*
******************************************************************************/

#include <stdio.h>
#include "platform.h"
#include "xparameters.h"
#include "eventhndlr.h"
#include "res/parser.h"

#define XVPRD_SW_VER  "v1.0"

/************************** Variable Definitions *****************************/
static XPeriph  PeriphInst;
static XInss    InssInst;
static XVprocSs VprocInst;
static XOutss   OutssInst;
static XVphyss  VphyInst;

XEvent_t SysEvents;

/************************** Function Prototypes ******************************/
static void CheckUserInput(XPeriph  *pPeriph,
		                   XInss    *pInss,
		                   XVprocSs *pVprocss,
		                   XOutss   *pOutss,
		                   XVphyss  *pVphyss);



/***************************************************************************
*  This is the main loop of the application
***************************************************************************/
int main(void)
{
  int status;

  /* Initialize platform */
  init_platform();

  xil_printf("\r\n--------------------------------------------------------\r\n");
  xil_printf("  Video Processing Subsystem Reference Design %s\r\n", XVPRD_SW_VER);
  xil_printf("  (XAPP1291) (c) 2016 by Xilinx Inc.\r\n");
  xil_printf("--------------------------------------------------------\r\n");
  xil_printf("  Build %s - %s\r\n", __DATE__, __TIME__);
  xil_printf("--------------------------------------------------------\r\n");

  Xil_ExceptionDisable();

  xil_printf("\r\nInitialize System Design...\r\n");
  XEvnthdlr_PowerOnInit(&SysEvents);
  status = XSys_Init(&PeriphInst,
		             &InssInst,
		             &VprocInst,
		             &OutssInst,
                     &VphyInst);
  if(status != XST_SUCCESS)
  {
	 xil_printf("CRITICAL ERR:: System Init Failed. Cannot recover from this error. Check HW\n\r");
	 XSys_CriticalError(&PeriphInst,
                        &InssInst,
                        &VprocInst,
                        &OutssInst);
  }

  xil_printf("\r\nStart System...\r\n");
  status = XSys_Start(&PeriphInst,
		              &InssInst,
		              &VprocInst,
		              &OutssInst);

  if(status != XST_SUCCESS)
  {
	 xil_printf("CRITICAL ERR:: Failed to start the design. Cannot recover from this error. Check HW\n\r");
	 XSys_CriticalError(&PeriphInst,
                        &InssInst,
                        &VprocInst,
                        &OutssInst);
  }

  while(1)
  {
	//Check for user interaction
	CheckUserInput(&PeriphInst,
			       &InssInst,
			       &VprocInst,
			       &OutssInst,
			       &VphyInst);

	//Process any pending events
	XEvnthdlr_ProcesPendingEvents(&SysEvents,
			                      &PeriphInst,
			                      &InssInst,
			                      &VprocInst,
			                      &OutssInst,
			                      &VphyInst);
  }

  /* Cleanup */
  cleanup_platform();

  return 0;
}

/***************************************************************************
*  Check for user interaction with the subsystem and Update Video Processing
*  Subsystem run time parameters accordingly
***************************************************************************/
static void CheckUserInput(XPeriph  *pPeriph,
		                   XInss    *pInss,
		                   XVprocSs *pVprocss,
		                   XOutss   *pOutss,
		                   XVphyss  *pVphyss)
{
  static u32 keyValPrev = 0;
  u32 keyValCurr;

  //Respond to user input on DIP switch only if stable input is present
  if(pVprocss->VidIn.VmId != XVIDC_VM_NO_INPUT) {
    //Check DIP Switches
    keyValCurr = XPeriph_ReadDIPSwitch(pPeriph);

    if(keyValPrev != keyValCurr) {
      keyValPrev = keyValCurr;

      if(keyValCurr > 0) {//On Key Press
        XSys_ProcessDipSiwtch(pVprocss, pOutss, keyValCurr);
      }
    }
  }

  //Check for UART activity
  keyValCurr = XPeriph_ReadUART(pPeriph);
  if(keyValCurr>0) {
    XParse_UartKey(keyValCurr,
		           pPeriph,
		           pInss,
		           pVprocss,
		           pOutss,
		           pVphyss);
  }
}
