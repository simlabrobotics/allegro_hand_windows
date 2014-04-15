//////////////////////////////////////////////////////////////////////////
// IXXAT Automation GmbH
//////////////////////////////////////////////////////////////////////////
/**
  Implementation of the socket select dialog class.

  @file "select.cpp"
*/
//////////////////////////////////////////////////////////////////////////
// (C) 2002-2011 IXXAT Automation GmbH, all rights reserved
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// compiler directives
//////////////////////////////////////////////////////////////////////////
#ifdef MICROSOFT_C
#pragma warning(disable:4100) // unreferenced formal parameter
#endif

//////////////////////////////////////////////////////////////////////////
// include files
//////////////////////////////////////////////////////////////////////////
#include <windows.h>

#include "select.hpp"

//////////////////////////////////////////////////////////////////////////
// static constants, types, macros, variables
//////////////////////////////////////////////////////////////////////////
#define WM_UPDATE  (WM_USER+1)    // update message

//------------------------------------------------------------------------
// Macro:
//  NUMELEM
//
// Description:
//  This macro retreives the number of elements within the specified array.
//
// Arguments:
//  a -> array for which to retrieve the number of elements
//
// Results:
//  Returns the number of elements within the specified array.
//------------------------------------------------------------------------
#ifndef NUMELEM
#define NUMELEM(a) (sizeof(a) / sizeof(a[0]))
#endif

//////////////////////////////////////////////////////////////////////////
// global variables
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// static function prototypes
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// global functions
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// static functions
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// external variables
//////////////////////////////////////////////////////////////////////////

extern HINSTANCE g_hModule;  // Handle to module instance

//////////////////////////////////////////////////////////////////////////
// message handlers for CSocketSelectDlg
//////////////////////////////////////////////////////////////////////////
BEGIN_DIALOG_PROC(CSocketSelectDlg)
 ON_MESSAGE( WM_INITDIALOG, OnWmInitdialog )
 ON_MESSAGE( WM_DESTROY,    OnWmDestroy    )
 ON_MESSAGE( WM_UPDATE,     OnWmUpdate     )
END_DIALOG_PROC(CDialogWnd)

//////////////////////////////////////////////////////////////////////////
// command handler for CCanSelectDlg
//////////////////////////////////////////////////////////////////////////
BEGIN_COMMAND_PROC(CSocketSelectDlg)
 ON_COMMAND( IDOK,        OnBtnOk      )
 ON_COMMAND( IDCANCEL,    OnBtnCancel  )
 ON_COMMAND( IDC_DEVLIST, OnLbxDevList )
 ON_COMMAND( IDC_SOCLIST, OnCbxSocList )
END_COMMAND_PROC(CDialogWnd)

//////////////////////////////////////////////////////////////////////////
/**
  Constructor for the socket select dialog window class.
*/
//////////////////////////////////////////////////////////////////////////
CSocketSelectDlg::CSocketSelectDlg()
{
  m_hThread = NULL;
  m_dwThdId = 0;
  m_hDevEnu = NULL;

  m_hCurDev = INVALID_HANDLE_VALUE;

  m_hSelDev = NULL;
  m_lCtrlNo = -1;
}

//////////////////////////////////////////////////////////////////////////
/**
  Destructor for the socket select dialog window class.
*/
//////////////////////////////////////////////////////////////////////////
CSocketSelectDlg::~CSocketSelectDlg()
{
  ThreadStop(TRUE);

  m_hCurDev = INVALID_HANDLE_VALUE;
  m_hSelDev = NULL;
  m_lCtrlNo = -1;
}

