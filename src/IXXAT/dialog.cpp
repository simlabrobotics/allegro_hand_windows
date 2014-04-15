//////////////////////////////////////////////////////////////////////////
// IXXAT Automation GmbH
//////////////////////////////////////////////////////////////////////////
/**
  Implementation of the CDialog class.

  @file "dialog.cpp"
*/
//////////////////////////////////////////////////////////////////////////
// (C) 2002-2011 IXXAT Automation GmbH, all rights reserved
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// compiler directives
//////////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
#pragma warning(push,4)           // enable warning level 4
#pragma warning(disable:4100)     // unreferenced formal parameter
#endif

//////////////////////////////////////////////////////////////////////////
// include files
//////////////////////////////////////////////////////////////////////////
#include "dialog.hpp"
#include "WinUser.h"

//////////////////////////////////////////////////////////////////////////
// static constants, types, macros, variables
//////////////////////////////////////////////////////////////////////////

typedef struct _DLGPARA
{
  CDialogWnd* pdlg;    // pointer to the dialog object
  int         minx;    // width of smalles control
  int         miny;    // heigth of smallest control
  int         dlgw;    // width of dialog window
  int         dlgh;    // height of dialog window
  int         winw;    // current width of dialog window
  int         winh;    // current height of dialog window
  int         scrx;    // current horizontal scroll range
  int         scry;    // current vertical scroll range
  int         curx;    // current horizontal scroll position
  int         cury;    // current vertical scroll position
  int         oldx;    // last horizontal scroll position
  int         oldy;    // last vertical scroll position
} DLGPARA, *PDLGPARA;

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



/*##########################################################################*/
/*### helper functions                                                   ###*/
/*##########################################################################*/

//////////////////////////////////////////////////////////////////////////
/**
  This function retrieves the dimensions of the smallest control in the
  dialog window.

  @param hwnd
    Handle to the dialog window.

  @return
    The function returns the width in the LOWORD and the height in the HIWORD 
    of the return value.
*/
//////////////////////////////////////////////////////////////////////////
static DWORD GetSmallestDlgCtrl(HWND hwnd)
{
  HWND hctl;
  RECT rect;
  WORD w, h;

  hctl = ::GetWindow(hwnd, GW_CHILD);

  w = 0xFFFF;
  h = 0xFFFF;

  while (hctl)
  {
    ::GetWindowRect(hctl, &rect);
    w = min(w, (WORD) (rect.right - rect.left));
    h = min(h, (WORD) (rect.bottom - rect.top));
    hctl = ::GetWindow(hctl, GW_HWNDNEXT);
  }

  if (!w || (w == 0xFFFF)) w = 1;
  if (!h || (w == 0xFFFF)) h = 1;

  return( MAKELONG(w, h) );
}

//////////////////////////////////////////////////////////////////////////
/**
  This function moves the position off all controls in the dialog window 
  relative to the current control position.

  @param hwnd
    Handle to the dialog window.
  @param dx
    relative horizontal movement in pixels
  @param dy
    relative vertical movement in pixels
*/
//////////////////////////////////////////////////////////////////////////
static void MoveDlgCtrls(HWND hwnd, int dx, int dy)
{
  RECT dlgr;
  HWND hctl;
  RECT ctlr;
  int  x, y;

  if (!dx && !dy) return;

  ::GetClientRect(hwnd, &dlgr);
  ::ClientToScreen(hwnd, (LPPOINT) &dlgr);

  hctl = ::GetWindow(hwnd, GW_CHILD);

  while (hctl)
  {
    ::GetWindowRect(hctl, &ctlr);

    x = (ctlr.left - dlgr.left) + dx;
    y = (ctlr.top  - dlgr.top ) + dy;

    ::SetWindowPos(hctl,NULL,x,y,0,0,SWP_NOZORDER|SWP_NOSIZE);
    hctl = ::GetWindow(hctl, GW_HWNDNEXT);
  }

  ::InvalidateRect(hwnd, NULL, TRUE);
  ::UpdateWindow(hwnd);
}

