//////////////////////////////////////////////////////////////////////////
// IXXAT Automation GmbH
//////////////////////////////////////////////////////////////////////////
/**
  Declarations for Windows dialog classes.

  @file "dialog.hpp"
*/
//////////////////////////////////////////////////////////////////////////
// (C) 2002-2011 IXXAT Automation GmbH, all rights reserved
//////////////////////////////////////////////////////////////////////////

#ifndef DIALOG_HPP
#define DIALOG_HPP

//////////////////////////////////////////////////////////////////////////
//  include files
//////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <windowsx.h>

//////////////////////////////////////////////////////////////////////////
//  constants and macros
//////////////////////////////////////////////////////////////////////////

#define DLGCBPARAM  WPARAM wParam, LPARAM lParam
#define CMDCBPARAM  WORD wCode, HWND hCtrl

//------------------------------------------------------------------------
// Macro:
//  ON_MESSAGE
//
// Description:
//  This macros expands to a case statement for the switch case declared
//  by the BEGIN_DIALOG_PROG macro.
//
// Arguments:
//  Number -> Number of the message (constant)
//  Method -> method to be called
//------------------------------------------------------------------------
#define ON_MESSAGE(Number, Method) \
        case (Number): return( Method##(wp, lp) );

//------------------------------------------------------------------------
// Macro:
//  ON_COMMAND
//
// Description:
//  This macros expands to a case statement for the switch case declared
//  by the BEGIN_COMMAND_PROC macro.
//
// Arguments:
//  CtrlID -> ID of the control
//  Method -> method to be called
//------------------------------------------------------------------------
#define ON_COMMAND(CtrlID, Method) \
        case (CtrlID): return( Method##(wCode, hCtrl) );

//------------------------------------------------------------------------
// Macro:
//  BEGIN_DIALOG_PROC
//
// Description:
//  This macros can be used to declare a message handler for a dialog box.
//  It expands to the ClassName::DialogProc function which implements a
//  switch case for message handling.
//
// Arguments:
//  Class -> Name of the dialog class.
//------------------------------------------------------------------------
#define BEGIN_DIALOG_PROC(Class) \
  BOOL Class::DlgProc(UINT msg, WPARAM wp, LPARAM lp) \
        { switch (msg) {

//------------------------------------------------------------------------
// Macro:
//  END_DIALOG_PROC
//
// Description:
//  This macros ends the message handler declared by the BEGIN_DIALOG_PROG
//  macro. The macro expands to code which closes the switch case and call
//  the DialogProc method of the specified base class for all messages not
//  handled by a DLGEVENT macro.
//
// Arguments:
//  Base -> Base class of the dialog (usually CDialog).
//------------------------------------------------------------------------
#define END_DIALOG_PROC(Base) \
        } return( Base::DlgProc(msg, wp, lp) ); }

//------------------------------------------------------------------------
// Macro:
//  BEGIN_COMMAND_PROC
//
// Description:
//  This macros can be used to declare a command message handler for a
//  dialog box. It expands to the ClassName::CommandProc function which
//  implements a switch case for WM_COMMAND message handling.
//
// Arguments:
//  Class -> Name of the dialog class.
//------------------------------------------------------------------------
#define BEGIN_COMMAND_PROC(Class) \
  BOOL Class::CmdProc(WORD wId, WORD wCode, HWND hCtrl) \
        { switch (wId) {

//------------------------------------------------------------------------
// Macro:
//  END_COMMAND_PROC
//
// Description:
//  This macros ends the message handler declared by the BEGIN_COMMAND_PROC
//  macro. The macro expands to code which close the switch case and call
//  the CommandProc method of the specified base class for all commands not
//  handled by a COMMAND macro.
//
// Arguments:
//  Base -> Base class of the dialog (usually CDialog).
//------------------------------------------------------------------------
#define END_COMMAND_PROC(Base) \
        } return( Base::CmdProc(wId, wCode, hCtrl) ); }

//////////////////////////////////////////////////////////////////////////
// data types
//////////////////////////////////////////////////////////////////////////



#ifdef __cplusplus

//////////////////////////////////////////////////////////////////////////
/**
  This class implements the standard dialog class.
*/
//////////////////////////////////////////////////////////////////////////
class CDialogWnd
{
  public:
    static BOOL RegisterClass  ( LPWNDCLASS lpWndClass );
    static BOOL RegisterClassEx( LPWNDCLASSEX lpWndClsEx );

    CDialogWnd();
    virtual ~CDialogWnd();

    #undef CreateDialog  // undefine macros from winuser.h
    #undef DialogBox

    BOOL    CreateDialog         ( HINSTANCE hInstance, LPCTSTR pTemplateName,
                                   HWND hWndParent, LPARAM dwInitParam = 0 );
    int     DialogBox            ( HINSTANCE hInstance, LPCTSTR pTemplateName,
                                   HWND hWndParent, LPARAM dwInitParam = 0 );
    BOOL    EndDialog            ( int nResult = 0 );
    BOOL    ShowWindow           ( int nCmdShow = SW_SHOWNORMAL );
    BOOL    UpdateWindow         ( void );
    BOOL    EnableWindow         ( BOOL fEnable );
    BOOL    DestroyWindow        ( void );
    LONG    GetWindowLong        ( int nIndex );
    LONG    SetWindowLong        ( int nIndex, LONG lNewLong );
    int     GetWindowText        ( PTSTR pString, int nMaxCount );
    BOOL    SetWindowText        ( PTSTR pString );

    LRESULT SendMessage          ( UINT uMsg, WPARAM wParam, LPARAM lParam );
    BOOL    PostMessage          ( UINT uMsg, WPARAM wParam, LPARAM lParam );
    BOOL    DispatchMessage      ( LPMSG pMsg );
    BOOL    ProcessMessage       ( BOOL fAllWindows,
                                   UINT wMsgFilterMin = 0,
                                   UINT wMsgFilterMax = 0,
                                   UINT wMsgTerminate = WM_QUIT );
    BOOL    ProcessMessages      ( BOOL fAllWindows,
                                   UINT wMsgFilterMin = 0,
                                   UINT wMsgFilterMax = 0,
                                   UINT wMsgTerminate = WM_QUIT );
    void    DisplayErrorMessage  ( LPTSTR pszError );
    void    DisplayErrorMessage  ( HRESULT hResult );
    HWND    GetHandle            ( void );
    HWND    GetItem              ( int nItemId );
    LRESULT SendItemMessage      ( int nItemId, UINT uMsg,
                                   WPARAM wParam, LPARAM lParam );
    BOOL    EnableControl        ( int nIdCtrl, BOOL fEnable );
    BOOL    CheckRadioButton     ( int nIdFirst, int nIdLast, int nIdCheck );
    int     GetCheckedRadioButton( int nIdFirst, int nIdLast );
    BOOL    CheckButton          ( int nIdButton, UINT uCheck );
    UINT    IsButtonChecked      ( int nIdButton );
    UINT    GetItemInt           ( int nIdItem, PBOOL pfTrans,
                                   BOOL fSigned = TRUE );
    BOOL    SetItemInt           ( int nIdItem, UINT uValue,
                                   BOOL fSigned = TRUE );
    UINT    GetItemText          ( int nIdItem, LPTSTR pString, int nCount );
    BOOL    SetItemText          ( int nIdCtrl, LPCTSTR pString );

  protected:
    virtual BOOL CmdProc( WORD wId, WORD wCode, HWND hCtrl );
    virtual BOOL DlgProc( UINT uMsg, WPARAM wParam, LPARAM lParam );

  private:
    HWND   m_hwnd; // handle of dialog window
    LPARAM m_init; // initialization parameter
    BOOL   m_fmod; // modal dialog box

    static INT_PTR CALLBACK StdDlgProc( HWND hwnd, UINT uMsg,
                                        WPARAM wParam, LPARAM lParam );
    static LRESULT CALLBACK DlgWndProc( HWND hwnd, UINT uMsg,
                                        WPARAM wParam, LPARAM lParam );
};

//////////////////////////////////////////////////////////////////////////
/**
  The function destroys a modal dialog box, causing the system to end
  any processing for the dialog box. To destroy a modeless dialog box
  use DestroyWindow.

  @param nResult
    Specifies the value to be returned to the application from the function 
    that created the dialog box.

  @return
    If the function succeeds, the return value is TRUE. If the function
    fails, the return value is FALSE. To get extended error information,
     call GetLastError.
*/
//////////////////////////////////////////////////////////////////////////
inline BOOL CDialogWnd::EndDialog(int nResult)
{
  return( ::EndDialog(m_hwnd, nResult) );
}

//////////////////////////////////////////////////////////////////////////
/**
  The function sets the specified window's show state.

  @param nCmdShow
    Specifies how the window is to be shown. This parameter can be one of 
    the SW_xxx values.

  @return
    If the window was previously visible, the return value is nonzero.
    If the window was previously hidden, the return value is zero. 
*/
//////////////////////////////////////////////////////////////////////////
inline BOOL CDialogWnd::ShowWindow(int nCmdShow)
{
  return( ::ShowWindow(m_hwnd, nCmdShow) );
}

//////////////////////////////////////////////////////////////////////////
/**
  This function updates the client area of the by sending a WM_PAINT
  message to the window if the window's update region is not empty.
  The function sends the WM_PAINT message directly to the window
  procedure, bypassing the applications queue. If the update region
  is empty, no message is sent. 

  @return
    If the function succeeds, the return value is TRUE. If the function
    fails, the return value is FALSE. To get extended error information,
    call GetLastError.
*/
//////////////////////////////////////////////////////////////////////////
inline BOOL CDialogWnd::UpdateWindow( )
{
  return( ::UpdateWindow(m_hwnd) );
}

//////////////////////////////////////////////////////////////////////////
/**
  This function enables or disables mouse and keyboard input to the
  dialog window. When input is disabled, the dialog window does not
  receive input such as mouse clicks and key presses. When input is
  enabled, the window receives all input.

  @param fEnable
    Specifies whether to enable or disable the window. If this parameter is 
    TRUE, the window is enabled. If the parameter is FALSE, the window is 
    disabled.

  @return
    If the window was previously disabled, the return value is FALSE. If
    the window was not previously disabled, the return value is TRUE. To
    get extended error information, call GetLastError.
*/
//////////////////////////////////////////////////////////////////////////
inline BOOL CDialogWnd::EnableWindow(BOOL fEnable)
{
  return( ~::EnableWindow(m_hwnd, fEnable) );
}

//////////////////////////////////////////////////////////////////////////
/**
  The function destroys a modeless dialog box, causing the system to
  end any processing for the dialog box. To destroy a modal dialog box
  use EndDialog.

  @return
    If the function succeeds, the return value is TRUE. If the function
    fails, the return value is FALSE. To get extended error information,
    call GetLastError.
*/
//////////////////////////////////////////////////////////////////////////
inline BOOL CDialogWnd::DestroyWindow( )
{
  return( ::DestroyWindow(m_hwnd) );
}

//////////////////////////////////////////////////////////////////////////
/**
  The function retrieves information about the specified window. The
  function also retrieves the 32-bit (long) value at the specified
  offset into the extra window memory.

  @param nIndex
    Specifies the zero-based offset to the value to be retrieved.
    Valid values are in the range zero through the number of
    bytes of extra window memory, minus four; for example, if
    you specified 12 or more bytes of extra memory, a value of
    8 would be an index to the third 32-bit integer. To retrieve
    any other value, specify one of the following values.
    - GWL_EXSTYLE
      Retrieves the extended window styles. For more information,
      see CreateWindowEx.  
    - GWL_STYLE
      Retrieves the window styles. 
    - GWL_WNDPROC
      Retrieves the address of the window procedure, or a handle
      representing the address of the window procedure. You must
      use the CallWindowProc function to call the window procedure.
    - GWL_HINSTANCE
      Retrieves a handle to the application instance. 
    - GWL_HWNDPARENT
      Retrieves a handle to the parent window, if any.
    - GWL_ID
      Retrieves the identifier of the window. 
    - GWL_USERDATA
      Retrieves the 32-bit value associated with the window. Each
      window has a corresponding 32-bit value intended for use by
      the application that created the window. This value is
      initially zero. 
    The following values are also available for dialog boxes.
    - DWL_DLGPROC
      Retrieves the address of the dialog box procedure, or
      a handle representing the address of the dialog box
      procedure. You must use the CallWindowProc function to
      call the dialog box procedure. 
    - DWL_MSGRESULT
      Retrieves the return value of a message processed in the
      dialog box procedure. 
    - DWL_USER
      Retrieves extra information private to the application,
      such as handles or pointers. 

  @return
    If the function succeeds, the return value is the requested 32-bit
    value. If the function fails, the return value is zero. To get
    extended error information, call GetLastError. 
*/
//////////////////////////////////////////////////////////////////////////
inline LONG CDialogWnd::GetWindowLong(int nIndex)
{
  return( ::GetWindowLong(m_hwnd, nIndex) );
}

//////////////////////////////////////////////////////////////////////////
/**
//  The function changes an attribute of the specified window. The
//  function also sets the 32-bit (long) value at the specified
//  offset into the extra window memory.

  @param nIndex
    Specifies the zero-based offset to the value to be set.
    Valid values are in the range zero through the number of
    bytes of extra window memory, minus 4; for example, if
    you specified 12 or more bytes of extra memory, a value
    of 8 would be an index to the third 32-bit integer. To
    set any other value, specify one of the following values.
    - GWL_EXSTYLE
      Sets a new extended window style. For more information,
      see CreateWindowEx. 
    - GWL_STYLE
      Sets a new window style.
    - GWL_WNDPROC
      Sets a new address for the window procedure. 
      Windows NT/2000: You cannot change this attribute if
      the window does not belong to the same process as the
      calling thread.
    - GWL_HINSTANCE
      Sets a new application instance handle.
    - GWL_ID
      Sets a new identifier of the window.
    - GWL_USERDATA
      Sets the 32-bit value associated with the window. Each
      window has a corresponding 32-bit value intended for
      use by the application that created the window. This
      value is initially zero.
    The following values are also available for dialog boxes.
    - DWL_DLGPROC
      Sets the new address of the dialog box procedure.
    - DWL_MSGRESULT
      Sets the return value of a message processed in the
      dialog box procedure.
    - DWL_USER
      Sets new extra information that is private to the
      application, such as handles or pointers.
  @param lNewLong
    @Specifies the replacement value. 

  @return
  //  If the function succeeds, the return value is the previous value of
  //  the specified 32-bit integer. If the function fails, the return value
  //  is zero. To get extended error information, call GetLastError. If the
  //  previous value of the specified 32-bit integer is zero, and the
  //  function succeeds, the return value is zero and a GetLastError result
  //  that is also zero.

*/
//////////////////////////////////////////////////////////////////////////
inline LONG CDialogWnd::SetWindowLong(int nIndex, LONG lNewLong)
{
  SetLastError(0);
  return( ::SetWindowLong(m_hwnd, nIndex, lNewLong) );
}

//////////////////////////////////////////////////////////////////////////
/**
  The function copies the text of the window's title bar (if it has one) 
  into a buffer.

  @param pszString
    Pointer to the buffer that will receive the text.
  @param nMaxCount
    Specifies the maximum number of characters to copy to the buffer, 
    including the NULL character. If the text exceeds this limit, it is 
    truncated.

  @return
    If the function succeeds, the return value is the length, in 
    characters, of the copied string, not including the terminating null 
    character. If the window has no title bar or text, if the title bar is 
    empty, the return value is zero. To get extended error information, call 
    GetLastError.
*/
//////////////////////////////////////////////////////////////////////////
inline int CDialogWnd::GetWindowText(PTSTR pszString, int nMaxCount)
{
  return( ::GetWindowText(m_hwnd, pszString, nMaxCount) );
}

//////////////////////////////////////////////////////////////////////////
/**
  The function changes the text of the window's title bar (if it has one).

  @param pszString
    Pointer to a null-terminated string to be used as the new title text. 

  @return
    If the function succeeds, the return value is nonzero. If the function 
    fails, the return value is zero. To get extended error information, call 
    GetLastError.
*/
//////////////////////////////////////////////////////////////////////////
inline BOOL CDialogWnd::SetWindowText(PTSTR pszString)
{
  return( ::SetWindowText(m_hwnd, pszString) );
}

//////////////////////////////////////////////////////////////////////////
/**
  The function sends a message to the dialog box window. It calls the dialog 
  procedure for the specified dialog box and does not return until the window 
  procedure has processed the message.

  @param uMsg
    Specifies the message to be sent. 
  @param wParam
    Specifies additional message-specific information. 
  @param lParam
    Specifies additional message-specific information.

  @return
    The return value specifies the result of the message processing it depends 
    on the message sent.
*/
//////////////////////////////////////////////////////////////////////////
inline LRESULT CDialogWnd::SendMessage( UINT   uMsg,
                                        WPARAM wParam,
                                        LPARAM lParam )
{
  return( ::SendMessage(m_hwnd, uMsg, wParam, lParam) );
}

//////////////////////////////////////////////////////////////////////////
/**
  The function places (posts) a message in the message queue associated with 
  the thread that created the dialog box window and returns without waiting 
  for the thread to process the message.

  @param uMsg
    Specifies the message to be posted.
  @param wParam
    Specifies additional message-specific information. 
  @param lParam
    Specifies additional message-specific information. 

  @return
    If the function succeeds, the return value is TRUE. If the function fails, 
    the return value is FALSE. To get extended error information, call 
    GetLastError. 
*/
//////////////////////////////////////////////////////////////////////////
inline BOOL CDialogWnd::PostMessage( UINT   uMsg,
                                     WPARAM wParam,
                                     LPARAM lParam )
{
  return( ::PostMessage(m_hwnd, uMsg, wParam, lParam) );
}

//////////////////////////////////////////////////////////////////////////
/**
  The function retrieve the handle of the dialog box window.

  @return
    If the function succeeds, the return value is the HWND of the dialog box 
    window. If the function fails, the return value is NULL.
*/
//////////////////////////////////////////////////////////////////////////
inline HWND CDialogWnd::GetHandle( )
{
  return( m_hwnd );
}

//////////////////////////////////////////////////////////////////////////
/**
  The function retrieves a handle to a control in the dialog box.

  @param nItemId
    Specifies the identifier of the control to be retrieved

  @return
    If the function succeeds, the return value is the window handle of the 
    specified control. If the function fails, the return value is NULL, 
    indicating an a nonexistent control. To get extended error information, 
    call GetLastError.
*/
//////////////////////////////////////////////////////////////////////////
inline HWND CDialogWnd::GetItem(int nItemId)
{
  return( ::GetDlgItem(m_hwnd, nItemId) );
}

//////////////////////////////////////////////////////////////////////////
/**
  The function sends a message to the specified control in the dialog box. 

  @param nItemId
  Specifies the identifier of the control that receives
  //             the message
  @param uMsg
    Specifies the message to be sent. 
  @param wParam
    Specifies additional message-specific information. 
  @param lParam
    Specifies additional message-specific information. 

  @return
    The return value specifies the result of the message processing and depends 
    on the message sent.
*/
//////////////////////////////////////////////////////////////////////////
inline LRESULT CDialogWnd::SendItemMessage( int    nItemId,
                                            UINT   uMsg,
                                            WPARAM wParam,
                                            LPARAM lParam )
{
  return( ::SendDlgItemMessage(m_hwnd, nItemId, uMsg, wParam, lParam) );
}

//////////////////////////////////////////////////////////////////////////
/**
  This function enables or disables mouse and keyboard input to the specified 
  control. When input is disabled, the control does not receive input such as 
  mouse clicks and key presses. When input is enabled, the control receives 
  all input. 

  @param nIDCtrl
    Specifies the identifier of the control. 
  @param fEnable
    Specifies whether to enable or disable the control.
    If this parameter is TRUE, the control is enabled.
    If the parameter is FALSE, the control is disabled. 

  @return
    If the control was previously disabled, the return value is FALSE. If
    the control was not previously disabled, the return value is TRUE. To
    get extended error information, call GetLastError.
*/
//////////////////////////////////////////////////////////////////////////
inline BOOL CDialogWnd::EnableControl(int nIDCtrl, BOOL fEnable)
{
  HWND hCtrl;
  
  hCtrl = ::GetDlgItem(m_hwnd, nIDCtrl);
  return( ~::EnableWindow(hCtrl, fEnable) );
}

//////////////////////////////////////////////////////////////////////////
/**
//  The function adds a check mark to (checks) a specified radio button
//  in a group and removes a check mark from (clears) all other radio
//  buttons in the group.

  @param nIDFirst
    Specifies the identifier of the first radio button in the group. 
  @param nIDLast
    Specifies the identifier of the last radio button in the group. 
  @param nIDCheck
    Specifies the identifier of the radio button to select. 

  @return
    If the function succeeds, the return value is nonzero. If the function 
    fails, the return value is zero. To get extended error information, 
    call GetLastError. 
*/
//////////////////////////////////////////////////////////////////////////
inline BOOL CDialogWnd::CheckRadioButton( int nIDFirst,
                                          int nIDLast,
                                          int nIDCheck )
{
  return( ::CheckRadioButton(m_hwnd, nIDFirst, nIDLast, nIDCheck) );
}

//////////////////////////////////////////////////////////////////////////
/**
  The function retrieves the ID of the checked button in a group of radio 
  buttons.

  @param nIDFirst
    Specifies the identifier of the first radio button in the group. 
  @param nIDLast
    Specifies the identifier of the last radio button in the group. 

  @return
    If the function succeeds, the return value is the ID of the checked radio 
    button. If the function fails, the return value is -1. To get extended 
    error information, call GetLastError. 
*/
//////////////////////////////////////////////////////////////////////////
inline int CDialogWnd::GetCheckedRadioButton(int nIDFirst, int nIDLast)
{
  int i;

  if (nIDLast < nIDFirst)
  {
    i = nIDFirst;
    nIDFirst = nIDLast;
    nIDLast = i;
  }

  for (i = nIDFirst; i <= nIDLast; i++)
  {
    if (IsButtonChecked(i))
      return(i);
  }

  return( -1 );
}

//////////////////////////////////////////////////////////////////////////
/**
  The function function changes the check state of a button control.

  @param nIDButton
    Specifies the identifier of the button to modify.
  @param uCheck
    Specifies the check state of the button. This parameter can be one of the 
    following values:
    * BST_CHECKED
      Sets the button state to checked.
    * BST_INDETERMINATE
      Sets the button state to grayed, indicating an
      indeterminate state. Use this value only if the
      button has the BS_3STATE or BS_AUTO3STATE style.
    * BST_UNCHECKED
      Sets the button state to cleared 

  @return
  //  If the function succeeds, the return value is nonzero.
  //  If the function fails, the return value is zero. To get
  //  extended error information, call GetLastError. 

  @note
    The function sends a BM_SETCHECK message to the specified button control.
*/
//////////////////////////////////////////////////////////////////////////
inline BOOL CDialogWnd::CheckButton(int nIDButton, UINT uCheck)
{
  LRESULT lResult = ::SendDlgItemMessage( m_hwnd, 
                                          nIDButton, 
                                          BM_SETCHECK, 
                                          uCheck, 
                                          0);
  return (BOOL) lResult;
}

//////////////////////////////////////////////////////////////////////////
/**
  The function adds a check mark to (checks) a specified radio button in a 
  group and removes a check mark from (clears) all other radio buttons in the 
  group.

  @param nIDButton
    Specifies the identifier of the button control.

  @return
    The return value from a button created with the BS_AUTOCHECKBOX,
    BS_AUTORADIOBUTTON, BS_AUTO3STATE, BS_CHECKBOX, BS_RADIOBUTTON,
    or BS_3STATE style can be one of the following. 
    * BST_CHECKED
      Button is checked. 
    * BST_INDETERMINATE
      Button is grayed, indicating an indeterminate state (applies only
      if the button has the BS_3STATE or BS_AUTO3STATE style). 
    * BST_UNCHECKED
      Button is cleared.
    If the button has any other style, the return value is zero.
*/
//////////////////////////////////////////////////////////////////////////
inline UINT CDialogWnd::IsButtonChecked(int nIDButton)
{
  LRESULT lResult = ::SendDlgItemMessage( m_hwnd, 
                                          nIDButton, 
                                          BM_GETCHECK, 
                                          0, 
                                          0);
  return (BOOL) lResult;
}

//////////////////////////////////////////////////////////////////////////
/**
  The function translates the text of a specified control in the dialog box 
  into an integer value.

  @param nIdItem
    Specifies the identifier of the control whose text is to be translated.
  @param pfTrans
    Pointer to a variable that receives a success or failure value (TRUE 
    indicates success, FALSE indicates failure). If this parameter is NULL, 
    the function returns no information about success or failure. 
  @param fSigned
    Specifies whether the function should examine the text for a minus sign at 
    the beginning and return a signed integer value if it finds one ( TRUE 
    specifies this should be done, FALSE that it should not).

  @return
    If the function succeeds, the variable pointed to by pfTrans is set to 
    TRUE, and the return value is the translated value of the control text. 
    If the function fails, the variable pointed to by pfTrans is set to FALSE,
    and the return value is zero. Note that, since zero is a possible translated 
    value, a return value of zero does not by itself indicate failure.
    If pfTrans is NULL, the function returns no information about success or 
    failure. If the fSigned parameter is TRUE, specifying that the value to be 
    retrieved is a signed integer value, cast the return value to an int type. 
    To get extended error information, call GetLastError.

  @note
    The function retrieves the text of the specified control by sending a 
    WM_GETTEXT message to the control. The function translates the retrieved 
    text by stripping any extra spaces at the beginning of the text and then 
    converting the decimal digits. The function stops translating when it 
    reaches the end of the text or encounters a nonnumeric character. The 
    function returns zero if the translated value is greater than INT_MAX 
    (for signed numbers) or UINT_MAX (for unsigned numbers).
*/
//////////////////////////////////////////////////////////////////////////
inline UINT CDialogWnd::GetItemInt(int   nIdItem,
                                   PBOOL pfTrans,
                                   BOOL  fSigned)
{
  return( ::GetDlgItemInt(m_hwnd, nIdItem, pfTrans, fSigned) );
}

//////////////////////////////////////////////////////////////////////////
/**
//  The function sets the text of a control in the dialog box to the
//  string representation of a specified integer value.

  @param nIdItem
    Specifies the control to be changed. 
  @param uValue
    Specifies the integer value used to generate the item text.
  @param fSigned
    Specifies whether the uValue parameter is signed or unsigned. If this 
    parameter is TRUE, uValue is signed. If this parameter is TRUE and uValue 
    is less than zero, a minus sign is placed before the first digit in the
    string. If this parameter is FALSE, uValue is unsigned. 

  @return
    If the function succeeds, the return value is nonzero. If the function 
    fails, the return value is zero. To get extended error information, call 
    GetLastError.

  @note
    To set the new text, this function sends a WM_SETTEXT message to the 
    specified control.
*/
//////////////////////////////////////////////////////////////////////////
inline BOOL CDialogWnd::SetItemInt(int nIdItem, UINT uValue, BOOL fSigned)
{
  return( ::SetDlgItemInt(m_hwnd, nIdItem, uValue, fSigned) );
}

//////////////////////////////////////////////////////////////////////////
/**
  The function  gets a text control in a dialog box. 

  @param nIdItem
    Specifies the identifier of the control whose title or text is to be 
    retrieved.
  @param pString
    Pointer to the buffer to receive the title or text. 
  @param nCount
    Specifies the maximum length, in TCHARs, of the string to be copied to the 
    buffer pointed to by pString. If the length of the string exceeds the limit, 
    the string is truncated. 

  @return
    If the function succeeds, the return value specifies the number of TCHARs 
    copied to the buffer, not including the terminating null character. If the 
    function fails, the return value is zero. To get extended error information, 
    call GetLastError.

  @note
    To set the new text, this function sends a WM_SETTEXT message to the 
    specified control.
*/
//////////////////////////////////////////////////////////////////////////
inline UINT CDialogWnd::GetItemText(int    nIdItem,
                                    LPTSTR pString,
                                    int    nCount)
{
  return( ::GetDlgItemText(m_hwnd, nIdItem, pString, nCount) );
}

//////////////////////////////////////////////////////////////////////////
/**
  The function sets the title or text of a control in the dialog box.

  @param nIdItem
    Specifies the control with a title or text to be set.
  @param pString
    Pointer to the null-terminated string that contains the text to be copied 
    to the control.

  @return
    If the function succeeds, the return value is nonzero. If the function 
    fails, the return value is zero. To get extended error information, 
    call GetLastError.

  @note
    To set the new text, this function sends a WM_SETTEXT message to the 
    specified control.
*/
//////////////////////////////////////////////////////////////////////////
inline BOOL CDialogWnd::SetItemText(int nIdItem, LPCTSTR pString)
{
  return( ::SetDlgItemText(m_hwnd, nIdItem, pString) );
}

#endif // __cplusplus

#endif //_DIALOG_HPP_