//////////////////////////////////////////////////////////////////////////
/**
  The function serves as the starting address for the monitor thread.
  The functions address is specified when creating the monitor thread.

  @param pDialog
    Pointer to the dialog object.

  @return
    alway S_OK ( 0 )
*/
//////////////////////////////////////////////////////////////////////////
DWORD CSocketSelectDlg::ThreadProc(CSocketSelectDlg* pDialog)
{
  while (pDialog->m_hDevEnu != NULL)
  {
    // We cannot send a message if the thread is terminating,
    // because the ThreadStop call within OnDestroy is waiting
    // for the thread to terminate and does not respons to the
    // message queue.
    pDialog->SendMessage(WM_UPDATE, 0, 0);
    vciEnumDeviceWaitEvent(pDialog->m_hDevEnu, INFINITE);
  }

  return( 0 );
}

//////////////////////////////////////////////////////////////////////////
/**
  This function starts execution of the device list monitor thread.

  @return
    The function returns NO_ERROR on success, otherwise an error value.
*/
//////////////////////////////////////////////////////////////////////////
HRESULT CSocketSelectDlg::ThreadStart( )
{
  HRESULT hResult;

  if (m_hThread == NULL)
  {
    m_hThread = CreateThread(NULL, 0, (PTHREAD_START_ROUTINE) ThreadProc,
                             (LPVOID) this, CREATE_SUSPENDED, &m_dwThdId);

    if (m_hThread != NULL)
    {
      hResult = vciEnumDeviceOpen((PHANDLE) &m_hDevEnu);

      if (hResult == VCI_OK)
        ResumeThread(m_hThread);
      else
        ThreadStop(TRUE);
    }
    else
    {
      hResult = GetLastError();
    }
  }
  else
  {
    hResult = NO_ERROR;
  }

  return(hResult);
}

//////////////////////////////////////////////////////////////////////////
/**
  This function stops execution of the device list monitor thread.

  @param fWaitFor
    If this parameter is set to TRUE the functions waits until the execution of 
    the thread ends.

  @return
    The function returns NO_ERROR on success, otherwise an error value.
*/
//////////////////////////////////////////////////////////////////////////
HRESULT CSocketSelectDlg::ThreadStop(BOOL fWaitFor)
{
  HRESULT hResult = NO_ERROR;

  if (m_hThread != NULL)
  {
    if (GetCurrentThreadId() != m_dwThdId)
    {
      //
      // signal thread termination
      //
      HANDLE hEnum = (HANDLE) InterlockedExchange((PLONG)&m_hDevEnu, 0);
      vciEnumDeviceClose(hEnum);

      //
      // wait for termination
      //
      if (fWaitFor == TRUE)
      {
        while(ResumeThread(m_hThread) > 1);

        switch( WaitForSingleObject(m_hThread, INFINITE) )
        {
          case WAIT_OBJECT_0 : hResult = NO_ERROR;          break;
          case WAIT_FAILED   : hResult = GetLastError();    break;
          default            : hResult = ERROR_GEN_FAILURE; break;
        }

        CloseHandle(m_hThread);
        m_hThread = NULL;
        m_dwThdId = 0;
      }
    }
    else
    {
      ExitThread(NO_ERROR); // this function never returns
    }
  }

  return( hResult );
}