/*##########################################################################*/
/*### Methods for CDialog                                                ###*/
/*##########################################################################*/


//////////////////////////////////////////////////////////////////////////
/**
  Default constructor for a new modal dialog box object.
*/
//////////////////////////////////////////////////////////////////////////
CDialogWnd::CDialogWnd()
{
  m_hwnd = NULL;
  m_init = 0;
  m_fmod = FALSE;
}

//////////////////////////////////////////////////////////////////////////
/**
  Destructor for a modal dialog box object.
*/
//////////////////////////////////////////////////////////////////////////
CDialogWnd::~CDialogWnd()
{
  if (m_hwnd != NULL)
  {
    EndDialog();
    DestroyWindow();
  }

  m_hwnd = NULL;
  m_init = 0;
  m_fmod = FALSE;
}

//////////////////////////////////////////////////////////////////////////
/**
  The function registers the window class used by CDialog windows. Note that 
  the function has been superseded by the RegisterClassEx function. You can
  still use this function, however, if you do not need to set the class small 
  icon.

  @param lpWndClass
    Pointer to a WNDCLASS structure. You must fill the structure with the 
    appropriate class attributes before passing it to the function.

  @return
    If the function succeeds, it returns TRUE. If the function fails, the 
    return value is FALSE. To get extended error information, call GetLastError.
*/
//////////////////////////////////////////////////////////////////////////
BOOL CDialogWnd::RegisterClass(LPWNDCLASS lpWndClass)
{
  BOOL     ok;
  WNDCLASS wc;

  if (lpWndClass)
  {
    ok = GetClassInfo(NULL, WC_DIALOG, &wc);
    if (ok)
    {
      lpWndClass->cbClsExtra    = wc.cbClsExtra;
      lpWndClass->cbWndExtra    = wc.cbWndExtra;
      lpWndClass->lpfnWndProc   = CDialogWnd::DlgWndProc;
      ok = (::RegisterClass(lpWndClass) != 0);
    }
  }
  else
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    ok = FALSE;
  }

  return( ok );
}

//////////////////////////////////////////////////////////////////////////
/**
  The function registers the window class used by CDialogWnd windows.

  @param lpWndClsEx
    Pointer to a WNDCLASSEX structure. You must fill the structure with the 
    appropriate class attributes before passing it to the function.

  @return
    If the function succeeds, it returns TRUE. If the function fails, the 
    return value is FALSE. To get extended error information, call GetLastError.
*/
//////////////////////////////////////////////////////////////////////////
BOOL CDialogWnd::RegisterClassEx(LPWNDCLASSEX lpWndClsEx)
{
  BOOL       ok;
  WNDCLASSEX wc;

  if (lpWndClsEx)
  {
    wc.cbSize = sizeof(WNDCLASSEX);
    ok = GetClassInfoEx(NULL, WC_DIALOG, &wc);
    if (ok)
    {
      lpWndClsEx->cbSize      = sizeof(WNDCLASSEX);
      lpWndClsEx->cbClsExtra  = wc.cbClsExtra;
      lpWndClsEx->cbWndExtra  = wc.cbWndExtra;
      lpWndClsEx->lpfnWndProc = CDialogWnd::DlgWndProc;
      ok = (::RegisterClassEx(lpWndClsEx) != 0);
    }
  }
  else
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    ok = FALSE;
  }

  return( ok );
}

