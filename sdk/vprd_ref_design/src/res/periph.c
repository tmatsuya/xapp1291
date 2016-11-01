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
* @file periph.c
*
* This is top level resource file that will initialize all system level
* peripherals
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
#include "xparameters.h"
#include "periph.h"
#include "xdebug.h"

/************************** Constant Definitions *****************************/
#define GPIO_DIP_SWITCH_CH         (1)
#define DEBOUNCE_WAIT              (5000)

/**************************** Type Definitions *******************************/

/**************************** Local Global *******************************/
/* Peripheral IP driver Instance */
#if defined(__arm__)
XScuGic Intc;        //Interrupt Controller
#else
XIntc Intc;         //Interrupt Controller
#endif

static XGpio Gpio;        //DIP Switch support
static XV_tpg Tpg;
static XTmrCtr Timer;     //Timer for delay
static XAxis_Switch SwitchIn;

/***************** Macros (Inline Functions) Definitions *********************/


/************************** Function Prototypes ******************************/
static void Xil_AssertCallbackRoutine(u8 *File, s32 Line);

/************************** Function Definition ******************************/

/*****************************************************************************/
/**
 * This function reports system wide common peripherals included in the design
 *
 * @param  InstancePtr is a pointer to the Subsystem instance to be
 *       worked on.
 *
 *****************************************************************************/
void XPeriph_ReportDeviceInfo(XPeriph *InstancePtr)
{
  u32 numInstances;

  xil_printf("\r\n  ->System Peripherals Included\r\n");

#if defined(__arm__)
  numInstances = XPAR_XSCUGIC_NUM_INSTANCES;
#else
  numInstances = XPAR_XINTC_NUM_INSTANCES;
#endif
  if(numInstances > 0) {
    xil_printf("    : %d Interrupt Controller\r\n", numInstances);
  }

  numInstances = XPAR_XGPIO_NUM_INSTANCES;
  if(numInstances > 2) {//2 GPIO core is always part of Video Processing Subsystem
    xil_printf("    : %d GPIO core(s)\r\n", (numInstances-2));
  }

  numInstances = XPAR_XIIC_NUM_INSTANCES;
  if(numInstances > 0) {
    xil_printf("    : %d I2C core(s)\r\n", numInstances);
  }

  numInstances = XPAR_XTMRCTR_NUM_INSTANCES;
  if(numInstances > 0) {
    xil_printf("    : %d Timer core(s)\r\n", numInstances);
  }

#if defined(__arm__)
  numInstances = XPAR_XUARTPS_NUM_INSTANCES;
#else
  numInstances = XPAR_XUARTLITE_NUM_INSTANCES;
#endif
  if(numInstances > 0) {
    xil_printf("    : %d UART-Lite core(s)\r\n", numInstances);
  }

  numInstances = XPAR_XV_TPG_NUM_INSTANCES;
  if(numInstances > 0) {
     xil_printf("    : %d TPG\r\n", numInstances);
  }

  numInstances = XPAR_XAXIS_SWITCH_NUM_INSTANCES;
  if(numInstances > 1) {//1 AXIS Switch core is always part of Video Processing Subsystem
    xil_printf("    : %d AXIS Switch core\r\n", (numInstances-1));
  }
}

/*****************************************************************************/
/**
 * This function initializes system wide common peripherals.
 *
 * @param  InstancePtr is a pointer to the Subsystem instance to be
 *       worked on.
 *
 * @return XST_SUCCESS/XST_FAILURE
 *
 *****************************************************************************/
