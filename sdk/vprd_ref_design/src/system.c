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
* @file system.c
*
* This resource file manages system level control flow
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who    Date     Changes
* ----- ---- -------- -------------------------------------------------------
* 1.00  rco   09/23/15   Initial Release
* 2.00  rco   04/06/16   Couple Mac+Phy into single entity
* </pre>
*
******************************************************************************/
#include "xparameters.h"
#include "platform_config.h"
#include "system.h"
#include "eventhndlr.h"

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/


/**************************** Local Global *******************************/
/* System Level Driver Instances of IP's in the design */
static XV_HdmiRxSs HdmiRxSubsys;
static XV_HdmiTxSs HdmiTxSubsys;
static XVphy       Vphy;

/***************** Macros (Inline Functions) Definitions *********************/


/************************** Function Prototypes ******************************/
static void SetTpgAsActiveSource(XPeriph  *pPeriph, XVprocSs *pVprocss);
static void SetHdmiAsActiveSource(XPeriph *pPeriph, XInss *pInpss);

/************************** Function Definition ******************************/

/*****************************************************************************/
/**
*
* This function is the system level initialization routine. It in turn calls
* each of the included subsystem initialization function
*
* @param	pPeriph is a pointer to the peripherals instance
* @param	pInss is a pointer to the input subsystem instance
* @param	pVprocss is a pointer to the video proc subsystem instance
* @param	pOutss is a pointer to the output subsystem instance
* @param	pVphy is a pointer to the video phy subsystem instance
*
* @return
*		- XST_SUCCESS if all subsystem init. was successful.
*		- XST_FAILURE if HDMI TX PIO ID mismatched.
*
* @note		None.
*
******************************************************************************/
int XSys_Init(XPeriph  *pPeriph,
	   	      XInss    *pInss,
		      XVprocSs *pVprocss,
		      XOutss   *pOutss,
		      XVphyss  *pVphyss)
{
  int status;
  XVprocSs_Config *VprocSsConfigPtr;

  Xil_AssertNonvoid(pPeriph  != NULL);
  Xil_AssertNonvoid(pInss    != NULL);
  Xil_AssertNonvoid(pVprocss != NULL);
  Xil_AssertNonvoid(pOutss   != NULL);
  Xil_AssertNonvoid(pVphyss  != NULL);

  //Init all instance variables to 0
  memset(pPeriph,  0, sizeof(XPeriph));
  memset(pInss,    0, sizeof(XInss));
  memset(pVprocss, 0, sizeof(XVprocSs));
  memset(pOutss,   0, sizeof(XOutss));
  memset(pVphyss,  0, sizeof(XVphyss));

  //Bind the Subsystem instances to corresponding drivers
  pInss->Hdmi.HdmiRxSsPtr  = &HdmiRxSubsys;
  pInss->Hdmi.VphyPtr      = &Vphy;

  pOutss->Hdmi.HdmiTxSsPtr = &HdmiTxSubsys;
  pOutss->Hdmi.VphyPtr     = &Vphy;

  /* Connect Shared blocks
   *   - Video Phy is shared between Rx & Tx
   */
  /* Connect Input/Output subsystem with Vphy instance */
  pVphyss->HdmiRx = &pInss->Hdmi;
  pVphyss->HdmiTx = &pOutss->Hdmi;

  xil_printf("\r\n  ->Initialize System Peripherals.... \r\n");
  status = XPeriph_PowerOnInit(pPeriph);
  if(status != XST_SUCCESS) {
	 xil_printf("ERR:: System Peripheral Init. error\n\r");
	 return(XST_FAILURE);
  }

  //Register Interrupt controller with subsystems
  XInss_RegisterSysIntc(pInss,     pPeriph->IntcPtr);
  XOutss_RegisterSysIntc(pOutss,   pPeriph->IntcPtr);
  XVphyss_RegisterSysIntc(pVphyss, pPeriph->IntcPtr);

  //Register System delay routine with subsystems
#ifdef __MICROBLAZE__
  XInss_SetUserTimerHandler(pInss,
		                    (XVidC_DelayHandler)sleepus,
		                    pPeriph->TimerPtr);
  XVprocSs_SetUserTimerHandler(pVprocss,
		                       (XVidC_DelayHandler)sleepus,
		                       pPeriph->TimerPtr);
  XOutss_SetUserTimerHandler(pOutss,
		                     (XVidC_DelayHandler)sleepus,
		                     pPeriph->TimerPtr);
#endif

  xil_printf("\r\n  ->Initialize Input Subsystem...\r\n");
  status = XInss_PowerOnInit(pInss);
  if(status != XST_SUCCESS) {
	xil_printf("INSS ERR:: Input Subsystem Initialization failed %d\r\n", status);
	return(XST_FAILURE);
  }

  xil_printf("\r\n  ->Initialize Video Processing Subsystem...\r\n");
  XVprocSs_SetFrameBufBaseaddr(pVprocss, USR_FRAME_BUF_BASEADDR);
  VprocSsConfigPtr = XVprocSs_LookupConfig(XPAR_V_PROC_SS_0_DEVICE_ID);
  if(VprocSsConfigPtr == NULL) {
  	xil_printf("ERR:: VprocSs device not found\r\n");
    return (XST_DEVICE_NOT_FOUND);
  }

  status = XVprocSs_CfgInitialize(pVprocss,
		                          VprocSsConfigPtr,
		                          VprocSsConfigPtr->BaseAddress);

  if(status != XST_SUCCESS) {
	 xil_printf("ERR:: Video Processing Subsystem Init. error\n\r");
	 return(XST_FAILURE);
  }

  xil_printf("\r\n  ->Initialize Output Subsystem...\r\n");
  status = XOutss_PowerOnInit(pOutss);
  if(status != XST_SUCCESS) {
	xil_printf("OUTSS ERR:: Output Subsystem Initialization failed %d\r\n", status);
	return(XST_FAILURE);
  }

  xil_printf("\r\n  ->Initialize Video PHY...\r\n");
  status = XVPhyss_PowerOnInit(pVphyss);
  if(status != XST_SUCCESS) {
	xil_printf("PHY ERR:: Video Phy Initialization failed %d\r\n", status);
	return(XST_FAILURE);
  }

  return(status);
}

