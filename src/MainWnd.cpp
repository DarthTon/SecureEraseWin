#include "Headers.h"
#include "MainWnd.h"

MainDlg::mapMsgProc MainDlg::Messages;


MainDlg::MainDlg()
    : _hMainDlg( 0 )
{
}

MainDlg::~MainDlg()
{
}

MainDlg& MainDlg::Instance() 
{
    static MainDlg ms_Instanse;
    return ms_Instanse;
}

INT_PTR MainDlg::Run()
{
    Messages[WM_INITDIALOG] = &MainDlg::OnInit;
    Messages[WM_COMMAND]    = &MainDlg::OnCommand;
    Messages[WM_CLOSE]      = &MainDlg::OnClose;
    Messages[WM_NOTIFY]     = &MainDlg::OnNotify;

    Events[IDC_SET_PASS]    = &MainDlg::OnSetSecurity;
    Events[IDC_RESET_PASS]  = &MainDlg::OnSetSecurity;
    Events[IDC_ERASE_N]     = &MainDlg::OnSetSecurity;
    Events[IDC_ERASE_S]     = &MainDlg::OnSetSecurity;

    Notifications[LVN_ITEMACTIVATE] = &MainDlg::OnSelChange;

    // Richedit support
    LoadLibraryW( L"Riched20.dll" );

    return DialogBox( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDD_MAIN ), NULL, &MainDlg::DlgProcMain );
}

INT_PTR CALLBACK MainDlg::DlgProcMain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (Messages.find( message ) != Messages.end())
        return (Instance().*Messages[message])( hDlg, message, wParam, lParam );

    return 0;
}


