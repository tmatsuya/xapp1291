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
* @file phy.c
*
* This is wrapper file for phy layer
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
#include "xparameters.h"
#include "physs.h"
#include "../eventhndlr.h"

/************************** Constant Definitions *****************************/
#if defined(__arm__)
#define XPAR_CPU_CORE_CLOCK_FREQ_HZ  (100000000)
#endif

/**************************** Type Definitions *******************************/

/**************************** Local Global *******************************/


/************************** Function Prototypes ******************************/
static void VphyHdmiTxInitCallback (void * CallbackRef);
static void VphyHdmiTxReadyCallback(void * CallbackRef);
static void VphyHdmiRxInitCallback (void * CallbackRef);
static void VphyHdmiRxReadyCallback(void * CallbackRef);
/***************** Macros (Inline Functions) Definitions *********************/

/*****************************************************************************/
/**
 * This function registers the System Interrupt controller with subsystem
 *
 * @param  IntcPtr is a pointer to the system interrupt controller instance
 * @return None
 *
 *****************************************************************************/
void XVphyss_RegisterSysIntc(XVphyss *InstancePtr, void *IntcPtr)
{
  InstancePtr->SysIntcPtr = IntcPtr;
}


/*****************************************************************************/
/**
 * This function initializes video phy subsystem.
 *
 * @param  InstancePtr is a pointer to the Subsystem instance to be worked on.
 *
 * @return XST_SUCCESS/XST_FAILURE
 *
 *****************************************************************************/
int  XVPhyss_PowerOnInit(XVphyss *InstancePtr)
{
  int Status;
  XVphy_Config *VphyConfigPtr;

  /* Verify arguments */
  Xil_AssertNonvoid(InstancePtr != NULL);
  Xil_AssertNonvoid(InstancePtr->SysIntcPtr  != NULL);

  VphyConfigPtr = XVphy_LookupConfig(XPAR_VID_PHY_CONTROLLER_0_DEVICE_ID);
  if(VphyConfigPtr == NULL) {
	  InstancePtr->HdmiRx->VphyPtr->IsReady = 0;
	  xil_printf("ERR PHY:: Video PHY device not found\r\n");
      return (XST_DEVICE_NOT_FOUND);
  }

  /* Initialize HDMI VPHY (Rx/Tx Share Same Phy) */
  Status = XVphy_HdmiInitialize(InstancePtr->HdmiRx->VphyPtr,
		                        0,  //Quad-Id
		                        VphyConfigPtr,
								XPAR_CPU_CORE_CLOCK_FREQ_HZ);

  if (Status != XST_SUCCESS) {
    xil_printf("ERR Phy:: HDMI VPHY initialization error\n\r");
    return XST_FAILURE;
  }

  /* Register VPHY Interrupt Handler */
#if defined(__arm__)
  Status = XScuGic_Connect(InstancePtr->SysIntcPtr,
		                   XPAR_FABRIC_VID_PHY_CONTROLLER_0_IRQ_INTR,
  	  		               (XInterruptHandler)XVphy_InterruptHandler,
  	  		               (void *)InstancePtr->HdmiRx->VphyPtr);
#else
  Status = XIntc_Connect(InstancePtr->SysIntcPtr,
		                 XPAR_PROCESSOR_SS_AXI_INTC_0_VID_PHY_CONTROLLER_0_IRQ_INTR,
  	  		             (XInterruptHandler)XVphy_InterruptHandler,
  	  		             (void *)InstancePtr->HdmiRx->VphyPtr);
#endif
  if (Status != XST_SUCCESS) {
    xil_printf("ERR Phy:: HDMI VPHY Interrupt Vec ID not found!\n\r");
    return XST_FAILURE;
  }

  /* Enable VPHY Interrupt */
#if defined(__arm__)
  XScuGic_Enable(InstancePtr->SysIntcPtr,
		         XPAR_FABRIC_VID_PHY_CONTROLLER_0_IRQ_INTR);
#else
  XIntc_Enable(InstancePtr->SysIntcPtr,
		       XPAR_PROCESSOR_SS_AXI_INTC_0_VID_PHY_CONTROLLER_0_IRQ_INTR);
#endif

  /* VPHY callback setup */
  XVphy_SetHdmiCallback(InstancePtr->HdmiTx->VphyPtr,
		                XVPHY_HDMI_HANDLER_TXINIT,
		                VphyHdmiTxInitCallback,
		                (void *)InstancePtr);

  XVphy_SetHdmiCallback(InstancePtr->HdmiTx->VphyPtr,
		                XVPHY_HDMI_HANDLER_TXREADY,
		                VphyHdmiTxReadyCallback,
		                (void *)InstancePtr);

  XVphy_SetHdmiCallback(InstancePtr->HdmiRx->VphyPtr,
		                XVPHY_HDMI_HANDLER_RXINIT,
		                VphyHdmiRxInitCallback,
		                (void *)InstancePtr);

  XVphy_SetHdmiCallback(InstancePtr->HdmiRx->VphyPtr,
		                XVPHY_HDMI_HANDLER_RXREADY,
		                VphyHdmiRxReadyCallback,
		                (void *)InstancePtr);

  return(XST_SUCCESS);
}


