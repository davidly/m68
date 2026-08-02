/*
    This file provides the layer Aztec C calls to do OS-specific work.
    Version of Aztec C: C68k/ROM v5.2a, for PCDOS/MSDOS Host Systems 

    This file implements: open lseek creat rename close unlink read ioctl write isatty

    It is very much tuned to CP/M 68K running on a 32-bit 68000.
*/

#include <stdio.h>
#include <stat.h>
#include <errno.h>
#include <math.h>
#include <fcntl.h>
#include <ctype.h>
#include <assert.h>

extern long bdos_cpm();
extern char * strchr();
extern long defdma; /* the default dma address for a cp/m app, set in cpm.asm at app startup */

typedef unsigned long int uint32_t;
typedef unsigned short int uint16_t;
typedef unsigned char uint8_t;
typedef long int int32_t;
typedef short int int16_t;
typedef char int8_t;
typedef int8_t bool;

#define true 1
#define false 0

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define _countof( X ) ( sizeof( X ) / sizeof( X[0] ) )

#define get_min( a, b ) ( ( a < b ) ? a : b )

struct FCBCPM68K
{
    uint8_t dr; 
    char n[ 8 ];
    char t[ 3 ];
    uint8_t ex; 
    uint8_t s1;
    uint8_t s2; 
    uint8_t rc; 
    uint8_t d[ 16 ];
    uint8_t cr;   
    uint8_t r0;  /* r0 and r1 are a 16-bit count of 128 byte records in CP/M 2.2. For CP/M 68K, reverse the byte ordering and add r2 */
    uint8_t r1;   
    uint8_t r2;
};

static struct FCBCPM68K g_afcb[ 20 ];
static uint32_t g_aoffsets[ 20 ];

long GetRandomIOOffset( struct FCBCPM68K * pfcb )
{
    return ( (uint32_t) pfcb->r0 << 16 ) | ( (uint32_t) pfcb->r1 << 8 ) | pfcb->r2;
} /*GetRandomIOOffset*/

void SetRandomIOOffset( struct FCBCPM68K * pfcb, uint32_t o )
{
    pfcb->r2 = ( 0xff & o );
    pfcb->r1 = ( 0xff & ( o >> 8 ) );
    pfcb->r0 = ( 0xff & ( o >> 16 ) );
} /*SetRandomIOOffset*/

void set_filename( struct FCBCPM68K * pfcb, const char * p )
{
    char * dot;
    memset( pfcb->n, ' ', 8 );
    memset( pfcb->t, ' ', 3 );
    dot = strchr( p, '.' );
    if ( dot )
    {
        memcpy( pfcb->n, p, (int) ( dot - p ) );
        memcpy( pfcb->t, dot + 1, strlen( dot + 1 ) );
    }
    else
        memcpy( pfcb->n, p, strlen( p ) );
} /*set_filename*/

void make_filename( struct FCBCPM68K * pfcb, char * p )
{
    int i;

    for ( i = 0; i < 8; i++ )
    {
        if ( ' ' == ( 0x7f & pfcb->n[ i ] ) )
            break;
        *p++ = ( 0x7f & pfcb->n[ i ] );
    }
    if ( ' ' != pfcb->t[ 0 ] )
    {
        *p++ = '.';
        for ( i = 0; i < 3; i++ )
        {
            if ( ' ' == ( 0x7f & pfcb->t[ i ] ) )
                break;
            *p++ = ( 0x7f & pfcb->t[ i ] );
        }
    }
    *p++ = 0;
} /*make_filename*/

