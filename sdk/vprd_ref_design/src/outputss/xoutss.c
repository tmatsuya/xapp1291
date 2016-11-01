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
* @file xoutss.c
*
* This is main code of Output Subsystem device driver based on imageon core
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
#include <string.h>
#include "../platform_config.h"
#include "xparameters.h"
#if defined(__arm__)
#include "sleep.h"
#elif defined(__MICROBLAZE__)
#include "microblaze_sleep.h"
#endif
#include "xoutss.h"
#include "xiic.h"
#include "si5324drv.h"
#include "dp159.h"
#include "../eventhndlr.h"
#include "vidpatgen.h"

/************************** Constant Definitions *****************************/
#define I2C_CLK_ADDR		    0x68	/**< I2C Clk Address */
#define MIXER_LOAD_VIDEO_DATA   (1)

#define MIXER_RESET_CH          (1)
/**************************** Type Definitions *******************************/

/**************************** Local Global *******************************/
static XV_Mix_l2 Mixer;
static XGpio MixerResetGpio;

/* Default Mixer Layer Configuration */
static const FrameBuf DefaultMixLayerConfig[7] =
{// FW   FH    X    Y     W    H    #F   MA   Color Format  Head
  {1600, 542, {0,   0,   960, 540}, 600, {0}, XVIDC_CSF_RGB, 0}, //Layer 1
  {960,  540, {960, 0,   960, 540}, 1,   {0}, XVIDC_CSF_RGB, 0}, //Layer 2
  {960,  540, {960, 540, 960, 540}, 1,   {0}, XVIDC_CSF_RGB, 0}, //Layer 3
  {256,  256, {12,  10,  256, 256}, 60,  {0}, XVIDC_CSF_RGB, 0}, //Layer 4
  {256,  256, {600, 10,  256, 256}, 60,  {0}, XVIDC_CSF_RGB, 0}, //Layer 5
  {256,  256, {12,  500, 256, 256}, 60,  {0}, XVIDC_CSF_RGB, 0}, //Layer 6
  {256,  256, {600, 500, 256, 256}, 60,  {0}, XVIDC_CSF_RGB, 0}, //Layer 7
};

static unsigned char Logo_R[];
static unsigned char Logo_G[];
static unsigned char Logo_B[];

static unsigned char imagedata[];
static unsigned char graphicsdata[];

/************************** Function Prototypes ******************************/
static int I2cClk(u32 Frequency);
static void TxConnectCallback(void * CallBackRef);
static void TxVsCallback(void * CallBackRef);
static void TxStreamUpCallback(void *CallBackRef);
static void MixerFrameDoneCallback(void *CallBackRef);
static void LoadVideoDataToMem(XOutss *InstancePtr);
static void ResetMixer(XGpio *ResetPtr, u32 Channel, u32 Reset);

/***************** Macros (Inline Functions) Definitions *********************/
/*****************************************************************************/
/**
 * This function registers the user defined delay/sleep function with subsystem
 *
 * @param  InstancePtr is a pointer to the System Output instance
 * @param  CallbackFunc is the address of user defined delay function
 * @param  CallbackRef is the pointer to timer instance
 *
 * @return None
 *
 *****************************************************************************/
void XOutss_SetUserTimerHandler(XOutss *InstancePtr,
		                        XVidC_DelayHandler CallbackFunc,
		                        void *CallbackRef)
{
  Xil_AssertVoid(InstancePtr != NULL);
  Xil_AssertVoid(CallbackFunc != NULL);
  Xil_AssertVoid(CallbackRef != NULL);

  XV_HdmiTxSs_SetUserTimerHandler(InstancePtr->Hdmi.HdmiTxSsPtr,
		                          CallbackFunc,
		                          CallbackRef);
}

/*****************************************************************************/
/**
 * This function registers the System Interrupt controller with subsystem
 *
 * @param  InstancePtr is a pointer to the Output instance
 * @param  IntcPtr is a pointer to the system interrupt controller instance
 *
 * @return None
 *
 *****************************************************************************/
void XOutss_RegisterSysIntc(XOutss *InstancePtr, void *IntcPtr)
{
  InstancePtr->SysIntcPtr = IntcPtr;
}

/*****************************************************************************/
/**
 * This function initializes output subsystem.
 *
 * @param  InstancePtr is a pointer to the System Output instance
 *
 * @return XST_SUCCESS/XST_FAILURE
 *
 *****************************************************************************/
