/**************************************************************************
*  Copyright (c) 2004 by Michael Fischer (www.emb4fun.de).
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
*  11.04.2004  mifi  First Version of NUTSetup.
*  02.09.2019  mifi  Rename NUTSetup project to xFind.
*  06.10.2019  mifi  Change name from nutsetup to Tiny Network Explorer.
*  18.07.2020  mifi  Added suppot for TNP_SETUP_RESPONSE_ES.
**************************************************************************/
#define __MAIN_C__

/*=======================================================================*/
/*  Includes                                                             */
/*=======================================================================*/
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <commctrl.h>

#include "resource.h"
#include "version.h"
#include "tnpsetupwin.h"

/*=======================================================================*/
/*  All Structures and Common Constants                                  */
/*=======================================================================*/
#define APPCLASS              "MF20200718_CLASS"
#define MUTEXNAME             "MF20200718"

#define WIDTH(x)              (x.right - x.left)
#define HEIGHT(x)             (x.bottom - x.top)

#define LIST_VIEW_MAX_HEADER  4

/*=======================================================================*/
/*  Definition of all global Data                                        */
/*=======================================================================*/

/*=======================================================================*/
/*  Definition of all extern Data                                        */
/*=======================================================================*/

/*=======================================================================*/
/*  Definition of all local Data                                         */
/*=======================================================================*/

static HINSTANCE   ghInstApp  = NULL;
static HINSTANCE   ghInstLang = NULL;
static HWND        ghAppWnd   = NULL;

static NM_LISTVIEW gSelectedItem;
static TNPITEM     gNutChangeItem;

static char ListViewHeader[LIST_VIEW_MAX_HEADER][16] = 
{
   "MAC-Address",
   "IP-Address",
   "Name",
   "Location"
};

/*=======================================================================*/
/*  Definition of all local Procedures                                   */
/*=======================================================================*/

/************************************************************/
/*  DoesInstanceExist                                       */
/************************************************************/
static BOOL DoesInstanceExist (int nCmdShow)
{
   HANDLE hMutex;
   HWND hMainWnd;

   //
   //  hPrevInstance-Check Method doesn't work in Win32
   //  reason: hPrevInstance remains NULL anyway
   //  to do:  use Mutex Method instead
   //
   if (hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, MUTEXNAME)) 
   {
      if (hMainWnd = FindWindow(APPCLASS, NULL)) 
      {
         ShowWindow(hMainWnd, nCmdShow);
         BringWindowToTop(hMainWnd);
         return (TRUE);
      }
   } 
   else 
   {
      //
      // If it does not exist, we create the mutex
      //
      hMutex = CreateMutex(NULL, FALSE, MUTEXNAME);
      hMutex = hMutex;
   }

  return(FALSE);
} /* DoesInstanceExist */

/************************************************************/
/*  DrawButtonBitmap                                        */
/************************************************************/
static BOOL DrawButtonBitmap (HINSTANCE hInst, 
                              HWND hDlg, HWND hWnd, HDC hDC, 
                              WORD wBitmap, BOOL fCenter, int ROPCode)
{
   BOOL     fError = FALSE;
   HDC      hMemoryDC;
   HBITMAP  hBitmap;
   BITMAP    bm;
   RECT    rcClient;
   LONG     lHeight, lWidth;

   if (hDC)
   {
      GetWindowRect(hWnd, (LPRECT)&rcClient);

      lHeight = rcClient.bottom - rcClient.top;
      lWidth  = rcClient.right  - rcClient.left;

      ScreenToClient(hDlg, (POINT FAR*)&rcClient); 
      hMemoryDC = CreateCompatibleDC(hDC);

      if (hMemoryDC) 
      {
         hBitmap   = LoadBitmap(hInst, MAKEINTRESOURCE(wBitmap));
         GetObject(hBitmap,sizeof(BITMAP),(LPSTR)&bm);
         SelectObject(hMemoryDC,hBitmap);

         lHeight = (lHeight - bm.bmHeight) >> 1;
         lWidth  = (lWidth  - bm.bmWidth) >> 1;

         if (fCenter)
         {
            BitBlt(hDC,
                   lWidth, lHeight,
                   bm.bmWidth, bm.bmHeight,
                   hMemoryDC, 0, 0, ROPCode);
         }
         else 
         {
            BitBlt(hDC,
                   0, 0,
                   bm.bmWidth, bm.bmHeight,
                   hMemoryDC, 0, 0, ROPCode);
         }

         DeleteObject (hBitmap);
         DeleteDC (hMemoryDC);
      }
   } 
   else
   {
      fError = TRUE;
   }  

   return(fError);
} /* DrawButtonBitmap */

