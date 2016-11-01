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
* @file parser.c
*
* This resource file implements CLI for the reference design
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
#include <ctype.h>
#include "parser.h"

#include "../eventhndlr.h"
#include "../platform_config.h"
#include "microblaze_sleep.h"

/************************** Constant Definitions *****************************/
#define MAX_OUTPUT_VID_MODES      (22)
#define MAX_TPG_VID_MODES         (16)
/**************************** Type Definitions *******************************/
typedef struct
{
  u8 index;
  XVidC_VideoMode mode;
}KeyVmIdmap;

const KeyVmIdmap outResTable[MAX_OUTPUT_VID_MODES] =
{
  {1,  XVIDC_VM_640x480_60_P},
  {2,  XVIDC_VM_720x480_60_P},
  {3,  XVIDC_VM_720x576_50_P},
  {4,  XVIDC_VM_800x600_60_P},
  {5,  XVIDC_VM_1024x768_60_P},
  {6,  XVIDC_VM_1280x720_50_P},
  {7,  XVIDC_VM_1280x720_60_P},
  {8,  XVIDC_VM_1280x768_60_P},
  {9,  XVIDC_VM_1280x1024_60_P},
  {10, XVIDC_VM_1600x1200_60_P},
  {11, XVIDC_VM_1680x1050_60_P},
  {12, XVIDC_VM_1920x1080_24_P},
  {13, XVIDC_VM_1920x1080_25_P},
  {14, XVIDC_VM_1920x1080_30_P},
  {15, XVIDC_VM_1920x1080_50_P},
  {16, XVIDC_VM_1920x1080_60_P},
  {17, XVIDC_VM_1920x1200_60_P},
  {18, XVIDC_VM_3840x2160_24_P},
  {19, XVIDC_VM_3840x2160_25_P},
  {20, XVIDC_VM_3840x2160_30_P},
  {21, XVIDC_VM_3840x2160_50_P},
  {22, XVIDC_VM_3840x2160_60_P}
};

const KeyVmIdmap tpgResTable[MAX_TPG_VID_MODES] =
{
  {1,  XVIDC_VM_720x480_60_I},
  {2,  XVIDC_VM_720x576_50_I},
  {3,  XVIDC_VM_1920x1080_60_I},
  {4,  XVIDC_VM_640x480_60_P},
  {5,  XVIDC_VM_720x480_60_P},
  {6,  XVIDC_VM_720x576_50_P},
  {7,  XVIDC_VM_800x600_60_P},
  {8,  XVIDC_VM_1024x768_60_P},
  {9,  XVIDC_VM_1280x720_50_P},
  {10, XVIDC_VM_1280x768_60_P},
  {11, XVIDC_VM_1280x1024_60_P},
  {12, XVIDC_VM_1600x1200_60_P},
  {13, XVIDC_VM_1680x1050_60_P},
  {14, XVIDC_VM_1920x1080_60_P},
  {15, XVIDC_VM_1920x1200_60_P},
  {16, XVIDC_VM_3840x2160_60_P}
};

/**************************** Local Global *******************************/


/************************** Function Prototypes ******************************/
static void show_HelpMenu(void);
static void show_PictureMenu(void);
static void show_OutputMenu(void);
static void show_TpgOutputMenu(void);
static void show_SetupMenu(void);
static void show_DebugMenu(void);
static void show_TPGMenu(void);
static s32 getUsrSel(XPeriph *pPeriph);
static int getUserWin(XPeriph  *pPeriph,
		              XVprocSs *pVprocss,
		              XVidC_VideoWindow *win);
static int validatWindowSize(const XVidC_VideoWindow *win,
		                     const XVidC_VideoTiming *Resolution);


static void ProcessTpgMenu(XPeriph  *pPeriph,
		                   XVprocSs *pVprocss,
						   XOutss   *pOutss);
static void ProcessPictureMenu(XPeriph  *pPeriph, XVprocSs *pVprocss);
static void ProcessSetupMenu(XPeriph  *pPeriph,
   	                         XInss    *pInpss,
	                         XVprocSs *pVprocss,
                             XOutss   *pOutss,
	                         XVphyss  *pVphyss);
static void ProcessDbgMenu(XPeriph  *pPeriph,
   	                       XInss    *pInpss,
	                       XVprocSs *pVprocss,
                           XOutss   *pOutss,
	                       XVphyss  *pVphyss);
static void ProcessMixerMenu(XPeriph  *pPeriph,
	                         XVprocSs *pVprocss,
                             XOutss   *pOutss);

/***************** Macros (Inline Functions) Definitions *********************/

/*****************************************************************************/
/**
* This macro reads a character from serial port
*
* @param  UartBaseAddress is the base address of the UART peripheral
*
* @return u8
*
******************************************************************************/
static __inline u8 UART_ReadChar(u32 UartBaseAddress)
{
#if defined(__arm__)
	  return(XUartPs_RecvByte(UartBaseAddress));
#else
    return(XUartLite_RecvByte(UartBaseAddress));
#endif
}

/*****************************************************************************/
/**
* This macro sends a character to serial port
*
* @param  address is the base address of the UART peripheral
*
* @return u8
*
******************************************************************************/
static __inline void UART_SendChar(u32 UartBaseAddress, u8 ByteToSend)
{
#if defined(__arm__)
  XUartPs_SendByte(UartBaseAddress, ByteToSend); //feedback key pressed to user
#else
  XUartLite_SendByte(UartBaseAddress, ByteToSend); //feedback key pressed to user
#endif
}

/************************** Function Definition ******************************/
/***************************************************************************
*  Show Output Resolution Help Menu for UART based commands
***************************************************************************/
static void show_OutputMenu(void)
{
  xil_printf("\r\n---------------------------------------------------\r\n");
  xil_printf("           SELECT SYSTEM OUPUT RESOLUTION          \r\n");
  xil_printf("---------------------------------------------------\r\n");
  xil_printf("  1:  640x480   60Hz\r\n");
  xil_printf("  2:  720x480   60Hz\r\n");
  xil_printf("  3:  720x576   50Hz\r\n");
  xil_printf("  4:  800x600   60Hz\r\n");
  xil_printf("  5: 1024x768   60Hz\r\n");
  xil_printf("  6: 1280x720   50Hz\r\n");
  xil_printf("  7: 1280x720   60Hz\r\n");
  xil_printf("  8: 1280x768   60Hz\r\n");
  xil_printf("  9: 1280x1024  60Hz\r\n");
  xil_printf(" 10: 1600x1200  60Hz\r\n");
  xil_printf(" 11: 1680x1050  60Hz\r\n");
  xil_printf(" 12: 1920x1080  24Hz\r\n");
  xil_printf(" 13: 1920x1080  25Hz\r\n");
  xil_printf(" 14: 1920x1080  30Hz\r\n");
  xil_printf(" 15: 1920x1080  50Hz\r\n");
  xil_printf(" 16: 1920x1080  60Hz\r\n");
  xil_printf(" 17: 1920x1200  60Hz\r\n");
  xil_printf(" 18: 3840x2160  24Hz\r\n");
  xil_printf(" 19: 3840x2160  25Hz\r\n");
  xil_printf(" 20: 3840x2160  30Hz\r\n");
  xil_printf(" 21: 3840x2160  50Hz\r\n");
  xil_printf(" 22: 3840x2160  60Hz\r\n");
  xil_printf("  0: Exit\r\n");
  xil_printf("---------------------------------------------------\r\n");
  xil_printf("Enter Selection -> ");
}