int XPeriph_PowerOnInit(XPeriph *InstancePtr)
{
  int status = XST_FAILURE;
  XGpio_Config *GpioCfgPtr;
  XAxis_Switch_Config *RouterCfgPtr;

  Xil_AssertNonvoid(InstancePtr != NULL);

  //Bind the peripheral instance to ip drivers
  InstancePtr->IntcPtr  = &Intc;
  InstancePtr->GpioPtr  = &Gpio;
  InstancePtr->TpgPtr   = &Tpg;
  InstancePtr->TimerPtr = &Timer;

  InstancePtr->InputMuxPtr  = &SwitchIn;

  InstancePtr->UartBaseAddr = STDIN_BASEADDRESS;

#if defined(__arm__)
  XScuGic_Config *IntcCfgPtr;
  IntcCfgPtr = XScuGic_LookupConfig(XPAR_PS7_SCUGIC_0_DEVICE_ID);
  if(IntcCfgPtr == NULL)
  {
	  print("ERR:: Interrupt Controller not found");
	  return (XST_DEVICE_NOT_FOUND);
  }
  status = XScuGic_CfgInitialize((XScuGic *)InstancePtr->IntcPtr,
		  	  	  	  	  	  	 IntcCfgPtr,
		  	  	  	  	  	  	 IntcCfgPtr->CpuBaseAddress);
#else
  status = XIntc_Initialize((XIntc *)InstancePtr->IntcPtr,
		                    XPAR_PROCESSOR_SS_AXI_INTC_0_DEVICE_ID);
#endif
  if(status != XST_SUCCESS) {
    xil_printf("PERIPH ERR:: Interrupt Controller Initialization failed %d\r\n", status);
	return(XST_FAILURE);
  }

  //GPIO
  GpioCfgPtr = XGpio_LookupConfig(XPAR_PROCESSOR_SS_AXI_GPIO_0_DEVICE_ID);
  if(GpioCfgPtr == NULL) {
    xil_printf("PERIPH ERR:: GPIO device not found\r\n");
    return(XST_DEVICE_NOT_FOUND);
  }
  status  = XGpio_CfgInitialize(InstancePtr->GpioPtr,
  		                        GpioCfgPtr,
  		                        GpioCfgPtr->BaseAddress);

  if(status != XST_SUCCESS) {
    xil_printf("PERIPH ERR:: GPIO Initialization failed %d\r\n", status);
    return(XST_FAILURE);
  }

  //Timer
  status = XTmrCtr_Initialize(InstancePtr->TimerPtr, XPAR_PROCESSOR_SS_AXI_TIMER_0_DEVICE_ID);
  if(status != XST_SUCCESS) {
	if(status == XST_DEVICE_NOT_FOUND) {
	  xil_printf("PERIPH ERR:: Timer device not found\r\n");
      return(XST_DEVICE_NOT_FOUND);
	} else {
      xil_printf("PERIPH ERR:: Timer Initialization failed %d\r\n", status);
	  return(XST_FAILURE);
	}
  }

  //TPG
  status = XV_tpg_Initialize(InstancePtr->TpgPtr, XPAR_V_TPG_0_DEVICE_ID);
  if(status == XST_DEVICE_NOT_FOUND) {
    xil_printf("PERIPH ERR:: TPG device not found\r\n");
    return(status);
  }

  //Input AXIS Switch
  if(InstancePtr->InputMuxPtr) {
    RouterCfgPtr  = XAxisScr_LookupConfig(XPAR_AXIS_SWITCH_0_DEVICE_ID);
    if(RouterCfgPtr == NULL) {
      xil_printf("PERIPH ERR:: AXIS Switch device not found\r\n");
      return(XST_DEVICE_NOT_FOUND);
    }
    status = XAxisScr_CfgInitialize(InstancePtr->InputMuxPtr,
  		                            RouterCfgPtr,
  		                            RouterCfgPtr->BaseAddress);

    if(status != XST_SUCCESS) {
      xil_printf("PERIPH ERR:: AXIS Switch Initialization failed %d\r\n", status);
      return(XST_FAILURE);
    }
  }

  /* Reset the hardware  */
  XPeriph_Reset(InstancePtr);

  //Set all user mode options to default
  InstancePtr->UsrSet.TpgVidMode   = XVIDC_VM_1920x1080_60_P;
  InstancePtr->UsrSet.TpgColorFmt  = XVIDC_CSF_RGB;
  InstancePtr->UsrSet.TpgSelPattrn = XTPG_BKGND_COLOR_BARS;
  InstancePtr->UsrSet.ActiveOutput = XPERIPH_OUTPUT_MUX_HDMI;

  return(status);
}

/*****************************************************************************/
/**
 * This function start system wide common peripherals.
 *
 * @param  InstancePtr is a pointer to the Subsystem instance to be
 *       worked on.
 * @param  CfgPtr points to the configuration structure associated with the
 *       Subsystem device.
 * @param  EffectiveAddr is the base address of the device. If address
 *       translation is being used, then this parameter must reflect the
 *       virtual base address. Otherwise, the physical address should be
 *       used.
 * @return XST_SUCCESS
 *
 *****************************************************************************/
void XPeriph_Start(XPeriph *InstancePtr)
{
  Xil_AssertVoid(InstancePtr != NULL);

  xil_printf("  ->Starting System Peripherals.... \r\n");

  /*
   * Start the interrupt controller such that interrupts are recognized
   * and handled by the processor
   */
#if defined(__arm__)
  //NOP
#else
  {
    int status;

    status = XIntc_Start((XIntc *)InstancePtr->IntcPtr, XIN_REAL_MODE);
    if(status != XST_SUCCESS) {
      xil_printf("PERIPH ERR:: System Error ->Failed to Start Interrupt Controller\r\n");
    }
  }
#endif
  /*
   * Initialize the exception table.
   */
  Xil_ExceptionInit();

  /*
   * Register the interrupt controller handler with the exception table.
   */
#if defined(__arm__)
  Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
	                		   (Xil_ExceptionHandler)XScuGic_InterruptHandler,
	                		   (XScuGic *)InstancePtr->IntcPtr);