/************************************************************/
/*  AboutProc                                               */
/************************************************************/
static LRESULT CALLBACK AboutProc (HWND hDlg, UINT iMsg,
                                   WPARAM wParam, LPARAM lParam)
{
   switch (iMsg)
   {
      case WM_INITDIALOG:
      {
         char szTmp[_MAX_PATH];

         _snprintf(szTmp, sizeof(szTmp), "Tiny Network Explorer - v%s", VER_PRODUCTVERSION_STR);
         SetDlgItemText(hDlg, IDC_VERSION, szTmp);

         ShowWindow(hDlg, SW_SHOW);

         SetFocus (hDlg);

         return(TRUE);
         break;
      } /* WM_INITDIALOG */

      case WM_DRAWITEM:
      {
         LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
         HDC  hDC  = pdis->hDC;
         HWND hWnd = pdis->hwndItem;
      
         switch (pdis->CtlID)
         {
            case IDC_CHIP:
            {
               if (pdis->itemAction == ODA_DRAWENTIRE)
               {
                  DrawButtonBitmap(ghInstApp, hDlg, hWnd, hDC, IDB_CHIP_SHAPE, FALSE, SRCPAINT);
                  DrawButtonBitmap(ghInstApp, hDlg, hWnd, hDC, IDB_CHIP,       FALSE, SRCAND);
               }
               break;
            }
         }
         break;
      } /* WM_DRAWITEM */

      case WM_COMMAND:
      {
         switch (LOWORD (wParam))
         {
            case IDOK:
            {
               EndDialog(hDlg,0);
               return(FALSE);
               break;                        
            } /* IDOK */
         }
         break;
      } /* WM_COMMAND */
    
      case WM_CLOSE:
      {
         EndDialog(hDlg,0);
         return(FALSE);
         break;
      } /* WM_CLOSE */
    
   } /* iMsg */

   return(FALSE);
} /* AboutProc */

/************************************************************/
/*  SetupMainWndControls                                    */
/************************************************************/
static int SetupMainWndControls (HWND hWnd)
{
   int        nError = FALSE;
   char      *pStr;
   LV_COLUMN lvColumn;
   int        nSize, nDelta, nRest;
   int        nIndex;
   HWND       hListView;

   hListView = GetDlgItem(hWnd, IDC_LIST1);

   for(nIndex=0; nIndex<LIST_VIEW_MAX_HEADER; nIndex++)
   {
      pStr = ListViewHeader[nIndex]; 

      lvColumn.mask       = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
      lvColumn.fmt        = LVCFMT_LEFT;
      lvColumn.pszText    = pStr;
      lvColumn.cchTextMax = sizeof(pStr);
      lvColumn.iSubItem   = nIndex;
      ListView_InsertColumn(hListView, nIndex, &lvColumn);
   }

   for(nIndex=0; nIndex<LIST_VIEW_MAX_HEADER; nIndex++)
   {
      ListView_SetColumnWidth(hListView, nIndex, LVSCW_AUTOSIZE_USEHEADER); 
   }
  
   nSize  = ListView_GetColumnWidth(hListView, 0);
   nSize += ListView_GetColumnWidth(hListView, 1);
   nSize += ListView_GetColumnWidth(hListView, 2);
   nSize += ListView_GetColumnWidth(hListView, 3);

   nDelta = nSize / 26;  
   ListView_SetColumnWidth(hListView, 0, 5*nDelta); 
   ListView_SetColumnWidth(hListView, 1, 5*nDelta); 
   ListView_SetColumnWidth(hListView, 2, 10*nDelta);
  
   nRest = nSize - (nDelta*20);   
   ListView_SetColumnWidth(hListView, 3, nRest); 

   ListView_SetExtendedListViewStyle(hListView, LVS_EX_GRIDLINES);
  
   return(nError);
} /* SetupMainWndControls */