//////////////////////////////////////////////////////////////////////////
/**
  This function updates the view of the dialog.

  @param hDevice
    Handle to the VCI Device of the currently selected device. This parameter is 
    set to NULL if no device is selected.
*/
//////////////////////////////////////////////////////////////////////////
void CSocketSelectDlg::UpdateView(HANDLE hDevice)
{
  VCIDEVICEINFO sInfo;
  TCHAR         szText[256];
  CHAR          szTmp[256];
  size_t        converted;

  //
  // update device info view
  //
  if (vciDeviceGetInfo(hDevice, &sInfo) == VCI_OK)
  {
#ifdef _UNICODE
    mbstowcs_s(&converted, szText, 256, sInfo.Description, 256);
    SetItemText(IDC_DESCRIPTION,  szText);
	mbstowcs_s(&converted, szText, 256, sInfo.Manufacturer, 256);
    SetItemText(IDC_MANUFACTURER, szText);
#else
    SetItemText(IDC_DESCRIPTION,  sInfo.Description);
    SetItemText(IDC_MANUFACTURER,  sInfo.Manufacturer);
#endif
    wsprintf(szText, TEXT("%d.%02d"),
             sInfo.DriverMajorVersion,
             sInfo.DriverMinorVersion);
    SetItemText(IDC_DRIVERVERSION,   szText);

    wsprintf(szText, TEXT("%d.%02d"),
             sInfo.HardwareMajorVersion,
             sInfo.HardwareMinorVersion);
    SetItemText(IDC_HARDWAREVERSION, szText);

#ifdef _UNICODE
    vciGuidToChar(sInfo.DeviceClass, szTmp, NUMELEM(szTmp));
    mbstowcs_s(&converted, szText, 256, szTmp, 256);
    SetItemText(IDC_DEVICECLASS, szText);
#else
    vciGuidToChar(sInfo.DeviceClass, szText, NUMELEM(szText));
    SetItemText(IDC_DEVICECLASS, szText);
#endif

#ifdef _UNICODE
    vciGuidToChar(sInfo.UniqueHardwareId.AsGuid, szTmp, NUMELEM(szTmp));
    mbstowcs_s(&converted, szText, 256, szTmp, 256);
    SetItemText(IDC_HARDWAREID, szText);
#else
    vciGuidToChar(sInfo.UniqueHardwareId.AsGuid, szText, NUMELEM(szText));
    SetItemText(IDC_HARDWAREID, szText);
#endif

#ifdef _UNICODE
    vciLuidToChar(sInfo.VciObjectId, szTmp, NUMELEM(szTmp));
    mbstowcs_s(&converted, szText, 256, szTmp, 256);
    SetItemText(IDC_VCIOBJECTID, szText);
#else
    vciLuidToChar(sInfo.VciObjectId, szText, NUMELEM(szText));
    SetItemText(IDC_VCIOBJECTID, szText);
#endif
  }
  else
  {
    SetItemText(IDC_DESCRIPTION,     TEXT(""));
    SetItemText(IDC_MANUFACTURER,    TEXT(""));
    SetItemText(IDC_DRIVERVERSION,   TEXT(""));
    SetItemText(IDC_HARDWAREVERSION, TEXT(""));
    SetItemText(IDC_DEVICECLASS,     TEXT(""));
    SetItemText(IDC_HARDWAREID,      TEXT(""));
    SetItemText(IDC_VCIOBJECTID,     TEXT(""));
  }
}

//////////////////////////////////////////////////////////////////////////
/**
  This function adds the specified device to the device list.

  @param vciid
    VCIID of the device to add

  @return
    The return value is the zero-based index of the added device. If an error 
    occurs, the return value is LB_ERR. If there is insufficient space to store 
    the new device, the return value is LB_ERRSPACE. 
*/
//////////////////////////////////////////////////////////////////////////
LRESULT CSocketSelectDlg::DeviceListInsert(VCIID vciid)
{
  HANDLE        hDevice;
  LRESULT       lIndex;
  TCHAR         szText[512];
  VCIDEVICEINFO sInfo;

  lIndex = DeviceListSearch(vciid);

  if (lIndex == LB_ERR)
  {
    if (vciDeviceOpen(vciid, &hDevice) == VCI_OK)
    {
      if (vciDeviceGetInfo(hDevice, &sInfo) == VCI_OK)
      {
        wsprintf(szText, TEXT("%08X%08X - %s"),
          vciid.AsLuid.HighPart, vciid.AsLuid.LowPart,  sInfo.Description);

        lIndex = SendItemMessage(IDC_DEVLIST,
                   LB_ADDSTRING, 0, (LPARAM) szText);

        if (lIndex >= 0)
        {
          SendItemMessage(IDC_DEVLIST,
            LB_SETITEMDATA, lIndex, (LPARAM) hDevice);
        }
        else
        {
          vciDeviceClose(hDevice);
        }
      }
      else
      {
        vciDeviceClose(hDevice);
      }
    }
  }

  return( lIndex );
}