/***************************************************************************
*  Show TPG Resolution Help Menu for UART based commands
***************************************************************************/
static void show_TpgOutputMenu(void)
{
  xil_printf("\r\n---------------------------------------------------\r\n");
  xil_printf("           SELECT TPG OUPUT RESOLUTION             \r\n");
  xil_printf("---------------------------------------------------\r\n");
  xil_printf("  1:  480i    \r\n");
  xil_printf("  2:  576i    \r\n");
  xil_printf("  3:  1080i   \r\n");
  xil_printf("  4:  640x480 \r\n");
  xil_printf("  5:  720x480 \r\n");
  xil_printf("  6:  720x576 \r\n");
  xil_printf("  7:  800x600 \r\n");
  xil_printf("  8: 1024x768 \r\n");
  xil_printf("  9: 1280x720 \r\n");
  xil_printf(" 10: 1280x768 \r\n");
  xil_printf(" 11: 1280x1024\r\n");
  xil_printf(" 12: 1600x1200\r\n");
  xil_printf(" 13: 1680x1050\r\n");
  xil_printf(" 14: 1920x1080\r\n");
  xil_printf(" 15: 1920x1200\r\n");
  xil_printf(" 16: 3840x2160\r\n");
  xil_printf("  0: Exit\r\n");
  xil_printf("---------------------------------------------------\r\n");
  xil_printf("Enter Selection -> ");
}

/***************************************************************************
*  Show Feature Help Menu for UART based commands
***************************************************************************/
static void show_SetupMenu(void)
{
  xil_printf("\r\n---------------------------------------------------\r\n");
  xil_printf("              SYSTEM SETUP MENU                   \r\n");
  xil_printf("---------------------------------------------------\r\n");
  xil_printf("  1: ZOOM Mode Toggle On/Off\r\n");
  xil_printf("  2: PIP Mode Toggle On/Off\r\n");
  xil_printf("  3: Zoom Window\r\n");
  xil_printf("  4: PIP Window\r\n");
  xil_printf("  5: PIP Background Color\r\n");
  xil_printf("  6: Select Active Input\r\n");
  xil_printf("  0: Exit\r\n");
  xil_printf("---------------------------------------------------\r\n");
  xil_printf("Enter Selection-> ");
}

/***************************************************************************
*  Show Mixer Help Menu for UART based commands
***************************************************************************/
static void show_MixerMenu(void)
{
  xil_printf("\r\n---------------------------------------------------\r\n");
  xil_printf("              MIXER MENU                     \r\n");
  xil_printf("---------------------------------------------------\r\n");
  xil_printf(" 1: Enable Layer\r\n");
  xil_printf(" 2: Disable Layer\r\n");
  xil_printf(" 3: Set Layer Alpha\r\n");
  xil_printf(" 4: Set Layer Scale Factor\r\n");
  xil_printf(" 5: Logo ColorKey\r\n");
  xil_printf(" 6: Set Layer Window\r\n");
  xil_printf(" 7: Move Layer Window\r\n");
  xil_printf(" 0: Exit\r\n");
  xil_printf("---------------------------------------------------\r\n");
  xil_printf("Enter Selection-> ");
}

/***************************************************************************
*  Show Debug Help Menu for UART based commands
***************************************************************************/
static void show_DebugMenu(void)
{
  xil_printf("\r\n---------------------------------------------------\r\n");
  xil_printf("            SELECT CORE TO DEBUG                   \r\n");
  xil_printf("---------------------------------------------------\r\n");
  xil_printf("  1: V Scaler \r\n");
  xil_printf("  2: H Scaler \r\n");
  xil_printf("  3: VDMA \r\n");
  xil_printf("  4: Letterbox \r\n");
  xil_printf("  5: H Chroma Resampler \r\n");
  xil_printf("  6: V Chroma Resampler - Input \r\n");
  xil_printf("  7: V Chroma Resampler - Output \r\n");
  xil_printf("  8: Color Correction \r\n");
  xil_printf("  9: Deinterlacer \r\n");
  xil_printf(" 10: Subsystem Switch\r\n");
  xil_printf(" 11: VPSS\r\n");
  xil_printf(" 12: VPSS Log\r\n");
  xil_printf(" 13: Input Switch\r\n");
  xil_printf(" 14: TPG\r\n");
  xil_printf(" 15: Video Phy Log\r\n");
  xil_printf(" 16: HDMI Rx\r\n");
  xil_printf(" 17: HDMI Tx\r\n");
  xil_printf(" 18: Mixer\r\n");
  xil_printf("  0: Exit\r\n");
  xil_printf("---------------------------------------------------\r\n");
  xil_printf("Enter Selection -> ");
}

/***************************************************************************
*  Show Picture Help Menu for UART based commands
***************************************************************************/
static void show_PictureMenu(void)
{
  xil_printf("\r\n---------------------------------------------------\r\n");
  xil_printf("           SELECT PICTURE SETTINGS                 \r\n");
  xil_printf("---------------------------------------------------\r\n");
  xil_printf("  1: Brightness\r\n");
  xil_printf("  2: Contrast  \r\n");
  xil_printf("  3: Saturation\r\n");
  xil_printf("  4: Red Gain  \r\n");
  xil_printf("  5: Green Gain\r\n");
  xil_printf("  6: Blue Gain \r\n");
  xil_printf("  7: Output Color Standard\r\n");
  xil_printf("  8: Output Range\r\n");
  xil_printf("  9: Demo Window \r\n");
  xil_printf("  0: Exit\r\n");
  xil_printf("---------------------------------------------------\r\n");
  xil_printf("Enter Selection -> ");
}