bool valid_cpm_filename( const char * pc )
{
    int i, len, invlen;
    char * pcInvalid, * pcdot;
    pcInvalid = "<>,;:=?[]%|()/\\ \t\n\r";

    if ( !strcmp( pc, "." ) )
        return false;

    if ( !strcmp( pc, ".." ) )
        return false;

    invlen = strlen( pcInvalid );    
    for ( i = 0; i < invlen; i++ )
        if ( strchr( pc, pcInvalid[ i ] ) )
            return false;

    len = strlen( pc );

    if ( len > 12 )
        return false;

    pcdot = strchr( pc, '.' );

    if ( !pcdot && ( len > 8 ) )
        return false;

    if ( pcdot && ( ( pcdot - pc ) > 8 ) )
        return false;

    if ( pcdot )
    {
        if ( strchr( pcdot + 1, '.' ) )
            return false;
        if ( strlen( pcdot + 1 ) > 3 )
            return false;
    }

    return true;
} /*valid_cpm_filename*/

int open( const char * uname, int mode, ... )
{
    int fd;
    long result;
    struct FCBCPM68K * pfcb;
    bool create;

    if ( !valid_cpm_filename( uname ) )
    {
        errno = EINVAL;
        return -1;
    }

    for ( fd = 3; fd < _countof( g_afcb ); fd++ )
        if ( 0 == g_afcb[ fd ].n[ 0 ] )
            break;

    if ( fd >= _countof( g_afcb ) )
    {
        errno = EMFILE;
        return -1;
    }

    pfcb = & g_afcb[ fd ];
    set_filename( pfcb, uname );

    result = 0;
    create = ( 0 != ( mode & ( O_CREAT | O_TRUNC ) ) );

    if ( create )
        result = bdos_cpm( (long) 22, (long) pfcb ); /* make */
    else
        result = bdos_cpm( (long) 15, (long) pfcb ); /* open */

    if ( 255 == result )
    {
        g_afcb[ fd ].n[ 0 ] = 0; /* mark this fd as free */
        if ( create )
            errno = EINVAL;
        else
            errno = ENOENT;
        return -1;
    }

    g_aoffsets[ fd ] = 0;
    return fd;
} /*open*/

int _open( const char * uname, int mode, ... )
{
    return open( uname, mode );
} /*_open*/

int creat( const char * name, int mode )
{
    return open( name, O_WRONLY | O_TRUNC | O_CREAT, mode );
} /*creat*/

int close( int fd )
{
    struct FCBCPM68K * pfcb;
    int result;

    if ( fd < 3 || fd >= _countof( g_afcb ) )
    {
        errno = EINVAL;
        return -1;
    }
    if ( 0 == g_afcb[ fd ].n[ 0 ] )
    {
        errno = EINVAL;
        return -1;
    }

    pfcb = & g_afcb[ fd ];
    result = bdos_cpm( (long) 16, (long) pfcb );
    if ( 255 == result )
    {
        errno = EINVAL;
        return -1;
    }

    pfcb->n[ 0 ] = 0;
    return 0;
} /*close*/

int _close( int fd )
{
    return close( fd );
} /*_close*/

long lseek( int fd, long offset, int whence )
{
    long result;
    struct FCBCPM68K * pfcb;

    if ( fd < 3 || fd >= _countof( g_afcb ) )
    {
        errno = EINVAL;
        return -1;
    }
    if ( 0 == g_afcb[ fd ].n[ 0 ] )
    {
        errno = EINVAL;
        return -1;
    }

    pfcb = & g_afcb[ fd ];

    if ( SEEK_SET == whence )
        g_aoffsets[ fd ] = offset;
    else if ( SEEK_CUR == whence )
        g_aoffsets[ fd ] += offset;
    else if ( SEEK_END == whence )
    {
        result = bdos_cpm( (long) 35, (long) pfcb ); /* compute file size */
        if ( 0 != result )
        {
            errno = EINVAL;
            return -1;
        }

        result = GetRandomIOOffset( pfcb ) * 128;
        g_aoffsets[ fd ] = result + offset;
    }
    else
    {
        errno = EINVAL;
        return -1;
    }

    return g_aoffsets[ fd ];
} /*lseek*/

long _lseek( int fd, long offset, int whence )
{
    return lseek( fd, offset, whence );
} /*_lseek*/