//////////////////////////////////////////////////////////////////////////
/**
  This function searches for the specified device within the device list.

  @param vciid
    VCIID of the device to search for.

  @return
    The return value is the zero-based index of the matching item, or LB_ERR if 
    the search was unsuccessful. 
*/
//////////////////////////////////////////////////////////////////////////
LRESULT CSocketSelectDlg::DeviceListSearch(VCIID vciid)
{
  LRESULT       lResult;
  LRESULT       lCount;
  LRESULT       lIndex;
  LRESULT       lData;
  VCIDEVICEINFO sInfo;

  lResult = LB_ERR;

  lCount = SendItemMessage(IDC_DEVLIST, LB_GETCOUNT, 0, 0);

  if (lCount != LB_ERR)
  {
    for (lIndex = 0; lIndex < lCount; lIndex++)
    {
      lData = SendItemMessage(IDC_DEVLIST, LB_GETITEMDATA, lIndex, 0);

      if ((lData != 0) && (lData != LB_ERR))
      {
        if ((vciDeviceGetInfo((HANDLE) lData, &sInfo) == VCI_OK) &&
            (sInfo.VciObjectId.AsInt64 == vciid.AsInt64))
        {
          lResult = lIndex;
          break;
        }
      }
    }
  }

  return( lResult );
}

//////////////////////////////////////////////////////////////////////////
/**
  This function clears the device list.
*/
//////////////////////////////////////////////////////////////////////////
void CSocketSelectDlg::DeviceListClear( )
{
  LRESULT lCount;
  LRESULT lData;

  lCount = SendItemMessage(IDC_DEVLIST, LB_GETCOUNT, 0, 0);

  if (lCount != LB_ERR)
  {
    for (LRESULT lIndex = 0; lIndex < lCount; lIndex++)
    {
      lData = SendItemMessage(IDC_DEVLIST, LB_GETITEMDATA, lIndex, 0);

      if ((lData != 0) && (lData != LB_ERR))
      {
        vciDeviceClose((HANDLE) lData);
        SendItemMessage(IDC_DEVLIST, LB_SETITEMDATA, lIndex, 0);
      }
    }
  }

  SendItemMessage(IDC_DEVLIST, LB_RESETCONTENT, 0, 0);
  m_hCurDev = INVALID_HANDLE_VALUE;
}

//////////////////////////////////////////////////////////////////////////
/**
  This function updates the device list.
*/
//////////////////////////////////////////////////////////////////////////
void CSocketSelectDlg::DeviceListUpdate( )
{
  LRESULT       lCount;
  LRESULT       lData;
  VCIDEVICEINFO sInfo;

  //
  // remove all unavailable devices from the device list
  //
  lCount = SendItemMessage(IDC_DEVLIST, LB_GETCOUNT, 0, 0);

  for (LRESULT lIndex = 0; lIndex < lCount; lIndex++)
  {
    lData = SendItemMessage(IDC_DEVLIST, LB_GETITEMDATA, lIndex, 0);
    if ((lData != LB_ERR) &&
        (vciDeviceGetInfo((HANDLE) lData, &sInfo) != VCI_OK))
    {
      vciDeviceClose((HANDLE) lData);
      SendItemMessage(IDC_DEVLIST, LB_DELETESTRING, lIndex, 0);
    }
  }

  //
  // The selection may change because the currently selected
  // item is deleted.
  //
  DeviceListSelect();

  //
  // enumerate all devices
  //
  if (m_hDevEnu != NULL)
  {
    vciEnumDeviceReset(m_hDevEnu);
    while (vciEnumDeviceNext(m_hDevEnu, &sInfo) == VCI_OK)
      DeviceListInsert(sInfo.VciObjectId);
  }
}
//////////////////////////////////////////////////////////////////////////
/**
  This function is called whenever a device is selected within the device list 
  combo box.
*/
//////////////////////////////////////////////////////////////////////////
void CSocketSelectDlg::DeviceListSelect( )
{
  LRESULT lIndex;
  LRESULT lData;
  HANDLE  hDevice = NULL;

  lIndex = SendItemMessage(IDC_DEVLIST, LB_GETCURSEL, 0, 0);

  if (lIndex >= 0)
  {
    lData = SendItemMessage(IDC_DEVLIST, LB_GETITEMDATA, lIndex, 0);
    if ((lData != 0) && (lData != LB_ERR))
      hDevice = (HANDLE) lData;
  }

  if (hDevice != m_hCurDev)
  {
    m_hCurDev = hDevice;
    //
    // update dialog view
    //
    UpdateView(hDevice);
    SocketListUpdate();
  }
}