/************************************************************/
/*  DeSelectAll                                             */
/************************************************************/
static void DeSelectAll (HWND hWnd)
{
   int  nIndex;
   int  nMaxItemCount;  
   HWND hListView;
  
   hListView = GetDlgItem(hWnd, IDC_LIST1);

   nMaxItemCount = ListView_GetItemCount(hListView);
  
   for(nIndex=0; nIndex<nMaxItemCount; nIndex++)
   {
      ListView_SetItemState(hListView, nIndex, 
                            0, 
                            LVIS_CUT | LVIS_DROPHILITED | LVIS_FOCUSED | LVIS_SELECTED);
   }
} /* DeSelectAll */

/************************************************************/
/*  DeSelectItem                                            */
/************************************************************/
static void DeSelectItem (HWND hListView)
{
   ListView_SetItemState(hListView, gSelectedItem.iItem, 
                         0, 
                         LVIS_CUT | LVIS_DROPHILITED | LVIS_FOCUSED | LVIS_SELECTED);
} /* DeSelectItem */

/************************************************************/
/*                                                          */
/*  ListViewCompareFunc                                     */
/*                                                          */
/*  Sorts the list view control. It is a comparison         */
/*  function. Returns a negative value if the first item    */
/*  should precede the second item, a positive value if the */
/*  first item should follow the second item, and zero if   */
/*  the items are equivalent.                               */
/*                                                          */
/*  lParam1 and lParam2                                     */
/*                                                          */
/*  Item data for the two items (in this case, pointers to  */
/*  application-defined TNPITEM structures)                 */
/*                                                          */
/*  lParamSort                                              */
/*                                                          */
/*  Value specified by the LVM_SORTITEMS message            */
/*  (in this case, the index of the column to sort)         */
/************************************************************/
static int CALLBACK ListViewCompareFunc (LPARAM lParam1, 
                                         LPARAM lParam2, 
                                         LPARAM lParamSort) 
{ 
   TNPITEM *pItem1 = (TNPITEM *) lParam1; 
   TNPITEM *pItem2 = (TNPITEM *) lParam2; 
   int        iCmp;

   // Compare the specified column. 
   iCmp = lstrcmpi(pItem1->aCols[lParamSort], 
                   pItem2->aCols[lParamSort]); 
 
   // Return the result if nonzero, or compare the 
   // first column otherwise. 
   return(iCmp != 0) ? iCmp : lstrcmpi(pItem1->aCols[0], pItem2->aCols[0]); 
} /* ListViewCompareFunc */ 