//////////////////////////////////////////////////////////////////////////
/**
  This function creates a modal dialog box from a dialog box template resource. 
  Before displaying the dialog box, the function passes an application-defined 
  value to the dialog box procedure as the lParam parameter of the 
  WM_INITDIALOG message. An application can use this value to initialize dialog 
  box controls.

  @param hInstance
    Handle to the module whose executable file contains the dialog box template.
  @param pTemplateName
    Specifies the dialog box template. This parameter is either the pointer to 
    a null-terminated character string that specifies the name of the dialog 
    box template or an integer value that specifies the resource identifier of
    the dialog box template. If the parameter specifies a resource identifier, 
    its high-order word must be zero and its low-order word must contain the 
    identifier. You can use the MAKEINTRESOURCE macro to create this value.
  @param hwndParent
    Handle to the window that owns the dialog box.
  @param dwInitParam
    Specifies the value to pass to the dialog box in the lParam parameter of 
    the WM_INITDIALOG message. 

  @return
    If the function succeeds, the return value is the value of the nResult
    parameter specified in the call to the Destroy function used to terminate
    the dialog box. If the function fails because the hWndParent parameter is
    invalid, the return value is zero. The function returns zero in this case
    for compatibility with previous versions of Windows. If the function fails
    for any other reason, the return value is -1. To get extended error
    information, call GetLastError.
*/
//////////////////////////////////////////////////////////////////////////
int CDialogWnd::DialogBox( HINSTANCE hInstance, 
                           LPCTSTR   pTemplateName,
                           HWND      hwndParent, 
                           LPARAM    dwInitParam)
{
  INT_PTR iResult;

  m_init = dwInitParam;
  m_fmod = TRUE;
  iResult = ::DialogBoxParam( hInstance, 
                              pTemplateName, 
                              hwndParent,
                              CDialogWnd::StdDlgProc, 
                              (LPARAM) this);
  return (int) iResult;
}

//////////////////////////////////////////////////////////////////////////
/**
  This function creates a modeless dialog box from a dialog box template 
  resource. Before displaying the dialog box, the function passes an 
  application-defined value to the dialog box procedure as the lParam 
  parameter of the WM_INITDIALOG message. An application can use this value 
  to initialize dialog box controls. 


  @param hInstance
    Handle to the module whose executable file contains the dialog box template.
  @param pTemplateName
    Specifies the dialog box template. This parameter is either the pointer to 
    a null-terminated character string that specifies the name of the dialog 
    box template or an integer value that specifies the resource identifier of
    the dialog box template. If the parameter specifies a resource identifier, 
    its high-order word must be zero and its low-order word must contain the 
    identifier. You can use the MAKEINTRESOURCE macro to create this value.
  @param hwndParent
    Handle to the window that owns the dialog box.
  @param dwInitParam
    Specifies the value to pass to the dialog box in the lParam parameter of 
    the WM_INITDIALOG message. 

  @return
    If the function succeeds, the return value is TRUE. If the function fails, 
    the return value is FALSE. To get extended error information, call 
    GetLastError.
*/
//////////////////////////////////////////////////////////////////////////
BOOL CDialogWnd::CreateDialog( HINSTANCE hInstance, 
                               LPCTSTR   pTemplateName,
                               HWND      hwndParent, 
                               LPARAM    dwInitParam )
{
  m_init = dwInitParam;
  m_fmod = FALSE;
  m_hwnd = ::CreateDialogParam(hInstance, pTemplateName, hwndParent,
                               CDialogWnd::StdDlgProc, (LPARAM) this);
  return( m_hwnd != NULL );
}

//////////////////////////////////////////////////////////////////////////
/**
  This function determines whether a message is intended for the dialog box 
  and, if it is, processes the message. 

  @param pMsg
    Pointer to an MSG structure that contains the message to be checked.

  @return
    If the message has been processed, the return value is TRUE. If the message 
    has not been processed, the return value is FALSE. To get extended error 
    information, call GetLastError.
*/
//////////////////////////////////////////////////////////////////////////
BOOL CDialogWnd::DispatchMessage(LPMSG pMsg)
{
  BOOL fResult;

  if (m_hwnd != NULL)
  {
    fResult = ::IsDialogMessage(m_hwnd, pMsg);

    if ((fResult == FALSE) && (pMsg->hwnd == m_hwnd))
    {
      TranslateMessage(pMsg);
      DispatchMessage(pMsg);
      fResult = TRUE;
    }
  }
  else
  {
    fResult = FALSE;
  }

  return( fResult );
}