/***************************************************************************
*  Show TPG Menu for UART based commands
***************************************************************************/
static void show_TPGMenu(void)
{
  xil_printf("\r\n---------------------------------------------------\r\n");
  xil_printf("                    TPG MENU                       \r\n");
  xil_printf("---------------------------------------------------\r\n");
  xil_printf("  1: Pattern - H Ramp     \r\n");
  xil_printf("  2: Pattern - V Ramp     \r\n");
  xil_printf("  3: Pattern - Solid Red  \r\n");
  xil_printf("  4: Pattern - Solid Green\r\n");
  xil_printf("  5: Pattern - Solid Blue \r\n");
  xil_printf("  6: Pattern - Solid Black\r\n");
  xil_printf("  7: Pattern - Solid White\r\n");
  xil_printf("  8: Pattern - Color Bars \r\n");
  xil_printf("  9: Pattern - Tartan Color Bars\r\n");
  xil_printf(" 10: Pattern - Cross Hatch\r\n");
  xil_printf(" 11: Pattern - Rainbow Colors\r\n");
  xil_printf(" 12: Pattern - H+V Ramp\r\n");
  xil_printf(" 13: Pattern - Checker Board\r\n");
  xil_printf(" 14: Pattern - PRBS\r\n");
  xil_printf(" 15: Resolution\r\n");
  xil_printf(" 16: Color Format\r\n");
  xil_printf("  0: Exit\r\n");
  xil_printf("---------------------------------------------------\r\n");
  xil_printf("Enter Selection -> ");
}

/***************************************************************************
*  Show HELP Menu for UART based commands
***************************************************************************/
static void show_HelpMenu(void)
{
  xil_printf("\r\n---------------------------------------------------\r\n");
  xil_printf("              HELP MENU                            \r\n");
  xil_printf("---------------------------------------------------\r\n");
  xil_printf("  i: System Info  \r\n");
  xil_printf("  v: System Status\r\n");
  xil_printf("  p: Picture Menu \r\n");
  xil_printf("  m: Mixer Menu \r\n");
  xil_printf("  s: System Setup Menu\r\n");
  xil_printf("  o: Output Resolution Select Menu\r\n");
  xil_printf("  t: Test Pattern Menu\r\n");
  xil_printf("  d: Debug Menu\r\n");
}

/*****************************************************************************/
/**
 * This function checks to make sure sub-frame is inside full frame
 *
 * @param  win is a pointer to the sub-frame window
 * @param  Resolution is a pointer to the current output resolution
 * @return T/F
 *
 *****************************************************************************/
static int validatWindowSize(const XVidC_VideoWindow *win,
		                     const XVidC_VideoTiming *Resolution)
{
  Xil_AssertNonvoid(win != NULL);
  Xil_AssertNonvoid(Resolution != NULL);

  //Check if window is within the active frame resolution
  if(((win->StartX < 0) || (win->StartX > Resolution->HActive)) ||
     ((win->StartY < 0) || (win->StartY > Resolution->VActive)) ||
     ((win->StartX + win->Width) > Resolution->HActive)         ||
     ((win->StartY + win->Height) > Resolution->VActive))
  {
	xil_printf("\r\nERR:: Window coordinates out of current resolution boundary..."
			    "Ignoring command\r\n");
	return(XST_FAILURE);
  }
  else
  {
	return(XST_SUCCESS);
  }
}

/*****************************************************************************/
/**
 * This function gets window co-ordinates from user and validates them against
 * current set output resolution
 *
 * @param  pPeriph is pointer to peripheral instance
 * @param  pVprocss is pointer to video processing subsystem instance
 * @param  win is a pointer to the sub-frame window
 * @return T/F
 *
 *****************************************************************************/
static int getUserWin(XPeriph *pPeriph,
		             XVprocSs *pVprocss,
		             XVidC_VideoWindow *win)
{
  int winValid = XST_FAILURE;

  xil_printf("Enter Win Start X: ");
  win->StartX = (u32)getUsrSel(pPeriph);
  xil_printf("Enter Win Start Y: ");
  win->StartY = (u32)getUsrSel(pPeriph);
  xil_printf("Enter Win Width: ");
  win->Width = (u32)getUsrSel(pPeriph);
  xil_printf("Enter Win Height: ");
  win->Height = (u32)getUsrSel(pPeriph);

  winValid = validatWindowSize(win, &pVprocss->VidOut.Timing);

  return(winValid);
}

/*****************************************************************************/
/**
 * This function accumulates user entry until carriage return is pressed
 *
 * @param  pPeriph is pointer to system peripherals
 *
 *****************************************************************************/
static s32 getUsrSel(XPeriph *pPeriph)
{
  u8 basePos = 10;
  u8 getval = 0;
  u16 val = 0;

  do
  {
	getval = UART_ReadChar(pPeriph->UartBaseAddr);
    if(isalpha(getval)) {
      UART_SendChar(pPeriph->UartBaseAddr, getval);
      xil_printf("\r\nERR:: Invalid input. Valid entry is only digits 0-9. Try again\r\n\r\n");
      xil_printf("Enter Selection -> ");
      val = 0;
    }  else if((getval >= '0') && (getval <='9')) {
      UART_SendChar(pPeriph->UartBaseAddr, getval);
      val = val*basePos+(getval-'0');
    } else if(getval == '\b') {//Backspace - Allow user to change input before commit
      UART_SendChar(pPeriph->UartBaseAddr, getval);
      val = val/basePos; //discard previous input
    }
  }while((getval != '\n') &&
	     (getval != '\r'));

  xil_printf("\r\n"); //blank line between selection and response
  return(val);
}

/*****************************************************************************/
/**
 * This function parses the UART key pressed
 *
 * @param  Key Value from UART
 *
 *****************************************************************************/