#else
  Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
	                		   (Xil_ExceptionHandler)XIntc_InterruptHandler,
	                		   (XIntc *)InstancePtr->IntcPtr);
#endif
   /*
    * Enable exceptions.
    */
   Xil_AssertSetCallback((Xil_AssertCallback) Xil_AssertCallbackRoutine);
   Xil_ExceptionEnable();
}

/*****************************************************************************/
/**
 * This function stops system wide common peripherals.
 *
 * @param  InstancePtr is a pointer to the Subsystem instance to be
 *       worked on.
 * @param  CfgPtr points to the configuration structure associated with the
 *       Subsystem device.
 * @param  EffectiveAddr is the base address of the device. If address
 *       translation is being used, then this parameter must reflect the
 *       virtual base address. Otherwise, the physical address should be
 *       used.
 * @return XST_SUCCESS
 *
 *****************************************************************************/
void XPeriph_Stop(XPeriph *InstancePtr)
{
  Xil_AssertVoid(InstancePtr != NULL);
  xil_printf("  ->Stop System Peripherals.... \r\n");

  XV_tpg_DisableAutoRestart(InstancePtr->TpgPtr);
}

/*****************************************************************************/
/**
 * This function resets system wide common peripherals.
 *
 * @param  InstancePtr is a pointer to the Subsystem instance to be
 *       worked on.
 * @param  CfgPtr points to the configuration structure associated with the
 *       Subsystem device.
 * @param  EffectiveAddr is the base address of the device. If address
 *       translation is being used, then this parameter must reflect the
 *       virtual base address. Otherwise, the physical address should be
 *       used.
 * @return XST_SUCCESS
 *
 *****************************************************************************/
void XPeriph_Reset(XPeriph *InstancePtr)
{
  Xil_AssertVoid(InstancePtr != NULL);
}


/*****************************************************************************/
/**
 * This function reads the DIP switch setting from the board.
 * 	5-bit DIP switch layout
 * 		s2 s3 s4 s6 s5
 * 		--------------
 * 		b4 b3 b2 b1 b0
 *
 * @param  InstancePtr is a pointer to the peripheral instance to be
 *       worked on.
 * @return XST_SUCCESS
 *
 *****************************************************************************/
u32 XPeriph_ReadDIPSwitch(XPeriph *InstancePtr)
{
  u32 val1, val2, count;

  val1 = XGpio_DiscreteRead(InstancePtr->GpioPtr, GPIO_DIP_SWITCH_CH);

  if(!val1) return(0);

  //Debounce
  for(count=0; count<DEBOUNCE_WAIT; ++count)
  {
	 val2 =  XGpio_DiscreteRead(InstancePtr->GpioPtr, GPIO_DIP_SWITCH_CH);
	 if(val1 != val2) {
		 return(0);
	 }
  }
  return(val1);
}

/*****************************************************************************/
/**
 * This function reads the UART to check if any character is in rx buffer
 *
 * @param  InstancePtr is a pointer to the peripheral instance to be
 *       worked on.
 * @return XST_SUCCESS
 *
 *****************************************************************************/
u8 XPeriph_ReadUART(XPeriph *InstancePtr)
{
  u32 recvBuf;
  u8 val = 0; //default buffer is empty

#if defined(__arm__)
  recvBuf = XUartPs_IsReceiveData(InstancePtr->UartBaseAddr);
  if(recvBuf) //Buffer not empty
  {
	  val = XUartPs_RecvByte(InstancePtr->UartBaseAddr);
  }
#else
  recvBuf = XUartLite_IsReceiveEmpty(InstancePtr->UartBaseAddr);
  if(!recvBuf) {//Buffer not empty
	val = XUartLite_RecvByte(InstancePtr->UartBaseAddr);
  }
#endif
  return(val);
}

/*****************************************************************************/
/**
 * This function configures TPG to user defined parameters
 *
 * @param  InstancePtr is a pointer to the peripheral instance
 *
 *****************************************************************************/
void XPeriph_ConfigTpg(XPeriph *InstancePtr)
{
  XV_tpg *pTpg = InstancePtr->TpgPtr;
  u32 width, height;
  XVidC_VideoTiming const *pTiming;

  pTiming = XVidC_GetTimingInfo(InstancePtr->UsrSet.TpgVidMode);

  width  = ((pTiming) ? pTiming->HActive : 0);
  height = ((pTiming) ? pTiming->VActive : 0);

  //Stop TPG
  XV_tpg_DisableAutoRestart(pTpg);

  XV_tpg_Set_height(pTpg, height);
  XV_tpg_Set_width(pTpg,  width);
  XV_tpg_Set_colorFormat(pTpg, InstancePtr->UsrSet.TpgColorFmt);
  XV_tpg_Set_bckgndId(pTpg, InstancePtr->UsrSet.TpgSelPattrn);
  XV_tpg_Set_ovrlayId(pTpg, 0);

  //Start TPG
  XV_tpg_EnableAutoRestart(pTpg);
  XV_tpg_Start(pTpg);
}

