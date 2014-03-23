#include "PhysicalDisk.h"
#include "Utils.hpp"

#include <Setupapi.h>
#include <Ntddstor.h>
#include <Ntddscsi.h>

#pragma comment( lib, "setupapi.lib" )

std::vector<PhysicalDisk*> PhysicalDisk::_disks;
PhysicalDisk* PhysicalDisk::g_currentDisk = nullptr;

PhysicalDisk::PhysicalDisk( const std::wstring& path )
    : _path( path )
{
    _hDisk = CreateFileW( path.c_str( ), FILE_GENERIC_READ | GENERIC_WRITE, 
                          FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
                          OPEN_EXISTING, 0, NULL );

    GetDiskIdentifyData();
}

PhysicalDisk::~PhysicalDisk()
{
    if (_hDisk != INVALID_HANDLE_VALUE)
        CloseHandle( _hDisk );
}

/// <summary>
/// Send IDENTIFY_DEVICE ATA request to disk
/// </summary>
/// <param name="hDisk">The h disk.</param>
/// <param name="data">The data.</param>
/// <returns>Error code</returns>
DWORD PhysicalDisk::GetDiskIdentifyData( )
{
    char buffer[512 + sizeof(ATA_PASS_THROUGH_EX)] = { 0 };
    ATA_PASS_THROUGH_EX& PTE = *(ATA_PASS_THROUGH_EX *)buffer;
    IDEREGS * ir = (IDEREGS*)PTE.CurrentTaskFile;
    DWORD bytes = 0;

    PTE.AtaFlags = ATA_FLAGS_DATA_IN | ATA_FLAGS_DRDY_REQUIRED;
    PTE.Length = sizeof(PTE);
    PTE.DataTransferLength = 512;
    PTE.TimeOutValue = 10;
    PTE.DataBufferOffset = sizeof(PTE);

    ir->bCommandReg = IDE_COMMAND_IDENTIFY;

    ZeroMemory( &_diskInfo, sizeof(_diskInfo) );

    BOOL rs = DeviceIoControl( _hDisk, IOCTL_ATA_PASS_THROUGH, &PTE, sizeof(PTE), buffer, sizeof(buffer), &bytes, NULL );

    //
    // Even after successful data transfer some controllers still return error in ATA Error register
    // because of previous command error.
    // Need to handle this somehow...
    //
    if (rs == TRUE /*&& !(ir->bCommandReg & IDE_STATUS_ERROR)*/)
    {
        _identified = true;

        _diskInfo = *(PIDENTIFY_DEVICE_DATA)(buffer + sizeof(PTE));

        // Remove invalid characters
        _diskInfo.ModelNumber[sizeof(_diskInfo.ModelNumber) - 1] = 0;
        _diskInfo.ModelNumber[sizeof(_diskInfo.ModelNumber) - 2] = 0;

        // Normalize some strings
        Utils::SwapEndianness( &_diskInfo.ModelNumber, sizeof(_diskInfo.ModelNumber) - 2 );
        Utils::SwapEndianness( &_diskInfo.SerialNumber, sizeof(_diskInfo.SerialNumber) );
        Utils::SwapEndianness( &_diskInfo.FirmwareRevision, sizeof(_diskInfo.FirmwareRevision) );
    }

    return GetLastError();
}

/// <summary>
/// Turn on disk security
/// </summary>
/// <param name="password">Security password.</param>
/// <returns>Error code</returns>
DWORD PhysicalDisk::EnableSecurity( const std::string& password )
{
    // Unsupported
    if (!_diskInfo.SecurityStatus.SecuritySupported)
        return ERROR_NOT_SUPPORTED;

    // Already enabled
    else if (_diskInfo.SecurityStatus.SecurityEnabled)
        return ERROR_ALREADY_EXISTS;

    char buffer[512 + sizeof(ATA_PASS_THROUGH_EX)] = { 0 };
    ATA_PASS_THROUGH_EX& PTE = *(ATA_PASS_THROUGH_EX *)buffer;
    IDEREGS* ir = (IDEREGS*)PTE.CurrentTaskFile;
    DEVICE_SET_PASSWORD* pPass = (DEVICE_SET_PASSWORD*)(buffer + sizeof(PTE));
    DWORD bytes = 0;

    PTE.AtaFlags = ATA_FLAGS_DATA_OUT | ATA_FLAGS_DRDY_REQUIRED;
    PTE.Length = sizeof(PTE);
    PTE.DataTransferLength = 512;
    PTE.TimeOutValue = 10;
    PTE.DataBufferOffset = sizeof(PTE);

    ir->bCommandReg = IDE_COMMAND_SECURITY_SET_PASSWORD;

    //
    // Setup password structure
    //
    USHORT revcode = _diskInfo.MasterPasswordID;
    if (revcode == 0xfffe)
        revcode = 0;
    revcode++;

    pPass->ControlWord.PasswordIdentifier = ATA_DEVICE_SET_PASSWORD_USER;
    pPass->ControlWord.MasterPasswordCapability = 0;    // High
    pPass->MasterPasswordIdentifier = revcode;

    strcpy_s( (PCHAR)pPass->Password, sizeof(pPass->Password), password.c_str() );

    BOOL rs = DeviceIoControl( _hDisk, IOCTL_ATA_PASS_THROUGH, &PTE, sizeof(PTE), buffer, sizeof(buffer), &bytes, NULL );
    if (rs == TRUE)
    {
    }

    return ERROR_SUCCESS;
}

/// <summary>
/// Perform disk software reset
/// </summary>
/// <returns>Error code</returns>
DWORD PhysicalDisk::Reset()
{
    DWORD err = SendATACommand( IDE_COMMAND_SLEEP );
    if (err)
        return err;

    return SendATACommand( IDE_COMMAND_ATAPI_RESET );
}