int unlink( const char * pathname )
{
    struct FCBCPM68K fcb;
    int result;

    if ( !valid_cpm_filename( pathname ) )
    {
        errno = EINVAL;
        return -1;
    }

    set_filename( &fcb, pathname );
    result = bdos_cpm( (long) 19, (long) & fcb ); /* delete file */
    if ( 0xff == ( result & 0xff ) )
    {
        errno = EINVAL;
        return -1;
    }

    return 0;
} /*unlink*/

int _unlink( const char * pathname )
{
    return unlink( pathname );
} /*_unlink*/

size_t write( int fd, void * buffer, size_t count )
{
    int i, result;
    struct FCBCPM68K * pfcb;
    uint8_t * pdma, * buf;
    int remaining, to_copy, remainder;
    long record;

    if ( 0 == count )
        return 0;

    if ( 1 == fd || 2 == fd ) /* stdout and stderr */
    {
        for ( i = 0; i < count; i++ )
            bdos_cpm( (long) 2, (long) (uint8_t) ( (char *) buffer )[ i ] ); /* console output */
    }
    else
    {
        if ( fd < 3 )
        {
            errno = EACCES;
            return -1;
        }
        if ( fd >= _countof( g_afcb ) )
        {
            errno = EINVAL;
            return -1;
        }
        if ( 0 == g_afcb[ fd ].n[ 0 ] )
        {
            errno = EINVAL;
            return -1;
        }

        pfcb = & ( g_afcb[ fd ] );
        pdma = (uint8_t *) defdma;
        buf = (uint8_t *) buffer;

        /*
           cp/m can only read and write a single 128-byte record-aligned record at a time.
           in some cases existing data will need to be read and merged with the write.
        */

        remaining = count;

        while ( 0 != remaining )
        {
            record = g_aoffsets[ fd ] / 128;
            SetRandomIOOffset( pfcb, (uint32_t) record );
            remainder = (int) ( g_aoffsets[ fd ] % 128 );
    
            if ( ( 0 != remainder ) || ( remaining < 128 ) ) /* read, update, then write a 128 byte record */
            {
                result = bdos_cpm( (uint32_t) 33, (long) pfcb ); /* random read of record to be updated */
                if ( 0xff == ( result & 0xff ) )
                {
                    errno = EINVAL;
                    return -1;
                }

                to_copy = 0;
                if ( 0 != remainder )
                {
                    to_copy = get_min( 128 - remainder, remaining );
                    memcpy( pdma + remainder, buf, to_copy );
                    buf += to_copy;
                    remaining -= to_copy;
                }
                else
                {
                    to_copy = remaining;
                    memcpy( pdma, buf, to_copy );
                    buf += to_copy;
                    remaining = 0;
                }

                result = bdos_cpm( (uint32_t) 34, (long) pfcb ); /* random write */
                if ( 0 != result )
                {
                    errno = EINVAL;
                    return -1;
                }

                g_aoffsets[ fd ] += to_copy;
            }
            else /* write a 128 byte record */
            {
                memcpy( pdma, buf, 128 );
                buf += 128;
                result = bdos_cpm( (uint32_t) 34, (long) pfcb ); /* random write */
                if ( 0 != result )
                {
                    errno = EINVAL;
                    return -1;
                }
                g_aoffsets[ fd ] += 128;
                remaining -= 128;
            }
        }
    }

    return count;
} /*write*/

size_t _write( int fd, void * buffer, size_t count )
{
    return write( fd, buffer, count );
} /*_write*/

static long file_size( struct FCBCPM68K * pfcb )
{
    long pos, size, result;

    pos = GetRandomIOOffset( pfcb ); /* bdos 35 destroys the current offset, so save it */
    result = bdos_cpm( (long) 35, (long) pfcb ); /* get file size */
    if ( 0 != result )
    {
        errno = EINVAL;
        return -1;
    }

    size = GetRandomIOOffset( pfcb ) * 128; /* 128 byte granularity on file size in CP/M */
    SetRandomIOOffset( pfcb, pos ); /* restore the position */
    return size;
} /*file_size*/