//////////////////////////////////////////////////////////////////////////
/**
  This function updates the bus socket list.
*/
//////////////////////////////////////////////////////////////////////////
void CSocketSelectDlg::SocketListUpdate( )
{
  VCIDEVICECAPS sDevCaps;
  TCHAR         szText[32];

  SendItemMessage(IDC_SOCLIST, CB_RESETCONTENT, 0, 0);
  EnableControl(IDC_SOCLIST, FALSE);

  if (vciDeviceGetCaps(m_hCurDev, &sDevCaps) == VCI_OK)
  {
    char cNumCtls = 0;

    for (UINT8 s = 0; s < sDevCaps.BusCtrlCount; s++)
    {
      if (VCI_BUS_TYPE(sDevCaps.BusCtrlTypes[s]) == m_BusType)
      {
        switch (m_BusType)
        {
          case VCI_BUS_CAN:
            wsprintf(szText, TEXT("CAN - %c"), cNumCtls + 'A');
            break;
          case VCI_BUS_LIN:
            wsprintf(szText, TEXT("LIN - %c"), cNumCtls + 'A');
            break;
        }

        cNumCtls++;
        SendItemMessage(IDC_SOCLIST, CB_ADDSTRING, 0, (LPARAM) szText);
      }
    }

    EnableControl(IDC_SOCLIST, cNumCtls > 0);
  }
}

//////////////////////////////////////////////////////////////////////////
/**
  This function is called whenever a CAN socket is selected within the CAN 
  socket list combo box.
*/
//////////////////////////////////////////////////////////////////////////
void CSocketSelectDlg::SocketListSelect( )
{
  LRESULT lIndex;

  //
  // retrieve currently selected socket
  //
  lIndex = SendItemMessage(IDC_SOCLIST, CB_GETCURSEL, 0, 0);

  if (lIndex >= 0)
    m_lCtrlNo = (LONG) lIndex;
  else
    m_lCtrlNo = -1;
}

//////////////////////////////////////////////////////////////////////////
/**
  This function is called when the window receives the WM_INITDIALOG message.

  @param wParam
    Handle to the control to receive the default keyboard focus. The system 
    assigns the default keyboard focus only if the dialog box procedure returns 
    TRUE. 
  @param lParam
    Specifies additional initialization data. This data is passed to the system 
    as the lParam parameter in a call to the Create function used to create the 
    dialog box.

  @return
    The dialog box procedure should return TRUE to direct the system to set the 
    keyboard focus to the control given by hwndFocus. Otherwise, it should 
    return FALSE to prevent the system from setting the default keyboard focus. 
    The dialog box procedure should return the value directly. The DWL_MSGRESULT
    value set by the SetWindowLong function is ignored. 
*/
//////////////////////////////////////////////////////////////////////////
BOOL CSocketSelectDlg::OnWmInitdialog(WPARAM wParam, LPARAM lParam)
{
  HRESULT hResult;

  m_hCurDev = INVALID_HANDLE_VALUE;
  m_lCtrlNo = -1;

  UpdateView(NULL);
  DeviceListClear();

  //
  // initialize the device list and a thread that waits on
  // the change event from the device enumerator
  //
  hResult = ThreadStart();

  vciDisplayError(CDialogWnd::GetHandle(), NULL, hResult);

  return(TRUE);
}

