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
* @file xinss.c
*
* This is main code of Input Subsystem device driver based on Xilinx HDMI Rx
* Please see xinss.h for more details of the driver.
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
#include <stdio.h>
#include "../platform_config.h"
#include "xparameters.h"
#if defined(__arm__)
#include "sleep.h"
#elif defined(__MICROBLAZE__)
#include "microblaze_sleep.h"
#endif
#include "xinss.h"
#include "../eventhndlr.h"

/************************** Constant Definitions *****************************/
/*
  EDID
*/
// Xilinx EDID
static const u8 Edid[] = {
0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x61, 0x98, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12,
0x1F, 0x19, 0x01, 0x03, 0x80, 0x59, 0x32, 0x78, 0x0A, 0xEE, 0x91, 0xA3, 0x54, 0x4C, 0x99, 0x26,
0x0F, 0x50, 0x54, 0x21, 0x08, 0x00, 0x71, 0x4F, 0x81, 0xC0, 0x81, 0x00, 0x81, 0x80, 0x95, 0x00,
0xA9, 0xC0, 0xB3, 0x00, 0x01, 0x01, 0x02, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C,
0x45, 0x00, 0x20, 0xC2, 0x31, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x58, 0x49, 0x4C,
0x49, 0x4E, 0x58, 0x20, 0x48, 0x44, 0x4D, 0x49, 0x0A, 0x20, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0C,
0x02, 0x03, 0x34, 0x71, 0x57, 0x61, 0x10, 0x1F, 0x04, 0x13, 0x05, 0x14, 0x20, 0x21, 0x22, 0x5D,
0x5E, 0x5F, 0x60, 0x65, 0x66, 0x62, 0x63, 0x64, 0x07, 0x16, 0x03, 0x12, 0x23, 0x09, 0x07, 0x07,
0x67, 0x03, 0x0C, 0x00, 0x10, 0x00, 0x78, 0x3C, 0xE3, 0x0F, 0x01, 0xE0, 0x67, 0xD8, 0x5D, 0xC4,
0x01, 0x78, 0x80, 0x07, 0x02, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C, 0x45, 0x00,
0x20, 0xC2, 0x31, 0x00, 0x00, 0x1E, 0x08, 0xE8, 0x00, 0x30, 0xF2, 0x70, 0x5A, 0x80, 0xB0, 0x58,
0x8A, 0x00, 0x20, 0xC2, 0x31, 0x00, 0x00, 0x1E, 0x04, 0x74, 0x00, 0x30, 0xF2, 0x70, 0x5A, 0x80,
0xB0, 0x58, 0x8A, 0x00, 0x20, 0x52, 0x31, 0x00, 0x00, 0x1E, 0x66, 0x21, 0x56, 0xAA, 0x51, 0x00,
0x1E, 0x30, 0x46, 0x8F, 0x33, 0x00, 0x50, 0x1D, 0x74, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x2E
};

/**************************** Type Definitions *******************************/

/**************************** Local Global *******************************/

/************************** Function Prototypes ******************************/
static void RxConnectCallback(void *CallBackRef);
static void RxLnkStaCallback(void *CallBackRef);
static void RxStreamDownCallback(void *CallBackRef);
static void RxStreamInitCallback(void *CallBackRef);
static void RxStreamUpCallback(void *CallBackRef);

/***************** Macros (Inline Functions) Definitions *********************/

/*****************************************************************************/
/**
 * This function registers the user defined delay/sleep function with subsystem
 *
 * @param  InstancePtr is a pointer to the Subsystem instance
 * @param  CallbackFunc is the address of user defined delay function
 * @param  CallbackRef is the pointer to timer instance
 * @return None
 *
 *****************************************************************************/
void XInss_SetUserTimerHandler(XInss *InstancePtr,
		                       XVidC_DelayHandler CallbackFunc,
		                       void  *CallbackRef)
{
  /* Verify arguments */
  Xil_AssertVoid(InstancePtr != NULL);
  Xil_AssertVoid(CallbackFunc != NULL);
  Xil_AssertVoid(CallbackRef != NULL);

  XV_HdmiRxSs_SetUserTimerHandler(InstancePtr->Hdmi.HdmiRxSsPtr,
		                          CallbackFunc,
		                          CallbackRef);
}

/*****************************************************************************/
/**
 * This function registers the System Interrupt controller with subsystem
 *
 * @param  IntcPtr is a pointer to the system interrupt controller instance
 * @return None
 *
 *****************************************************************************/
void XInss_RegisterSysIntc(XInss *InstancePtr, void *IntcPtr)
{
  InstancePtr->SysIntcPtr = IntcPtr;
}