/*****************************************************************************/
/**
*
* This function starts the system design. It in turn calls each of the included
* subsystem start function
*
* @param	pPeriph is a pointer to the peripherals instance
* @param	pInss is a pointer to the input subsystem instance
* @param	pVprocss is a pointer to the video proc subsystem instance
* @param	pOutss is a pointer to the output subsystem instance
*
* @return
*		- XST_SUCCESS if all subsystem init. was successful.
*		- XST_FAILURE if HDMI TX PIO ID mismatched.
*
* @note		None.
*
******************************************************************************/
int XSys_Start(XPeriph  *pPeriph,
	   	       XInss    *pInss,
		       XVprocSs *pVprocss,
		       XOutss   *pOutss)

{
  int status = XST_SUCCESS;

  Xil_AssertNonvoid(pPeriph  != NULL);
  Xil_AssertNonvoid(pInss    != NULL);
  Xil_AssertNonvoid(pVprocss != NULL);
  Xil_AssertNonvoid(pOutss   != NULL);

  //Start System Peripherals - start INTC before all subsystems
  XPeriph_Start(pPeriph);

  //Start Output Subsystem
  XOutss_Start(pOutss);

  //Start Processing Subsystem
  XVprocSs_Start(pVprocss);

  //Start Input Subsystem
  XInss_Start(pInss);

  /* Set HDMI as Default Input Source */
  XSys_SetActiveInput(pPeriph,
     		          pInss,
					  pVprocss,
					  pOutss,
					  XINSS_SOURCE_HDMI_RX);

  /* Set HDMI as Default Output Sink */
  XSys_SetActiveOutput(pPeriph,
		               pVprocss,
					   pOutss,
					   XOUTSS_SINK_HDMI_TX);

  return(status);
}

/*****************************************************************************/
/**
*
* This function stops the system design. It in turn calls each of the included
* subsystem stop function
*
* @param	pPeriph is a pointer to the peripherals instance
* @param	pInss is a pointer to the input subsystem instance
* @param	pVprocss is a pointer to the video proc subsystem instance
* @param	pOutss is a pointer to the output subsystem instance
*
* @return
*		- XST_SUCCESS if all subsystem init. was successful.
*		- XST_FAILURE if HDMI TX PIO ID mismatched.
*
* @note		None.
*
******************************************************************************/
int XSys_Stop(XPeriph  *pPeriph,
	   	      XInss    *pInss,
		      XVprocSs *pVprocss,
		      XOutss   *pOutss)