//////////////////////////////////////////////////////////////////////////
/**
  This function is called when the window receives the WM_DESTROY message.

  @param wParam
    not used
  @param lParam
    not used

  @return
    This function should always return FALSE.
*/
//////////////////////////////////////////////////////////////////////////
BOOL CSocketSelectDlg::OnWmDestroy(WPARAM wParam, LPARAM lParam)
{
  //
  // terminate update thread
  //
  ThreadStop(TRUE);

  //
  // clear device list
  //
  DeviceListClear();
  UpdateView(NULL);

  return(FALSE);
}

//////////////////////////////////////////////////////////////////////////
/**
  This function is called when the window receives the WM_UPDATE message.

  @param wParam
    not used
  @param lParam
    not used

  @return
    This function should always return TRUE.
*/
//////////////////////////////////////////////////////////////////////////
BOOL CSocketSelectDlg::OnWmUpdate(WPARAM wParam, LPARAM lParam)
{
  DeviceListUpdate();
  return(TRUE);
}

//////////////////////////////////////////////////////////////////////////
/**
  This function is called for notifications from the device list.

  @param wCode
    Action code
  @param hCtrl
    Handle of control

  @return
    This function should always return TRUE.
*/
//////////////////////////////////////////////////////////////////////////
BOOL CSocketSelectDlg::OnLbxDevList(WORD wCode, HWND hCtrl)
{
  switch (wCode)
  {
    case LBN_SELCHANGE :
      DeviceListSelect();
      break;
  }

  return(TRUE);
}

//////////////////////////////////////////////////////////////////////////
/**
  This function is called for notifications from the bus socket list.

  @param wCode
    Action code
  @param hCtrl
    Handle of control

  @return
    This function should always return TRUE.
*/
//////////////////////////////////////////////////////////////////////////
BOOL CSocketSelectDlg::OnCbxSocList(WORD wCode, HWND hCtrl)
{
  switch (wCode)
  {
    case CBN_SELCHANGE :
      SocketListSelect();
      break;
  }

  return(TRUE);
}

//////////////////////////////////////////////////////////////////////////
/**
  This function is called whenever the "OK" button is pressed.

  @param wCode
    Action code
  @param hCtrl
    Handle of control

  @return
    This function should always return TRUE.
*/
//////////////////////////////////////////////////////////////////////////
BOOL CSocketSelectDlg::OnBtnOk(WORD wCode, HWND hCtrl)
{
  HRESULT       hResult;
  VCIDEVICEINFO sInfo;

  if (m_hCurDev != NULL)
  {
    if (m_lCtrlNo >= 0)
    {
      //
      // re-open the selected device. This is neccessary because the
      // handle in m_hCurDev is invalid after the dialog has closed.
      //
      hResult = vciDeviceGetInfo(m_hCurDev, &sInfo);
      if (hResult == VCI_OK)
        hResult = vciDeviceOpen(sInfo.VciObjectId, &m_hSelDev);

      if (hResult == VCI_OK)
      {
        m_nSelDev = sInfo.VciObjectId;
        EndDialog(IDOK);
      }
      else
      {
        DeviceListUpdate();
      }
    }
    else
    {
      TCHAR szTitle[256];
      GetWindowText(szTitle, 256);
      MessageBox(CDialogWnd::GetHandle(),
                 TEXT("Please select a BUS line !"),
                 szTitle, MB_OK | MB_ICONSTOP);
    }
  }
  else
  {
    TCHAR szTitle[256];
    GetWindowText(szTitle, 256);
    MessageBox(CDialogWnd::GetHandle(),
               TEXT("Please select a device !"),
               szTitle, MB_OK | MB_ICONSTOP);
  }

  return(TRUE);
}

//////////////////////////////////////////////////////////////////////////
/**
  This function is called whenever the "Cancel" button is pressed.

  @param wCode
    Action code
  @param hCtrl
    Handle of control

  @return
    This function should always return TRUE.
*/
//////////////////////////////////////////////////////////////////////////
BOOL CSocketSelectDlg::OnBtnCancel(WORD wCode, HWND hCtrl)
{
  EndDialog(IDCANCEL);
  return(TRUE);
}