/// <summary>
/// Put device into sleep power state
/// </summary>
/// <returns>Error code</returns>
DWORD PhysicalDisk::Sleep()
{
    return SendATACommand( IDE_COMMAND_SLEEP );
}

DWORD PhysicalDisk::SendATACommand( BYTE cmd )
{
    char buffer[sizeof(ATA_PASS_THROUGH_EX)] = { 0 };
    ATA_PASS_THROUGH_EX& PTE = *(ATA_PASS_THROUGH_EX *)buffer;
    IDEREGS* ir = (IDEREGS*)PTE.CurrentTaskFile;
    REG_STATUS* eir = (REG_STATUS*)&ir->bCommandReg;
    DWORD bytes = 0;

    // TODO: implement flags
    PTE.AtaFlags = 0;
    PTE.Length = sizeof(PTE);
    PTE.TimeOutValue = 10;

    ir->bCommandReg = cmd;

    BOOL rs = DeviceIoControl( _hDisk, IOCTL_ATA_PASS_THROUGH, &PTE, sizeof(PTE), buffer, sizeof(buffer), &bytes, NULL );
    
    if (rs == FALSE)
        return GetLastError();

    if (eir->ERR)
        return ERROR_FUNCTION_FAILED;

    return ERROR_SUCCESS;
}

/// <summary>
/// Get disk geometry
/// </summary>
/// <param name="pdg">Geometry.</param>
/// <returns>Total disk capacity in bytes</returns>
uint64_t PhysicalDisk::geometry( DISK_GEOMETRY *pdg /*= nullptr*/) const
{
    DISK_GEOMETRY dg = { 0 }; 
    if (pdg == nullptr)
        pdg = &dg;

    DWORD junk = 0;
    BOOL res = DeviceIoControl( _hDisk, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, pdg, sizeof(*pdg), &junk, NULL );

    return pdg->Cylinders.QuadPart * pdg->TracksPerCylinder *pdg->SectorsPerTrack * pdg->BytesPerSector;
}



/// <summary>
/// Get physical disks
/// </summary>
/// <param name="disks">Found disks</param>
/// <returns>Error code</returns>
DWORD PhysicalDisk::EnumDisks( std::vector<PhysicalDisk*>& disks )
{
    g_currentDisk = nullptr;

    for (auto pDisk : _disks)
        delete pDisk;

    _disks.clear();

    std::vector<std::wstring> drives;
    if (GetPhysicalPaths( drives ) == ERROR_SUCCESS)
    {
        for (auto& path: drives)
            _disks.emplace_back( new PhysicalDisk( path ) );

        disks = _disks;
    }

    return GetLastError();
}


/// <summary>
/// Get physical device paths.
/// </summary>
/// <param name="drives">Found drives</param>
/// <returns>Error code</returns>
DWORD PhysicalDisk::GetPhysicalPaths( std::vector<std::wstring>& drives )
{
    HDEVINFO diskClassDevices = nullptr;
    GUID diskClassDeviceInterfaceGuid = GUID_DEVINTERFACE_DISK;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData = { 0 };
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData = nullptr;
    DWORD requiredSize = 0;
    DWORD deviceIndex = 0;

    HANDLE disk = INVALID_HANDLE_VALUE;
    STORAGE_DEVICE_NUMBER diskNumber = { 0 };
    DWORD bytesReturned = 0;

    //
    // Get the handle to the device information set for installed
    // disk class devices. Returns only devices that are currently
    // present in the system and have an enabled disk device
    // interface.
    //
    diskClassDevices = SetupDiGetClassDevs( &diskClassDeviceInterfaceGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE );
    if (diskClassDevices == INVALID_HANDLE_VALUE)
        return GetLastError();

    ZeroMemory( &deviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA) );
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (; SetupDiEnumDeviceInterfaces( diskClassDevices, NULL, &diskClassDeviceInterfaceGuid, deviceIndex, &deviceInterfaceData ); ++deviceIndex)
    {
        SetupDiGetDeviceInterfaceDetailW( diskClassDevices, &deviceInterfaceData, NULL, 0, &requiredSize, NULL );
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            goto Exit;

        deviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc( requiredSize );

        ZeroMemory( deviceInterfaceDetailData, requiredSize );
        deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (!SetupDiGetDeviceInterfaceDetail( diskClassDevices, &deviceInterfaceData, deviceInterfaceDetailData, requiredSize, NULL, NULL ))
            goto Exit;

        disk = CreateFile( deviceInterfaceDetailData->DevicePath,
                           GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

        if (disk == INVALID_HANDLE_VALUE)
            goto Exit;

        if (!DeviceIoControl( disk, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &diskNumber, sizeof(STORAGE_DEVICE_NUMBER), &bytesReturned, NULL ))
            goto Exit;

        CloseHandle( disk );
        disk = INVALID_HANDLE_VALUE;

        drives.emplace_back( L"\\\\?\\PhysicalDrive" + std::to_wstring( diskNumber.DeviceNumber ) );

        if (deviceInterfaceDetailData)
        {
            free( deviceInterfaceDetailData );
            deviceInterfaceDetailData = nullptr;
        }
    }

Exit:

    if (INVALID_HANDLE_VALUE != diskClassDevices)
        SetupDiDestroyDeviceInfoList( diskClassDevices );

    if (INVALID_HANDLE_VALUE != disk)
        CloseHandle( disk );

    if (deviceInterfaceDetailData)
        free( deviceInterfaceDetailData );

    return GetLastError();
}