//////////////////////////////////////////////////////////////////////////
/**
  This function checks the thread message queue for a posted message intended 
  either for the dialog box, or any other window, and process a single message 
  (if any exist).

  @param fAllWindows
    If set to TRUE the function process all messages for all windows of the 
    thread. If set to FALSE the function processes only those messages intended 
    for the dialog box.
  @param wMsgFilterMin
    Specifies the value of the first message in the range of messages to be 
    examined.
  @param wMsgFilterMax
    Specifies the value of the last message in the range of messages to be 
    examined.
  @param wMsgTerminate
    Specifies the value of the message which terminates the message processing. 
    The message specified here will not be processed.

  @return
    If the message specified by wMsgTerminate or the WM_QUIT message has been 
    received, the return value is FALSE, otherwise the return value is TRUE.

  @note
    The function processes only messages associated with the dialog box or any
    of its children as specified by the IsChild function, and within the range
    of message values given by the wMsgFilterMin and wMsgFilterMax parameters.
    If fAllWindows is TRUE, the function processes messages for any window that
    belongs to the current thread making the call.
    If wMsgFilterMin and wMsgFilterMax are both zero, the function processes all
    available messages (that is, no range filtering is performed). Note that the
    WM_QUIT message will always terminate the message processing. The function
    does not process messages for windows that belong to other threads.
*/
//////////////////////////////////////////////////////////////////////////
BOOL CDialogWnd::ProcessMessage ( BOOL fAllWindows, 
                                  UINT wMsgFilterMin,
                                  UINT wMsgFilterMax, 
                                  UINT wMsgTerminate)
{
  HWND hwnd;
  MSG  sMsg;
  BOOL fRes;

  hwnd = fAllWindows ? NULL : m_hwnd;
  fRes = TRUE;

  if (::PeekMessage(&sMsg, hwnd, wMsgFilterMin, wMsgFilterMax, PM_REMOVE))
  {
    if ((sMsg.message == WM_QUIT) || (sMsg.message == wMsgTerminate))
    {
      fRes = FALSE;
    }
    else
    {
      if (!::IsDialogMessage(m_hwnd, &sMsg))
      {
        TranslateMessage(&sMsg);
        DispatchMessage(&sMsg);
      }
    }
  }

  return( fRes );
}

//////////////////////////////////////////////////////////////////////////
/**
  This function checks the thread message queue for posted messages intended 
  either for the dialog box, or any other window, and processes the messages 
  (if any exist).

  @param fAllWindows
    If set to TRUE the function process all messages for all windows of the 
    thread. If set to FALSE the function processes only those messages 
    intended for the dialog box. 
  @param wMsgFilterMin
    Specifies the value of the first message in the range of messages to be 
    examined.
  @param wMsgFilterMax
    Specifies the value of the last message in the range of messages to be 
    examined.
  @param wMsgTerminate
    Specifies the value of the message which terminates the message processing. 
    The message specified here will not be processed.

  @return
    If the message specified by wMsgTerminate or the WM_QUIT message has been 
    received, the return value is FALSE, otherwise the return value is TRUE.

  @note
    The function processes only messages associated with the dialog box or any
    of its children as specified by the IsChild function, and within the range
    of message values given by the wMsgFilterMin and wMsgFilterMax parameters.
    If fAllWindows is TRUE, the function processes messages for any window that
    belongs to the current thread making the call.
    If wMsgFilterMin and wMsgFilterMax are both zero, the function processes all
    available messages (that is, no range filtering is performed). Note that the
    WM_QUIT message will always terminate the message processing. The function
    does not process messages for windows that belong to other threads.
*/
//////////////////////////////////////////////////////////////////////////
BOOL CDialogWnd::ProcessMessages( BOOL fAllWindows, 
                                  UINT wMsgFilterMin,
                                  UINT wMsgFilterMax, 
                                  UINT wMsgTerminate)
{
  HWND hwnd;
  MSG  sMsg;
  BOOL fRes;

  hwnd = fAllWindows ? NULL : m_hwnd;
  fRes = TRUE;

  while (::PeekMessage(&sMsg, hwnd, wMsgFilterMin, wMsgFilterMax, PM_REMOVE))
  {
    if ((sMsg.message == WM_QUIT) || (sMsg.message == wMsgTerminate))
    {
      fRes = FALSE;
      break;
    }
    else
    {
      if (!::IsDialogMessage(m_hwnd, &sMsg))
      {
        TranslateMessage(&sMsg);
        DispatchMessage(&sMsg);
      }
    }
  }

  return( fRes );
}