//////////////////////////////////////////////////////////////////////////
/**
  This function runs a modal dialog box to select a bus controller.

  @param hWndParent
    Handle to the window that owns the dialog box.
  @param bBusType
    Type of the bus socket to select. This can be one of the VCI_BUS_ constants.
  @param phDevice
    Pointer to a variable where the function stores the handle of the selected 
    device.
  @param plCtrlNo
    Pointer to a variable where the function stores the number of the selected 
    bus controller.

  @return
    If the function succeeds it returns VCI_OK, otherwise an error code other 
    than VCI_OK. The function returns VCI_E_ABORT if the user cancels the 
    dialog box.

  @note
    The caller is responsible to close the handle returned in <phDevice>.
*/
//////////////////////////////////////////////////////////////////////////
HRESULT CSocketSelectDlg::RunModal(HWND    hWndParent, 
                                   UINT8   bBusType,
                                   PHANDLE phDevice,   
                                   PLONG   plCtrlNo)

{
  INT_PTR iResult;
  HRESULT hResult;

  if ((phDevice != NULL) && (plCtrlNo != NULL) &&
      ((bBusType == VCI_BUS_CAN) || (bBusType == VCI_BUS_LIN)))
  {
    *phDevice = NULL;
    *plCtrlNo = -1;

    m_BusType = bBusType;
    m_hSelDev = NULL;

    iResult = DialogBox(GetModuleHandle(NULL),
                        TEXT("SOCSELECTDLG"), hWndParent);

    if (iResult == IDOK)
    {
      if ((m_hCurDev != NULL) && (m_lCtrlNo >= 0))
      {
        *phDevice = m_hSelDev;
        *plCtrlNo = m_lCtrlNo;
        hResult = VCI_OK;
      }
      else
      {
        hResult = VCI_E_FAIL; // this should never happen
      }
    }
    else
    {
      if (iResult == IDCANCEL)
        hResult = VCI_E_ABORT;
      else
        hResult = GetLastError();
    }
  }
  else
  {
    hResult = VCI_E_INVALIDARG;
  }

  return( hResult );
}

//////////////////////////////////////////////////////////////////////////
/**
  This function runs a modal dialog box to select a bus controller.

  @param hWndParent
    Handle to the window that owns the dialog box.
  @param bBusType
    Type of the bus socket to select. This can be one of the VCI_BUS_ constants.
  @param phDevice
    Pointer to a variable where the function stores the handle of the selected 
    device.
  @param plCtrlNo
    Pointer to a variable where the function stores the number of the selected 
    bus controller.

  @return
    If the function succeeds it returns VCI_OK, otherwise an error code other 
    than VCI_OK. The function returns VCI_E_ABORT if the user cancels the 
    dialog box.

  @note
    The caller is responsible to close the handle returned in <phDevice>.
*/
//////////////////////////////////////////////////////////////////////////
EXTERN_C HRESULT SocketSelectDlg(HWND    hWndParent, 
                                 UINT8   bBusType,
                                 PHANDLE phDevice,   
                                 PLONG   plCtrlNo)
{
  static BOOL fIsInit = FALSE;
  HRESULT     hResult = NO_ERROR;

  if (!fIsInit)
  {
    WNDCLASS wc;

    wc.style         = CS_VREDRAW | CS_HREDRAW;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = TEXT("CDialogWnd");

    fIsInit = CDialogWnd::RegisterClass(&wc);

    if (!fIsInit)
      hResult = GetLastError();
  }

  if (hResult == NO_ERROR)
  {
    CSocketSelectDlg oDialog;
    hResult = oDialog.RunModal(hWndParent, bBusType, phDevice, plCtrlNo);
  }

  return( hResult );
}


#ifdef MICROSOFT_C
#pragma warning(default:4100) // unreferenced formal parameter
#endif