size_t read( int fd, void * buffer, size_t count )
{
    int i, result, remaining, to_copy, remainder;
    long record;
    struct FCBCPM68K * pfcb;
    uint8_t * pdma, * buf;
    long size;

    if ( 0 == count )
        return 0;

    if ( 0 == fd )
    {
        /* todo: handle read from stdin */
        return -1;
    }
    else
    {
        if ( fd < 3 || fd >= _countof( g_afcb ) )
        {
            errno = EINVAL;
            return -1;
        }
        if ( 0 == g_afcb[ fd ].n[ 0 ] )
        {
            errno = EINVAL;
            return -1;
        }

        pfcb = & g_afcb[ fd ];
        size = file_size( pfcb );
        pdma = (uint8_t *) defdma;
        buf = (uint8_t *) buffer;
        remaining = count;

        while ( 0 != remaining )
        {
            record = g_aoffsets[ fd ] / 128;
            SetRandomIOOffset( pfcb, (uint32_t) record );

            if ( size == ( record * 128 ) )
                break;

            remainder = g_aoffsets[ fd ] % 128;
    
            if ( ( 0 != remainder ) || ( remaining < 128 ) )
            {
                int result = bdos_cpm( (long) 33, (long) pfcb ); /* random read of record */
                if ( 0xff == ( result & 0xff ) )
                {
                    errno = EINVAL;
                    return -1;
                }

                to_copy = 0;
                if ( 0 != remainder )
                {
                    to_copy = get_min( 128 - remainder, remaining );
                    memcpy( buf, pdma + remainder, to_copy );
                    buf += to_copy;
                    remaining -= to_copy;
                }
                else
                {
                    to_copy = remaining;
                    memcpy( buf, pdma, to_copy );
                    buf += to_copy;
                    remaining = 0;
                }

                g_aoffsets[ fd ] += to_copy;
            }
            else /* read a 128 byte record */
            {
                result = bdos_cpm( (long) 33, (long) pfcb ); /* random read */
                if ( 0 != result && 1 != result ) /* success or success and now at end of file */
                {
                    errno = EINVAL;
                    return -1;
                }
                memcpy( buf, pdma, 128 );
                buf += 128;
                g_aoffsets[ fd ] += 128;
                remaining -= 128;

                if ( 1 == result ) /* at end of file, so can't read more */
                    break;
            }
        }
    }

    return count - remaining;
} /*read*/

size_t _read( int fd, void * buffer, size_t count )
{
    return read( fd, buffer, count );
} /*_read*/

int isatty( int fd )
{
    if ( fd >= 0 && fd < 3 ) /* broken when stdin/stdout/stderr are redirected */
        return 1;

    if ( ( fd < 0 ) || ( fd >= _countof( g_afcb ) ) || ( 0 == g_afcb[ fd ].n[ 0 ] ) )
    {
        errno = EINVAL;
        return -1;
    }

    errno = 0;
    return 0;
} /*isatty*/

int rename( const char * oldpath, const char * newpath )
{
    struct FCBCPM68K * pold, * pnew;
    uint8_t buf[ 32 ]; /* two 16-byte FCB starting fields */
    int result;

    if ( !valid_cpm_filename( oldpath ) || !valid_cpm_filename( newpath ) )
    {
        errno = EINVAL;
        return -1;
    }

    pold = (struct FCBCPM68K *) & buf;
    set_filename( pold, oldpath );

    pnew = (struct FCBCPM68K *) & buf[ 16 ];
    set_filename( pnew, newpath );

    result = bdos_cpm( (long) 23, (long) & buf ); /* rename file */

    if ( 0 != result )
    {
        errno = EINVAL;
        return -1;
    }

    return 0;
} /*rename*/