void XParse_UartKey(u32      keyval,
		            XPeriph  *pPeriph,
			   	    XInss    *pInpss,
				    XVprocSs *pVprocss,
				    XOutss   *pOutss,
				    XVphyss  *pVphyss)
{
  u32 validSelection = FALSE;
  u8 selval;

  switch(keyval)
  {
    case ('d'): //Debug
    case ('D'):
      ProcessDbgMenu(pPeriph,
      		       pInpss,
      		       pVprocss,
      		       pOutss,
      		       pVphyss);
  	  break;

    case ('h'): //Help
    case ('H'):
        show_HelpMenu();
        break;

    case ('i'): //System Info
    case ('I'):
   	    //Report out what is included in the design
        XSys_ReportSystemInfo(pPeriph,
    		                  pInpss,
    		                  pVprocss,
    		                  pOutss);
    	break;

    case ('m'): //Mixer Menu
    case ('M'):
	    ProcessMixerMenu(pPeriph,
	    		         pVprocss,
						 pOutss);
	    break;

    case ('o'): //Output Resolution Select
    case ('O'):
        while(!validSelection)
    	{
          int resIndex = 0;

      	  show_OutputMenu();
          selval = getUsrSel(pPeriph);

          if(selval == 0) {
        	validSelection = TRUE;
          } else if(selval<=MAX_OUTPUT_VID_MODES) {
    		resIndex = outResTable[selval-1].mode;
    		XSys_SetOutputResolution(pVprocss, pOutss, resIndex);
    		/* Reset Pip/Zoom mode */
            XVprocSs_ResetPipModeFlag(pVprocss);
    		XVprocSs_ResetZoomModeFlag(pVprocss);
    		validSelection = TRUE;
    	  } else {
            xil_printf("\r\nERR:: value out of range. Command ignored.. \r\n");
    	  }
    	}//while
    	break;

    case ('p'): //Picture Menu
    case ('P'):
    	if(!XInss_IsStreamValid(pInpss))
    	{
    	  xil_printf("\r\nERR:: NO INPUT DETECTED - Picture Settings Menu Disabled\r\n");
    	  return;
    	}
        ProcessPictureMenu(pPeriph, pVprocss);
        break;

    case ('s'):  //Setup
    case ('S'):
        ProcessSetupMenu(pPeriph,
    		             pInpss,
    		             pVprocss,
    		             pOutss,
    		             pVphyss);
    	break;

    case 't':
    case 'T':
    	if(XInss_GetInputSourceSel(pInpss) == XINSS_SOURCE_TPG) {
    	   ProcessTpgMenu(pPeriph, pVprocss, pOutss);
    	} else {
    	   xil_printf("\r\nERR:: TPG is not active input. Command ignored.. \r\n");
    	}
    	break;

    case ('v'): //System & Link Status
    case ('V'):
    	XSys_ReportSystemStatus(pPeriph,
    			                pInpss,
    			                pVprocss,
    			                pVphyss);
    	XSys_ReportLinkStatus(pInpss, pOutss, pVphyss);
    	break;

    default:
    	break;
  }

  xil_printf("\r\n\r\nReturn to static scheduler... (Press 'h' for help) \r\n");
}

/***************************************************************************
*  Process TPG Menu commands
***************************************************************************/
static void ProcessTpgMenu(XPeriph  *pPeriph,
		                   XVprocSs *pVprocss,
						   XOutss   *pOutss)
{
  u32 validSelection = FALSE;
  u8 selval;

  while(!validSelection)
  {
    u32 newVal;
    u32 tpgPatSel = XTPG_BKGND_SOLID_BLUE;

    show_TPGMenu();
    selval = getUsrSel(pPeriph);

    if((selval>0) && (selval<15))
    {
  	  switch(selval)
  	  {
  	    case 1: tpgPatSel  = XTPG_BKGND_H_RAMP; break;
  	    case 2: tpgPatSel  = XTPG_BKGND_V_RAMP; break;
  	    case 3: tpgPatSel  = XTPG_BKGND_SOLID_RED; break;
  	    case 4: tpgPatSel  = XTPG_BKGND_SOLID_GREEN; break;
  	    case 5: tpgPatSel  = XTPG_BKGND_SOLID_BLUE; break;
  	    case 6: tpgPatSel  = XTPG_BKGND_SOLID_BLACK; break;
  	    case 7: tpgPatSel  = XTPG_BKGND_SOLID_WHITE; break;
  	    case 8: tpgPatSel  = XTPG_BKGND_COLOR_BARS; break;
  	    case 9: tpgPatSel  = XTPG_BKGND_TARTAN_COLOR_BARS; break;
  	    case 10: tpgPatSel = XTPG_BKGND_CROSS_HATCH; break;
  	    case 11: tpgPatSel = XTPG_BKGND_RAINBOW_COLOR; break;
  	    case 12: tpgPatSel = XTPG_BKGND_HV_RAMP; break;
  	    case 13: tpgPatSel = XTPG_BKGND_CHECKER_BOARD; break;
  	    case 14: tpgPatSel = XTPG_BKGND_PBRS; break;
  	  }
  	  XV_tpg_Set_bckgndId(pPeriph->TpgPtr, tpgPatSel);
  	  pPeriph->UsrSet.TpgSelPattrn = tpgPatSel;
    } else {
      switch(selval)
      {
        case 15: //Active Resolution
          while(!validSelection)
    	  {
            int vmid = 0;

      	    show_TpgOutputMenu();
            selval = getUsrSel(pPeriph);

            if(selval == 0) {
        	  validSelection = TRUE;
            } else if(selval<=MAX_TPG_VID_MODES) {
    		  vmid = tpgResTable[selval-1].mode;
    		  XOutss_VideoMute(pOutss, TRUE);
    		  pPeriph->UsrSet.TpgVidMode = vmid;
      	  	  XSys_SetVpssToTpgResolution(pPeriph, pVprocss);
   	  	      XOutss_VideoMute(pOutss, FALSE);
    	    } else {
    	        xil_printf("\r\nERR:: value out of range. Command ignored.. \r\n");
    	    }
    	  }//while
    	  break;

        case 16: //Color Format
          {
            XVidC_ColorFormat cfmt;

            //Get current TPG setting
            cfmt = XV_tpg_Get_colorFormat(pPeriph->TpgPtr);
      	    xil_printf("\r\n\r\n");
            xil_printf("TPG Color Format (%s) [0:RGB, 1:YUV444, 2:YUV422, 3:YUV420, 4:Exit] -> ",
            	  	      XVidC_GetColorFormatStr(cfmt));
      	    newVal = getUsrSel(pPeriph);
            if(newVal == 4) {
        	    //nop
            } else if(newVal < 4) {

      		  XOutss_VideoMute(pOutss, TRUE);
    	      XPeriph_SetTPGColorFormat(pPeriph, newVal);
      	  	  XSys_SetVpssToTpgResolution(pPeriph, pVprocss);
   	  	      XOutss_VideoMute(pOutss, FALSE);
      	    } else {
  	           xil_printf("\r\nERR:: value out of range. Command ignored.. \r\n");
      	    }
          }
    	  break;

        case 0:
    	  validSelection = TRUE;
    	  break;

        default:
  	      break;
      }//switch
    }//if
  }//while
}