{
  int status = XST_SUCCESS;

  Xil_AssertNonvoid(pPeriph  != NULL);
  Xil_AssertNonvoid(pInss    != NULL);
  Xil_AssertNonvoid(pVprocss != NULL);
  Xil_AssertNonvoid(pOutss   != NULL);

  //Stop System Peripherals
  XPeriph_Stop(pPeriph);

  //Stop Output Subsystem
  XOutss_Stop(pOutss);

  //Stop Processing Subsystem
  XVprocSs_Stop(pVprocss);

  //Stop Input Subsystem
  XInss_Stop(pInss);

  return(status);

}

/*****************************************************************************/
/**
*
* This function resets the system design. It in turn calls each of the included
* subsystem reset function
*
* @param	pPeriph is a pointer to the peripherals instance
* @param	pInss is a pointer to the input subsystem instance
* @param	pVprocss is a pointer to the video proc subsystem instance
* @param	pOutss is a pointer to the output subsystem instance
*
* @return
*		- XST_SUCCESS if all subsystem init. was successful.
*		- XST_FAILURE if HDMI TX PIO ID mismatched.
*
* @note		None.
*
******************************************************************************/
int XSys_Reset(XPeriph  *pPeriph,
	   	       XInss    *pInss,
		       XVprocSs *pVprocss,
		       XOutss   *pOutss)

{
  int status = XST_SUCCESS;

  Xil_AssertNonvoid(pPeriph  != NULL);
  Xil_AssertNonvoid(pInss    != NULL);
  Xil_AssertNonvoid(pVprocss != NULL);
  Xil_AssertNonvoid(pOutss   != NULL);

  //Reset System Peripherals
  XPeriph_Reset(pPeriph);

  //Reset Output Subsystem
  XOutss_Reset(pOutss);

  //Reset Processing Subsystem
  XVprocSs_Reset(pVprocss);

  //Reset Input Subsystem
  XInss_Reset(pInss);

  //Reset all events
  XEvnthdlr_ResetEvents(pSysEvent);

  return(status);
}

/*****************************************************************************/
/**
*
* This function a wrapper that calls relevant subsystem API's to set the
* active input source
*
* @param	pInss is a pointer to the input subsystem instance
* @param	pPeriph is a pointer to system peripherals that owns the mux
* @param    Source is the new active source
*
* @return   None
*
* @note		None.
*
******************************************************************************/
void XSys_SetActiveInput(XPeriph    *pPeriph,
		                 XInss      *pInpss,
						 XVprocSs   *pVprocss,
						 XOutss     *pOutss,
		                 XInput_Sel Source)
{
  XOutss_VideoMute(pOutss, TRUE);
  XInss_SetActiveSource(pInpss, Source);
  switch(Source)
  {
    case XINSS_SOURCE_TPG:
    	SetTpgAsActiveSource(pPeriph, pVprocss);
    	break;

    case XINSS_SOURCE_HDMI_RX:
    	SetHdmiAsActiveSource(pPeriph, pInpss);
    	break;

    default:
    	break;
  }
  XOutss_VideoMute(pOutss, FALSE);
}

/*****************************************************************************/
/**
*
* This function a wrapper that calls relevant subsystem API's to set the output
* resolution
*
* @param	pPeriph is a pointer to the system peripheral subsystem instance
* @param	pVprocss is a pointer to the video proc subsystem instance
* @param	pOutss is a pointer to the output subsystem instance
* @param    Sink is the identifier tag for requested output protocol
*
* @return   None
*
* @note		None.
*
******************************************************************************/
void XSys_SetActiveOutput(XPeriph *pPeriph,
		                  XVprocSs *pVprocss,
		                  XOutss   *pOutss,
		                  XOutput_Sel Sink)
{
  XOutss_SetActiveSink(pOutss, Sink);
  XSys_SetOutputResolution(pVprocss, pOutss, pVprocss->VidOut.VmId);
  /* For design with Mixer, TPG and VPSS color format should follow Mixer
   * Master Layer color format
   */
  {
	XVidC_ColorFormat Cfmt;

	XVMix_GetLayerColorFormat(pOutss->MixerPtr, XVMIX_LAYER_MASTER, &Cfmt);
    XPeriph_SetTPGColorFormat(pPeriph, Cfmt);
    XVprocSs_SetStreamColorFormat(&pVprocss->VidOut, Cfmt);
  }
}

