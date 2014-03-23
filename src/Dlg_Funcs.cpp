#include "MainWnd.h"
#include "PhysicalDisk.h"

#include <shellapi.h>
#include <Richedit.h>

INT_PTR MainDlg::OnInit( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    _hMainDlg = hDlg;
    LVCOLUMNW lvc = { 0 };

    HWND hList = GetDlgItem( hDlg, IDC_LIST_DISKS );

    ListView_SetExtendedListViewStyle( hList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );

    //
    // Insert columns
    //
    lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.pszText = L"Path";
    lvc.cx = 150;

    ListView_InsertColumn( hList, 0, &lvc );

    lvc.pszText = L"Model";
    lvc.iSubItem = 1;
    lvc.cx = 200;

    ListView_InsertColumn( hList, 1, &lvc );

    lvc.pszText = L"Serial";
    lvc.iSubItem = 2;
    lvc.cx = 200;

    ListView_InsertColumn( hList, 2, &lvc );

    RefreshDiskList();

    // Rich edit font
    CHARFORMAT2W cformat;
    cformat.dwMask = CFM_FACE;
    cformat.cbSize = sizeof(cformat);
    wcscpy_s( cformat.szFaceName, ARRAYSIZE(cformat.szFaceName), L"Courier" );

    SendDlgItemMessageW( _hMainDlg, IDC_DISK_INFO, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cformat );

    return TRUE;
}

INT_PTR MainDlg::OnCommand( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    if (Events.count( LOWORD( wParam ) ))
        return (this->*Events[LOWORD( wParam )])(hDlg, message, wParam, lParam);

    if (Events.count( HIWORD( wParam ) ))
        return (this->*Events[HIWORD( wParam )])(hDlg, message, wParam, lParam);

    return FALSE;
}

INT_PTR MainDlg::OnNotify( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    UINT nmsg = ((LPNMHDR)lParam)->code;

    if (Notifications.count( nmsg ))
        return (this->*Notifications[nmsg])(hDlg, nmsg, wParam, lParam);

    return FALSE;
}

INT_PTR MainDlg::OnClose( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    EndDialog( hDlg, 0 );
    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////

INT_PTR MainDlg::OnFileExit( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    EndDialog( hDlg, 0 );
    return TRUE;
}

INT_PTR MainDlg::OnSelChange( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    LPNMITEMACTIVATE lpnmia = (LPNMITEMACTIVATE)lParam;
    LVITEMW lvi = { 0 };

    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem = lpnmia->iItem;

    ListView_GetItem( GetDlgItem( _hMainDlg, IDC_LIST_DISKS ), &lvi );

    PhysicalDisk::g_currentDisk = (PhysicalDisk*)lvi.lParam;
    PrintDiskInfo( PhysicalDisk::g_currentDisk );  

    return TRUE;
}


INT_PTR MainDlg::OnSetSecurity( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    if (PhysicalDisk::g_currentDisk == nullptr)
        MessageBoxW( hDlg, L"No disk selected", L"Error", MB_ICONERROR );
    else if (PhysicalDisk::g_currentDisk->identified() && PhysicalDisk::g_currentDisk->id_data().SecurityStatus.SecurityFrozen)
        MessageBoxW( hDlg, L"Not available while disk security is frozen", L"Error", MB_ICONERROR );
    else if (PhysicalDisk::g_currentDisk->identified() && !PhysicalDisk::g_currentDisk->id_data().SecurityStatus.SecuritySupported)
        MessageBoxW( hDlg, L"Operation not supported by current disk", L"Error", MB_ICONERROR );
    else
        MessageBoxW( hDlg, L"Function not implemented", L"Error", MB_ICONERROR );

    return TRUE;
}