/***************************************************************************
*  Process TPG Menu commands
***************************************************************************/
static void ProcessPictureMenu(XPeriph  *pPeriph, XVprocSs *pVprocss)
{
  u32 validSelection = FALSE;
  u8 selval;

  while(!validSelection)
  {
    u16 newVal;

    show_PictureMenu();
    selval = getUsrSel(pPeriph);
    xil_printf("\r\n");
    switch(selval)
    {
      case 1:
          xil_printf("Brightness [0-100] (%d) -> ", XVprocSs_GetPictureBrightness(pVprocss));
          newVal = getUsrSel(pPeriph);
          if(newVal<=100) {
          	XVprocSs_SetPictureBrightness(pVprocss, newVal);
          } else {
            xil_printf("\r\nERR:: value out of range. Command ignored.. \r\n");
          }
      	  break;

      case 2:
          xil_printf("Contrast [0-100] (%d) -> ", XVprocSs_GetPictureContrast(pVprocss));
          newVal = getUsrSel(pPeriph);
          if(newVal<=100) {
          	XVprocSs_SetPictureContrast(pVprocss, newVal);
          } else {
            xil_printf("\r\nERR:: value out of range. Command ignored.. \r\n");
          }
      	  break;

      case 3:
          xil_printf("Saturation [0-100] (%d) -> ", XVprocSs_GetPictureSaturation(pVprocss));
          newVal = getUsrSel(pPeriph);
          if(newVal<=100) {
          	XVprocSs_SetPictureSaturation(pVprocss, newVal);
          } else {
            xil_printf("\r\nERR:: value out of range. Command ignored.. \r\n");
          }
      	  break;

      case 4:
          xil_printf("Red Gain [0-100] (%d) -> ", XVprocSs_GetPictureGain(pVprocss, XVPROCSS_COLOR_CH_Y_RED));
          newVal = getUsrSel(pPeriph);
          if(newVal<=100) {
          	XVprocSs_SetPictureGain(pVprocss, XVPROCSS_COLOR_CH_Y_RED, newVal);
          } else {
            xil_printf("\r\nERR:: value out of range. Command ignored.. \r\n");
          }
      	  break;

      case 5:
          xil_printf("Green Gain [0-100] (%d) -> ", XVprocSs_GetPictureGain(pVprocss, XVPROCSS_COLOR_CH_CB_GREEN));
          newVal = getUsrSel(pPeriph);
          if(newVal<=100) {
          	XVprocSs_SetPictureGain(pVprocss, XVPROCSS_COLOR_CH_CB_GREEN, newVal);
          } else {
            xil_printf("\r\nERR:: value out of range. Command ignored.. \r\n");
          }
      	  break;

      case 6:
          xil_printf("Blue Gain [0-100] (%d) -> ", XVprocSs_GetPictureGain(pVprocss, XVPROCSS_COLOR_CH_CR_BLUE));
          newVal = getUsrSel(pPeriph);
          if(newVal<=100) {
          	XVprocSs_SetPictureGain(pVprocss, XVPROCSS_COLOR_CH_CR_BLUE, newVal);
          } else {
            xil_printf("\r\nERR:: value out of range. Command ignored.. \r\n");
          }
      	  break;

      case 7:
         xil_printf("Output Color Std (%d) [0:BT2020, 1:709, 2:601, 3:Exit] -> ",
         		     XVprocSs_GetPictureColorStdOut(pVprocss));
          newVal = getUsrSel(pPeriph);
          if(newVal == 3) {
        	  //nop
          } else if(newVal<3) {
            XVprocSs_SetPictureColorStdOut(pVprocss, newVal);
          } else {
            xil_printf("\r\nERR:: value out of range. Command ignored.. \r\n");
          }
      	  break;

      case 8:
      	  xil_printf("Output Range (%d) [0:16-235, 1:16-240, 2:0-255, 3:Exit] -> ",
      			    XVprocSs_GetPictureColorRange(pVprocss));
       	  newVal = getUsrSel(pPeriph);
          if(newVal == 3) {
        	  //nop
          } else if(newVal<3) {
            XVprocSs_SetPictureColorRange(pVprocss, newVal);
       	  } else {
            xil_printf("\r\nERR:: value out of range. Command ignored.. \r\n");
          }
      	  break;

      case 9: //Active Win
          {
    	    XVidC_VideoWindow win;
    	    int status;

    	    status = getUserWin(pPeriph, pVprocss, &win);
    	    if(status == XST_SUCCESS) {
    		  XVprocSs_SetPictureDemoWindow(pVprocss, &win);
    	    }
          }
    	  break;

      case 0:
          validSelection = TRUE;
    	  break;

      default:
  	      break;
    }//switch
  }//while
}

/***************************************************************************
*  Process Debug Menu commands
***************************************************************************/
static void ProcessDbgMenu(XPeriph  *pPeriph,
   	                       XInss    *pInpss,
	                       XVprocSs *pVprocss,
                           XOutss   *pOutss,
	                       XVphyss  *pVphyss)
{
  u32 validSelection = FALSE;
  u32 SubMenuExit = FALSE;
  u8 selval;

  while(!validSelection)
  {
    show_DebugMenu();
    selval = getUsrSel(pPeriph);

    switch(selval)
    {
      case 1:
       	XVprocSs_ReportSubcoreStatus(pVprocss, XVPROCSS_SUBCORE_SCALER_V);
    	break;

      case 2:
    	XVprocSs_ReportSubcoreStatus(pVprocss, XVPROCSS_SUBCORE_SCALER_H);
    	break;

      case 3:
    	XVprocSs_ReportSubcoreStatus(pVprocss, XVPROCSS_SUBCORE_VDMA);
    	break;

      case 4:
    	XVprocSs_ReportSubcoreStatus(pVprocss, XVPROCSS_SUBCORE_LBOX);
    	break;

      case 5:
    	XVprocSs_ReportSubcoreStatus(pVprocss, XVPROCSS_SUBCORE_CR_H);
    	break;

      case 6:
    	XVprocSs_ReportSubcoreStatus(pVprocss, XVPROCSS_SUBCORE_CR_V_IN);
    	break;

      case 7:
    	XVprocSs_ReportSubcoreStatus(pVprocss, XVPROCSS_SUBCORE_CR_V_OUT);
    	break;

      case 8:
    	XVprocSs_ReportSubcoreStatus(pVprocss, XVPROCSS_SUBCORE_CSC);
    	break;

      case 9:
    	XVprocSs_ReportSubcoreStatus(pVprocss, XVPROCSS_SUBCORE_DEINT);
    	break;

      case 10:
        {
          u32 count, offset;
          u32 val;

          xil_printf("\r\n\r\n----->Video Subsystem Switch<----\r\n");

          for(count=0; count<XVPROCSS_SUBCORE_MAX; ++count)
          {
        	offset = XAXIS_SCR_MI_MUX_START_OFFSET+count*4;
        	val = XAxisScr_ReadReg(pVprocss->RouterPtr->Config.BaseAddress,
        			               offset);
        	if(val<XVPROCSS_SUBCORE_MAX) {
          	  xil_printf("M%d: S%d\r\n",count,val);
        	} else {
        	  xil_printf("M%d: 0x%x\r\n",count,val);
        	}
          }
        }
    	break;

      case 11:
    	XVprocSs_ReportSubsystemConfig(pVprocss);
    	break;

	  case 12:
	    XVprocSs_LogDisplay(pVprocss);
	    break;

      case 13:
        {
    	  int data;

    	  data = XAxisScr_ReadReg(pPeriph->InputMuxPtr->Config.BaseAddress,
    		 	                  XAXIS_SCR_MI_MUX_START_OFFSET);
    	  xil_printf("\r\nInput Switch Sel: %d (%s)\r\n",
    			     data, ((data==0) ? "TPG" : "HDMI"));
        }
        break;

      case 14:
    	XPeriph_TpgDbgReportStatus(pPeriph);
    	break;

      case 15:
    	XVphy_LogDisplay(pVphyss->HdmiRx->VphyPtr);
    	break;

      case 16: //HDMI Rx
    	xil_printf("****** HDMI Rx Status ******\r\n");
    	XV_HdmiRxSs_ReportTiming(pInpss->Hdmi.HdmiRxSsPtr);
    	XV_HdmiRxSs_ReportLinkQuality(pInpss->Hdmi.HdmiRxSsPtr);
    	break;

      case 17: //HDMI Tx
    	xil_printf("****** HDMI Tx Status ******\r\n");
    	XV_HdmiTxSs_ReportTiming(pOutss->Hdmi.HdmiTxSsPtr);
    	break;

      case 18: //Mixer
        XVMix_DbgReportStatus(pOutss->MixerPtr);
        SubMenuExit = FALSE;
        while(!SubMenuExit)
        {
    		u32 layerIndex;

    		xil_printf("\r\n\r\n");
    		xil_printf("Layer Index [0:Master, 1-7:Layer, 8:Logo, 9:Exit] -> ");
    		layerIndex = getUsrSel(pPeriph);
    		if(layerIndex == 9) {
    			SubMenuExit = TRUE;
    		} else if(layerIndex < 9) {
    			XVMix_DbgLayerInfo(pOutss->MixerPtr, layerIndex);
    		} else {
          	    xil_printf("\r\nERR:: value out of range. Command ignored.. \r\n");
    		}
    	}
    	break;

      case 0:
        validSelection = TRUE;
    	break;

      default:
  	    break;
    }//switch
  }//while
}