int XOutss_PowerOnInit(XOutss *InstancePtr)
{
  int Status;
  XV_HdmiTxSs_Config *HdmiTxSsConfigPtr;
  XV_HdmiTxSs *HdmiTxSsPtr;
  XGpio_Config *GpioCfgPtr;

  /* Verify arguments */
  Xil_AssertNonvoid(InstancePtr != NULL);
  Xil_AssertNonvoid(InstancePtr->Hdmi.HdmiTxSsPtr != NULL);
  Xil_AssertNonvoid(InstancePtr->Hdmi.VphyPtr != NULL);
  Xil_AssertNonvoid(InstancePtr->SysIntcPtr != NULL);

  HdmiTxSsPtr = InstancePtr->Hdmi.HdmiTxSsPtr;

  //Bind system core instance to ip drivers
  InstancePtr->MixerPtr = &Mixer;
  InstancePtr->MixerResetPtr = &MixerResetGpio;

  /* Initialize external clock generator */
  Si5324_Init(XPAR_PROCESSOR_SS_AXI_IIC_0_BASEADDR, I2C_CLK_ADDR);

  HdmiTxSsConfigPtr = XV_HdmiTxSs_LookupConfig(XPAR_V_HDMI_TX_SS_0_V_HDMI_TX_DEVICE_ID);
  if(HdmiTxSsConfigPtr == NULL) {
	  HdmiTxSsPtr->IsReady = 0;
	  xil_printf("ERR OUTSS:: HDMI TX Subsystem device not found\r\n");
      return (XST_DEVICE_NOT_FOUND);
  }

  //Initialize top level and all included sub-cores
  Status = XV_HdmiTxSs_CfgInitialize(HdmiTxSsPtr,
		                             HdmiTxSsConfigPtr,
		                             HdmiTxSsConfigPtr->BaseAddress);
  if(Status != XST_SUCCESS) {
    xil_printf("ERR OUTSSS:: HDMI TX Subsystem Initialization failed %d\r\n", Status);
    return(XST_FAILURE);
  }

  //Register HDMI TX SS Interrupt Handler with Interrupt Controller
#if defined(__arm__)
  status = XScuGic_Connect(InstancePtr->SysIntcPtr,
		                   XPAR_FABRIC_V_HDMI_TX_SS_0_IRQ_INTR,
  		                   (XInterruptHandler)XV_HdmiTxSS_HdmiTxIntrHandler,
  		                   (void *)HdmiTxSsPtr);
#else
  Status |= XIntc_Connect(InstancePtr->SysIntcPtr,
		                  XPAR_PROCESSOR_SS_AXI_INTC_0_V_HDMI_TX_SS_0_IRQ_INTR,
	  		              (XInterruptHandler)XV_HdmiTxSS_HdmiTxIntrHandler,
	  		              (void *)HdmiTxSsPtr);
#endif

  if (Status == XST_SUCCESS){
#if defined(__arm__)
	  XScuGic_Enable(InstancePtr->SysIntcPtr,
			         XPAR_FABRIC_V_HDMI_TX_SS_0_IRQ_INTR);
#else
	  XIntc_Enable(InstancePtr->SysIntcPtr,
			       XPAR_PROCESSOR_SS_AXI_INTC_0_V_HDMI_TX_SS_0_IRQ_INTR);
#endif
  }
  else {
	  xil_printf("OUTSS ERR:: Unable to register HDMI TX interrupt handler");
	  return XST_FAILURE;
  }

  /* Set Application Level Call Backs */
  XV_HdmiTxSs_SetCallback(HdmiTxSsPtr,
		                  XV_HDMITXSS_HANDLER_CONNECT,
		                  TxConnectCallback,
		                  &InstancePtr->Hdmi);

  XV_HdmiTxSs_SetCallback(HdmiTxSsPtr,
		                  XV_HDMITXSS_HANDLER_VS,
		                  TxVsCallback,
						  &InstancePtr->Hdmi);

  XV_HdmiTxSs_SetCallback(HdmiTxSsPtr,
		                  XV_HDMITXSS_HANDLER_STREAM_UP,
		                  TxStreamUpCallback,
						  &InstancePtr->Hdmi);

  //Mixer Reset
  GpioCfgPtr = XGpio_LookupConfig(XPAR_MIXER_RESET_DEVICE_ID);
  if(GpioCfgPtr == NULL) {
    xil_printf("PERIPH ERR:: Mixer Reset device not found\r\n");
    return(XST_DEVICE_NOT_FOUND);
  }
  Status = XGpio_CfgInitialize(InstancePtr->MixerResetPtr,
  		                       GpioCfgPtr,
  		                       GpioCfgPtr->BaseAddress);

  if(Status != XST_SUCCESS) {
    xil_printf("PERIPH ERR:: Mixer Reset Device Initialization failed %d\r\n", Status);
    return(XST_FAILURE);
  }

  //Release Mixer Reset before it can be initialized
  ResetMixer(InstancePtr->MixerResetPtr, MIXER_RESET_CH, FALSE);

  //Mixer
  Status = XVMix_Initialize(InstancePtr->MixerPtr, XPAR_V_MIX_0_DEVICE_ID);
  if(Status != XST_SUCCESS) {
    xil_printf("OUTSS ERR:: Mixer device not found\r\n");
    return(XST_DEVICE_NOT_FOUND);
  }

  /* Connect mixer interrupt service routine */
#if defined(__arm__)
  //Register Mixer ISR
  Status = XScuGic_Connect(InstancePtr->SysIntcPtr,
						   XPAR_FABRIC_V_MIX_0_IRQ_INTR
		                   (XInterruptHandler)XVMix_InterruptHandler,
		                   (void *)InstancePtr->MixerPtr);
#else
  Status = XIntc_Connect(InstancePtr->SysIntcPtr,
		                 XPAR_PROCESSOR_SS_AXI_INTC_0_V_MIX_0_INTERRUPT_INTR,
						 (XInterruptHandler)XVMix_InterruptHandler,
						 InstancePtr->MixerPtr);
#endif

  if (Status != XST_SUCCESS) {
    xil_printf("ERR:: Mixer interrupt connect failed!\r\n");
    return XST_FAILURE;
  }

  /* Enable the interrupt vector at the interrupt controller */
#if defined(__arm__)
  XScuGic_Enable(InstancePtr->SysIntcPtr,
		         XPAR_FABRIC_V_MIX_0_IRQ_INTR);
#else
  XIntc_Enable(InstancePtr->SysIntcPtr,
		       XPAR_PROCESSOR_SS_AXI_INTC_0_V_MIX_0_INTERRUPT_INTR);
#endif

  /* Register Mixer Callback */
  XVMix_SetCallback(InstancePtr->MixerPtr,
		            MixerFrameDoneCallback,
		            InstancePtr);

  /* Load images to DDR for Mixer */
  LoadVideoDataToMem(InstancePtr);

  return(XST_SUCCESS);
}


