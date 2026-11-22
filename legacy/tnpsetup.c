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
*  11.04.2019  mifi  First Version.
*  06.10.2019  mifi  Change name from nutsetup to Tiny Network Explorer.
*  15.04.2021  mifi  Added support for multiple ethernet interfaces.
**************************************************************************/
#define __TNPSETUP_C__

/*=======================================================================*/
/*  Includes                                                             */
/*=======================================================================*/
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <commctrl.h>
#include <process.h>
#include "resource.h"

#include "tnpsetupwin.h"
#include "tnpsetup.h"

/*=======================================================================*/
/*  All Structures and Common Constants                                  */
/*=======================================================================*/

#define MAX_IFACE_CNT  8

#define SIO_GET_INTERFACE_LIST   _IOR ('t', 127, ULONG)

typedef struct in6_addr
{
   union 
   {
      UCHAR  Byte[16];
      USHORT Word[8];
   } u;
} IN6_ADDR, *PIN6_ADDR, *LPIN6_ADDR;

struct sockaddr_in6_old
{
   SHORT    sin6_family;
   USHORT   sin6_port;
   ULONG    sin6_flowinfo;
   IN6_ADDR sin6_addr;
};

typedef union sockaddr_gen 
{
   struct sockaddr         Address;
   struct sockaddr_in      AddressIn;
   struct sockaddr_in6_old AddressIn6;
} sockaddr_gen;

typedef struct _INTERFACE_INFO 
{
   ULONG        iiFlags;
   sockaddr_gen iiAddress;
   sockaddr_gen iiBroadcastAddress;
   sockaddr_gen iiNetmask;
} INTERFACE_INFO, *LPINTERFACE_INFO;

/*=======================================================================*/
/*  Definition of all global Data                                        */
/*=======================================================================*/

/*=======================================================================*/
/*  Definition of all extern Data                                        */
/*=======================================================================*/

/*=======================================================================*/
/*  Definition of all local Data                                         */
/*=======================================================================*/

typedef struct _iface_
{
   SOCKET  Socket;
   DWORD  dAddress;
} IFACE;

static int    nFirstTime = TRUE;

static int    nIfaceCount;
static IFACE   IfaceList[MAX_IFACE_CNT];

/*=======================================================================*/
/*  Definition of all local Procedures                                   */
/*=======================================================================*/

/************************************************************/
/*  GetInterfaceList                                        */
/************************************************************/
static void GetInterfaceList (void)
{
   int                  rc;
   SOCKET               Socket;
   INTERFACE_INFO       InterfaceList[MAX_IFACE_CNT];
   struct sockaddr_in *pAddress;
   int                 nInterfacesCnt;
   DWORD               dBytesReturn;
   DWORD               dValue;
   int                 nIndex;
   int                 nOptionValue;

   /* Default, clear data */
   nIfaceCount = 0;
   memset(&IfaceList, 0x00, sizeof(IfaceList));

   /* Get interface list */
   Socket = socket(AF_INET, SOCK_DGRAM, 0);
   if (Socket != INVALID_SOCKET)
   {
      /* Try to get the Interface List info */
      rc = WSAIoctl(Socket, SIO_GET_INTERFACE_LIST, NULL, 0,
                    InterfaceList, sizeof(InterfaceList),
                    &dBytesReturn, NULL, NULL);
      if (0 == rc)
      {
         /* The socket is not needed anymore */
         closesocket(Socket);

         /* Get interface count */
         nInterfacesCnt = dBytesReturn / sizeof(INTERFACE_INFO);

         nIfaceCount = 0;
         for (nIndex = 0; nIndex < nInterfacesCnt; nIndex++)
         {
            /* Address*/
            pAddress = (struct sockaddr_in *)&(InterfaceList[nIndex].iiAddress);
            dValue = ntohl(pAddress->sin_addr.s_addr);

            /* Check for 127.0.0.1, this is not needed here */
            if ((dValue != 0x7F000001) && (nIfaceCount < MAX_IFACE_CNT))
            {
               /* Create interface socket */
               IfaceList[nIfaceCount].Socket = socket(AF_INET, SOCK_DGRAM, 0);
               if (INVALID_SOCKET == IfaceList[nIfaceCount].Socket)
               {
                  /* Fatal error */
                  exit(0);
               }

               /* Add socket option BROADCAST */
               nOptionValue = TRUE;
               rc = setsockopt(IfaceList[nIfaceCount].Socket, SOL_SOCKET, SO_BROADCAST, (char *)&nOptionValue, sizeof(BOOL));

               /* Save address info */
               IfaceList[nIfaceCount].dAddress = dValue;

               nIfaceCount++;
            }
         }
      }
   }

} /* GetInterfaceList */