/************************************************************/
/*  ChangeProc                                              */
/************************************************************/
static LRESULT CALLBACK ChangeProc (HWND hDlg, UINT iMsg,
                                    WPARAM wParam, LPARAM lParam)
{
   struct in_addr iaEthernut;
   char           *pAddress;  

   switch (iMsg)
   {
      case WM_INITDIALOG:
      {
         char szTmp[_MAX_PATH];

         sprintf(szTmp, "Change setting of %02X:%02X:%02X:%02X:%02X:%02X",
                 gNutChangeItem.bMACAddress[0], gNutChangeItem.bMACAddress[1],
                 gNutChangeItem.bMACAddress[2], gNutChangeItem.bMACAddress[3],
                 gNutChangeItem.bMACAddress[4], gNutChangeItem.bMACAddress[5]);     
         SetWindowText(hDlg,szTmp);
         ShowWindow(hDlg, SW_SHOW);

         //
         // Set Name
         //
         sprintf(szTmp, "Name: %s", gNutChangeItem.szName);
         SetDlgItemText(hDlg, IDC_NAME, szTmp);

         //
         // Set IPAddress
         // 
         iaEthernut.s_addr = gNutChangeItem.dAddress;
         pAddress = inet_ntoa(iaEthernut);
         SetDlgItemText(hDlg, IDC_IPADDRESS, pAddress);

         //
         // Set Mask
         // 
         iaEthernut.s_addr = gNutChangeItem.dMask;
         pAddress = inet_ntoa(iaEthernut);
         SetDlgItemText(hDlg, IDC_MASK, pAddress);

         //
         // Set Gateway
         // 
         iaEthernut.s_addr = gNutChangeItem.dGateway;
         pAddress = inet_ntoa(iaEthernut);
         SetDlgItemText(hDlg, IDC_GATEWAY, pAddress);

         //
         // Set Info
         //
         sprintf(szTmp, "%s", gNutChangeItem.szLocation);
         SetDlgItemText(hDlg, IDC_LOCATION, szTmp);
         SendMessage(GetDlgItem(hDlg, IDC_LOCATION), EM_LIMITTEXT, 16, 0);

         //
         // Set DHCP
         // 
         if (1 == gNutChangeItem.wUseDHCP)
         {
            CheckDlgButton(hDlg, IDC_DHCP, BST_CHECKED);

            EnableWindow(GetDlgItem(hDlg, IDC_IPADDRESS), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_MASK),      FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_GATEWAY),   FALSE);
         }
         else
         {
            CheckDlgButton(hDlg, IDC_DHCP, BST_UNCHECKED);

            EnableWindow(GetDlgItem(hDlg, IDC_IPADDRESS), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_MASK),      TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_GATEWAY),   TRUE);
         }

         EnableWindow(GetDlgItem(hDlg, IDC_LOCATION), TRUE);
         
         /* Handle IsES */
         if (0 == gNutChangeItem.bIsES)
         {
            EnableWindow(GetDlgItem(hDlg, IDC_DHCP),      FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_IPADDRESS), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_MASK),      FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_GATEWAY),   FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_LOCATION),  FALSE);
            EnableWindow(GetDlgItem(hDlg, IDOK),          FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_DEFAULT),   FALSE);
         }

         SetFocus(hDlg);

         return(TRUE);
         break;
      } /* WM_INITDIALOG */

      case WM_COMMAND:
      {
         switch (LOWORD (wParam))
         {
            case IDC_DHCP:
            {
               UINT Result;

               Result = IsDlgButtonChecked(hDlg, IDC_DHCP);
               if (Result == BST_CHECKED)
               {
                  gNutChangeItem.wUseDHCP = 1;

                  EnableWindow(GetDlgItem(hDlg, IDC_IPADDRESS), FALSE);
                  EnableWindow(GetDlgItem(hDlg, IDC_MASK),      FALSE);
                  EnableWindow(GetDlgItem(hDlg, IDC_GATEWAY),   FALSE);
               }
               else
               {
                  gNutChangeItem.wUseDHCP = 0;

                  EnableWindow(GetDlgItem(hDlg, IDC_IPADDRESS), TRUE);
                  EnableWindow(GetDlgItem(hDlg, IDC_MASK),      TRUE);
                  EnableWindow(GetDlgItem(hDlg, IDC_GATEWAY),   TRUE);
               }
               break;
            } /* IDC_DHCP */
                   
            case IDOK:
            {
               char szTmp[_MAX_PATH];
               UINT   Result;

               Result = IsDlgButtonChecked(hDlg, IDC_DHCP);
               if (Result == BST_CHECKED)
               {
                  gNutChangeItem.wUseDHCP = 1;
               }
               else
               {
                  gNutChangeItem.wUseDHCP = 0;
               }

               GetDlgItemText(hDlg, IDC_IPADDRESS, szTmp, _MAX_PATH);
               gNutChangeItem.dAddress = inet_addr(szTmp);

               GetDlgItemText(hDlg, IDC_MASK, szTmp, _MAX_PATH);
               gNutChangeItem.dMask = inet_addr(szTmp);

               GetDlgItemText(hDlg, IDC_GATEWAY, szTmp, _MAX_PATH);
               gNutChangeItem.dGateway = inet_addr(szTmp);

               GetDlgItemText(hDlg, IDC_LOCATION, szTmp, _MAX_PATH);
               _snprintf(gNutChangeItem.szLocation, sizeof(gNutChangeItem.szLocation)-1, "%s", szTmp);

               //
               // Check for correct input
               //
               if( (gNutChangeItem.dAddress != -1) &&
                   (gNutChangeItem.dMask    != -1) &&
                   (gNutChangeItem.dGateway != -1) )
               {
                  EndDialog(hDlg,1);
               } 
               else
               {
                  EndDialog(hDlg,-1);
               }

               return(FALSE);
               break;                        
            } /* IDOK */

            case IDC_DEFAULT:
            {
               SetDlgItemText(hDlg, IDC_IPADDRESS, "192.168.1.200");
               SetDlgItemText(hDlg, IDC_MASK,      "255.255.255.0");
               SetDlgItemText(hDlg, IDC_GATEWAY,   "0.0.0.0");
               SetDlgItemText(hDlg, IDC_LOCATION,  "***");
                
               CheckDlgButton(hDlg, IDC_DHCP, BST_CHECKED);
               EnableWindow(GetDlgItem(hDlg, IDC_IPADDRESS), FALSE);
               EnableWindow(GetDlgItem(hDlg, IDC_MASK),      FALSE);
               EnableWindow(GetDlgItem(hDlg, IDC_GATEWAY),   FALSE);
               break;
            } /* IDC_DEFAULT */
            
            case IDCANCEL:
            {
               EndDialog(hDlg,-1);
               return(FALSE);
               break;                        
            } /* IDCANCEL */
         }
         break;
      } /* WM_COMMAND */
    
      case WM_CLOSE:
      {
         EndDialog(hDlg,-1);
         return(FALSE);
         break;
      } /* WM_CLOSE */
    
   } /* iMsg */

   return(FALSE);
} /* ChangeProc */