/*****************************************************************************/
/**
*
* This function a wrapper that calls relevant subsystem API's to set the output
* resolution
*
* @param	pVprocss is a pointer to the video proc subsystem instance
* @param	pInss is a pointer to the output subsystem instance
* @param    NewVmId is the identifier tag for requested output resolution
*
* @return   None
*
* @note		None.
*
******************************************************************************/
void XSys_SetOutputResolution(XVprocSs *pVprocss,
		                      XOutss   *pOutss,
		                      XVidC_VideoMode NewVmId)
{
  int status;
  XVidC_VideoMode CurVideoMode;
  XVidC_VideoTiming const *TimingPtr;

  if(NewVmId > XVIDC_VM_NUM_SUPPORTED) {
	xil_printf("\r\n****ERROR: Selected Output Resolution Currently Not Supported****\r\n");
	return;
  }

  xil_printf("\r\n");

  /* Save current video mode */
  CurVideoMode = pVprocss->VidOut.VmId;

  /* Get timing information for new mode */
  TimingPtr = XVidC_GetTimingInfo(NewVmId);

  //Set new output resolution for Video Processing subsystem
  XVprocSs_SetStreamResolution(&pVprocss->VidOut, NewVmId, TimingPtr);

  //configure output subsystem for new resolution
  status = XOutss_SetOutputStream(pOutss, &pVprocss->VidOut);

  if(status == XST_FAILURE) {//unable to set the output resolution

    xil_printf("INFO> Returning to previously set resolution: %s\r\n", \
  		       XVidC_GetVideoModeStr(CurVideoMode));
    /* restore current resolution */
    /* Get timing information for previous mode */
	TimingPtr = XVidC_GetTimingInfo(CurVideoMode);
    XVprocSs_SetStreamResolution(&pVprocss->VidOut, CurVideoMode, TimingPtr);
    XOutss_SetOutputStream(pOutss, &pVprocss->VidOut);
  }

  //Configure Mixer after resolution change
  XOutss_SetupMixer(pOutss, &pVprocss->VidOut);
}

/*****************************************************************************/
/**
*
* This function a wrapper that calls relevant subsystem API's to set the output
* resolution
*
* @param	pPeriph is a pointer to the peripherals instance
* @param	pVprocss is a pointer to the video proc subsystem instance
*
* @return   None
*
******************************************************************************/
void XSys_SetVpssToTpgResolution(XPeriph *pPeriph, XVprocSs *pVprocss)
{
  u16 width, height;
  XVidC_VideoStream Stream = {0};
  XVidC_VideoTiming const *pTiming;
  int Status;

  pTiming = XVidC_GetTimingInfo(pPeriph->UsrSet.TpgVidMode);

  width  = ((pTiming) ? pTiming->HActive : 0);
  height = ((pTiming) ? pTiming->VActive : 0);

  //Setup Video Processing Subsystem for TPG Input Use Case
  Stream.VmId           = pPeriph->UsrSet.TpgVidMode;
  Stream.Timing.HActive = width;
  Stream.Timing.VActive = height;
  Stream.ColorFormatId  = pPeriph->UsrSet.TpgColorFmt;
  Stream.ColorDepth     = pVprocss->Config.ColorDepth;
  Stream.PixPerClk      = pVprocss->Config.PixPerClock;
  Stream.FrameRate      = XVidC_GetFrameRate(Stream.VmId);
  Stream.IsInterlaced   = XVidC_GetVideoFormat(Stream.VmId);
  XVprocSs_SetVidStreamIn(pVprocss, &Stream);

  /* Reset input domain feature */
  XVprocSs_ResetZoomModeFlag(pVprocss);

  //Config video processing subsystem
  Status = XVprocSs_SetSubsystemConfig(pVprocss);

  if(Status == XST_SUCCESS) {
    //config tpg and start - vpss resets tpg
    XPeriph_ConfigTpg(pPeriph);
  } else {
    xil_printf("SYS ERR:: VProcss Configuration Failed. \r\n");
  }
}

/*****************************************************************************/
/**
*
* This function configures TPG as an input source.
*
* @param	pPeriph is a pointer to the peripherals instance
* @param	pVprocss is a pointer to the video proc subsystem instance
*
* @return   None
*
* @note		None.
*
******************************************************************************/\
static void SetTpgAsActiveSource(XPeriph  *pPeriph, XVprocSs *pVprocss)
{
  /* Select TPG as Active Stream from Switch */
  XPeriph_SetInputMux(pPeriph, XPERIPH_INPUT_MUX_TPG);
  XSys_SetVpssToTpgResolution(pPeriph, pVprocss);
}