//////////////////////////////////////////////////////////////////////////
/**
  This function displays an error message box for the specified error.

  @param pszError
    Points to the error message to be displayed. If this parameter is NULL, 
    no message box is displayed by the function.
*/
//////////////////////////////////////////////////////////////////////////
void CDialogWnd::DisplayErrorMessage(PTCHAR pszError)
{
  if (pszError)
    MessageBox(m_hwnd, pszError, TEXT("Error"), MB_OK|MB_ICONERROR);
}

//////////////////////////////////////////////////////////////////////////
/**
  This function displays an error message box for the specified error.

  @param hResult
    Error code or -1 to retrieve the error code by a call to GetLastError. 
    If this parameter is set to NO_ERROR, no message box is displayed by the 
    function
*/
//////////////////////////////////////////////////////////////////////////
void CDialogWnd::DisplayErrorMessage(HRESULT hResult)
{
  static PTCHAR szErrMsg[] =
  {
    TEXT("This operation returned because the timeout period expired."),
    NULL
  };

  TCHAR szError[1024];
  DWORD len;

  if (hResult != NO_ERROR)
  {
    if (hResult == -1)
      hResult = GetLastError();

    szError[0] = 0;

    len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                        hResult, 0, szError, sizeof(szError), NULL);
    if (len == 0)
    {
      switch (hResult & 0x0000FFFF)
      {
        case ERROR_TIMEOUT:
          lstrcpy(szError, szErrMsg[0]);
          break;

        default:
          wsprintf(szError,TEXT("Error code: 0x%08X (%d)"), hResult, hResult);
          break;
      }
    }

    DisplayErrorMessage(szError);
  }
}

//////////////////////////////////////////////////////////////////////////
/**
  This function is an application-defined callback function which is called
  on every WM_COMMAND message sent to the modal dialog box.

  @param wId
    Specifies the identifier of the menu item, control, or accelerator.
  @param wCode
    Specifies the notification code if the message is from a control. If the 
    message is from an accelerator, this parameter is 1. If the message is 
    from a menu, this parameter is 0. 
  @param hCtrl
    Handle to the control sending the message if the message is from a control. 
    Otherwise, this parameter is NULL. 

  @return
    If the function processes the message it returns TRUE, otherwise the 
    function returns FALSE.
*/
//////////////////////////////////////////////////////////////////////////
BOOL CDialogWnd::CmdProc(WORD wId, WORD wCode, HWND hCtrl)
{
  BOOL fResult;

  if (m_fmod == TRUE)
  {
    //
    // default WM_COMMAND handler for modal dialog boxes
    //
    switch(wId)
    {
      case IDOK    : // fall through
      case IDCANCEL: // fall through
      case IDABORT : // fall through
      case IDRETRY : // fall through
      case IDIGNORE: // fall through
      case IDYES   : // fall through
      case IDNO    :
        EndDialog(wId);
        fResult = TRUE;
        break;

      default:
        fResult = FALSE;
        break;
    }
  }
  else
  {
    //
    // default WM_COMMAND handler for modeless dialog boxes
    //
    fResult = FALSE;
  }

  return( fResult );
}