/*****************************************************************************/
/**
*
* This function is called when the GT TX reference input clock has changed.
*
* @param CallBackRef is ptr to subsystem instance that generated the interrupt
*
* @return None
*
******************************************************************************/
static void VphyHdmiTxInitCallback (void * CallbackRef)
{
  XVphyss *pVphyss = (XVphyss *)CallbackRef;

  XV_HdmiTxSs_RefClockChangeInit(pVphyss->HdmiTx->HdmiTxSsPtr);

  //Clear Panel lock flag
  XVphyss_SetSPanelLockMode(pVphyss, FALSE);
}

/*****************************************************************************/
/**
*
* This function is called when the GT TX has been initialized
*
* @param CallBackRef is ptr to subsystem instance that generated the interrupt
*
* @return None
*
******************************************************************************/
static void VphyHdmiTxReadyCallback(void * CallbackRef)
{
  //NOP
}

/*****************************************************************************/
/**
*
* This function is called when the GT RX reference input clock has changed.
*
* @param CallBackRef is ptr to subsystem instance that generated the interrupt
*
* @return None
*
******************************************************************************/
static void VphyHdmiRxInitCallback (void * CallbackRef)
{
  XVphyss *pVphyss = (XVphyss *)CallbackRef;

  XV_HdmiRxSs_RefClockChangeInit(pVphyss->HdmiRx->HdmiRxSsPtr);
  pVphyss->HdmiRx->VphyPtr->HdmiRxTmdsClockRatio = pVphyss->HdmiRx->HdmiRxSsPtr->TMDSClockRatio;
}

/*****************************************************************************/
/**
*
* This function is called when the GT RX has been initialized.
*
* @param CallBackRef is ptr to subsystem instance that generated the interrupt
*
* @return None
*
******************************************************************************/
static void VphyHdmiRxReadyCallback(void * CallbackRef)
{
  XVphyss *pVphyss = (XVphyss *)CallbackRef;
  XVphy_PllType RxPllType;
  u32 LineRateCpll, LineRateQpll, LineRate;

  RxPllType = XVphy_GetPllType(pVphyss->HdmiRx->VphyPtr,
		                       0,
							   XVPHY_DIR_RX,
							   XVPHY_CHANNEL_ID_CH1);

  LineRateQpll = (pVphyss->HdmiRx->VphyPtr->Quads[0].Plls[XVPHY_CHANNEL_ID_CMN0 -
				              XVPHY_CHANNEL_ID_CH1].LineRateHz / 1000000);
  LineRateCpll = (pVphyss->HdmiRx->VphyPtr->Quads[0].Plls[0].LineRateHz/1000000);

  LineRate = (!(RxPllType == XVPHY_PLL_TYPE_CPLL) ? LineRateQpll: LineRateCpll);

  XV_HdmiRxSs_SetStream(pVphyss->HdmiRx->HdmiRxSsPtr,
			            pVphyss->HdmiRx->VphyPtr->HdmiRxRefClkHz,
						LineRate);
}