/************************************************************/
/*  OnDoubleClick                                           */
/*                                                          */
/*  Processes the NM_DBLCLK notification message.           */
/*                                                          */
/************************************************************/
static void OnDoubleClick (HWND hWnd, HWND hListView, NMITEMACTIVATE *pnmv)
{
   BOOL            fOK;
   int             nResult;
   int             nSelektion;
   LV_ITEM        lvItem;
   TNPITEM        *pItem;
   LVHITTESTINFO    Info;
   struct in_addr iaEthernut;
   char           *pAddress;
   char           szURL[_MAX_PATH];  


   Info.pt.x = pnmv->ptAction.x;
   Info.pt.y = pnmv->ptAction.y;

   nSelektion = ListView_SubItemHitTest(hListView, &Info);
   if ((nSelektion != -1) && (Info.iSubItem == 1))
   {
      /*
       * Start the browser
       */ 
      lvItem.mask  = LVIF_PARAM;
      lvItem.iItem = Info.iItem;
      fOK = ListView_GetItem(hListView, &lvItem);
      if (fOK)
      {
         pItem = (TNPITEM *)lvItem.lParam;
         iaEthernut.s_addr = pItem->dAddress;
         pAddress = inet_ntoa(iaEthernut);

         if (pItem->szMDNSName[0] != 0)
         {
            _snprintf(szURL, sizeof(szURL), "http://%s", pItem->szMDNSName);
         }
         else
         {
            _snprintf(szURL, sizeof(szURL), "http://%s", pAddress);
         }   
         ShellExecute(NULL, NULL, szURL, NULL, NULL, SW_MAXIMIZE ); 
      }               
   }


   /*
    * Check for change request
    */
   if (pnmv->iItem != -1)
   {
      lvItem.mask  = LVIF_PARAM;
      lvItem.iItem = pnmv->iItem;
      fOK = ListView_GetItem(hListView, &lvItem);
      if (fOK)
      {
         pItem = (TNPITEM *)lvItem.lParam;      

         memcpy(&gNutChangeItem, pItem, sizeof(TNPITEM));

         nResult = DialogBox(ghInstApp, MAKEINTRESOURCE(IDD_CHANGE),
                             hWnd, ChangeProc);
         if (nResult == 1)
         {
            //
            // Check if we got some new values
            //
            if( (pItem->wUseDHCP != gNutChangeItem.wUseDHCP)                || 
				(pItem->dAddress != gNutChangeItem.dAddress)                ||
                (pItem->dMask    != gNutChangeItem.dMask)                   ||
                (pItem->dGateway != gNutChangeItem.dGateway)                ||
                (strcmp(pItem->szLocation, gNutChangeItem.szLocation) != 0) )
            { 
               tnp_SetupUpdate(hWnd, lvItem.iItem, pItem, &gNutChangeItem); 
            }
         }
      }    
   }
} /* OnDoubleClick */

/************************************************************/
/*  OnGetDispInfo                                           */
/*                                                          */
/*  Processes the LVN_GETDISPINFO notification message.     */
/*                                                          */
/************************************************************/
static void OnGetDispInfo (HWND hListView, LV_DISPINFO *pnmv) 
{ 
   TNPITEM *pItem;

   // Provide the item or subitem's text, if requested. 
   if (pnmv->item.mask & LVIF_TEXT)
   { 
      pItem = (TNPITEM *)(pnmv->item.lParam); 
      lstrcpy(pnmv->item.pszText, 
              pItem->aCols[pnmv->item.iSubItem]); 
   } 
} /* OnGetDispInfo */