//////////////////////////////////////////////////////////////////////////
/**
  This function is an application-defined callback function used with the
  DialogBox function. It processes messages sent to a modal dialog box.

  @param uMsg
    Specifies the message.
  @param wParam
    Specifies additional message information. The contents of this parameter 
    depend on the value of the uMsg parameter. 
  @param lParam
    Specifies additional message information. The contents of this parameter 
    depend on the value of the uMsg parameter. 

  @return
    Typically, the dialog box procedure should return TRUE if it processed the
    message, and FALSE if it did not. If the dialog box procedure returns FALSE,
    the dialog manager performs the default dialog operation in response to the
    message.
*/
//////////////////////////////////////////////////////////////////////////
BOOL CDialogWnd::DlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_COMMAND:
      return( CmdProc(LOWORD(wParam), HIWORD(wParam), (HWND) lParam) );
  }
  return( FALSE );
}

//////////////////////////////////////////////////////////////////////////
/**
  The function is an class-defined callback function used with the DialogBox 
  function. It processes messages sent to a modal dialog box. The DLGPROC type 
  defines a pointer to this callback function.

  @param hwnd
    Handle to the dialog box window.
  @param uMsg
    Specifies the message.
  @param wParam
    Specifies additional message information. The contents of this parameter 
    depend on the value of the uMsg parameter. 
  @param lParam
    Specifies additional message information. The contents of this parameter 
    depend on the value of the uMsg parameter. 

  @return
    Typically, the dialog box procedure should return TRUE if it processed the 
    message, and FALSE if it did not. If the dialog box procedure returns FALSE,
    the dialog manager performs the default dialog operation in response to the
    message.
*/
//////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK CDialogWnd::StdDlgProc( HWND   hwnd, 
                                      UINT   uMsg,
                                      WPARAM wParam, 
                                      LPARAM lParam)
{
  PDLGPARA pPar;
  BOOL     fRes;

//  pPar = (PDLGPARA) ::GetWindowLong(hwnd, GWL_USERDATA);
  pPar = (PDLGPARA) ::GetWindowLongPtr ( hwnd, GWLP_USERDATA);

  if ((pPar == NULL) || (pPar->pdlg == NULL))
  {
    fRes = FALSE;
  }
  else
  {
    pPar->pdlg->m_hwnd = hwnd;
    fRes = pPar->pdlg->DlgProc(uMsg, wParam, lParam);
    if (uMsg == WM_DESTROY) pPar->pdlg->m_hwnd = NULL;
  }

  return( fRes );
}
//////////////////////////////////////////////////////////////////////////
/**
  This function is the class-defined function that processes messages sent to 
  a dialog window.

  @param hwnd
    Handle to the window.
  @param uMsg
    Specifies the message.
  @param wParam
    Specifies additional message information. The contents of this parameter 
    depend on the value of the uMsg parameter.  
  @param lParam
    Specifies additional message information. The contents of this parameter 
    depend on the value of the uMsg parameter.

  @return
    The return value is the result of the message processing and depends on the 
    message sent.
*/
//////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK CDialogWnd::DlgWndProc( HWND   hwnd, 
                                         UINT   uMsg,
                                         WPARAM wParam, 
                                         LPARAM lParam )
{
  switch (uMsg)
  {
    //--------------------------------------------------------------------
    case WM_CREATE:
    //--------------------------------------------------------------------
    {
      LRESULT  lRes;
      PDLGPARA pPar;
      RECT     rect;

      lRes = ::DefDlgProc(hwnd, uMsg, wParam, lParam);
      if (lRes) return(lRes);

      pPar = (PDLGPARA) ::LocalAlloc(LPTR, sizeof(DLGPARA));
      if (!pPar) return(-1);

      ::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG) pPar);
      ::GetClientRect(hwnd, &rect);

      pPar->minx = 1;
      pPar->miny = 1;
      pPar->dlgw = rect.right;
      pPar->dlgh = rect.bottom;
      pPar->winw = rect.right;
      pPar->winh = rect.bottom;
      pPar->scrx = 0;
      pPar->scry = 0;
      pPar->curx = 0;
      pPar->cury = 0;
      pPar->oldx = 0;
      pPar->oldy = 0;

      return(lRes);
    }
    //--------------------------------------------------------------------
    case WM_DESTROY:
    //--------------------------------------------------------------------
    {
      LRESULT lRes;
      HLOCAL  hMem;

      lRes = ::DefDlgProc(hwnd, uMsg, wParam, lParam);
      hMem = (HLOCAL) ::SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
      if (hMem) LocalFree(hMem);

      return(lRes);
    }
    //--------------------------------------------------------------------
    case WM_INITDIALOG:
    //--------------------------------------------------------------------
    {
      PDLGPARA pPar;

      pPar = (PDLGPARA) ::GetWindowLongPtr(hwnd, GWLP_USERDATA);
      if (pPar)
      {
        DWORD Dims = GetSmallestDlgCtrl(hwnd);
        pPar->minx = LOWORD(Dims);
        pPar->miny = HIWORD(Dims);
      }

      pPar->pdlg = (CDialogWnd*) lParam;
      if (pPar->pdlg)
      {
        pPar->pdlg->m_hwnd = hwnd;
        lParam = pPar->pdlg->m_init;
      }

      return( ::DefDlgProc(hwnd, uMsg, wParam, lParam) );
    }
    //--------------------------------------------------------------------
    case WM_SIZE:
    //--------------------------------------------------------------------
    {
      PDLGPARA   pPar;
      SCROLLINFO six;
      SCROLLINFO siy;

      switch (wParam)
      {
        case SIZE_MAXHIDE  : return(::DefDlgProc(hwnd,uMsg,wParam,lParam));
        case SIZE_MAXSHOW  : return(::DefDlgProc(hwnd,uMsg,wParam,lParam));
        case SIZE_MINIMIZED: return(::DefDlgProc(hwnd,uMsg,wParam,lParam));
        case SIZE_MAXIMIZED: break;
        case SIZE_RESTORED : break;
      }

      pPar = (PDLGPARA) ::GetWindowLongPtr(hwnd, GWLP_USERDATA);
      if (!pPar) return( ::DefDlgProc(hwnd, uMsg, wParam, lParam) );

      six.cbSize = sizeof(SCROLLINFO);
      six.fMask  = SIF_ALL;
      if (!::GetScrollInfo(hwnd, SB_HORZ, &six)) six.nPos = 0;

      siy.cbSize = sizeof(SCROLLINFO);
      siy.fMask  = SIF_ALL;
      if (!::GetScrollInfo(hwnd, SB_VERT, &siy)) siy.nPos = 0;

      pPar->winw = (int) LOWORD(lParam);
      pPar->winh = (int) HIWORD(lParam);
      pPar->scrx = max(0, pPar->dlgw - pPar->winw);
      pPar->scry = max(0, pPar->dlgh - pPar->winh);
      pPar->curx = min(six.nPos, pPar->scrx);
      pPar->cury = min(siy.nPos, pPar->scry);

      six.nMin  = 0;
      six.nMax  = pPar->dlgw - 1;
      six.nPage = pPar->winw;
      six.nPos  = pPar->curx;
      ::SetScrollInfo(hwnd, SB_HORZ, &six, TRUE);

      siy.nMin  = 0;
      siy.nMax  = pPar->dlgh - 1;
      siy.nPage = pPar->winh;
      siy.nPos  = pPar->cury;
      ::SetScrollInfo(hwnd, SB_VERT, &siy, TRUE);

      MoveDlgCtrls(hwnd, pPar->oldx - pPar->curx, pPar->oldy - pPar->cury);
      pPar->oldx = pPar->curx;
      pPar->oldy = pPar->cury;

      return( ::DefDlgProc(hwnd, uMsg, wParam, lParam) );
    }
    //--------------------------------------------------------------------
    case WM_HSCROLL:
    //--------------------------------------------------------------------
    {
      PDLGPARA   pPar;
      SCROLLINFO si;
      int        pgsz;
      int        xmin;
      int        xmax;
      int        xold;
      int        x, y;

      if (lParam && ((HWND) lParam != hwnd))
        return( ::DefDlgProc(hwnd, uMsg, wParam, lParam) );

      pPar = (PDLGPARA) ::GetWindowLongPtr(hwnd, GWLP_USERDATA);
      if (!pPar) return( ::DefDlgProc(hwnd, uMsg, wParam, lParam) );

      si.cbSize = sizeof(SCROLLINFO);
      si.fMask  = SIF_ALL;
      ::GetScrollInfo(hwnd, SB_HORZ, &si);

      pgsz = (int) si.nPage;
      xmin = si.nMin;
      xmax = max(0, pPar->dlgw - pgsz);;
      xold = si.nPos;

      switch (LOWORD(wParam))
      {
        case SB_TOP          : x = xmin;                         break;
        case SB_BOTTOM       : x = xmax;                         break;
        case SB_PAGEUP       : x = max(xmin, xold - pgsz);       break;
        case SB_PAGEDOWN     : x = min(xmax, xold + pgsz);       break;
        case SB_LINEUP       : x = max(xmin, xold - pPar->minx); break;
        case SB_LINEDOWN     : x = min(xmax, xold + pPar->minx); break;
        case SB_THUMBPOSITION: x = HIWORD(wParam);               break;
        case SB_THUMBTRACK   : x = HIWORD(wParam);               break;
        case SB_ENDSCROLL    :
        default              : x = xold;
      }

      if (x == xold) return(0);

      si.fMask = SIF_POS;
      si.nPos  = x;
      ::SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
      ::GetScrollInfo(hwnd, SB_VERT, &si);
      y = si.nPos;

      MoveDlgCtrls(hwnd, pPar->oldx - x, pPar->oldy - y);
      pPar->oldx = x;
      pPar->oldy = y;

      return(0);
    }
    //--------------------------------------------------------------------
    case WM_VSCROLL:
    //--------------------------------------------------------------------
    {
      PDLGPARA   pPar;
      SCROLLINFO si;
      int        pgsz;
      int        ymin;
      int        ymax;
      int        yold;
      int        x, y;

      if (lParam && ((HWND) lParam != hwnd))
        return( ::DefDlgProc(hwnd, uMsg, wParam, lParam) );

      pPar = (PDLGPARA) ::GetWindowLongPtr(hwnd, GWLP_USERDATA);
      if (!pPar) return( ::DefDlgProc(hwnd, uMsg, wParam, lParam) );

      si.cbSize = sizeof(SCROLLINFO);
      si.fMask  = SIF_ALL;
      ::GetScrollInfo(hwnd, SB_VERT, &si);

      pgsz = (int) si.nPage;
      ymin = si.nMin;
      ymax = max(0, pPar->dlgh - pgsz);
      yold = si.nPos;

      switch (LOWORD(wParam))
      {
        case SB_TOP          : y = ymin;                         break;
        case SB_BOTTOM       : y = ymax;                         break;
        case SB_PAGEUP       : y = max(ymin, yold - pgsz);       break;
        case SB_PAGEDOWN     : y = min(ymax, yold + pgsz);       break;
        case SB_LINEUP       : y = max(ymin, yold - pPar->miny); break;
        case SB_LINEDOWN     : y = min(ymax, yold + pPar->miny); break;
        case SB_THUMBPOSITION: y = HIWORD(wParam);               break;
        case SB_THUMBTRACK   : y = HIWORD(wParam);               break;
        case SB_ENDSCROLL    :
        default              : y = yold;
      }

      if (y == yold) return(0);

      si.fMask = SIF_POS;
      si.nPos  = y;
      ::SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
      ::GetScrollInfo(hwnd, SB_HORZ, &si);
      x = si.nPos;

      MoveDlgCtrls(hwnd, pPar->oldx - x, pPar->oldy - y);
      pPar->oldx = x;
      pPar->oldy = y;

      return(0);
    }
  }

  return( ::DefDlgProc(hwnd, uMsg, wParam, lParam) );
}

#ifdef _MSC_VER
#pragma warning(default:4100)     // unreferenced formal parameter
#pragma warning(pop)              // enable previous warning level
#endif

