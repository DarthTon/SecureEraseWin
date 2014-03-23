#pragma once

#include "Headers.h"
#include "ATAApi.h"

class PhysicalDisk
{
public:
    PhysicalDisk( const std::wstring& path );
    ~PhysicalDisk();

    inline const std::wstring&          path()       const { return _path;       }
    inline HANDLE                       handle()     const { return _hDisk;      }
    inline const IDENTIFY_DEVICE_DATA&  id_data()    const { return _diskInfo;   }
    inline const bool                   identified() const { return _identified; }

    /// <summary>
    /// Get disk geometry
    /// </summary>
    /// <param name="pdg">Geometry.</param>
    /// <returns>Total disk capacity in bytes</returns>
    uint64_t geometry( DISK_GEOMETRY *pdg = nullptr ) const;

    /// <summary>
    /// Turn on disk security
    /// </summary>
    /// <param name="password">Security password.</param>
    /// <returns>Error code</returns>
    DWORD EnableSecurity( const std::string& password );

    /// <summary>
    /// Perform disk software reset
    /// </summary>
    /// <returns>Error code</returns>
    DWORD Reset();

    /// <summary>
    /// Put device into sleep power state
    /// </summary>
    /// <returns>Error code</returns>
    DWORD Sleep();


    /// <summary>
    /// Get physical disks
    /// </summary>
    /// <param name="disks">Found disks</param>
    /// <returns>Error code</returns>
    static DWORD EnumDisks( std::vector<PhysicalDisk*>& disks );
    static std::vector<PhysicalDisk*>& disks() { return _disks; }

public:
    static PhysicalDisk* g_currentDisk;

private:
    DWORD SendATACommand( BYTE cmd );

    /// <summary>
    /// Get physical device paths.
    /// </summary>
    /// <param name="drives">Found drives</param>
    /// <returns>Error code</returns>
    static DWORD GetPhysicalPaths( std::vector<std::wstring>& drives );

    /// <summary>
    /// Send IDENTIFY_DEVICE ATA request to disk
    /// </summary>
    /// <param name="hDisk">The h disk.</param>
    /// <param name="data">The data.</param>
    /// <returns>Error code</returns>
    DWORD GetDiskIdentifyData();
private:
    HANDLE _hDisk = INVALID_HANDLE_VALUE;
    std::wstring _path;
    IDENTIFY_DEVICE_DATA _diskInfo;
    bool _identified = false;

    static std::vector<PhysicalDisk*> _disks;
};


