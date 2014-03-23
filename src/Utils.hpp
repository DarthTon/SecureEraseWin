#pragma once

#include "Headers.h"

class Utils
{
public:
    static void SwapEndianness( void* ptr, size_t size )
    {
        struct u16
        {
            uint8_t high;
            uint8_t low;
        };

        for (u16* pStruct = (u16*)ptr; pStruct < (u16*)ptr + size / 2; pStruct++)
            std::swap( pStruct->low, pStruct->high );
    }

    static inline std::string BoolToString( uint8_t val )
    {
        return (val == 0) ? "No" : "Yes";
    }
};