/***************************************************************************
*  Process Setup Menu commands
***************************************************************************/
static void ProcessSetupMenu(XPeriph  *pPeriph,
   	                         XInss    *pInpss,
	                         XVprocSs *pVprocss,
                             XOutss   *pOutss,
	                         XVphyss  *pVphyss)
{
  u32 validSelection = FALSE;
  u8 selval;

  while(!validSelection)
  {
    show_SetupMenu();
    selval = getUsrSel(pPeriph);

    switch(selval)
    {
      case 1:
        //toggle zoom mode
    	XOutss_VideoMute(pOutss, TRUE);
        XVprocSs_SetZoomMode(pVprocss, !(XVprocSs_IsZoomModeOn(pVprocss)));
   	    XEvnthdlr_GenEvent(pSysEvent, XEVENT_DISP_UPSCALE_ZOOM_MODE);
   	    validSelection = TRUE;
	    break;

      case 2:
        //toggle pip mode
        {
 	       XOutss_VideoMute(pOutss, TRUE);
           XVprocSs_SetPipMode(pVprocss, !(XVprocSs_IsPipModeOn(pVprocss)));
           XEvnthdlr_GenEvent(pSysEvent, XEVENT_DISP_DNSCALE_PIP_MODE);
           validSelection = TRUE;
        }
	    break;

      case 3: //ZOOM Win
        {
          XVidC_VideoWindow win;
    	  int status;

    	  if(XVprocSs_IsZoomModeOn(pVprocss)) {
    	    xil_printf("\r\nERR:: Cannot change window when Zoom Mode is ON\r\n");
    	  } else {
  	        xil_printf("\r\n\r\n");
    	    status = getUserWin(pPeriph, pVprocss, &win);
    	    if(status == XST_SUCCESS) {
    	      XVprocSs_SetZoomPipWindow(pVprocss, XVPROCSS_ZOOM_WIN, &win);
    	    }
    	  }
        }
        break;

      case 4: //PIP Win
        {
          XVidC_VideoWindow win;
    	  int status;

    	  if(XVprocSs_IsPipModeOn(pVprocss)) {
    	    xil_printf("\r\nERR:: Cannot change window when PIP Mode is ON\r\n");
    	  } else {
  	        xil_printf("\r\n\r\n");
    	    status = getUserWin(pPeriph, pVprocss, &win);
    	    if(status == XST_SUCCESS) {
    		  XVprocSs_SetZoomPipWindow(pVprocss, XVPROCSS_PIP_WIN, &win);
    	    }
    	  }
        }
        break;

      case 5: //PIP Bkgnd Color
        {
          u16 newVal;
          u32 SubMenuExit = FALSE;
          XLboxColorId BkgndCol[] = {
             XLBOX_BKGND_BLACK,
             XLBOX_BKGND_WHITE,
             XLBOX_BKGND_RED,
             XLBOX_BKGND_GREEN,
             XLBOX_BKGND_BLUE};

          while(!SubMenuExit)
          {
  	        xil_printf("\r\n\r\n");
  	        xil_printf(" 1: Black\r\n");
  	        xil_printf(" 2: White\r\n");
  	        xil_printf(" 3: Red\r\n");
  	        xil_printf(" 4: Green\r\n");
  	        xil_printf(" 5: Blue\r\n");
  	        xil_printf(" 0: Exit\r\n");
  	        xil_printf("Enter Selection -> ");
            newVal = getUsrSel(pPeriph);
            if(newVal == 0) {
              SubMenuExit = TRUE;
            } else if(newVal<6) {
              XVprocSs_SetPIPBackgroundColor(pVprocss, BkgndCol[newVal-1]);
            } else {
              xil_printf("\r\nERR:: value out of range. Command ignored.. \r\n");
            }
          }
        }
        break;

      case 6: //Select active input
        {
          u32 newVal;

    	  xil_printf("\r\n\r\n");
          xil_printf("Active Input [0:TPG, 1:HDMI] -> ");
      	  newVal = getUsrSel(pPeriph);

      	  switch(newVal) {
      	     case 0: //TPG
      	    	if((XInss_GetInputSourceSel(pInpss) != XINSS_SOURCE_TPG)) {
  	  	             XSys_SetActiveInput(pPeriph,
  	  	        		                 pInpss,
									     pVprocss,
									     pOutss,
									     XINSS_SOURCE_TPG);
      	             validSelection = TRUE;
      	    	} else {
   	        	     xil_printf("\r\nERR:: TPG is the active input. Command ignored\r\n");
      	    	}
      	        break;

      	     case 1: //HDMI - Also set it as default input source
      	        if((XInss_GetInputSourceSel(pInpss) != XINSS_SOURCE_HDMI_RX)) {
      	  	         XSys_SetActiveInput(pPeriph,
      	  	        		             pInpss,
										 pVprocss,
										 pOutss,
										 XINSS_SOURCE_HDMI_RX);
       	             validSelection = TRUE;
      	        } else {
    	        	 xil_printf("\r\nERR:: HDMI is the active input. Command ignored\r\n");
      	        }
      	        break;

      	     default:
    	        xil_printf("\r\nERR:: value out of range. Command ignored.. \r\n");
      	        break;
          }
        }
        break;

      case 0:
        validSelection = TRUE;
        break;

      default:
  	    break;
    }//switch
  }//while
}

