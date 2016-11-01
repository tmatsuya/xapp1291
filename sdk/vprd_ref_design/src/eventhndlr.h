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
* @file eventhndlr.h
*
* This is header for system wide event handling
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who    Date     Changes
* ----- ---- -------- -------------------------------------------------------
* 1.00  rco   09/11/15 First release
*
* </pre>
*
******************************************************************************/
#ifndef XEVENT_HNDLR_H		 /* prevent circular inclusions */
#define XEVENT_HNDLR_H		 /* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

#include "system.h"

/************************** Constant Definitions *****************************/
typedef enum
{
  XEVENT_HDMI_NO_INPUT           = 0x00000001,
  XEVENT_RSVD1_NO_INPUT          = 0x00000002,
  XEVENT_RSVD2_NO_INPUT          = 0x00000004,
  XEVENT_HDMI_INPUT_LOCKED       = 0x00000008,
  XEVENT_RSVD1_INPUT_LOCKED      = 0x00000010,
  XEVENT_RSVD2_INPUT_LOCKED      = 0x00000020,
  XEVENT_HDMI_OUTPUT_LOCKED      = 0x00000040,
  XEVENT_RSVD1_OUTPUT_LOCKED     = 0x00000080,
  XEVENT_RSVD2_OUTPUT_LOCKED     = 0x00000100,
  XEVENT_DISP_DNSCALE_PIP_MODE   = 0x00000200,
  XEVENT_DISP_UPSCALE_ZOOM_MODE  = 0x00000400,
  XEVENT_SYS_VSYNC               = 0x00000800,
  XEVENT_MAX_TRIGGER             = 0x80000000
}XEVENT_TRIGGERS;

/************************** Structure Definitions *****************************/
typedef struct
{
  volatile u32 event;
}XEvent_t;


/***************** Macros (Inline Functions) Definitions *********************/

/*****************************************************************************/
/**
 * This macro reads the pending triggers
 *
 * @param  pEvent is a pointer to the Event triggers
 * @return none
 *
 *****************************************************************************/
#define XEvnthdlr_GetPendingEvents(pEvent)    ((pEvent)->event)

/*****************************************************************************/
/**
 * This macro register the event trigger
 *
 * @param  pEvent is a pointer to the Event triggers
 * @return none
 *
 *****************************************************************************/
#define XEvnthdlr_GenEvent(pEvent, trigger) \
                 ((pEvent)->event = (XEvnthdlr_GetPendingEvents(pEvent) | trigger))

/*****************************************************************************/
/**
 * This macro clears the event trigger
 *
 * @param  pEvent is a pointer to the Event triggers
 * @return none
 *
 *****************************************************************************/
#define XEvnthdlr_ClearEvent(pEvent, trigger) \
                 ((pEvent)->event = (XEvnthdlr_GetPendingEvents(pEvent) & (~trigger)))


/************************** Exported APIs ************************************/
extern XEvent_t *pSysEvent; //system global variable

void XEvnthdlr_PowerOnInit(XEvent_t *pEvent);
void XEvnthdlr_ResetEvents(XEvent_t *pEvent);
void XEvnthdlr_ProcesPendingEvents(XEvent_t *pEvent,
		                           XPeriph  *pPeriph,
			   	                   XInss    *pInpss,
				                   XVprocSs *pVprocss,
				                   XOutss   *pOutss,
				                   XVphyss  *pVphyss);


#ifdef __cplusplus
}
#endif

#endif /* End of protection macro */