/*****************************************************************************/
/**
*
* This function is called to set HDMI as the active source
*
*
* @param	pPeriph is a pointer to the peripherals instance
* @param    pInpss is a pointer to the input subsystem instance
*
* @return   None
*
******************************************************************************/
static void SetHdmiAsActiveSource(XPeriph *pPeriph, XInss *pInpss)
{
  XPeriphInputMux SwitchSel;

  //Stop TPG
  XPeriph_DisableTpg(pPeriph);

  SwitchSel = XPERIPH_INPUT_MUX_HDMI;

  XInss_StartStream(pInpss);
  XPeriph_SetInputMux(pPeriph, SwitchSel);
}

/*****************************************************************************/
/**
*
* This function is called when DIP switch is pressed on the board
* For video ref design switch controls PIP/Zoom window movement on the screen
*
* @param	pVprocss is a pointer to the video proc subsystem instance
* @param    keypress is the dip switch activated
*
* @return   None
*
* @notes    Window H Size default is defined by the subsystem. User can
*           use the default or set his own, provided it is aligned to vdma
*           aximm width
******************************************************************************/
void XSys_ProcessDipSiwtch(XVprocSs *pVprocss,
		                   XOutss   *pOutss,
		                   u32 keypress)
{
  XVidC_VideoWindow win;
  u32 mode, destY, destX;
  u16 step = XVprocSs_GetPipZoomWinHStepSize(pVprocss);

  switch(keypress)
  {
    case MOVE_WIN_UP:
    	if((XVprocSs_IsZoomModeOn(pVprocss)) ||
    	   (XVprocSs_IsPipModeOn(pVprocss)))
    	{
    	  mode = ((XVprocSs_IsZoomModeOn(pVprocss)) ? XVPROCSS_ZOOM_WIN
    			                                    : XVPROCSS_PIP_WIN);
    	  XVprocSs_GetZoomPipWindow(pVprocss, mode, &win);

   	      //Move window UP by 16 lines
          win.StartY =  ((win.StartY <= 16) ? 0 : (win.StartY-16));
 	      XVprocSs_SetZoomPipWindow(pVprocss, mode, &win);
 	      //PIP/Zoom already ON perform in-place update
 	      XOutss_SetVSyncEvntPend(pOutss, TRUE);
    	}
    	break;

    case MOVE_WIN_DN:
    	if((XVprocSs_IsZoomModeOn(pVprocss)) ||
    	   (XVprocSs_IsPipModeOn(pVprocss)))
    	{
    	  mode = ((XVprocSs_IsZoomModeOn(pVprocss)) ? XVPROCSS_ZOOM_WIN
    			                                    : XVPROCSS_PIP_WIN);
    	  XVprocSs_GetZoomPipWindow(pVprocss, mode, &win);

    	  //Move window down by 16 lines
    	  destY = ((XVprocSs_IsZoomModeOn(pVprocss)) ? pVprocss->CtxtData.VidInHeight
    	      			                             : pVprocss->VidOut.Timing.VActive);
    	  win.StartY =  (((win.StartY+16+win.Height) > destY) ? win.StartY : (win.StartY+16));
 	      XVprocSs_SetZoomPipWindow(pVprocss, mode, &win);
 	      //PIP/Zoom already ON perform in-place update
 	      XOutss_SetVSyncEvntPend(pOutss, TRUE);
    	}
    	break;

    case MOVE_WIN_LEFT:
    	if((XVprocSs_IsZoomModeOn(pVprocss)) ||
    	   (XVprocSs_IsPipModeOn(pVprocss)))
    	{
    	  mode = ((XVprocSs_IsZoomModeOn(pVprocss)) ? XVPROCSS_ZOOM_WIN
    			                                    : XVPROCSS_PIP_WIN);
    	  XVprocSs_GetZoomPipWindow(pVprocss, mode, &win);

    	  //Move window left "step" pixels
    	  win.StartX = ((win.StartX <= step) ? 0 : (win.StartX-step));
 	      XVprocSs_SetZoomPipWindow(pVprocss, mode, &win);
 	      //PIP/Zoom already ON perform in-place update
 	      XOutss_SetVSyncEvntPend(pOutss, TRUE);
    	}
    	break;

    case MOVE_WIN_RIGHT:
    	if((XVprocSs_IsZoomModeOn(pVprocss)) ||
    	   (XVprocSs_IsPipModeOn(pVprocss)))
    	{
    	  mode = ((XVprocSs_IsZoomModeOn(pVprocss)) ? XVPROCSS_ZOOM_WIN
    			                                    : XVPROCSS_PIP_WIN);
    	  XVprocSs_GetZoomPipWindow(pVprocss, mode, &win);

    	  //Move window right by "step" pixels
    	  destX = ((XVprocSs_IsZoomModeOn(pVprocss)) ? (pVprocss->CtxtData.VidInWidth)
    			                                     : (pVprocss->VidOut.Timing.HActive));
    	  win.StartX = (((win.StartX+step+win.Width) > destX) ? win.StartX : (win.StartX+step));
 	      XVprocSs_SetZoomPipWindow(pVprocss, mode, &win);
 	      //PIP/Zoom already ON perform in-place update
 	      XOutss_SetVSyncEvntPend(pOutss, TRUE);
    	}
    	break;

    case ZP_MODE_OFF:
    	if(XVprocSs_IsZoomModeOn(pVprocss))
    	{
    	  XVprocSs_SetZoomMode(pVprocss, FALSE);
    	  XEvnthdlr_GenEvent(pSysEvent, XEVENT_DISP_UPSCALE_ZOOM_MODE);
    	}
    	else if(XVprocSs_IsPipModeOn(pVprocss))
    	{
    	  XVprocSs_SetPipMode(pVprocss, FALSE);
   	      XEvnthdlr_GenEvent(pSysEvent, XEVENT_DISP_DNSCALE_PIP_MODE);
    	}
    	else
    	{
       	  xil_printf("  :PIP/Zoom Mode are OFF\r\n");
    	}
    	break;
  }
}