/*****************************************************************************/
/**
 * This function initializes input subsystem.
 *
 * @param  InstancePtr is a pointer to the Subsystem instance to be worked on.
 *
 * @return XST_SUCCESS/XST_FAILURE
 *
 *****************************************************************************/
int XInss_PowerOnInit(XInss *InstancePtr)
{
  int Status;
  XV_HdmiRxSs_Config *HdmiRxSsConfigPtr;
  XV_HdmiRxSs *HdmiRxSsPtr;

  /* Verify arguments */
  Xil_AssertNonvoid(InstancePtr != NULL);
  Xil_AssertNonvoid(InstancePtr->Hdmi.HdmiRxSsPtr != NULL);
  Xil_AssertNonvoid(InstancePtr->Hdmi.VphyPtr != NULL);
  Xil_AssertNonvoid(InstancePtr->SysIntcPtr != NULL);

  HdmiRxSsConfigPtr = XV_HdmiRxSs_LookupConfig(XPAR_V_HDMI_RX_SS_0_V_HDMI_RX_DEVICE_ID);
  if(HdmiRxSsConfigPtr == NULL) {
	  InstancePtr->Hdmi.HdmiRxSsPtr->IsReady = 0;
	  xil_printf("ERR INSS:: HDMI RX Subsystem device not found\r\n");
	  return (XST_DEVICE_NOT_FOUND);
  }

  HdmiRxSsPtr = InstancePtr->Hdmi.HdmiRxSsPtr;

  //Initialize top level and all included sub-cores
  Status = XV_HdmiRxSs_CfgInitialize(HdmiRxSsPtr,
		                             HdmiRxSsConfigPtr,
		                             HdmiRxSsConfigPtr->BaseAddress);
  if(Status != XST_SUCCESS) {
    xil_printf("ERR INSS:: HDMI RX Subsystem Initialization failed %d\r\n", Status);
    return(XST_FAILURE);
  }

  /* Load EDID */
  XV_HdmiRxSs_SetEdidParam(HdmiRxSsPtr, (u8*)&Edid, sizeof(Edid));
  XV_HdmiRxSs_LoadDefaultEdid(HdmiRxSsPtr);

  //Register HDMI RX SS Interrupt Handler with Interrupt Controller
#if defined(__arm__)
  Status |= XScuGic_Connect(InstancePtr->SysIntcPtr,
		                    XPAR_FABRIC_V_HDMI_RX_SS_0_IRQ_INTR,
	  		                (XInterruptHandler)XV_HdmiRxSS_HdmiRxIntrHandler,
	  		                (void *)HdmiRxSsPtr);
#else
  Status |= XIntc_Connect(InstancePtr->SysIntcPtr,
		                  XPAR_PROCESSOR_SS_AXI_INTC_0_V_HDMI_RX_SS_0_IRQ_INTR,
	  		              (XInterruptHandler)XV_HdmiRxSS_HdmiRxIntrHandler,
	  		              (void *)HdmiRxSsPtr);
#endif

  if (Status == XST_SUCCESS){
#if defined(__arm__)
	  XScuGic_Enable(InstancePtr->SysIntcPtr,
			         XPAR_FABRIC_V_HDMI_RX_SS_0_IRQ_INTR);
#else
	  XIntc_Enable(InstancePtr->SysIntcPtr,
			       XPAR_PROCESSOR_SS_AXI_INTC_0_V_HDMI_RX_SS_0_IRQ_INTR);
#endif
  } else {
	  xil_printf("INSS ERR:: Unable to register HDMI RX interrupt handler");
	  return XST_FAILURE;
  }

  /* Set Application Level Call Backs */
  XV_HdmiRxSs_SetCallback(HdmiRxSsPtr,
		                  XV_HDMIRXSS_HANDLER_CONNECT,
		                  RxConnectCallback,
		                  &InstancePtr->Hdmi);

  XV_HdmiRxSs_SetCallback(HdmiRxSsPtr,
		                  XV_HDMIRXSS_HANDLER_LNKSTA,
		                  RxLnkStaCallback,
		                  &InstancePtr->Hdmi);

  XV_HdmiRxSs_SetCallback(HdmiRxSsPtr,
		                  XV_HDMIRXSS_HANDLER_STREAM_DOWN,
		                  RxStreamDownCallback,
		                  &InstancePtr->Hdmi);

  XV_HdmiRxSs_SetCallback(HdmiRxSsPtr,
		                  XV_HDMIRXSS_HANDLER_STREAM_INIT,
		                  RxStreamInitCallback,
		                  &InstancePtr->Hdmi);

  XV_HdmiRxSs_SetCallback(HdmiRxSsPtr,
		                  XV_HDMIRXSS_HANDLER_STREAM_UP,
		                  RxStreamUpCallback,
		                  &InstancePtr->Hdmi);

  XEvnthdlr_ClearEvent(pSysEvent, XEVENT_HDMI_INPUT_LOCKED);

  return(XST_SUCCESS);
}