/*****************************************************************************/
/**
 * This function starts the Outut subsystem.
 *
 * @param  InstancePtr is a pointer to the System Output instance
 *
 * @return None
 *
 *****************************************************************************/
void XOutss_Start(XOutss *InstancePtr)
{
  Xil_AssertVoid(InstancePtr != NULL);

  xil_printf("  ->Start Output Subsystem.... \r\n");
  XV_HdmiTxSs_Start(InstancePtr->Hdmi.HdmiTxSsPtr);
}

/*****************************************************************************/
/**
 * This function stops the Outut subsystem.
 *
 * @param  InstancePtr is a pointer to the System Output instance
 *
 * @return None
 *
 *****************************************************************************/
void XOutss_Stop(XOutss *InstancePtr)
{
  Xil_AssertVoid(InstancePtr != NULL);

  xil_printf("  ->Stop Output Subsystem.... \r\n");
  XV_HdmiTxSs_Stop(InstancePtr->Hdmi.HdmiTxSsPtr);
}

/*****************************************************************************/
/**
 * This function resets the outut subsystem.
 *
 * @param  InstancePtr is a pointer to the System Output instance
 *
 * @return None
 *
 *****************************************************************************/
void XOutss_Reset(XOutss *InstancePtr)
{
  Xil_AssertVoid(InstancePtr != NULL);

  /* Assert Subsystem reset */
  XV_HdmiTxSs_Reset(InstancePtr->Hdmi.HdmiTxSsPtr);
}

/*****************************************************************************/
/**
 * This function sets the active output
 *
 * @param  InstancePtr is a pointer to the System Output instance
 * @param  Sink is the active output user selected
 *
 * @return none
 *
 *****************************************************************************/
void XOutss_SetActiveSink(XOutss *InstancePtr, XOutput_Sel Sink)
{
  switch(Sink)
  {
    case XOUTSS_SINK_HDMI_TX:
        xil_printf("\r\nINFO> HDMI Tx is set as active sink\r\n");
  	    XV_HdmiTxSs_Start(InstancePtr->Hdmi.HdmiTxSsPtr);
    	break;

    default:
        xil_printf("\r\nOUTSS ERR: Unknown sink\r\n");
    	break;
  }
  XOutss_SetOutputSinkSel(InstancePtr, Sink);
}