/*****************************************************************************/
/**
*
 * This function does not return. If control ever reaches this point the only
 * way to recover is system reset (unless watchdog is implemented)
*
* @param	pPeriph is a pointer to the peripherals instance
* @param	pInss is a pointer to the input subsystem instance
* @param	pVprocss is a pointer to the video proc subsystem instance
* @param	pOutss is a pointer to the output subsystem instance
*
* @return
*		- XST_SUCCESS if all subsystem init. was successful.
*		- XST_FAILURE if HDMI TX PIO ID mismatched.
*
* @note		To Debug: Set breakpoint in this function and analyze stack to determine the
 *           last executed function that caused the critical error
*
******************************************************************************/
void XSys_CriticalError(XPeriph  *pPeriph,
	   	                XInss    *pInss,
		                XVprocSs *pVprocss,
		                XOutss   *pOutss)
{
	while(1)
	{
	  //If watchdog is implemented wait for the kick, then reset the system
#if WATCHDOG_TIMER_PRESENT
	  XSys_Reset(pPeriph,
			   	 pInss,
				 pVprocss,
				 pOutss);
	  return;
#else
	  //NOP
#endif
	}
}

/*****************************************************************************/
/**
*
* This function reports all the IP's included in the system design.
* It calls each subsystem API to report its child nodes
*
* @param	pPeriph is a pointer to the peripherals instance
* @param	pInss is a pointer to the input subsystem instance
* @param	pVprocss is a pointer to the video proc subsystem instance
* @param	pOutss is a pointer to the output subsystem instance
*
* @return   None
*
* @note		None.
*
******************************************************************************/
void XSys_ReportSystemInfo(XPeriph  *pPeriph,
	   	                   XInss    *pInss,
		                   XVprocSs *pVprocss,
		                   XOutss   *pOutss)
{
  Xil_AssertVoid(pPeriph  != NULL);
  Xil_AssertVoid(pInss    != NULL);
  Xil_AssertVoid(pVprocss != NULL);
  Xil_AssertVoid(pOutss   != NULL);

  xil_printf("\r\n\r\r****Reporting System Design Info****\r\n");

  //Report Peripherals found at System level
  XPeriph_ReportDeviceInfo(pPeriph);

  //Report cores in Input subsystem
  XV_HdmiRxSs_ReportCoreInfo(pInss->Hdmi.HdmiRxSsPtr);

  //Report cores in Video Processing subsystem
  XVprocSs_ReportSubsystemCoreInfo(pVprocss);

  //Report cores in Output subsystem
  XV_HdmiTxSs_ReportCoreInfo(pOutss->Hdmi.HdmiTxSsPtr);
}

