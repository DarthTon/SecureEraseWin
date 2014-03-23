#pragma once

#include "Headers.h"
#include "resource.h"

class MainDlg
{
    typedef INT_PTR ( MainDlg::*PDLGPROC)(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    typedef std::map<UINT, PDLGPROC> mapMsgProc;
    typedef std::map<UINT, PDLGPROC> mapCtrlProc;

public:
	
	~MainDlg();

    static MainDlg& Instance();

    INT_PTR Run();

	static INT_PTR CALLBACK DlgProcMain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

private:
    MainDlg();
    MainDlg( MainDlg& root );
    MainDlg& operator=(MainDlg&);

private:
    DWORD RefreshDiskList();
    DWORD PrintDiskInfo( class PhysicalDisk* pDisk );

//////////////////////////////////////////////////////////////////////////////////
    INT_PTR OnInit(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);      //
    INT_PTR OnCommand(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);   //
    INT_PTR OnNotify( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
    INT_PTR OnClose(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);     //
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
    INT_PTR OnFileExit(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);  //
    INT_PTR OnSelChange(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR OnSetSecurity( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
    //////////////////////////////////////////////////////////////////////////////

private:
	HWND                _hMainDlg;
    static mapMsgProc   Messages;
    mapCtrlProc         Events;
    mapCtrlProc         Notifications;
};