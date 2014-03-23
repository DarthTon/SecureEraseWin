#include "MainWnd.h"
#include "PhysicalDisk.h"
#include "Utils.hpp"

#include <sstream>
#include <iomanip>

DWORD MainDlg::RefreshDiskList()
{
    std::vector<PhysicalDisk*> disks;
    PhysicalDisk::EnumDisks( disks );

    HWND hList = GetDlgItem( _hMainDlg, IDC_LIST_DISKS );

    ListView_DeleteAllItems( hList );

    for (auto& disk : disks)
    {
        auto& id_data = disk->id_data();
        LVITEMW lvi = { 0 };
        int idx = 0;
        
        lvi.mask = LVIF_TEXT | LVIF_PARAM;

        lvi.pszText = (LPWSTR)disk->path().c_str();
        lvi.cchTextMax = (int)disk->path().length() + 1;
        lvi.lParam = reinterpret_cast<LPARAM>(disk);

        int pos = ListView_InsertItem( hList, &lvi );

        LVITEMA lva = { 0 };
        for (idx = sizeof(id_data.ModelNumber) - 3; idx >= 0; idx--)
            if (id_data.ModelNumber[idx] != ' ')
                break;

        std::string tmpstr( (LPSTR)id_data.ModelNumber, (LPSTR)id_data.ModelNumber + idx );

        lva.iSubItem = 1;
        lva.pszText = disk->identified() ? (LPSTR)tmpstr.c_str() : "Unknown";

        SendMessage( hList, LVM_SETITEMTEXTA, pos, (LPARAM)&lva );

        for (idx = 0; idx < sizeof(id_data.SerialNumber); idx++)
            if (id_data.SerialNumber[idx] != ' ')
                break;

        tmpstr = std::string( (LPSTR)(id_data.SerialNumber + idx), (LPSTR)(id_data.SerialNumber + sizeof(id_data.SerialNumber)) );

        lva.iSubItem = 2;
        lva.pszText = disk->identified() ? (LPSTR)tmpstr.c_str() : "Unknown";

        SendMessage( hList, LVM_SETITEMTEXTA, pos, (LPARAM)&lva );
    }

    return ERROR_SUCCESS;
}


DWORD MainDlg::PrintDiskInfo( PhysicalDisk* pDisk )
{
    std::stringstream sstream;
    auto& id_data = pDisk->id_data();
    auto& security = id_data.SecurityStatus;

    if (!pDisk->identified())
    {
        sstream << "IDENTIFY command failed\r\n";
    }
    else
    {
        sstream << id_data.ModelNumber << "\r\n\r\n";

        sstream << "Capacity: " 
                << pDisk->geometry() / (1024 * 1024 * 1024) << "GB\r\n\r\n";

        sstream << "--------Features--------\r\n\r\n";

        sstream << std::setw( 25 ) 
                << std::left << "Device reset: " 
                << Utils::BoolToString( id_data.CommandSetSupport.DeviceReset ) << "\r\n";

        sstream << std::setw( 25 ) 
                << std::left << "Sanitize: " 
                << Utils::BoolToString( id_data.SanitizeFeatureSupported ) << "\r\n";

        sstream << "\r\n";

        sstream << "--------Security Status--------\r\n\r\n";

        sstream << std::setw( 25 ) 
                << std::left 
                << "Supported: " << Utils::BoolToString( security.SecuritySupported ) << "\r\n";

        sstream << std::setw( 25 ) 
                << std::left << "Enabled: " 
                << Utils::BoolToString( security.SecurityEnabled ) << "\r\n";

        sstream << std::setw( 25 ) 
                << std::left << "Locked: " 
                << Utils::BoolToString( security.SecurityLocked ) << "\r\n";

        sstream << std::setw( 25 ) 
                << std::left << "Frozen: " 
                << Utils::BoolToString( security.SecurityFrozen ) << "\r\n";;

        sstream << std::setw( 25 ) 
                << std::left << "Enhanced Erase: " 
                << Utils::BoolToString( security.EnhancedSecurityEraseSupported ) << "\r\n";

        sstream << std::setw( 25 ) 
                << std::left << "SecurityLevel: " 
                << Utils::BoolToString( security.SecurityLevel ) << "\r\n\r\n\r\n";

        sstream << "--------Erase Timings--------\r\n\r\n";

        sstream << std::setw( 25 ) 
                << std::left << "Normal erase time: " 
                << id_data.NormalSecurityEraseUnit.TimeRequired << " min \r\n";

        sstream << std::setw( 25 ) 
                << std::left 
                << "Enhanced erase time: ";

        if (security.EnhancedSecurityEraseSupported)
            sstream << id_data.EnhancedSecurityEraseUnit.TimeRequired << " min";
        else
            sstream << "Not supported";

        sstream << "\r\n";
    }

    SetDlgItemTextA( _hMainDlg, IDC_DISK_INFO, sstream.str().c_str() );
    
    return ERROR_SUCCESS;
}