/************************************************************/
/*  OnChanged                                               */
/*                                                          */
/*  Processes the LVN_ITEMCHANGED notification message.     */
/*                                                          */
/************************************************************/
static void OnChanged (HWND hListView, NM_LISTVIEW *pnmv) 
{ 
   if (pnmv->uNewState & LVIS_SELECTED)
   {
      if (gSelectedItem.iItem != -1)
      {
         //
         // Deselect the old item
         //
         DeSelectItem(hListView);
      } 

      gSelectedItem  = *pnmv;

      //
      // Hilite the new one
      //
      ListView_SetItemState(hListView, gSelectedItem.iItem, 
                            LVIS_DROPHILITED,
                            LVIS_CUT | LVIS_DROPHILITED | LVIS_FOCUSED | LVIS_SELECTED);
   }
} /* OnChanged */ 

/************************************************************/
/*  MainWndProc                                             */
/************************************************************/
static LRESULT CALLBACK MainWndProc (HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
   switch (iMsg)
   {
      case WM_CREATE:
      {
         HMENU  hMenu;
         RECT   rDesktop;
         RECT   rApp;
         int    x, y;
         char szTmp[_MAX_PATH];

         //
         // Add new entry to the system menu
         //
         hMenu = GetSystemMenu(hWnd, FALSE);
         AppendMenu(hMenu, MF_SEPARATOR, 0, "");
         AppendMenu(hMenu, MF_STRING, IDC_ABOUT, "About");

         // 
         // Set the window caption
         //
         _snprintf(szTmp, sizeof(szTmp), "Tiny Network Explorer - v%s", VER_PRODUCTVERSION_STR);      
         SetWindowText(hWnd, szTmp);

         //
         // Set the new position in the middle of the desktop
         //
         GetWindowRect(GetDesktopWindow(), &rDesktop);
         GetWindowRect(hWnd, &rApp);
         x = (WIDTH(rDesktop) - WIDTH(rApp)) / 2;
         y = (HEIGHT(rDesktop) - HEIGHT(rApp)) / 2;
         MoveWindow(hWnd, x, y, WIDTH(rApp), HEIGHT(rApp), TRUE);
         break;
      } /* WM_CREATE */


      //
      // 1. Receive WM_CLOSE
      // 
      case WM_CLOSE:
      {
         //
         // Free all Appliction-Data 
         //
         tnp_SetupStop(hWnd);
         break;
      } /* WM_CLOSE */


      //
      // 2. After WM_CLOSE receive WM_DESTROY
      //
      case WM_DESTROY:
      {
         //
         //  Clear all AppData
         //
         PostQuitMessage(0);
         break;
      } /* WM_DESTROY */


      case WM_NOTIFY:
      { 
         int     idCtrl    = (int)wParam;
         LPNMHDR pnmh      = (LPNMHDR)lParam;
         HWND    hListView = GetDlgItem(hWnd, IDC_LIST1);

         if (idCtrl == IDC_LIST1) 
         {
            switch (pnmh->code)
            {
               case NM_DBLCLK:
               {
                  OnDoubleClick(hWnd, hListView, (NMITEMACTIVATE *)lParam); 
                  break;
               }

               case LVN_GETDISPINFO:
               {
                  OnGetDispInfo(hListView, (LV_DISPINFO *)lParam); 
                  break;
               }

               case LVN_ITEMCHANGED:
               {
                  OnChanged(hListView, (NM_LISTVIEW *)lParam);
                  break;
               } 

               // Process LVN_COLUMNCLICK to sort items by column. 
               case LVN_COLUMNCLICK:
               {
                  //
                  // Deselect the old item
                  //
                  DeSelectAll(hWnd);
    
                  #define pnm ((NM_LISTVIEW *)lParam) 
                  ListView_SortItems(pnm->hdr.hwndFrom, 
                                     ListViewCompareFunc, 
                                     (LPARAM)(pnm->iSubItem) ); 
                  #undef pnm 
                  break; 
               }
            }
         }
         break;
      } /* WM_NOTIFY */

      case WM_PAINT:
      {
         PAINTSTRUCT ps;
         HDC hDC;

         hDC = BeginPaint(hWnd, &ps);

         EndPaint(hWnd, &ps);
         break;
      } /* WM_PAINT */

      case WM_SYSCOMMAND:
      {
         switch(wParam)
         {
            case IDC_ABOUT:
            {
               SendMessage(hWnd, WM_COMMAND, IDC_LOGO, 0);
               break;
            }
            case SC_SCREENSAVE:
            {
               return(TRUE); // No ScreenSaver allowed
               break;
            }
         }
         break;
      } /* WM_SYSCOMMAND */

      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDC_LOGO:
            {
               DialogBox(ghInstApp, MAKEINTRESOURCE(IDD_ABOUT),
                         hWnd, AboutProc);
               break;
            }
            case IDC_SEARCH:
            {
               tnp_SetupSearch(hWnd);
               break;
            }
            case IDC_QUIT:
            {
               SendMessage(hWnd, WM_CLOSE, 0, 0); 
               break;
            }
         } /* end switch (LOWORD (wParam)) */
         break;
      } /* WM_COMMAND */

      case SM_EVENT_1:
      case SM_EVENT_2:
      case SM_EVENT_3:
      case SM_EVENT_4:
      case SM_EVENT_5:
      case SM_EVENT_6:
      case SM_EVENT_7:
      case SM_EVENT_8:
      {
         if (WSAGETSELECTEVENT(lParam) == FD_READ)
         {
            tnp_SetupRead(hWnd, iMsg - SM_EVENT_1);         
         }
         break;
      } /* SM_EVENT */

   } /* iMsg */

   return(DefWindowProc(hWnd, iMsg, wParam, lParam));
} /* MainWndProc */