/***************************************************************************
*  Process Mixer Menu commands
***************************************************************************/
static void ProcessMixerMenu(XPeriph  *pPeriph,
	                         XVprocSs *pVprocss,
                             XOutss   *pOutss)
{
  u32 validSelection = FALSE;
  u32 SubMenuExit;
  u8 selval;

  while(!validSelection)
  {
	show_MixerMenu();
	SubMenuExit = FALSE;
    selval = getUsrSel(pPeriph);

    switch(selval)
    {
      case 1: //Mixer Layer Enable
        while(!SubMenuExit)
        {
  	      u32 layerIndex;
  	      u32 NumLayers;

  	      xil_printf("\r\n\r\n");
  	      NumLayers = XVMix_GetNumLayers(pOutss->MixerPtr);
  	      xil_printf("Layer Index [0:Master, 1-%d:Layer, 8:Logo, 9:Exit] -> ", (NumLayers-1));
  	      layerIndex = getUsrSel(pPeriph);
  	      if(layerIndex == 9) {
  	    	SubMenuExit = TRUE;
  	      } else if((layerIndex < NumLayers) ||
  	    		    (layerIndex == 8)) {
  	      	XVMix_LayerEnable(pOutss->MixerPtr, layerIndex);
  	      } else {
            xil_printf("\r\nERR:: value out of range. Command ignored.. \r\n");
  	      }
        }
        break;

      case 2: //Mixer Layer Disable
        while(!SubMenuExit)
        {
    	  u32 layerIndex;
    	  u32 NumLayers;

    	  xil_printf("\r\n\r\n");
  	      NumLayers = XVMix_GetNumLayers(pOutss->MixerPtr);
    	  xil_printf("Layer Index [0:Master, 1-%d:Layer, 8:Logo, 9:Exit] -> ",(NumLayers-1));
    	  layerIndex = getUsrSel(pPeriph);
    	  if(layerIndex == 9) {
    		  SubMenuExit = TRUE;
  	      } else if((layerIndex < NumLayers) ||
  	    		    (layerIndex == 8)) {
    	  	XVMix_LayerDisable(pOutss->MixerPtr, layerIndex);
    	  } else {
            xil_printf("\r\nERR:: value out of range. Command ignored.. \r\n");
    	  }
        }
        break;

      case 3: //Mixer Alpha
        while(!SubMenuExit)
        {
  	      u32 layerIndex, newVal;
  	      u32 IsEnabled, NumLayers;

  	      xil_printf("\r\n\r\n");
  	      NumLayers = XVMix_GetNumLayers(pOutss->MixerPtr);
  	      xil_printf("Layer Index [1-%d:Layer, 8:Logo, 9:Exit] -> ",(NumLayers-1));
  	      layerIndex = getUsrSel(pPeriph);

  	      if(layerIndex == 9) {
  	    	SubMenuExit = TRUE;
  	      } else if((layerIndex < NumLayers) ||
  	    		    (layerIndex == 8)) {

  	  	    //Check if alpha is possible
  	  	    if(layerIndex < 8) {
  	  	    	IsEnabled = XVMix_IsAlphaEnabled(pOutss->MixerPtr, layerIndex);
  	  	    } else { //logo
  	  	    	IsEnabled = XVMix_IsLogoEnabled(pOutss->MixerPtr);
  	  	    }

  	  	    if(IsEnabled) {
	  	      xil_printf("\r\n\r\n");
	  	      xil_printf("Enter Alpha Value (0-256) -> ");
	  	      newVal = getUsrSel(pPeriph);
	  	      if(newVal <= XVMIX_ALPHA_MAX) {
	  	        XVMix_SetLayerAlpha(pOutss->MixerPtr, layerIndex, newVal);
	  	      } else {
	  	        xil_printf("\r\nERR:: value out of range. Try again \r\n");
	  	      }
  	        } else {
    	        xil_printf("\r\n <Alpha is disabled in hw for this layer> \r\n");
  	        }
  	      } else {
            xil_printf("\r\nERR:: value out of range. Command ignored.. \r\n");
  	      }
        }
        break;

      case 4: //Mixer Scale Factor
        while(!SubMenuExit)
        {
  	      u32 layerIndex, newVal;
  	      u32 IsEnabled, NumLayers;
  	      int Status;

  	      xil_printf("\r\n\r\n");
  	      NumLayers = XVMix_GetNumLayers(pOutss->MixerPtr);
  	      xil_printf("Layer Index [1-%d:Layer, 8:Logo, 9:Exit] -> ", (NumLayers-1));
  	      layerIndex = getUsrSel(pPeriph);

  	      if(layerIndex == 9) {
  	    	SubMenuExit = TRUE;
  	      } else if((layerIndex < NumLayers) ||
  	    		    (layerIndex == 8)) {

  	  	    if(layerIndex < 8) {
  	  	    	IsEnabled = XVMix_IsScalingEnabled(pOutss->MixerPtr, layerIndex);
  	  	    } else { //logo
  	  	    	IsEnabled = XVMix_IsLogoEnabled(pOutss->MixerPtr);
  	  	    }

  	  	    if(IsEnabled) {
	  	  	  xil_printf("\r\n\r\n");
	  	  	  xil_printf("Enter Scale Value (1, 2, 4) -> ");
	  	  	  newVal = getUsrSel(pPeriph);
	  	  	  if ((newVal == 1) || (newVal == 2) || (newVal == 4)) {
	  	  	    newVal = ((newVal == 1) ? XVMIX_SCALE_FACTOR_1X :
	  	  		  	      (newVal == 2) ? XVMIX_SCALE_FACTOR_2X :
	  	  			    	 	          XVMIX_SCALE_FACTOR_4X);

	  	  	    XVMix_LayerDisable(pOutss->MixerPtr, layerIndex);
	  	  	    MB_Sleep(100);

	  	  	    Status = XVMix_SetLayerScaleFactor(pOutss->MixerPtr, layerIndex, newVal);

	  	  	    MB_Sleep(100);
	  	  	    XVMix_LayerEnable(pOutss->MixerPtr, layerIndex);

	  	  	    if(Status != XST_SUCCESS) { //command failed
	  	  	    	//Let user know why
	  	  	    	if(Status == XVMIX_ERR_LAYER_WINDOW_INVALID) {
	  	  	    	  xil_printf("ERR:: Scale Factor will cause window to go out of frame boundary\r\n");
	  	  	    	}
	  	  	    }
	  	  	  } else {
	  	  	    xil_printf("\r\nERR:: Scale value out of range. Try again \r\n");
	  	  	  }
  	  	    } else {
	  	  	    xil_printf("\r\n <Scaling is disabled in hw for this layer> \r\n");
  	  	    }
  	      } else {
            xil_printf("\r\nERR:: value out of range. Command ignored.. \r\n");
  	      }
        }
        break;

      case 5: //Mixer Logo ColorKey
        while(!SubMenuExit)
        {
  	      u8 index;
  	      XVMix_LogoColorKey Data;
  	      char ChStr[3] = {'R', 'G', 'B'};

  	      xil_printf("\r\n\r\n");
  	      xil_printf("Enter Color Key RGB Min Max (0-255)\r\n");
  	      for (index=0; index<3; ++index) {
              xil_printf("  Enter %c Min: ",ChStr[index]);
              Data.RGB_Min[index] = getUsrSel(pPeriph);
  	      }
  	      xil_printf("\r\n");
  	      for (index=0; index<3; ++index) {
              xil_printf("  Enter %c Max: ",ChStr[index]);
              Data.RGB_Max[index] = getUsrSel(pPeriph);
  	      }
  	      XVMix_SetLogoColorKey(pOutss->MixerPtr, Data);
  	      SubMenuExit = TRUE;
      }
      break;

      case 6: //Mixer Window
        while(!SubMenuExit)
        {
  	      u32 layerIndex, Stride, NumLayers;
  	      int Status;
  	      XVidC_VideoWindow Win;
  	      FrameBuf *BufCfgPtr;

  	      xil_printf("\r\n\r\n");
  	      NumLayers = XVMix_GetNumLayers(pOutss->MixerPtr);
  	      xil_printf("Layer Index [1-%d:Layer, 8:Exit] -> ", (NumLayers-1));
  	      layerIndex = getUsrSel(pPeriph);
  	      if(layerIndex == 8) {
  	    	SubMenuExit = TRUE;
  	      } else if((layerIndex > 0) && (layerIndex < NumLayers)) {
	  	  	xil_printf("\r\n\r\n");
	  	  	xil_printf("(StartX and Width must be multiple of %d)\r\n",
	  	  			       pOutss->MixerPtr->Mix.Config.PixPerClk);

            Status = getUserWin(pPeriph, pVprocss, &Win);
	  	    if(Status == XST_SUCCESS) {

	  	      BufCfgPtr = &pOutss->MixerFrameBuf[layerIndex-1];
	  	      if((Win.Width  <= BufCfgPtr->FrameWidth) &&
	  	         (Win.Height <= BufCfgPtr->FrameHeight)) {

	  	        Stride = ((BufCfgPtr->CFmt == XVIDC_CSF_YCRCB_422) ? 2 : 4);
	  	        Stride *= BufCfgPtr->FrameWidth;

	  	  	    XVMix_LayerDisable(pOutss->MixerPtr, layerIndex);
	  	  	    MB_Sleep(100);

	  	        Status = XVMix_SetLayerWindow(pOutss->MixerPtr,
	  	         	  	                      layerIndex,
	  	         		                      &Win,
	  	         		                      Stride);

	  	  	    MB_Sleep(100);
	  	  	    XVMix_LayerEnable(pOutss->MixerPtr, layerIndex);

	  	        if(Status != XST_SUCCESS) { //command failed
	  	          if (Status == XVMIX_ERR_LAYER_WINDOW_INVALID) {
	  	  	        xil_printf("ERR:: New Window will go out of frame boundary\r\n");
	  	          } else {
	  	            xil_printf("ERR:: Unable to Set Layer %d window (%d, %d, %d, %d)\r\n", layerIndex,
	  	         	   	       Win.StartX, Win.StartY, Win.Width, Win.Height);
	  	          }
	  	        } else {
	  	        	//Update window width
	  	            BufCfgPtr->Win = Win;
	  	        }
	  	      } else {
            	 xil_printf("\r\nERR:: Window coordinates out of default config. range Ignoring command\r\n");
	  	      }
	  	    }
  	      } else {
        	 xil_printf("\r\nERR:: value out of range. Command ignored.. \r\n");
  	      }
        }
        break;

      case 7: //Move Mixer Window
        while(!SubMenuExit)
        {
    	  u32 layerIndex, Stride, NumLayers;
  	      int Status;
  	      XVidC_VideoWindow Win;
  	      FrameBuf *BufCfgPtr;

  	      xil_printf("\r\n\r\n");
  	      NumLayers = XVMix_GetNumLayers(pOutss->MixerPtr);
  	      xil_printf("Layer Index [1-%d:Layer, 8:Exit] -> ", (NumLayers-1));
  	      layerIndex = getUsrSel(pPeriph);
  	      if(layerIndex == 8) {
  	    	SubMenuExit = TRUE;
  	      } else if((layerIndex > 0) && (layerIndex < NumLayers)) {

	 	    xil_printf("\r\n\r\n");
	 	    xil_printf("(StartX must be multiple of %d)\r\n",
	 	  		       pOutss->MixerPtr->Mix.Config.PixPerClk);
	 	    xil_printf("Enter New Start X: ");
	 	    Win.StartX = getUsrSel(pPeriph);
	 	    xil_printf("Enter New Start Y: ");
	 	    Win.StartY = getUsrSel(pPeriph);

            BufCfgPtr = &pOutss->MixerFrameBuf[layerIndex-1];
            Win.Width  = BufCfgPtr->Win.Width;
            Win.Height = BufCfgPtr->Win.Height;
            Stride = ((BufCfgPtr->CFmt == XVIDC_CSF_YCRCB_422) ? 2 : 4);
	 	    Stride *= BufCfgPtr->FrameWidth;

	 	    Status = XVMix_SetLayerWindow(pOutss->MixerPtr,
	 	       	  	                      layerIndex,
	 	        		                  &Win,
	 	        		                  Stride);
            if(Status != XST_SUCCESS) {
              if (Status == XVMIX_ERR_LAYER_WINDOW_INVALID) {
  	    	    xil_printf("ERR:: Move will cause window to go out of frame boundary\r\n");
              } else {
	 	        xil_printf("ERR:: Unable to Set Layer %d window (%d, %d, %d, %d)\r\n", layerIndex,
	 	         			    Win.StartX, Win.StartY, Win.Width, Win.Height);
              }
	 	    }
  	      } else {
     	     xil_printf("\r\nERR:: value out of range. Command ignored.. \r\n");
  	      }
        }
        break;


      case 0:
  	    validSelection = TRUE;
  	    break;

      default:
    	break;
    }
  }
}