/*****************************************************************************/
/**
 * This function configures the output subsystem resolution to panel resolution
 *
 * @param  InstancePtr is a pointer to the System Output instance
 * @param  strmOut is a pointer to requested stream
 *
 * @return XST_SUCCESS/XST_FAILURE
 *
 *****************************************************************************/
int XOutss_SetOutputStream(XOutss *InstancePtr, XVidC_VideoStream *strmOut)
{
  int status;
  u32 TmdsClock;
  XVidC_VideoStream *HdmiTxSsVidStreamPtr;
  XVidC_ColorDepth Bpc;
  XOutput_Sel ActiveSink;

  xil_printf( "\r\n->Output Resolution set to %s \n\r", XVidC_GetVideoModeStr(strmOut->VmId));

  ActiveSink = XOutss_GetActiveSink(InstancePtr);
  switch(ActiveSink)
  {
    case XOUTSS_SINK_HDMI_TX:
        HdmiTxSsVidStreamPtr = XV_HdmiTxSs_GetVideoStream(InstancePtr->Hdmi.HdmiTxSsPtr);

        // Disable TX TDMS clock
        XVphy_Clkout1OBufTdsEnable(InstancePtr->Hdmi.VphyPtr, XVPHY_DIR_TX, (FALSE));

    	//Default color depth
    	Bpc = strmOut->ColorDepth;
    	if ((strmOut->ColorFormatId == XVIDC_CSF_RGB) ||
    		(strmOut->ColorFormatId == XVIDC_CSF_YCRCB_444)) {
    
    		if((strmOut->VmId > XVIDC_VM_3840x2160_30_P) &&
    		   (strmOut->VmId != XVIDC_VM_CUSTOM)) {
    			Bpc = XVIDC_BPC_8; //4K RGB/YUV44 @10bit not supported
    		}
    	}

        /* Set HDMI Tx Stream and get TMDS Clk*/
        TmdsClock = XV_HdmiTxSs_SetStream(InstancePtr->Hdmi.HdmiTxSsPtr,
        		                            strmOut->VmId,
        			                        strmOut->ColorFormatId,
        			                        Bpc, NULL);

        if(TmdsClock != 0) {
          /* Set Tx Reference clock */
          InstancePtr->Hdmi.VphyPtr->HdmiTxRefClkHz = TmdsClock;

          /* Set GT TX parameters */
          status = XVphy_SetHdmiTxParam(InstancePtr->Hdmi.VphyPtr,
                                        0, //Quad-id
          		                        XVPHY_CHANNEL_ID_CHA,
          		    				    HdmiTxSsVidStreamPtr->PixPerClk,
          		    				    HdmiTxSsVidStreamPtr->ColorDepth,
          		    				    HdmiTxSsVidStreamPtr->ColorFormatId);
        } else { //Tx could not set the resolution.
          status = XST_FAILURE;
        }

        if(status == XST_SUCCESS) {
           /* Program external clock generator in free running mode */
           status = I2cClk(InstancePtr->Hdmi.VphyPtr->HdmiTxRefClkHz);

        } else {
           xil_printf("\r\nOUTSS ERR:: Unable to set requested video resolution\r\n");
        }
        break;

    default:
        xil_printf("\r\nOUTSS ERR:: Unknown output sink\r\n");
        status = FALSE;
    	break;
  }
  return(status);
}

/*****************************************************************************/
/**
 *
 * This function programs the SI5324 clock generator in free running mode.
 *
 * @param  Frequency is the required frequency to be generated by PLL
 *
 * @return
 *		- XST_FAILURE if error in programming external clock.
 *		- XST_SUCCESS if programmed external clock.
 *
 * @note		None.
 *
 ******************************************************************************/
static int I2cClk(u32 Frequency)
{
  int Status;

  /* Free running mode */
  Status = Si5324_SetClock((XPAR_PROCESSOR_SS_AXI_IIC_0_BASEADDR),
		                   (I2C_CLK_ADDR),
  		                   (SI5324_CLKSRC_XTAL),
  		                   (SI5324_XTAL_FREQ),
						   Frequency);

  if(Status != (SI5324_SUCCESS)) {
  	xil_printf("OUTSS ERR:: Error programming External Clock Generator (SI5324)\n\r");
  	return(XST_FAILURE);
  }

  return(XST_SUCCESS);
}

/*****************************************************************************/
/**
 * This function is called when a TX connect event has occurred.
 *
 * @param CallBackRef is ptr to subsystem instance that generated the interrupt
 *
 * @return None
 *
 *****************************************************************************/