/*****************************************************************************/
/**
*
* This function reports the Input and Output Subsystem link status.
*
* @param	pInss is a pointer to the input subsystem instance
* @param	pOutss is a pointer to the output subsystem instance
*
* @return  None
*
* @note		None.
*
******************************************************************************/
void XSys_ReportLinkStatus(XInss   *pInss,
		                   XOutss  *pOutss,
		                   XVphyss *pVphyss)
{
  XV_HdmiRxSs_ReportSubcoreVersion(pInss->Hdmi.HdmiRxSsPtr);
  XV_HdmiTxSs_ReportSubcoreVersion(pOutss->Hdmi.HdmiTxSsPtr);

  xil_printf("\n\r--------------\n\r");
  xil_printf("HDMI Link Info\n\r");
  xil_printf("--------------\n\r\n\r");

  xil_printf("HDMI Rx Timing\n\r");
  xil_printf("--------------\n\r");
  XV_HdmiRxSs_ReportTiming(pInss->Hdmi.HdmiRxSsPtr);

  xil_printf("HDMI Tx Timing\n\r");
  xil_printf("--------------\n\r");
  XV_HdmiTxSs_ReportTiming(pOutss->Hdmi.HdmiTxSsPtr);

  xil_printf("Phy status\n\r");
  xil_printf("--------------\n\r");
  xil_printf("TX reference clock frequency %0d Hz\n\r", \
		  XVphy_ClkDetGetRefClkFreqHz(pVphyss->HdmiTx->VphyPtr, XVPHY_DIR_TX));
  xil_printf("RX reference clock frequency %0d Hz\n\r", \
		  XVphy_ClkDetGetRefClkFreqHz(pVphyss->HdmiRx->VphyPtr, XVPHY_DIR_RX));

  if(pVphyss->HdmiRx->VphyPtr->Config.DruIsPresent == TRUE) {
    xil_printf("DRU reference clock frequency %0d Hz\n\r", \
    		XVphy_DruGetRefClkFreqHz(pVphyss->HdmiRx->VphyPtr));
  }
  XVphy_HdmiDebugInfo(pVphyss->HdmiRx->VphyPtr, 0, XVPHY_CHANNEL_ID_CH1);

  xil_printf("Link Quality\n\r");
  xil_printf("--------------\n\r");
  XV_HdmiRxSs_ReportLinkQuality(pInss->Hdmi.HdmiRxSsPtr);

  xil_printf("\n\r");

  xil_printf("Audio\n\r");
  xil_printf("---------\n\r");
  XV_HdmiRxSs_ReportAudio(pInss->Hdmi.HdmiRxSsPtr);

  xil_printf("Infoframe\n\r");
  xil_printf("---------\n\r");
  XV_HdmiRxSs_ReportInfoFrame(pInss->Hdmi.HdmiRxSsPtr);

  xil_printf("---------\n\r\n\r");
}

/*****************************************************************************/
/**
*
* This function reports the system status flags.
*
* @param	pPeriph is a pointer to the system peripheral instance
* @param	pInss is a pointer to the input subsystem instance
* @param	pVprocss is a pointer to the vpss instance
* @param	pVphyss is a pointer to the video phy instance
*
* @return  None
*
* @note		None.
*
******************************************************************************/
void XSys_ReportSystemStatus(XPeriph  *pPeriph,
		                     XInss    *pInss,
		                     XVprocSs *pVprocss,
		                     XVphyss  *pVphyss)
{
  xil_printf("\r\n-------------\r\n");
  xil_printf("System Status\r\n");
  xil_printf("-------------\r\n\r\n");

  xil_printf("Input Locked:   %d\r\n",XInss_IsStreamValid(pInss));
  xil_printf("Output Locked:  %d\r\n",XVphyss_IsPanelConnected(pVphyss));
  xil_printf("Active Input:   %s\r\n", \
		 ((XInss_GetInputSourceSel(pInss) == XINSS_SOURCE_TPG)) ? "Tpg" : "Hdmi");
  xil_printf("Pending Events: %x\r\n\r\n",XEvnthdlr_GetPendingEvents(pSysEvent));
}
