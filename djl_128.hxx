#pragma once

// taken from various places on stackoverflow

#include <inttypes.h>

class CMultiply128
{
    private:
        static uint64_t umul_32_32( uint32_t x, uint32_t y ) { return ( (uint64_t) x ) * y; }
    
    public:
        static void mul_u64_u64( uint64_t *rh, uint64_t *rl, uint64_t x, uint64_t y )
        {
            uint32_t xlo = (uint32_t) x;
            uint32_t xhi = (uint32_t) ( x >> 32 );
            uint32_t ylo = (uint32_t) y;
            uint32_t yhi = (uint32_t) ( y >> 32 );
        
            uint64_t m0 = umul_32_32( xlo, ylo );
            uint64_t m1 = umul_32_32( xhi, ylo );
            uint64_t m2 = umul_32_32( xlo, yhi );
            uint64_t m3 = umul_32_32( xhi, yhi );
            m1 += ( m0 >> 32 );
            m3 += ( m2 >> 32 );
            m1 += ( m2 & UINT32_MAX );
        
            *rh = m3 + ( m1 >> 32 );
            *rl = ( m1 << 32 ) | ( m0 & UINT32_MAX );
        } //mul_u64_u64

        static void mul_s64_s64( int64_t *rh, int64_t *rl, int64_t x, int64_t y )
        {
            mul_u64_u64( (uint64_t *) rh, (uint64_t *) rl, (uint64_t) x, (uint64_t) y );

            if ( x < 0 )
                *rh -= y;
            if ( y < 0 )
                *rh -= x;
        } //mul_s64_s64

        static uint64_t mul_u64_u64( uint64_t x, uint64_t y, uint64_t *rh )
        {
            uint64_t rl;
            mul_u64_u64( rh, &rl, x, y );
            return rl;
        } //mul_u64_u64

        static int64_t mul_s64_s64( int64_t x, int64_t y, int64_t *rh )
        {
            int64_t rl;
            mul_s64_s64( rh, &rl, x, y );
            return rl;
        } //mul_s64_s64
};
