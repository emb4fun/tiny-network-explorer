/**************************************************************************
*  Copyright (c) 2004 by Michael Fischer.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without 
*  modification, are permitted provided that the following conditions 
*  are met:
*  
*  1. Redistributions of source code must retain the above copyright 
*     notice, this list of conditions and the following disclaimer.
*
*  2. Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the 
*     documentation and/or other materials provided with the distribution.
*
*  3. Neither the name of the author nor the names of its contributors may 
*     be used to endorse or promote products derived from this software 
*     without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL 
*  THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS 
*  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
*  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF 
*  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
*  SUCH DAMAGE.
*
***************************************************************************
*  History:
*
*  11.04.2004  mifi  First Version.
*  06.10.2019  mifi  Change name from nutsetup to Tiny Network Explorer.
*  15.04.2021  mifi  Added support for multiple ethernet interfaces.
**************************************************************************/
#if !defined(__TNPSETUPWIN_H__)
#define __TNPSETUPWIN_H__

/**************************************************************************
*  Includes
**************************************************************************/
#include <windows.h>
#include <commctrl.h>

#include "tnpsetup.h"

/**************************************************************************
*  Global Definitions
**************************************************************************/

#define SM_EVENT_1   (WM_USER+1)
#define SM_EVENT_2   (WM_USER+2)
#define SM_EVENT_3   (WM_USER+3)
#define SM_EVENT_4   (WM_USER+4)
#define SM_EVENT_5   (WM_USER+5)
#define SM_EVENT_6   (WM_USER+6)
#define SM_EVENT_7   (WM_USER+7)
#define SM_EVENT_8   (WM_USER+8)

enum {
  TNP_ITEM_MAC = 0,
  TNP_ITEM_IP,
  TNP_ITEM_NAME,
  TNP_ITEM_LOC,
  TNP_ITEM_LAST
};

typedef struct {
  LPSTR  aCols[TNP_ITEM_LAST];
  
  BYTE   bIsES;   
  BYTE   bMACAddress[6];
  WORD   wUseDHCP;
  DWORD  dAddress;
  DWORD  dMask;
  DWORD  dGateway;
  char  szName[TNP_NAME_LEN+1];
  char  szLocation[TNP_LOCATION_LEN+1];
  char  szMDNSName[TNP_MDNS_NAME_LEN+1];
} TNPITEM;


/**************************************************************************
*  Macro Definitions
**************************************************************************/

/**************************************************************************
*  Functions Definitions
**************************************************************************/

int  tnp_SetupStart  (HWND hWnd);
void tnp_SetupStop   (HWND hWnd);

void tnp_SetupRead   (HWND hWnd, int nInterfaceIndex);

void tnp_SetupSearch (HWND hWnd);

void tnp_SetupUpdate (HWND hWnd, int nIndex, TNPITEM *pOld, TNPITEM *pNew);

#endif /* !__TNPSETUPWIN_H__ */

/*** EOF ***/
