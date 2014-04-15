//////////////////////////////////////////////////////////////////////////
// IXXAT Automation GmbH
//////////////////////////////////////////////////////////////////////////
/**
  Declarations for the socket select dialog class.

  @file "select.hpp"
*/
//////////////////////////////////////////////////////////////////////////
// (C) 2002-2011 IXXAT Automation GmbH, all rights reserved
//////////////////////////////////////////////////////////////////////////

#ifndef SELECT_HPP
#define SELECT_HPP

//////////////////////////////////////////////////////////////////////////
//  include files
//////////////////////////////////////////////////////////////////////////
#include <IXXAT/vcinpl.h>
#include "dialog.hpp"

#include "cancon.rh"

//////////////////////////////////////////////////////////////////////////
//  constants and macros
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// data types
//////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
//////////////////////////////////////////////////////////////////////////
/**
  This class implements a dialog window to select a bus socket.
*/
//////////////////////////////////////////////////////////////////////////
class CSocketSelectDlg : public CDialogWnd
{
  public:
    //---------------------------------------------------------------
    // public methods
    //---------------------------------------------------------------
    HRESULT RunModal( HWND    hWndParent, UINT8 bBusType,
                      PHANDLE phDevice,   PLONG plCtrlNo );

    //---------------------------------------------------------------
    // constructor / destructor
    //---------------------------------------------------------------
    CSocketSelectDlg();
   ~CSocketSelectDlg();

  protected:
    //---------------------------------------------------------------
    // required by CDialogWnd implementation
    //---------------------------------------------------------------
    virtual BOOL CmdProc(WORD wId, WORD wCode, HWND hCtrl);
    virtual BOOL DlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

  private:
    //---------------------------------------------------------------
    // Handler for dialog messages
    //---------------------------------------------------------------
    BOOL OnWmInitdialog(WPARAM wParam, LPARAM lParam);
    BOOL OnWmDestroy   (WPARAM wParam, LPARAM lParam);
    BOOL OnWmUpdate    (WPARAM wParam, LPARAM lParam);

    //---------------------------------------------------------------
    // Handler for control messages
    //---------------------------------------------------------------
    BOOL OnBtnOk     (WORD wCode, HWND hCtrl);
    BOOL OnBtnCancel (WORD wCode, HWND hCtrl);
    BOOL OnLbxDevList(WORD wCode, HWND hCtrl);
    BOOL OnCbxSocList(WORD wCode, HWND hCtrl);

    //---------------------------------------------------------------
    // device list support functions
    //---------------------------------------------------------------
    void DeviceListClear ( void );
    void DeviceListUpdate( void );
    void DeviceListSelect( void );

    LRESULT DeviceListSearch( VCIID vciid );
    LRESULT DeviceListInsert( VCIID vciid );

    //---------------------------------------------------------------
    // socket list support functions
    //---------------------------------------------------------------
    void SocketListUpdate( void );
    void SocketListSelect( void );

    //---------------------------------------------------------------
    // utility functions
    //---------------------------------------------------------------
    void UpdateView( HANDLE hDevice );

    //---------------------------------------------------------------
    // device list monitor functions
    //---------------------------------------------------------------
    HRESULT ThreadStart( void );
    HRESULT ThreadStop ( BOOL fWaitFor );

    //---------------------------------------------------------------
    // data members
    //---------------------------------------------------------------
    HANDLE m_hThread; // handle to change monitor thread
    DWORD  m_dwThdId; // id to change monitor thread
    HANDLE m_hDevEnu; // handle to the device enumerator
    UINT8  m_BusType; // type of the listed bus sockets
    HANDLE m_hCurDev; // handle of the currently selected device

    HANDLE m_hSelDev; // handle of the selected device
    VCIID  m_nSelDev; // VCIID of the selected device
    LONG   m_lCtrlNo; // number of the selected bus controller

    static DWORD WINAPI ThreadProc( CSocketSelectDlg* pDialog );
};

#endif // __cplusplus


EXTERN_C HRESULT SocketSelectDlg( IN  HWND    hWndParent,
                                  IN  UINT8   bBusType,
                                  OUT PHANDLE phDevice,
                                  OUT PLONG   plCtrlNo );

#endif //_SOCDLG_HPP_