/************************************************************/
/*  ClearListView                                           */
/************************************************************/
static void ClearListView (HWND hWnd)
{
   BOOL     fOK;
   int      x;
   int      nIndex;
   int      nMaxItemCount;
   HWND     hListView = GetDlgItem(hWnd, IDC_LIST1);
   LV_ITEM lvItem;
   TNPITEM *pItem;
  
   nMaxItemCount = ListView_GetItemCount(hListView);

   for (nIndex = 0; nIndex < nMaxItemCount; nIndex++)
   {
      lvItem.mask  = LVIF_PARAM;
      lvItem.iItem = nIndex;
      fOK = ListView_GetItem(hListView, &lvItem);
      if (fOK)
      {
         pItem = (TNPITEM *)lvItem.lParam;

         for (x = 0; x < 4; x++)
         {
            if (pItem->aCols[x] != NULL)
            {
               free(pItem->aCols[x]);
               pItem->aCols[x] = NULL;
            } 
         }
      
         free(pItem);
         ListView_DeleteItem(hListView, nIndex);  
      }  
   }
   
} /* ClearListView */

/************************************************************/
/*  NewTNPItem                                              */
/************************************************************/
static TNPITEM* NewTNPItem (TNP_SETUP *pSetup)
{
   int             nIndex;
   TNPITEM        *pItem;
   char           *pStr; 
   char           *pAddress;
   struct in_addr iaClient;   
   char           szTmp[_MAX_PATH];
  
   pItem = (TNPITEM *)malloc(sizeof(TNPITEM));
   if (pItem != NULL)
   {
      memset(pItem, 0x00, sizeof(TNPITEM) );

      /*
       * Is ES
       */
      pItem->bIsES = (pSetup->bMode == TNP_SETUP_RESPONSE_ES) ? 1 : 0;
  
      /*
       * MAC
       */
      sprintf(szTmp, "%02X:%02X:%02X:%02X:%02X:%02X",
               pSetup->bMACAddress[0], pSetup->bMACAddress[1], pSetup->bMACAddress[2],
               pSetup->bMACAddress[3], pSetup->bMACAddress[4], pSetup->bMACAddress[5]);
      pStr = (LPSTR)malloc(lstrlen(szTmp)+1);
      strcpy(pStr, szTmp);
      pItem->aCols[TNP_ITEM_MAC] = pStr;

      /*
       * DHCP
       */
      pItem->wUseDHCP = pSetup->bUseDHCP;

      /*
       * IP-Address
       */
      iaClient.s_addr = pSetup->dAddress;
      pAddress = inet_ntoa(iaClient);
      pStr = (LPSTR)malloc(lstrlen(pAddress)+1);
      strcpy(pStr, pAddress);
      pItem->aCols[TNP_ITEM_IP] = pStr;
    
      /*
       * Name
       */
      sprintf(szTmp, "%s - v%d.%02d", pSetup->Name, pSetup->dFWVersion/100, pSetup->dFWVersion%100);
      pStr = (LPSTR)malloc(lstrlen(szTmp)+1);
      strcpy(pStr, szTmp);
      pItem->aCols[TNP_ITEM_NAME] = pStr;

      /*
       * Location
       */
      pStr = (LPSTR)malloc(lstrlen(pSetup->Location)+1);
      strcpy(pStr, pSetup->Location);
      pItem->aCols[TNP_ITEM_LOC] = pStr;
    
      for (nIndex = 0; nIndex < 6; nIndex++)
      {
         pItem->bMACAddress[nIndex] = pSetup->bMACAddress[nIndex];
      }
    
      pItem->dAddress = pSetup->dAddress;  
      pItem->dMask    = pSetup->dMask;
      pItem->dGateway = pSetup->dGateway;  
    
      memcpy(pItem->szName, pSetup->Name, TNP_NAME_LEN+1);
      memcpy(pItem->szLocation, pSetup->Location, TNP_LOCATION_LEN+1);
      memcpy(pItem->szMDNSName, pSetup->MDNSName, TNP_MDNS_NAME_LEN+1);
   }
  
   return(pItem);
} /* NewTNPItem */

