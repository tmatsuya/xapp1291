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
* @file eventhndlr.c
*
* This resource file provides Event Handling mechanism (Polling based)
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
#include "xstatus.h"
#include "eventhndlr.h"

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/****************************** Local Global *********************************/
XEvent_t *pSysEvent;

/************************** Function Prototypes ******************************/

/************************** Function Definition ******************************/
/*****************************************************************************/
/**
 * This function Initializes the event handler
 *
 * @param  Pointer to Event Handler
 *
 *****************************************************************************/
void XEvnthdlr_PowerOnInit(XEvent_t *pEvent)
{
  Xil_AssertVoid(pEvent != NULL);

  pEvent->event = 0;
  pSysEvent = pEvent; //bind Event scheduler instance to global pointer
}

/*****************************************************************************/
/**
 * This function resets the pending events
 *
 * @param  Pointer to Event Handler
 *
 *****************************************************************************/
void XEvnthdlr_ResetEvents(XEvent_t *pEvent)
{
  Xil_AssertVoid(pEvent != NULL);

  pEvent->event = 0;
}

/*****************************************************************************/
/**
 * This function processes the pending events (1 event processed/iteration)
 * System Application can schedule events (set triggers) at any instant.
 * Ideal call point:   Vertical Blanking interrupt
 * Current Call point: Main Loop (Polling Mechanism)
  *
 * @param  Pointer to Event Handler and all the included subsystems
 *
 *****************************************************************************/
void XEvnthdlr_ProcesPendingEvents(XEvent_t *pEvent,
                                   XPeriph  *pPeriph,
                                   XInss    *pInpss,
                                   XVprocSs *pVprocss,
                                   XOutss   *pOutss,
                                   XVphyss  *pVphyss)
{
  u32 validStream = FALSE;
  u32 eventMask = 1;
  u32 procEvent = 0;
  int status = XST_SUCCESS;

  //check if any event is pending
  if(XEvnthdlr_GetPendingEvents(pEvent))
  {
	/* Multiple events could be pending - extract event to process
	 * starting from highest priority to lowest (right to left in
	 * eventmask bit order)
	 */
    while(!(XEvnthdlr_GetPendingEvents(pEvent) & eventMask)) {
	  eventMask <<= 1;
    }
    procEvent = eventMask;

    //process 1 event per iteration
    switch(procEvent)
    {
      case XEVENT_HDMI_NO_INPUT:
    	  if((XInss_GetInputSourceSel(pInpss) == XINSS_SOURCE_HDMI_RX) &&
    		 (procEvent == XEVENT_HDMI_NO_INPUT)) {

      	    XEvnthdlr_ClearEvent(pEvent, procEvent);
            XInss_SetStreamLockMode(pInpss,  FALSE);
  	        XOutss_VideoMute(pOutss, TRUE);
    	  } else {  //Interrupting source and selected source are different - ignore event
        	XEvnthdlr_ClearEvent(pEvent, procEvent);
    	  }
      	  break;

      case XEVENT_HDMI_INPUT_LOCKED:
    	  if((XInss_GetInputSourceSel(pInpss) == XINSS_SOURCE_HDMI_RX) &&
    		  (procEvent == XEVENT_HDMI_INPUT_LOCKED)) {

    		XEvnthdlr_ClearEvent(pEvent, procEvent);

            //Get Input Stream Information
            validStream = XInss_GetStreamInfo(pInpss, &pVprocss->VidIn);
      	    if(validStream) {
      	      XInss_SetStreamLockMode(pInpss, TRUE);
       	  	  xil_printf("\r\nINFO> %s Input Stream Detected\r\n",
          				  XVidC_GetVideoModeStr(pVprocss->VidIn.VmId));
       	  	  xil_printf("\r\nINFO> Configuring Video Processing Subsystem....\r\n");

      	  	  status = XVprocSs_SetSubsystemConfig(pVprocss);
      	  	  if(status == XST_SUCCESS) {
      	  	      XOutss_VideoMute(pOutss, FALSE);
      	  	  } else {
      	  	      xil_printf("ERR:: VProcss Configuration Failed\r\n");
      	  	  }
      	    }
    	  } else { //Interrupting source and selected source are different - ignore event
      		XEvnthdlr_ClearEvent(pEvent, procEvent);
    	  }
      	  break;

      case XEVENT_HDMI_OUTPUT_LOCKED:
    	  if((XOutss_GetActiveSink(pOutss) == XOUTSS_SINK_HDMI_TX) &&
    		  (procEvent == XEVENT_HDMI_OUTPUT_LOCKED)) {

    	    XEvnthdlr_ClearEvent(pEvent, procEvent);
    	    XVphyss_SetSPanelLockMode(pVphyss, TRUE);
     	    xil_printf("\r\nINFO> Panel Connection Established\r\n");
      	    status = XVprocSs_SetSubsystemConfig(pVprocss);
      	    if(status == XST_SUCCESS) {

       		  if((XInss_GetInputSourceSel(pInpss) == XINSS_SOURCE_TPG)) {
          		 /* Video processing Subsystem Resets TPG. Configure tpg
          	       */
          	      XPeriph_ConfigTpg(pPeriph);
       	    	  XOutss_VideoMute(pOutss, FALSE);
       	      } else if(XInss_IsStreamValid(pInpss)) { //Only if valid input is present
      	    	 XOutss_VideoMute(pOutss, FALSE);
      	      }
      	    } else {
    	  	  xil_printf("ERR:: VProcss Configuration Failed\r\n");
      	    }
    	  } else { //Interrupting source and selected source are different - ignore event
      		XEvnthdlr_ClearEvent(pEvent, procEvent);
    	  }
      	  break;

      case XEVENT_DISP_DNSCALE_PIP_MODE:
      case XEVENT_DISP_UPSCALE_ZOOM_MODE:
      	  XEvnthdlr_ClearEvent(pEvent, procEvent);
      	  status = XVprocSs_SetSubsystemConfig(pVprocss);
      	  if(status == XST_SUCCESS) {
              /* Update Mixer window coordinates for PIP Mode On/Off */
      		  XOutss_UpdateMixerWindow(pOutss,
               		                   &pVprocss->VidOut,
            					       XVprocSs_IsPipModeOn(pVprocss));
      		  if((XInss_GetInputSourceSel(pInpss) == XINSS_SOURCE_TPG)) {
        		  /* Video processing Subsystem Resets TPG
        	         Configure tpg
        	       */
        	      XPeriph_ConfigTpg(pPeriph);
        	  }
   	    	  XOutss_VideoMute(pOutss, FALSE);
      	  } else {
    		xil_printf("ERR:: VProcss Configuration Failed\r\n");
      	  }
      	  break;

      case XEVENT_SYS_VSYNC:
    	  if((XOutss_GetActiveSink(pOutss) == XOUTSS_SINK_HDMI_TX) &&
    		  (procEvent == XEVENT_SYS_VSYNC)) {

        	XEvnthdlr_ClearEvent(pEvent, procEvent);
    	    if(XOutss_IsVsyncEvntPend(pOutss)) {
    		  /* If Zoom/Pip mode is ON and window is moved, update during
    		   * VSync
    		   */
    		  XVprocSs_UpdateZoomPipWindow(pVprocss);
    		  XOutss_SetVSyncEvntPend(pOutss, FALSE);
    	    }
    	  } else { //Interrupting source and selected source are different - ignore event
      		XEvnthdlr_ClearEvent(pEvent, procEvent);
    	  }
          break;

      default:
  		  xil_printf("\r\nINFO> Unknown event\r\n");
      	  break;
    }
  }
}