/*=======================================================================*/
/*  All code exported                                                    */
/*=======================================================================*/

/************************************************************/
/*                       M A I N                            */
/************************************************************/
int WINAPI WinMain (HINSTANCE hInstance,
                    HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
   int      nError;
   MSG       msg;
   WNDCLASS  wndclass;
   TCHAR   szError[_MAX_PATH];     // Error message string
   WSADATA   WSAData;              // Contains details of the 
                                   // Winsocket implementation
   WORD     wVersion = WINSOCK_VERSION;

   if (DoesInstanceExist(nCmdShow))
   {
      return(FALSE);
   }

   //
   // Init common control dynamic-link library
   //
   InitCommonControls();

   // Initialize Winsocket 
   if (WSAStartup(wVersion, &WSAData) != 0)
   {
      wsprintf(szError, TEXT("WSAStartup failed. Error: %d"), WSAGetLastError());
      MessageBox(NULL, szError, TEXT("Error"), MB_OK);
      return(FALSE);
   }

   //
   // Fill in window class structure with 
   // parameters that describe the main window.
   //
   wndclass.style         = CS_HREDRAW | CS_VREDRAW;
   wndclass.lpfnWndProc   = (WNDPROC) MainWndProc;
   wndclass.cbClsExtra    = 0;
   wndclass.cbWndExtra    = DLGWINDOWEXTRA;
   wndclass.hInstance     = hInstance;
   wndclass.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
   wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
   wndclass.hbrBackground = (HBRUSH) (COLOR_WINDOW);
   wndclass.lpszMenuName  = NULL;
   wndclass.lpszClassName = APPCLASS;

   // Register our new window class
   if (RegisterClass(&wndclass) == 0)
   {
      return(FALSE);
   }
  
   // Store application instance handle in global variable
   ghInstApp = hInstance;

   // Create a modeless dialog
   ghAppWnd = CreateDialog(ghInstApp, (LPCSTR) IDD_MAINDIALOG, NULL, NULL);
   if (ghAppWnd == NULL)
   {
      return(FALSE);
   }

   nError = SetupMainWndControls(ghAppWnd);
   if (nError)
   {
      SendMessage(ghAppWnd, WM_CLOSE, 0, 0);
   }
  
   nError = tnp_SetupStart(ghAppWnd);
   if (nError)
   {
      MessageBox(NULL, "tnp_SetupStart", "Error", MB_OK | MB_ICONERROR);
      SendMessage(ghAppWnd, WM_CLOSE, 0, 0);
   } 
   else
   {
      tnp_SetupSearch(ghAppWnd);
   }  

   ShowWindow(ghAppWnd, SW_SHOW);

   // Main message loop
   while (GetMessage(&msg, NULL, 0, 0))
   {
      if (!IsDialogMessage(ghAppWnd, &msg))
      {
         TranslateMessage(&msg); // Translate virtual key codes 
         DispatchMessage(&msg);  // Dispatches message to window
      }
   }

   WSACleanup();

   return(FALSE);
} /* main */

/*** EOF ***/