/************************************************************/
/*  AddNutItemAgain                                         */
/************************************************************/
static void AddNutItemAgain (HWND hListView, TNPITEM   *pItem,
                             int  nIndex,    TNP_SETUP *pSetup)
{
   char           *pStr;
   char           *pAddress;
   struct in_addr iaClient;
   char           szTmp[_MAX_PATH];

   /*
    * Update the ListView
    */

   /* Is ES */
   pItem->bIsES = (pSetup->bMode == TNP_SETUP_RESPONSE_ES) ? 1 : 0;
   
   /* IP-Address */
   free(pItem->aCols[TNP_ITEM_IP]);  
   iaClient.s_addr = pSetup->dAddress;
   pAddress = inet_ntoa(iaClient);
   pStr = (LPSTR)malloc(lstrlen(pAddress)+1);
   strcpy(pStr, pAddress);
   pItem->aCols[TNP_ITEM_IP] = pStr;
   pItem->dAddress = pSetup->dAddress;

   /* Name + Version */   
   sprintf(szTmp, "%s - v%d.%02d", pSetup->Name, pSetup->dFWVersion / 100, pSetup->dFWVersion % 100);

   free(pItem->aCols[TNP_ITEM_NAME]);
   pStr = (LPSTR)malloc(lstrlen(szTmp)+1);
   strcpy(pStr, szTmp);
   pItem->aCols[TNP_ITEM_NAME] = pStr;

   /* Location */
   free(pItem->aCols[TNP_ITEM_LOC]);  
   pStr = (LPSTR)malloc(lstrlen(pSetup->Location)+1);
   strcpy(pStr, pSetup->Location);
   pItem->aCols[TNP_ITEM_LOC] = pStr;
   memcpy(pItem->szLocation, pSetup->Location, TNP_LOCATION_LEN+1);

   ListView_Update (hListView, nIndex);
   
} /* AddNutItemAgain */

/************************************************************/
/*  AddNutItem                                              */
/************************************************************/
static void AddNutItem (HWND hListView, TNP_SETUP *pSetup)
{
   TNPITEM  *pItem;
   LV_ITEM  lvItem;
  
   pItem = NewTNPItem(pSetup);
   if (pItem != NULL)
   {
      lvItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
      lvItem.state      = 0;
      lvItem.stateMask  = 0;
      lvItem.iItem      = 0;
      lvItem.iSubItem   = 0;
      lvItem.pszText    = LPSTR_TEXTCALLBACK;
      lvItem.lParam     = (LPARAM)pItem;
      ListView_InsertItem(hListView, &lvItem);
   }
   
} /* AddNutItem */ 

/************************************************************/
/*  HandleResponse                                          */
/************************************************************/
static void HandleResponse (HWND hWnd, TNP_SETUP *pSetup)
{
   BOOL     fOK;
   int      nIndex;
   int      nFound;
   int      nMaxItemCount;
   HWND     hListView = GetDlgItem(hWnd, IDC_LIST1);
   LV_ITEM lvItem;
   TNPITEM *pItem;
  
   nMaxItemCount = ListView_GetItemCount(hListView);
  
   /*
    * Test if the Setup is always in the list
    */
   nFound = FALSE;
   for (nIndex = 0; nIndex < nMaxItemCount; nIndex++)
   {
      lvItem.mask  = LVIF_PARAM;
      lvItem.iItem = nIndex;
      fOK = ListView_GetItem(hListView, &lvItem);
      if (fOK)
      {
         /*
          * Check if the MAC address is equal
          */
         pItem = (TNPITEM *)lvItem.lParam;  
         if( (pItem->bMACAddress[0] == pSetup->bMACAddress[0]) &&
             (pItem->bMACAddress[1] == pSetup->bMACAddress[1]) &&
             (pItem->bMACAddress[2] == pSetup->bMACAddress[2]) &&
             (pItem->bMACAddress[3] == pSetup->bMACAddress[3]) &&
             (pItem->bMACAddress[4] == pSetup->bMACAddress[4]) &&
             (pItem->bMACAddress[5] == pSetup->bMACAddress[5]) )
         {
            nFound = TRUE;
            break;    
         }    
      }    
   }

   if (nFound == FALSE)
   {
      /* Put the new Setup in the list */
      AddNutItem(hListView, pSetup);
   } 
   else
   {
      /* Add the item again */
      AddNutItemAgain(hListView, pItem, lvItem.iItem, pSetup); 
   } 
    
} /* HandleResponse */