static void TxConnectCallback(void *CallBackRef)
{
  XOutss *pOutpss = (XOutss *)CallBackRef;

  if(XOutss_GetActiveSink(pOutpss) == XOUTSS_SINK_HDMI_TX) {

    if(pOutpss->Hdmi.HdmiTxSsPtr->IsStreamConnected) {
  	   /* Check HDMI sink version */
  	   XV_HdmiTxSs_DetectHdmi20(pOutpss->Hdmi.HdmiTxSsPtr);
  	   XVphy_IBufDsEnable(pOutpss->Hdmi.VphyPtr, 0, XVPHY_DIR_TX, (TRUE));
  	} else {
       XVphy_IBufDsEnable(pOutpss->Hdmi.VphyPtr, 0, XVPHY_DIR_TX, (FALSE));
    }
  }
}

/*****************************************************************************/
/**
 * This function is called when a TX vsync has occurred.
 *
 * @param CallBackRef is ptr to subsystem instance that generated the interrupt
 *
 * @return None
 *
 *****************************************************************************/
static void TxVsCallback(void *CallBackRef)
{
  XEvnthdlr_GenEvent(pSysEvent, XEVENT_SYS_VSYNC);
}

/*****************************************************************************/
/**
 *
 * This function is called when the TX stream is up.
 *
 * @param CallBackRef is ptr to subsystem instance that generated the interrupt
 *
 * @return None
 *
 ******************************************************************************/
static void TxStreamUpCallback(void *CallBackRef)
{
  XOutss *pOutpss = (XOutss *)CallBackRef;
  XVphy_PllType TxPllType;
  u64 TxLineRate;

  if(XOutss_GetActiveSink(pOutpss) == XOUTSS_SINK_HDMI_TX) {

    TxPllType = XVphy_GetPllType(pOutpss->Hdmi.VphyPtr, 0, XVPHY_DIR_TX, XVPHY_CHANNEL_ID_CH1);
    if ((TxPllType == XVPHY_PLL_TYPE_CPLL)) {
  	  TxLineRate = pOutpss->Hdmi.VphyPtr->Quads[0].Plls[0].LineRateHz;
    }
    else if((TxPllType == XVPHY_PLL_TYPE_QPLL0)){
  	  TxLineRate = pOutpss->Hdmi.VphyPtr->Quads[0].Plls[XVPHY_CHANNEL_ID_CMN0 -
  	                                     		  XVPHY_CHANNEL_ID_CH1].LineRateHz;
    }
    else {
  	  TxLineRate = pOutpss->Hdmi.VphyPtr->Quads[0].Plls[XVPHY_CHANNEL_ID_CMN1 -
  	                                     		  XVPHY_CHANNEL_ID_CH1].LineRateHz;
    }

    i2c_dp159(pOutpss->Hdmi.VphyPtr, 0, TxLineRate);

    /* Enable TX TMDS clock*/
    XVphy_Clkout1OBufTdsEnable(pOutpss->Hdmi.VphyPtr, XVPHY_DIR_TX, (TRUE));

    /* Copy Sampling Rate */
    XV_HdmiTxSs_SetSamplingRate(pOutpss->Hdmi.HdmiTxSsPtr,
    		                    pOutpss->Hdmi.VphyPtr->HdmiTxSampleRate);

    XEvnthdlr_GenEvent(pSysEvent, XEVENT_HDMI_OUTPUT_LOCKED);
  }
}

/*****************************************************************************/
/**
 * This function configures mixer after reset
 *
 * @param  InstancePtr is a pointer to the System Output instance
 * @param  StrmOut is the pointer to output stream
 *
 * @return none
 *
 *****************************************************************************/
