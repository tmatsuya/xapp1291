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
* @file sleep.c
*
* This resource file provides system delay routines
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
#ifdef __MICROBLAZE__
#include "sleep.h"

/************************** Constant Definitions *****************************/
#define PROCESSOR_CLOCK_FREQUENCY       XPAR_CPU_CORE_CLOCK_FREQ_HZ

/**************************** Type Definitions *******************************/

/**************************** Local Global *******************************/


/*****************************************************************************/
/**
 * This function suspends processor for user specified time in sec
 *
 * @param  pTmr Pointer to system timer
 * @param  sec  delay time
 *
 *****************************************************************************/
void sleepsec(XTmrCtr *pTmr, u32 sec)
{
  u32 delayVal;

  //compute delay count in useconds
  delayVal = sec * 1000000;
  sleepus(pTmr, delayVal);
}


/*****************************************************************************/
/**
 * This function suspends processor for user specified time in milliseconds
 *
 * @param  pTmr Pointer to system timer
 * @param  msec  delay time
 *
 *****************************************************************************/
void sleepms(XTmrCtr *pTmr, u32 msec)
{
  u32 delayVal;

  //compute delay count in useconds
  delayVal = msec * 1000;
  sleepus(pTmr, delayVal);
}

/*****************************************************************************/
/**
 * This function suspends processor for user specified time in microseconds
 *
 * @param  pTmr Pointer to system timer
 * @param  usec  delay time
 *
 *****************************************************************************/
void sleepus(XTmrCtr *pTmr, u32 usec)
{
  u32 delayCount = usec * (PROCESSOR_CLOCK_FREQUENCY / 1000000);
  u32 currCount;

  Xil_AssertVoid(pTmr != NULL);

  if(pTmr)
  {
    XTmrCtr_SetResetValue(pTmr, 0, 0);
    XTmrCtr_Start(pTmr, 0);

    do
    {
	  currCount = XTmrCtr_GetValue(pTmr, 0);
    }while(currCount<=delayCount);

    XTmrCtr_Stop(pTmr, 0);
  }
}
#endif