/*=======================================================================*/
/*  All code exported                                                    */
/*=======================================================================*/

/************************************************************/
/*  tnp_SetupStart                                          */
/************************************************************/
int tnp_SetupStart (HWND hWnd)
{
   int           nError = TRUE;
   int           nRet;
   int           nIndex;
   SOCKADDR_IN  saSource;
  
   if (nFirstTime == TRUE)
   {
      nFirstTime = FALSE;

      GetInterfaceList();

      for (nIndex = 0; nIndex < nIfaceCount; nIndex++)
      {
         /*
          * Set address and port
          */
         saSource.sin_addr.s_addr = htonl(IfaceList[nIndex].dAddress);
         saSource.sin_port        = htons(TNP_UDP_PORT);
         saSource.sin_family      = AF_INET;

         nRet = bind(IfaceList[nIndex].Socket, (const struct sockaddr*)&saSource, sizeof(SOCKADDR_IN));
    
         nRet = WSAAsyncSelect(IfaceList[nIndex].Socket, hWnd, SM_EVENT_1 + nIndex, FD_READ);
         if (nRet != SOCKET_ERROR)
         {
            nError = FALSE;
         } 
      }                                  
   }

   return(nError);
} /* tnp_SetupStart */

/************************************************************/
/*  tnp_SetupStop                                           */
/************************************************************/
void tnp_SetupStop (HWND hWnd)
{
   int nIndex;

   if (nFirstTime == FALSE)
   {
      for (nIndex = 0; nIndex < nIfaceCount; nIndex++)
      {
         closesocket(IfaceList[nIndex].Socket);
      }
   }

} /* tnp_SetupStop */

/************************************************************/
/*  tnp_SetupRead                                           */
/*                                                          */
/*  We have receive some data, read it out.                 */
/*                                                          */
/************************************************************/
void tnp_SetupRead (HWND hWnd, int nInterface)
{
   int          nRet;
   int          nAddressLen;
   SOCKADDR_IN saSource;
   TNP_SETUP     Setup;

   if ((nInterface >= 0) && (nInterface < MAX_IFACE_CNT))
   {
      nAddressLen = sizeof(SOCKADDR_IN);
      nRet = recvfrom(IfaceList[nInterface].Socket, 
                      (char *)&Setup, 
                      sizeof(TNP_SETUP), 
                      0,
                      (struct sockaddr*)&saSource,
                      &nAddressLen);

      if( (nRet           != SOCKET_ERROR)       &&
          (Setup.dMagic1  == TNP_HEADER_MAGIC_1) &&
          (Setup.dMagic2  == TNP_HEADER_MAGIC_2) &&
          (Setup.wSize    == sizeof(TNP_SETUP))  &&
          (Setup.wVersion == TNP_HEADER_VERSION) )
      {    
         /* Check if MAC addres is != 0 */
         if ((0 == Setup.bMACAddress[0]) && 
             (0 == Setup.bMACAddress[1]) && 
             (0 == Setup.bMACAddress[2]) && 
             (0 == Setup.bMACAddress[3]) && 
             (0 == Setup.bMACAddress[4]) && 
             (0 == Setup.bMACAddress[5]) )
         {
            /* Do nothing, wrong MAC address */
         }
         else
         {
            if( (Setup.bMode == TNP_SETUP_RESPONSE)    ||
                (Setup.bMode == TNP_SETUP_RESPONSE_ES) )
            {             
               HandleResponse(hWnd, &Setup);      
            }            
         }   
      }  
   }
  
} /* tnp_SetupRead */