void XOutss_SetupMixer(XOutss *InstancePtr, XVidC_VideoStream *StrmOut)
{
  XVidC_VideoWindow Win;
  int index, Status;
  u32 Stride, NumLayers, MemAddr;
  u32 BytesPerPixel;
  XVMix_LogoColorKey Data ={{10,10,10},{40,40,40}};

  Xil_AssertVoid(InstancePtr != NULL);
  Xil_AssertVoid(StrmOut != NULL);

  xil_printf("INFO> Setup Mixer \r\n");

  /* Toggle Mixer Reset */
  ResetMixer(InstancePtr->MixerResetPtr, MIXER_RESET_CH, TRUE);
  ResetMixer(InstancePtr->MixerResetPtr, MIXER_RESET_CH, FALSE);

  /* Set Master Stream */
  XVMix_SetVidStream(InstancePtr->MixerPtr, StrmOut);

  /* Set Memory Layer Base Addresses */
  NumLayers = XVMix_GetNumLayers(InstancePtr->MixerPtr);
  MemAddr = XVMIX_LAYER1_BASEADDR;
  for(index = XVMIX_LAYER_1; index < NumLayers; ++index) {

	  if(!XVMix_IsLayerInterfaceStream(InstancePtr->MixerPtr, index)) {
	    Status = XVMix_SetLayerBufferAddr(InstancePtr->MixerPtr, index, MemAddr);
	    if(Status != XST_SUCCESS) {
		   xil_printf("MIXER ERR:: Unable to set layer %d buffer addr to 0x%X\r\n",
				      index, MemAddr);
	    } else {
	       MemAddr += XVMIX_LAYER_ADDR_OFFSET;
	    }
	  }
  }

  Win.StartX = StrmOut->Timing.HActive - 300;
  Win.StartY = 50;
  Win.Width  = 64;
  Win.Height = 64;

  XVMix_LoadLogo(InstancePtr->MixerPtr,
		         &Win,
	             Logo_R,
	             Logo_G,
	             Logo_B);
  XVMix_SetLogoColorKey(InstancePtr->MixerPtr, Data);
  XVMix_SetLayerAlpha(InstancePtr->MixerPtr, XVMIX_LAYER_LOGO, XVMIX_ALPHA_MAX);
  XVMix_SetBackgndColor(InstancePtr->MixerPtr, XVMIX_BKGND_BLUE, StrmOut->ColorDepth);

  /* Set Default layer windows */
  for(index=XVMIX_LAYER_1; index<NumLayers; ++index) {

    Win.Width  = InstancePtr->MixerFrameBuf[index-1].Win.Width;
	Win.Height = InstancePtr->MixerFrameBuf[index-1].Win.Height;
  	Win.StartX = InstancePtr->MixerFrameBuf[index-1].Win.StartX;
  	Win.StartY = InstancePtr->MixerFrameBuf[index-1].Win.StartY;

  	BytesPerPixel = ((InstancePtr->MixerFrameBuf[index-1].CFmt == XVIDC_CSF_YCRCB_422) ? 2 : 4);
  	Stride = InstancePtr->MixerFrameBuf[index-1].FrameWidth * BytesPerPixel;

    Status = XVMix_SetLayerWindow(InstancePtr->MixerPtr, index, &Win, Stride);
    if(Status != XST_SUCCESS) {
    	xil_printf("Unable to Set Layer %d window (%d, %d, %d, %d)\r\n", index,
    			 Win.StartX, Win.StartY, Win.Width, Win.Height);
    }
 	XVMix_SetLayerAlpha(InstancePtr->MixerPtr, index, XVMIX_ALPHA_MAX);
  }

  XVMix_InterruptEnable(InstancePtr->MixerPtr);
  /* Set autostart bit */
  XV_mix_EnableAutoRestart(&InstancePtr->MixerPtr->Mix);

  XVMix_Start(InstancePtr->MixerPtr);
}

/*****************************************************************************/
/**
 * This function updates mixer layer window coordinates for PIP mode
 * functionality
 *
 * @param  InstancePtr is a pointer to the System Output instance
 * @param  StrmOut is the pointer to output stream
 * @param  PipModeOn is the state of vpss Pip Mode feature
 *
 * @return none
 *
 *****************************************************************************/