/*****************************************************************************/
/**
 * This function start the Input subsystem.
 *
 * @param  InstancePtr is a pointer to the Subsystem instance to be worked on.
 *       
 * @return None
 *
 *****************************************************************************/
void XInss_Start(XInss *InstancePtr)
{
  Xil_AssertVoid(InstancePtr != NULL);

  xil_printf("  ->Start Input Subsystem.... \r\n");

  if(XInss_GetInputSourceSel(InstancePtr) == XINSS_SOURCE_HDMI_RX) {
    XV_HdmiRxSs_Start(InstancePtr->Hdmi.HdmiRxSsPtr);
  } else {
    //NOP
  }
}

/*****************************************************************************/
/**
 * This function stops the Input subsystem
 *
 * @param  InstancePtr is a pointer to the Subsystem instance to be worked on.
 *
 * @return None
 *
 *****************************************************************************/
void XInss_Stop(XInss *InstancePtr)
{
  Xil_AssertVoid(InstancePtr != NULL);
  xil_printf("  ->Stop Input Subsystem.... \r\n");

  if(XInss_GetInputSourceSel(InstancePtr) == XINSS_SOURCE_HDMI_RX) {
    XV_HdmiRxSs_Stop(InstancePtr->Hdmi.HdmiRxSsPtr);
  }
}

/*****************************************************************************/
/**
 * This function resets the Input subsystem
 *
 * @param  InstancePtr is a pointer to the Subsystem instance to be worked on.
 *
 * @return None
 *
 *****************************************************************************/
void XInss_Reset(XInss *InstancePtr)
{
  Xil_AssertVoid(InstancePtr != NULL);

  /* Assert Subsystem reset */
  if(XInss_GetInputSourceSel(InstancePtr) == XINSS_SOURCE_HDMI_RX) {
    XV_HdmiRxSs_Reset(InstancePtr->Hdmi.HdmiRxSsPtr);
  } else {
	//NOP
  }
}

/*****************************************************************************/
/**
 * This function sets the active input
 *
 * @param  InstancePtr is pointer to the input subsystem
 * @param  Source is the active source user selected
 * @return none
 *
 *****************************************************************************/
void XInss_SetActiveSource(XInss *InstancePtr, XInput_Sel Source)
{
  switch(Source)
  {
    case XINSS_SOURCE_TPG:
        xil_printf("\r\nINFO> TPG is set as active source\r\n");
    	break;

    case XINSS_SOURCE_HDMI_RX:
        xil_printf("\r\nINFO> HDMI Rx is set as active source\r\n");
        if(XInss_IsStreamValid(InstancePtr)) {
   	      XEvnthdlr_GenEvent(pSysEvent, XEVENT_HDMI_INPUT_LOCKED);
        } else {
     	  XEvnthdlr_GenEvent(pSysEvent, XEVENT_HDMI_NO_INPUT);
        }
    	break;

    default:
    	break;
  }
  InstancePtr->InputSrcSel = Source;
}


/*****************************************************************************/
/**
 * This function start the Input subsystem by releasing any pending resets
 *
 * @param  InstancePtr is a pointer to the Subsystem instance to be worked on.
 *
 * @return None
 *
 *****************************************************************************/
void XInss_StartStream(XInss *InstancePtr)
{
  Xil_AssertVoid(InstancePtr != NULL);

  //HDMI - nop
}

/*****************************************************************************/
/**
 * This function extracts the detected input from subsystem
 *
 * @param  InstancePtr is a pointer to the Subsystem instance to be worked on.
 * @param  Stream is a pointer to user defined video stream structure where
 *         detected stream information is read into
 *
 * @return XST_SUCCESS
 *
 *****************************************************************************/
u32 XInss_GetStreamInfo(XInss *InstancePtr, XVidC_VideoStream *Stream)
{
  XVidC_VideoStream *InStrmPtr;
  XInput_Sel ActiveSource;
  u32 Status;

  ActiveSource = XInss_GetInputSourceSel(InstancePtr);

  switch(ActiveSource)
  {
    case XINSS_SOURCE_HDMI_RX:
   	    xil_printf("\r\nINSS INFO> Input Source -> HDMI\r\n");
        InStrmPtr = XV_HdmiRxSs_GetVideoStream(InstancePtr->Hdmi.HdmiRxSsPtr);

        if((InStrmPtr->VmId < XVIDC_VM_NUM_SUPPORTED) ||
           (InStrmPtr->VmId == XVIDC_VM_CUSTOM)) {
      	   //copy input detected stream to user defined stream pointer
          memcpy(Stream, InStrmPtr, sizeof(XVidC_VideoStream));
          Status = TRUE;
        } else {
          xil_printf("INSS ERR:: Input Resolution %d x %d @ %dHz Not Supported\n\r",
          		InStrmPtr->Timing.HActive,InStrmPtr->Timing.VActive, InStrmPtr->FrameRate);
          Status = FALSE;
        }
    	break;

    default:
        xil_printf("INSS ERR:: Unknown Input source\n\r");
        Status = FALSE;
    	break;
  }

  return(Status);
}