/*****************************************************************************/
/**
 * This function stops TPG IP
 *
 * @param  InstancePtr is a pointer to the peripheral instance
 *
 *****************************************************************************/
void XPeriph_DisableTpg(XPeriph *InstancePtr)
{
  //Stop TPG
  XV_tpg_DisableAutoRestart(InstancePtr->TpgPtr);
}

/*****************************************************************************/
/**
 * This function reports TPG Status
 *
 * @param  InstancePtr is a pointer to the peripheral instance
 *
 *****************************************************************************/
void XPeriph_TpgDbgReportStatus(XPeriph *InstancePtr)
{
  u32 done, idle, ready, ctrl;
  u32 width, height, bgid, cfmt;
  XV_tpg *pTpg = InstancePtr->TpgPtr;

  if(pTpg)
  {
    xil_printf("\r\n\r\n----->TPG STATUS<----\r\n");

	done  = XV_tpg_IsDone(pTpg);
	idle  = XV_tpg_IsIdle(pTpg);
	ready = XV_tpg_IsReady(pTpg);
	ctrl  = XV_tpg_ReadReg(pTpg->Config.BaseAddress, XV_TPG_CTRL_ADDR_AP_CTRL);

	width  = XV_tpg_Get_width(pTpg);
	height = XV_tpg_Get_height(pTpg);
	bgid   = XV_tpg_Get_bckgndId(pTpg);
	cfmt   = XV_tpg_Get_colorFormat(pTpg);

    xil_printf("IsDone:  %d\r\n", done);
    xil_printf("IsIdle:  %d\r\n", idle);
    xil_printf("IsReady: %d\r\n", ready);
    xil_printf("Ctrl:    0x%x\r\n\r\n", ctrl);

    xil_printf("Width:        %d\r\n",width);
    xil_printf("Height:       %d\r\n",height);
	xil_printf("Backgnd Id:   %d\r\n",bgid);
	xil_printf("Color Format: %d\r\n",cfmt);
  }
}

/*****************************************************************************/
/**
* This function programs the input switch to select the active stream
*
* @param  InstancePtr is a pointer to the peripheral instance
* @apram  MuxSel is the active input
*
******************************************************************************/
void XPeriph_SetInputMux(XPeriph *InstancePtr, XPeriphInputMux MuxSel)
{
  /* Select Active input Stream from Switch */
  XAxisScr_RegUpdateDisable(InstancePtr->InputMuxPtr);
  XAxisScr_MiPortDisableAll(InstancePtr->InputMuxPtr);
  XAxisScr_MiPortEnable(InstancePtr->InputMuxPtr, 0, MuxSel);
  XAxisScr_RegUpdateEnable(InstancePtr->InputMuxPtr);
}

/*****************************************************************************/
/**
* This function programs the output switch to select the active stream
*
* @param  InstancePtr is a pointer to the peripheral instance
* @apram  MuxSel is the active output
*
******************************************************************************/
void XPeriph_SetOutputMux(XPeriph *InstancePtr, XPeriphOutputMux MuxSel)
{
  //Reserved for future use
}

/*****************************************************************************/
/**
 * This function disables all interrupts
 *
 * @param  IntcPtr is driver handle to the interrupt controller
 * @return none
 *
 *****************************************************************************/
void XPeriph_DisableAllInterrupts(void *IntcPtr)
{
#if defined(__arm__)
	  XScuGic_Disable((XScuGic *)IntcPtr, XPAR_PS7_SCUGIC_0_DEVICE_ID);
#else
 	  XIntc_MasterDisable((((XIntc *)IntcPtr)->BaseAddress));
#endif
}

/*****************************************************************************/
/**
 * This function enables all interrupts
 *
 * @param  IntcPtr is driver handle to the interrupt controller
 * @return none
 *
 *****************************************************************************/
void XPeriph_EnableAllInterrupts(void *IntcPtr)
{
#if defined(__arm__)
    XScuGic_Enable((XScuGic *)IntcPtr, XPAR_PS7_SCUGIC_0_DEVICE_ID);
#else
    XIntc_MasterEnable(((XIntc *)IntcPtr)->BaseAddress);
#endif
}

/*****************************************************************************/
/**
 * This function enables prints the line that caused the exception
 *
 * @param  File is pointer to file where exception occurred
 * @param  Line is source code line no that caused the exception
 *
 * @return none
 *
 *****************************************************************************/
static void Xil_AssertCallbackRoutine(u8 *File, s32 Line)
{
  xil_printf("***\r\nERR:: Assertion in File %s, on line %0d***\n\r", File, Line);
}