void XOutss_UpdateMixerWindow(XOutss *InstancePtr,
		                      XVidC_VideoStream *StrmOut,
					          u32 PipModeOn)
{
  XVidC_VideoWindow Win;
  int index, Status;
  u32 Stride, NumLayers;
  u32 BytesPerPixel, Align;

  Xil_AssertVoid(InstancePtr != NULL);

  NumLayers = XVMix_GetNumLayers(InstancePtr->MixerPtr);

  /* Set Default layer windows */
  for(index=XVMIX_LAYER_1; index<NumLayers; ++index) {

	if((PipModeOn) && (index <XVMIX_LAYER_4)) {
	    Win.Width  = InstancePtr->MixerFrameBuf[index-1].Win.Width;
		Win.Height = InstancePtr->MixerFrameBuf[index-1].Win.Height;

		switch(index)
		{
		  case XVMIX_LAYER_1:
		  	  Win.StartY = InstancePtr->MixerFrameBuf[index-1].Win.StartY;
		  	  Win.StartX = StrmOut->Timing.HActive>>1;
		  	  Align = InstancePtr->MixerPtr->Mix.Config.PixPerClk;
		  	  Win.StartX = (((Win.StartX + (Align-1))/Align) * Align);
			  break;

		  case XVMIX_LAYER_2:
			  Win.StartX = 0;
		  	  Win.StartY = StrmOut->Timing.VActive>>1;
		  	  break;

		  case XVMIX_LAYER_3:
		  	  Win.StartY = StrmOut->Timing.VActive>>1;
		  	  Win.StartX = StrmOut->Timing.HActive>>1;
		  	  Align = InstancePtr->MixerPtr->Mix.Config.PixPerClk;
		  	  Win.StartX = (((Win.StartX + (Align-1))/Align) * Align);
		  	  break;

		  default:
			  break;
		}
	} else {
	    Win.Width  = InstancePtr->MixerFrameBuf[index-1].Win.Width;
		Win.Height = InstancePtr->MixerFrameBuf[index-1].Win.Height;
	  	Win.StartX = InstancePtr->MixerFrameBuf[index-1].Win.StartX;
	  	Win.StartY = InstancePtr->MixerFrameBuf[index-1].Win.StartY;
	}

  	BytesPerPixel = ((InstancePtr->MixerFrameBuf[index-1].CFmt == XVIDC_CSF_YCRCB_422) ? 2 : 4);
  	Stride = InstancePtr->MixerFrameBuf[index-1].FrameWidth * BytesPerPixel;

    Status = XVMix_SetLayerWindow(InstancePtr->MixerPtr, index, &Win, Stride);
    if(Status != XST_SUCCESS) {
    	xil_printf("Unable to Set Layer %d window (%d, %d, %d, %d)\r\n", index,
    			 Win.StartX, Win.StartY, Win.Width, Win.Height);
    }
  }

  /* When PIP mode is disabled, disable all mixer Layers */
  if(!PipModeOn) {
     for(index=XVMIX_LAYER_1; index<XVMIX_LAYER_4; ++index) {
    	 XVMix_LayerDisable(InstancePtr->MixerPtr, index);
     }
  }
}

/*****************************************************************************/
/**
 *
 * This function is called when the Mixer has processed frame
 *
 * @param CallBackRef is ptr to mixer instance that generated the interrupt
 *
 * @return None
 *
 ******************************************************************************/
static void MixerFrameDoneCallback(void *CallBackRef)
{
  XOutss *pOutpss = (XOutss *)CallBackRef;
  FrameBuf *BufPtr;
  u32 MemAddr, HeadIndex;
  u32 NumLayers, index;

  /* Every frame update memory address to next frame boundary for each layer
   */
  NumLayers = XVMix_GetNumLayers(pOutpss->MixerPtr);
  for(index=XVMIX_LAYER_1; index<NumLayers; ++index) {

	 /* Update buffer pointer only if layer is enabled and
	  * interface type is memory
	  */
	 if(XVMix_IsLayerEnabled(pOutpss->MixerPtr, index) &&
		!XVMix_IsLayerInterfaceStream(pOutpss->MixerPtr, index)) {

	   BufPtr = &pOutpss->MixerFrameBuf[index-1];

	   //Update Head Index for layer
	   HeadIndex = BufPtr->HeadPos;
	   HeadIndex = ((HeadIndex+1) % BufPtr->NumFrames);
	   BufPtr->HeadPos = HeadIndex;

	   //Get next frame address for the layer
	   MemAddr = BufPtr->MemAddr[HeadIndex];

	   //update buffer pointer for layer
	   XVMix_SetLayerBufferAddr(pOutpss->MixerPtr, index, MemAddr);
	 }
  }
}

/*****************************************************************************/
/**
 *
 * This function defines the default layer window properties and if enabled
 * loads test pattern data to DDR for Mixer memory layer
 *
 * @param  InstancePtr is a pointer to the System Output instance
 *
 * @return None
 *
 ******************************************************************************/