/************************************************************/
/*  tnp_SetupSearch                                         */
/*                                                          */
/*  Send a SEARCH_REQUEST to the network                    */
/*                                                          */
/************************************************************/
void tnp_SetupSearch (HWND hWnd)
{
   int          nIndex; 
   SOCKADDR_IN saDest;
   TNP_SETUP     Setup; 

   /*
    * First we must delete all the LitViewItems,
    * we must do it twice, to delete the last one too.
    */
   ClearListView(hWnd);
   ClearListView(hWnd);
  
   /*
    * Fill the packet
    */ 
   memset(&Setup, 0x00, sizeof(TNP_SETUP));
   Setup.dMagic1  = TNP_HEADER_MAGIC_1;
   Setup.dMagic2  = TNP_HEADER_MAGIC_2;
   Setup.wSize    = sizeof(TNP_SETUP);
   Setup.wVersion = TNP_HEADER_VERSION;
   Setup.bMode    = TNP_SETUP_REQUEST;

   /*
    * Send to all interfaces
    */
   for (nIndex = 0; nIndex < nIfaceCount; nIndex++)
   {
      /* Set address and port */
      saDest.sin_addr.s_addr = INADDR_BROADCAST;
      saDest.sin_port        = htons(TNP_UDP_PORT);
      saDest.sin_family      = AF_INET;
   
      /* Send the packet */
      sendto(IfaceList[nIndex].Socket, (const char *)&Setup, sizeof(TNP_SETUP), 0, 
             (const struct sockaddr*)&saDest, sizeof(SOCKADDR_IN));
   }

} /* tnp_SetupSearch */

/************************************************************/
/*  tnp_SetupUpdate                                         */
/*                                                          */
/*  Update a ListViewItem, set the new state to Reboot.     */
/*  After this we must update the Ethernut.                 */
/*  Ethernut will make a reboot after the update.           */
/*                                                          */
/************************************************************/
void tnp_SetupUpdate (HWND hWnd, int nItem, TNPITEM *pOld, TNPITEM *pNew)
{
   HWND            hListView = GetDlgItem(hWnd, IDC_LIST1);
   char           *pStr;
   char           *pAddress;
   struct in_addr iaClient;
   char           szTmp[_MAX_PATH];
   int             nIndex;
   SOCKADDR_IN    saDest;
   TNP_SETUP        Setup;  
  
   /*
    * Update the ListView
    */
   free(pOld->aCols[TNP_ITEM_IP]);
  
   iaClient.s_addr = pNew->dAddress;
   pAddress = inet_ntoa(iaClient);
   pStr = (LPSTR)malloc(lstrlen(pAddress)+1);
   strcpy(pStr, pAddress);
   pOld->aCols[TNP_ITEM_IP] = pStr;

   pOld->dAddress = pNew->dAddress;
   pOld->dMask    = pNew->dMask;
   pOld->dGateway = pNew->dGateway;

   pOld->wUseDHCP = pNew->wUseDHCP;

   free(pOld->aCols[TNP_ITEM_NAME]);
   sprintf(szTmp,"Reboot, please wait...");
   pStr = (LPSTR)malloc(lstrlen(szTmp)+1);
   strcpy(pStr, szTmp);
   pOld->aCols[TNP_ITEM_NAME] = pStr;

   ListView_Update (hListView, nItem);
  
   /*
    * Send the Update to the client
    */
  
   /*
    * Fill the packet
    */ 
   memset(&Setup, 0x00, sizeof(TNP_SETUP));
   Setup.dMagic1  = TNP_HEADER_MAGIC_1;
   Setup.dMagic2  = TNP_HEADER_MAGIC_2;
   Setup.wSize    = sizeof(TNP_SETUP);
   Setup.wVersion = TNP_HEADER_VERSION;
   Setup.bMode    = TNP_SETUP_SET;
   
   for (nIndex = 0; nIndex < 6; nIndex++)
   {
      Setup.bMACAddress[nIndex] = pNew->bMACAddress[nIndex];
   }

   Setup.bUseDHCP = (BYTE)(pNew->wUseDHCP & 0x00FF);

   Setup.dAddress = pNew->dAddress;
   Setup.dMask    = pNew->dMask;
   Setup.dGateway = pNew->dGateway;      

   memcpy(Setup.Location, pNew->szLocation, TNP_MAX_LOCATION_LEN);
    
   /*
    * Send to all interfaces
    */
   for (nIndex = 0; nIndex < nIfaceCount; nIndex++)
   {
      /* Set address and port */
      saDest.sin_addr.s_addr = INADDR_BROADCAST;
      saDest.sin_port        = htons(TNP_UDP_PORT);
      saDest.sin_family      = AF_INET;

      /* Send the packet */
      sendto(IfaceList[nIndex].Socket, (const char *)&Setup, sizeof(TNP_SETUP), 0, 
             (const struct sockaddr*)&saDest, sizeof(SOCKADDR_IN));
   }
 
} /* tnp_SetupUpdate */

/*** EOF ***/
 