/*****************************************************************************/
/**
* This function is called when a RX link status irq has occurred.
*
* @param CallBackRef is ptr to subsystem instance that generated the interrupt
*
* @return None
*
*****************************************************************************/
static void RxLnkStaCallback(void *CallBackRef)
{
  //NOP
}

/*****************************************************************************/
/**
*
* This function is called when the RX stream init
*
* @param CallBackRef is ptr to subsystem instance that generated the interrupt
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static void RxStreamInitCallback(void *CallBackRef)
{
  XInss *pInss = (XInss *)CallBackRef;
  XVidC_VideoStream *HdmiRxSsVidStreamPtr;
  XVidC_ColorDepth	ColorDepth;
  u32 Status;

  /* Calculate RX MMCM parameters
	  - HDMI transports YUV422 in 8 bits.
	  - Force color depth to 8 bits when the format is YUV422
  */
  HdmiRxSsVidStreamPtr = XV_HdmiRxSs_GetVideoStream(pInss->Hdmi.HdmiRxSsPtr);

  ColorDepth = ((HdmiRxSsVidStreamPtr->ColorFormatId == XVIDC_CSF_YCRCB_422)
		                 ? XVIDC_BPC_8
		                 : HdmiRxSsVidStreamPtr->ColorDepth);
  Status = XVphy_HdmiCfgCalcMmcmParam(pInss->Hdmi.VphyPtr,
				                      0,
				                      XVPHY_CHANNEL_ID_CH1,
				                      XVPHY_DIR_RX,
				                      HdmiRxSsVidStreamPtr->PixPerClk,
									  ColorDepth);
  if( Status == XST_FAILURE) {
	  xil_printf("INSS ERR:: Rx Stream Init Failed \r\n;");
	  return;
  }

  // Enable and configure RX MMCM
  XVphy_MmcmStart(pInss->Hdmi.VphyPtr, 0, XVPHY_DIR_RX);
}

/*****************************************************************************/
/**
*
* This function is called when a RX stream is up.
*
* @param CallBackRef is ptr to subsystem instance that generated the interrupt
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static void RxStreamUpCallback(void *CallBackRef)
{
  XEvnthdlr_ClearEvent(pSysEvent, XEVENT_HDMI_NO_INPUT);
  XEvnthdlr_GenEvent(pSysEvent, XEVENT_HDMI_INPUT_LOCKED);
}

/*****************************************************************************/
/**
*
* This function is called when a RX stream is down.
*
* @param CallBackRef is ptr to subsystem instance that generated the interrupt
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static void RxStreamDownCallback(void *CallBackRef)
{
  XEvnthdlr_ClearEvent(pSysEvent, XEVENT_HDMI_INPUT_LOCKED);
  XEvnthdlr_GenEvent(pSysEvent, XEVENT_HDMI_NO_INPUT);
}

/*****************************************************************************/
/**
*
* This function is called when a RX cable is connected/disconnected
*
* @param CallBackRef is ptr to subsystem instance that generated the interrupt
*
* @return None.
*
* @note   None.
*
******************************************************************************/
static void RxConnectCallback(void *CallBackRef)
{
  XInss *pInss = (XInss *)CallBackRef;

  // Is the cable connected?
  if (pInss->Hdmi.HdmiRxSsPtr->IsStreamConnected) {
	XVphy_IBufDsEnable(pInss->Hdmi.VphyPtr, 0, XVPHY_DIR_RX, (TRUE));
  } else {// RX cable is disconnected
	pInss->Hdmi.VphyPtr->HdmiRxTmdsClockRatio = 0; // Clear GT RX TMDS clock ratio
	XVphy_IBufDsEnable(pInss->Hdmi.VphyPtr, 0, XVPHY_DIR_RX, (FALSE));
  }
  XEvnthdlr_ClearEvent(pSysEvent, XEVENT_HDMI_INPUT_LOCKED);
  XEvnthdlr_GenEvent(pSysEvent, XEVENT_HDMI_NO_INPUT);
}