static void LoadVideoDataToMem(XOutss *InstancePtr)
{
  FrameBuf *BufCfgPtr;
  int NumLayers, index;
  int count;
  int Stride, PixIncr;
  int StartPos;
  int BytesPerPixel;
  int AlignBytes;
  XVidC_ColorFormat CFmt;
  u32 MemAddr;

  NumLayers = XVMix_GetNumLayers(InstancePtr->MixerPtr);

#if MIXER_LOAD_VIDEO_DATA
  xil_printf("\r\nLoad Video Data to DDR for %d Mixer Layers.....\r\n", (NumLayers-1));
#else
  xil_printf("\r\nLoading Mixer Layers Default Configuration.....\r\n");
#endif

  for(index=XVMIX_LAYER_1; index<NumLayers; ++index) {

	 BufCfgPtr = &InstancePtr->MixerFrameBuf[index-1];

	 //Copy default configuration
	 *BufCfgPtr = DefaultMixLayerConfig[index-1];

	 //Get layer Color Format from HW
	 XVMix_GetLayerColorFormat(InstancePtr->MixerPtr, index, &CFmt);
	 BufCfgPtr->CFmt = CFmt;

	 if(XVMix_IsLayerInterfaceStream(InstancePtr->MixerPtr, index)) {
		 xil_printf("   - Layer %d is Stream. Skip loading frame...\r\n", index);
		 continue;
	 }

	 AlignBytes = 2 * InstancePtr->MixerPtr->Mix.Config.PixPerClk * 4;
	 BytesPerPixel = ((BufCfgPtr->CFmt == XVIDC_CSF_YCRCB_422) ? 2 : 4);
	 Stride = BufCfgPtr->FrameWidth * BytesPerPixel;

	 if(index < XVMIX_LAYER_3) { //Load pre-defined images
		   char *dst;
		   int x,y;

		   xil_printf("Load %dx%d Image for Layer %d....\r\n",
				       BufCfgPtr->FrameWidth, BufCfgPtr->FrameHeight, index);
	       Stride = BufCfgPtr->FrameWidth * BytesPerPixel;
	       MemAddr = (XVMIX_LAYER1_BASEADDR + ((index-1) * XVMIX_LAYER_ADDR_OFFSET));
	       dst = (char *)MemAddr;
	       for(y=0; y<BufCfgPtr->FrameHeight; ++y) {
	    	   for(x=0; x<Stride; ++x) {
	    		   dst[y*Stride+x] = ((index == 1) ? imagedata[y*Stride+x]
												   : graphicsdata[y*Stride+x]);
	    	   }
	       }
	       for(count=0; count<BufCfgPtr->NumFrames; ++count)
	       {
	    	 //Align memory address per IP requirement
	         BufCfgPtr->MemAddr[count] = MemAddr;
	         MemAddr = (MemAddr+AlignBytes);
	       }
     } else { //if #Layers > 3

#if MIXER_LOAD_VIDEO_DATA
       xil_printf("\r\n-->Loading %2d frames for Layer %d\r\n",
    		       BufCfgPtr->NumFrames, index);
#else
       Stride = Stride; //to squash compiler warning
#endif

       PixIncr = index; //Each layer will have different speed
       StartPos = 0;
       MemAddr = (XVMIX_LAYER1_BASEADDR + ((index-1) * XVMIX_LAYER_ADDR_OFFSET));
       for(count=0; count<BufCfgPtr->NumFrames; ++count)
       {
#if MIXER_LOAD_VIDEO_DATA
   	     xil_printf("   Load Frame %2d @0x%X...\r\n", count, MemAddr);
#endif
   	     //Align memory address per IP requirement
         MemAddr = ((MemAddr + AlignBytes) & ~(AlignBytes));
         BufCfgPtr->MemAddr[count] = MemAddr;

#if MIXER_LOAD_VIDEO_DATA
         //Write pattern to memory
         PatGen_ColorBars((char *)MemAddr,
   			              BufCfgPtr->Win.Width,
   			              BufCfgPtr->Win.Height,
   			              Stride,
   			              StartPos,
   			              BufCfgPtr->CFmt);
#endif

         //Adjust start position for next frame
   	     StartPos = ((StartPos+PixIncr) % BufCfgPtr->Win.Width);

   	     //Compute next frame address
         MemAddr += BufCfgPtr->Win.Width*BufCfgPtr->Win.Height*BytesPerPixel;
       }
    }
  } //for layer 1..N
}

/*****************************************************************************/
/**
 *
 * This function resets the mixer block
 *
 * @param ResetPtr is ptr to reset block
 * @param channel is the GPIO channel id
 * @param Reset specifies TRUE/FALSE value to either assert or release mixer
 *        reset
 *
 * @return None
 *
 ******************************************************************************/
static void ResetMixer(XGpio *ResetPtr, u32 Channel, u32 Reset)
{
  XGpio_DiscreteWrite(ResetPtr, Channel, !Reset); //reset is active low
}

/*****************************************************************************/
/**
 * This function implements system video mute feature by enabling/disabling
 * video mixer master layer
 *
 * @param  InstancePtr is a pointer to the System Output instance
 * @param  Enable is 2 state variable that defines the requested state (T/F)
 *
 * @return   None
 *
 ******************************************************************************/
void XOutss_VideoMute(XOutss *InstancePtr, u32 Enable)
{
  if(Enable) {
	  XVMix_LayerDisable(InstancePtr->MixerPtr, XVMIX_LAYER_MASTER);
  } else {
	  XVMix_LayerEnable(InstancePtr->MixerPtr, XVMIX_LAYER_MASTER);
  }
  MB_Sleep(50);
}
