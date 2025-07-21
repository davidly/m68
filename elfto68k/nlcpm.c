/*
    This file provides the layer newlib calls to do OS-specific work.
    It is very much tuned to CP/M 68K running on a 32-bit 68000.
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdnoreturn.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <math.h>
#include <dirent.h>

#define _countof( X ) ( sizeof( X ) / sizeof( X[0] ) )

template <class T> inline T get_max( T a, T b )
{
    if ( a > b )
        return a;
    return b;
} //get_max

template <class T> inline T get_min( T a, T b )
{
    if ( a < b )
        return a;
    return b;
} //get_min

#pragma pack( push, 1 )

struct FCBCPM68K // file control block for cp/m
{
    uint8_t dr;         // drive
    char n[ 8 ];        // filename
    char t[ 3 ];        // file type
    uint8_t ex;         // current extent number. normally set to 0 by user but is in range 0-31
    uint8_t s1;
    uint8_t s2;         // reserved for system use. set to 0 for open/make/search
    uint8_t rc;         // record count, reserved for system use
    uint32_t current_offset; // not part of an official FCB -- state for this C runtime
    uint8_t d[ 12 ];
    uint8_t cr;         // current reacord to be read or written for sequential read/write operations. apps must set approprately.
    uint8_t r0;         // (optional if app is doing random I/O) random record number. most significant byte in r0 then r1 then r2 as a 24-bit record
    uint8_t r1;         // the byte order is the opposite of CP/M 2.2 for 8080 plus r2 is actually used!
    uint8_t r2;

    void set_filename( const char * p )
    {
        memset( n, ' ', 8 );
        memset( t, ' ', 3 );
        char * dot = strchr( p, '.' );
        if ( dot )
        {
            memcpy( n, p, (size_t) ( dot - p ) );
            memcpy( t, dot + 1, strlen( dot + 1 ) );
        }
        else
            memcpy( n, p, strlen( p ) );
    }

    void make_filename( char * p )
    {
        for ( int i = 0; i < 8; i++ )
        {
            if ( ' ' == ( 0x7f & n[ i ] ) )
                break;
            *p++ = ( 0x7f & n[ i ] );
        }
        if ( ' ' != t[ 0 ] )
        {
            *p++ = '.';
            for ( int i = 0; i < 3; i++ )
            {
                if ( ' ' == ( 0x7f & t[ i ] ) )
                    break;
                *p++ = ( 0x7f & t[ i ] );
            }
        }
        *p++ = 0;
    }

    // r0 and r1 are a 16-bit count of 128 byte records in CP/M 2.2. For CP/M 68K, reverse the byte ordering and add r2

    uint32_t GetRandomIOOffset() { return ( (uint32_t) this->r0 << 16 ) | ( (uint32_t) this->r1 << 8 ) | this->r2; }

    void SetRandomIOOffset( uint32_t o )
    {
        this->r2 = ( 0xff & o );
        this->r1 = ( 0xff & ( o >> 8 ) );
        this->r0 = ( 0xff & ( o >> 16 ) );
    } //SetRandomIOOffset

    void UpdateSequentialOffset( uint32_t offset )
    {
        cr = (uint8_t) ( ( offset % ( (uint32_t) 16 * 1024 ) ) / (uint32_t) 128 );
        ex = (uint8_t) ( ( offset % ( (uint32_t) 512 * 1024 ) ) / ( (uint32_t) 16 * 1024 ) );
        s2 = (uint8_t) ( offset / ( (uint32_t) 512 * 1024 ) );
        //printf( "  new offset: %u, s2 %u, ex %u, cr %u\n", offset, s2, ex, cr );
    } //UpdateSequentialOffset

    uint32_t GetSequentialOffset()
    {
        uint32_t curr = (uint32_t) cr * 128;
        curr += ( (uint32_t) ex * ( (uint32_t) 16 * 1024 ) );
        curr += ( (uint32_t) s2 * ( (uint32_t) 512 * 1024 ) );
        return curr;
    } //GetSequentialOffset

    void Trace( bool justArg = false ) // justArg is the first 16 bytes at app startup
    {
        printf( "  FCB\n" );
        printf( "    drive:    %#x == %c\n", dr, ( 0 == dr ) ? 'A' : 'A' + dr - 1 );
        printf( "    filename: '%c%c%c%c%c%c%c%c'\n", 0x7f & n[0], 0x7f & n[1], 0x7f & n[2], 0x7f & n[3],
                                                            0x7f & n[4], 0x7f & n[5], 0x7f & n[6], 0x7f & n[7] );
        printf( "    filetype: '%c%c%c'\n", 0x7f & t[0], 0x7f & t[1], 0x7f & t[2] );
        printf( "    R S A:    %d %d %d\n", 0 != ( 0x80 & t[0] ), 0 != ( 0x80 & t[1] ), 0 != ( 0x80 & t[2] ) );
        printf( "    ex:       %d\n", ex );
        printf( "    s1:       %u\n", s1 );
        printf( "    s2:       %u\n", s2 );
        printf( "    rc:       %u\n", rc );
        if ( !justArg )
        {
            printf( "    cr:       %u\n", cr );
            printf( "    r0:       %u\n", r0 );
            printf( "    r1:       %u\n", r1 );
            printf( "    r2:       %u\n", r2 );
        }
    } //Trace
};

struct BasePageCPM // base page for cp/m -- the first 256 bytes in memory for an app
{
    uint32_t lowest_tpa;                // 0
    uint32_t highest_tpa;               // 4
    uint32_t start_text;                // 8
    uint32_t cb_text;                   // c
    uint32_t start_data;                // 10
    uint32_t cb_data;                   // 14
    uint32_t start_bss;                 // 18
    uint32_t cb_bss;                    // 1c
    uint32_t cb_after_bss;              // 20
    uint8_t drive;                      // 24 where the app was loaded
    uint8_t reserved[ 19 ];             // 25 reserved. used for exit trap when an app returns from _start
    FCBCPM68K secondFCB;                // 38
    FCBCPM68K firstFCB;                 // 5c
    uint8_t cb_command_tail;            // 80 also start of default DMA buffer
    uint8_t command_tail[ 127 ];        // 81

    void Trace()
    {
        printf( "base page:\n" );
        printf( "  lowest tpa:       %lx\n", lowest_tpa );
        printf( "  highest tpa:      %lx\n", highest_tpa );
        printf( "  start_text:       %lx\n", start_text );
        printf( "  cb_text:          %lx\n", cb_text );
        printf( "  start_data:       %lx\n", start_data );
        printf( "  cb_data:          %lx\n", cb_data );
        printf( "  start_bss:        %lx\n", start_bss );
        printf( "  cb_bss:           %lx\n", cb_bss );
        printf( "  cb_after_bss:     %lx\n", cb_after_bss );
        printf( "  cb_command_tail:  %x\n",  cb_command_tail );
    }
};

#pragma pack(pop)

struct BasePageCPM * g_base_page = 0; // 256 bytes below _start
uint32_t g_eh_data = 0; // C++ eh frame data for __register_frame
char * g_current_brk = 0;
struct FCBCPM68K g_afcb[ 20 ]; // file descriptors index into this array

extern "C" void __attribute__((noreturn)) exit_cpm( int status );
extern "C" long bdos_cpm( long number, long arg0 );
extern "C" int kill( pid_t pid, int sig ) { exit_cpm( 0 ); }
extern "C" pid_t getpid( void ) { return 0x4955; } // IU is the best
extern "C" void _exit( int code ) { exit_cpm( code ); }
extern "C" void __register_frame( uint32_t eh_data );
extern "C" void __deregister_frame( uint32_t eh_data );

extern "C" void _init_nlcpm()
{
    g_current_brk = (char *) ( g_base_page->start_bss + g_base_page->cb_bss );

    if ( 0 != g_eh_data )
        __register_frame( g_eh_data );

    for ( int i = 0; i < ( sizeof( g_afcb ) / sizeof( g_afcb[ 0 ] ) ); i++ )
        g_afcb[ i ].n[ 0 ] = '*'; // indicate that it's free
} //_init_nlcpm

extern "C" void _fini_nlcpm()
{
    if ( 0 != g_eh_data )
        __deregister_frame( g_eh_data );
} //_fini_nlcpm

extern "C" void exit( int status )
{
    exit_cpm( status );
} //exit

extern "C" void * sbrk( intptr_t increment )
{
    char * pbase = (char *) g_base_page;

    if ( ( g_current_brk + increment ) <= (char *) ( g_base_page->start_bss + g_base_page->cb_bss + g_base_page->cb_after_bss ) )
    {
        char * oldbrk = g_current_brk;
        g_current_brk += increment;
        return oldbrk;
    }

    errno = ENOMEM;
    return (void *) -1;
} //sbrk

extern "C" long sysconf( int name )
{
    if ( _SC_CLK_TCK == name )
        return 100;

    return -1;
} //sysconf

bool ValidCPMFilename( const char * pc )
{
    if ( !strcmp( pc, "." ) )
        return false;

    if ( !strcmp( pc, ".." ) )
        return false;

    const char * pcinvalid = "<>,;:=?[]%|()/\\";
    for ( size_t i = 0; i < strlen( pcinvalid ); i++ )
        if ( strchr( pc, pcinvalid[i] ) )
            return false;

    size_t len = strlen( pc );

    if ( len > 12 )
        return false;

    char * pcdot = strchr( pc, '.' );

    if ( !pcdot && ( len > 8 ) )
        return false;

    if ( pcdot && ( ( pcdot - pc ) > 8 ) )
        return false;

    return true;
} //ValidCPMFilename

uint32_t file_size( FCBCPM68K & fcb )
{
    uint32_t pos = fcb.GetRandomIOOffset(); // bdos 35 destroys the current offset, so save it
    long result = bdos_cpm( 35, (long) & fcb ); // get file size
    if ( 0 != result )
    {
        errno = EINVAL;
        return -1;
    }

    uint32_t size = fcb.GetRandomIOOffset() * 128; // 128 byte granularity on file size in CP/M
    fcb.SetRandomIOOffset( pos ); // restore the position
    return size;
} //file_size

extern "C" int open( const char * pathname, int flags, ... )
{
    if ( !ValidCPMFilename( pathname ) )
    {
        errno = EINVAL;
        return -1;
    }

    int fd;
    for ( fd = 3; fd < _countof( g_afcb ); fd++ )
        if ( '*' == g_afcb[ fd ].n[ 0 ] )
            break;

    if ( fd >= _countof( g_afcb ) )
    {
        errno = EMFILE;
        return -1;
    }

    FCBCPM68K & fcb = g_afcb[ fd ];
    fcb.set_filename( pathname );

    int mode = 0;

    if ( 0 != ( flags & O_CREAT ) )
    {
        va_list ap;
        va_start( ap, flags );
        mode = va_arg( ap, int );
        va_end( ap );
    }

    uint32_t result = 0;
    bool create = flags & ( O_CREAT | O_TRUNC );

    if ( create )
        result = bdos_cpm( 22, (long) & fcb ); // make
    else
        result = bdos_cpm( 15, (long) & fcb ); // open

    if ( 255 == result )
    {
        g_afcb[ fd ].n[ 0 ] = '*'; // mark this fd as free
        if ( create )
            errno = EINVAL;
        else
            errno = ENOENT;
        return -1;
    }

    fcb.current_offset = 0;
    return fd;
} //open

extern "C" int openat( int dirfd, const char * pathname, int flags, ... )
{
    int mode = 0;

    if ( 0 != ( flags & O_CREAT ) )
    {
        va_list ap;
        va_start( ap, flags );
        mode = va_arg( ap, int );
        va_end( ap );
    }

    return open( pathname, flags, mode ); // no folders so ignore dirfd
} //openat

extern "C" int close( int fd )
{
    if ( fd < 3 || fd >= _countof( g_afcb ) )
    {
        errno = EINVAL;
        return -1;
    }
    if ( '*' == g_afcb[ fd ].n[ 0 ] )
    {
        errno = EINVAL;
        return -1;
    }

    FCBCPM68K & fcb = g_afcb[ fd ];
    int result = bdos_cpm( 16, (long) &fcb );
    if ( 255 == result )
    {
        errno = EINVAL;
        return -1;
    }

    fcb.n[ 0 ] = '*';
    return 0;
} //close

extern "C" off_t lseek( int fd, off_t offset, int whence )
{
    if ( fd < 3 || fd >= _countof( g_afcb ) )
    {
        errno = EINVAL;
        return -1;
    }
    if ( '*' == g_afcb[ fd ].n[ 0 ] )
    {
        errno = EINVAL;
        return -1;
    }

    FCBCPM68K & fcb = g_afcb[ fd ];

    if ( SEEK_SET == whence )
        fcb.current_offset = offset;
    else if ( SEEK_CUR == whence )
        fcb.current_offset += offset;
    else if ( SEEK_END == whence )
    {
        long result = bdos_cpm( 35, (long) & fcb );
        if ( 0 != result )
        {
            errno = EINVAL;
            return -1;
        }

        uint32_t file_len = fcb.GetRandomIOOffset() * 128;
        fcb.current_offset = file_len + offset;
    }
    else
    {
        errno = EINVAL;
        return -1;
    }


    return fcb.current_offset;
} //lseek

extern "C" _READ_WRITE_RETURN_TYPE write( int fd, const void * buffer, size_t count )
{
    if ( 0 == count )
        return 0;

    if ( 1 == fd )
    {
        for ( size_t i = 0; i < count; i++ )
            bdos_cpm( 2, (uint32_t) ( (char *) buffer )[ i ] );
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
        if ( '*' == g_afcb[ fd ].n[ 0 ] )
        {
            errno = EINVAL;
            return -1;
        }

        FCBCPM68K & fcb = g_afcb[ fd ];
        uint8_t * pdma = & g_base_page->cb_command_tail;
        uint8_t * buf = (uint8_t *) buffer;

        // cp/m can only read and write a single 128-byte record-aligned record at a time.
        // in some cases existing data will need to be read and merged with the write.

        uint32_t remaining = count;

        while ( 0 != remaining )
        {
            uint32_t record = fcb.current_offset / 128;
            fcb.SetRandomIOOffset( record );
            uint32_t remainder = fcb.current_offset % 128;
    
            if ( ( 0 != remainder ) || ( remaining < 128 ) ) // read, update, then write a 128 byte record
            {
                int result = bdos_cpm( 33, (long) & fcb ); // random read of record to be updated
                if ( 0xff == ( result & 0xff ) )
                {
                    errno = EINVAL;
                    return -1;
                }

                uint32_t to_copy = 0;
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

                result = bdos_cpm( 34, (long) & fcb ); // random write
                if ( 0 != result )
                {
                    errno = EINVAL;
                    return -1;
                }

                fcb.current_offset += to_copy;
            }
            else // write a 128 byte record
            {
                memcpy( pdma, buf, 128 );
                buf += 128;
                int result = bdos_cpm( 34, (long) & fcb ); // random write
                if ( 0 != result )
                {
                    errno = EINVAL;
                    return -1;
                }
                fcb.current_offset += 128;
                remaining -= 128;
            }
        }
    }

    return count;
} //write

extern "C" _READ_WRITE_RETURN_TYPE read( int fd, void * buffer, size_t count )
{
    uint32_t remaining = 0;

    if ( 0 == count )
        return 0;

    if ( 0 == fd )
    {
        // todo: handle this
        return -1;
    }
    else
    {
        if ( fd < 3 || fd >= _countof( g_afcb ) )
        {
            errno = EINVAL;
            return -1;
        }
        if ( '*' == g_afcb[ fd ].n[ 0 ] )
        {
            errno = EINVAL;
            return -1;
        }

        FCBCPM68K & fcb = g_afcb[ fd ];
        uint32_t size = file_size( fcb );


        uint8_t * pdma = & g_base_page->cb_command_tail;
        uint8_t * buf = (uint8_t *) buffer;
        remaining = count;

        while ( 0 != remaining )
        {
            uint32_t record = fcb.current_offset / 128;
            fcb.SetRandomIOOffset( record );

            if ( size == ( record * 128 ) )
                break;

            uint32_t remainder = fcb.current_offset % 128;
    
            if ( ( 0 != remainder ) || ( remaining < 128 ) ) // read, update, then write a 128 byte record
            {
                int result = bdos_cpm( 33, (long) & fcb ); // random read of record
                if ( 0xff == ( result & 0xff ) )
                {
                    errno = EINVAL;
                    return -1;
                }

                uint32_t to_copy = 0;
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

                fcb.current_offset += to_copy;
            }
            else // read a 128 byte record
            {
                int result = bdos_cpm( 33, (long) & fcb ); // random read
                if ( 0 != result && 1 != result ) // success or success and now at end of file
                {
                    errno = EINVAL;
                    return -1;
                }
                memcpy( buf, pdma, 128 );
                buf += 128;
                fcb.current_offset += 128;
                remaining -= 128;

                if ( 1 == result ) // at end of file, so can't read more
                    break;
            }
        }
    }

    return count - remaining;
} //read

extern "C" int unlink( const char * pathname )
{
    if ( !ValidCPMFilename( pathname ) )
    {
        errno = EINVAL;
        return -1;
    }

    FCBCPM68K fcb;
    fcb.set_filename( pathname );
    int result = bdos_cpm( 19, (long) & fcb ); // delete file
    if ( 0xff == ( result & 0xff ) )
    {
        errno = EINVAL;
        return -1;
    }

    return 0;
} //unlink

extern "C" int unlinkat( int dirfd, const char * path, int flags )
{
    if ( !ValidCPMFilename( path ) )
    {
        errno = EINVAL;
        return -1;
    }

    return unlink( path ); // no folders, so just unlink the path
} //unlinkat

extern "C" int fsync( int fd )
{
    bdos_cpm( 48, 0 ); // flush buffers
    return 0;
} //fsync

extern "C" int fdatasync( int fd )
{
    bdos_cpm( 48, 0 ); // flush buffers
    return 0;
} //fdatasync

extern "C" int fstatat( int fd, const char * path, struct stat * statbuf, int flag )
{
printf( "fstatat error\n" );
    return -1;
} //fstatat

extern "C" int fstat( int fd, struct stat * statbuf )
{
    if ( fd < 3 || fd >= _countof( g_afcb ) )
    {
        errno = EINVAL;
        return -1;
    }
    if ( '*' == g_afcb[ fd ].n[ 0 ] )
    {
        errno = EINVAL;
        return -1;
    }

    FCBCPM68K & fcb = g_afcb[ fd ];
    memset( statbuf, 0, sizeof( struct stat ) );

    uint32_t pos = fcb.GetRandomIOOffset();
    long result = bdos_cpm( 35, (long) & fcb );
    if ( 0 != result )
    {
        errno = EINVAL;
        return -1;
    }

    statbuf->st_size = fcb.GetRandomIOOffset() * 128; // only 128 byte granularity
    fcb.SetRandomIOOffset( pos ); // restore the position
    return 0;
} //fstat

extern "C" int stat( const char * pathname, struct stat * statbuf )
{
    int fd = open( pathname, 0 );
    if ( -1 == fd )
        return -1;

    int result = fstat( fd, statbuf );
    close( fd );
    return result;
} //stat

const int fdEnumeration = 100;
static bool g_EnumerationActive = false;
static FCBCPM68K g_fcbEnumeration;

DIR * fdopendir( int fd )
{
    DIR * pd = (DIR *) malloc( sizeof( DIR ) );
    pd->dd_fd = fd; // ownership transfer
    pd->dd_loc = 0;
    pd->dd_seek = 0;
    pd->dd_buf = (char *) malloc( 256 );
    pd->dd_len = 256;
    pd->dd_size = 0;

    return pd;
} //fdopendir

DIR * opendir( const char * name )
{
    if ( strcmp( name, "." ) ) // cp/m doesn't have folders. only support the current directory
    {
        errno = EINVAL;
        return 0;
    }

    return fdopendir( fdEnumeration );
} //opendir

struct dirent * readdir( DIR * dir )
{
    if ( 0 == dir )
        return 0;

    uint8_t * pdma = & g_base_page->cb_command_tail;
    static struct dirent de = { 0 };
    de.d_ino = 0x67;
    de.d_off = 0x69646c65; // mirror allergy
    de.d_reclen = sizeof( de );
    de.d_type = 8;

    if ( !g_EnumerationActive ) // find the first file in the numeration
    {
        g_fcbEnumeration.set_filename( "????????.???" );
        int result = bdos_cpm( 17, (long) & g_fcbEnumeration ); // find first
        if ( result >= 0 && result <= 3 )
        {
            FCBCPM68K * pfcb = (FCBCPM68K *) ( pdma + result * 32 );
            pfcb->make_filename( de.d_name );
            g_EnumerationActive = true;
        }
        else
            return 0;
    }
    else // find next
    {
        int result = bdos_cpm( 18, (long) & g_fcbEnumeration ); // find next
        if ( result >= 0 && result <= 3 )
        {
            FCBCPM68K * pfcb = (FCBCPM68K *) ( pdma + result * 32 );
            pfcb->make_filename( de.d_name );
        }
        else
            return 0;
    }

    return &de;
} //readdir

int closedir( DIR * dir )
{
    if ( 0 == dir )
        return -1;

    if ( fdEnumeration != dir->dd_fd )
    {
        errno = EINVAL;
        return -1;
    }

    g_EnumerationActive = false;
    free( dir->dd_buf );
    free( dir );
    return 0;
} //closedir

extern "C" int select( int nfds, fd_set * readfds, fd_set * writefds, fd_set * exceptfds, struct timeval * timeout )
{
    // just a tiny subset of select() is implemented

    if ( 1 == nfds && 0 != readfds )
    {
        // return 1 if a keystroke is available and 0 otherwise

        return bdos_cpm( 6, 0xfe );
    }

    return 0; // lie and say no I/O is ready
} //select

extern "C" int rename( const char * oldpath, const char * newpath )
{
    if ( !ValidCPMFilename( oldpath ) || !ValidCPMFilename( newpath ) )
    {
        errno = EINVAL;
        return -1;
    }

    // two 16-byte FCB starting fields

    uint8_t buf[ 32 ];
    FCBCPM68K * pold = (FCBCPM68K *) & buf;
    pold->set_filename( oldpath );

    FCBCPM68K * pnew = (FCBCPM68K *) & buf[ 16 ];
    pnew->set_filename( newpath );

    int result = bdos_cpm( 23, (long) & buf );

    if ( 0 != result )
    {
        errno = EINVAL;
        return -1;
    }

    return 0;
} //rename

// stubs for calls where CP/M 68K has no equivalent

extern "C" int usleep( useconds_t usec ) { return 0; }
extern "C" int nanosleep( const struct timespec * duration, struct timespec * rem ) { return 0; }
extern "C" int isatty( int fd ) { return -1; /* probably some way to make this work */ }
extern "C" int gettimeofday( struct timeval *tv, void *tz ) { return -1; }
extern "C" char * getcwd( char * buf, size_t size ) { strcpy( buf, "." ); return buf; }
extern "C" int chdir( const char * path ) { return 0; }
extern "C" int mkdirat( int dirfd, const char * path, mode_t mode ) { return 0; }
extern "C" clock_t times( struct tms * buf ) { return 0; }
int getrusage( int who, struct rusage *usage ) { return 0; }
extern "C" long syscall( long number, ... ) { return 0; } // no calling into Linux syscalls here

extern "C" int clock_gettime( clockid_t id, struct timespec * res )
{
    res->tv_sec = 0;
    res->tv_nsec = 0;

    return 0;
} //clock_gettime

/***********************************************************************************/
/* the newlib with this compiler doesn't support printing floating point numbers,  */
/* 64-bit integers, or size_t %zd.                                                 */
/* So this ancient code from Apple is used instead with minor revisions            */
/* Newlib can be built to include floating point support, but apparently not       */
/* 64-bit integers. e and a format specifiers aren't implemented.                  */
/* There is no buffering, so performance is pretty terrible                        */

static FILE * g_fprintf_FILE = 0;
static int printf_full_len = 0;
static int fprintf_full_len = 0;
static bool lf_to_crlf = true; // CP/M expects this behavior by apps, often handled by the C runtime

static void printf_putc( char c )
{
    if ( lf_to_crlf && 10 == c ) // this is CP/M 68k, whose C runtime converts 10 to 13 + 10
    {
        printf_full_len++;
        char lf = 13;
        write( 1, &lf, 1 );
    }

    printf_full_len++;
    write( 1, &c, 1 );
} //printf_putc

static void fprintf_putc( char c )
{
    if ( lf_to_crlf && 10 == c ) // this is CP/M 68k, whose C runtime converts 10 to 13 + 10
    {
        printf_full_len++;
        char lf = 13;
        fwrite( &lf, 1, 1, g_fprintf_FILE );
    }

    fprintf_full_len++;
    fwrite( &c, 1, 1, g_fprintf_FILE );
} //fprintf_putc

#define isdigit(d) ( (d) >= '0' && (d) <= '9' )
#define Ctod(c) ( (c) - '0' )

#define MAXBUF ( sizeof( uint64_t ) * 8 )  // enough for binary

static void print_ui64( uint64_t u, int base, void (*putc)(char) )
{
    char buf[ MAXBUF ];
    char * p = & buf[ MAXBUF - 1 ];
    static char digs[] = "0123456789abcdef";

    do
    {
        *p-- = digs[ u % base ];
        u /= base;
    } while ( 0 != u );

    while ( ++p != & buf[ MAXBUF ] )
        (*putc)( *p );
} //print_ui64

static double set_d_sign( double d, bool sign )
{
    uint64_t val = sign ? ( ( * (uint64_t *) &d ) | 0x8000000000000000 ) : ( ( * (uint64_t *) &d ) & 0x7fffffffffffffff );
    return * (double *) &val;
} //set_d_sign

static bool get_d_sign( double d )
{
    return ( 0ull != ( ( * (uint64_t *) &d ) & 0x8000000000000000 ) );
} //get_d_sign

static bool round_up( double fraction, int precision )
{
    // true if only nines exit through precision and next digit after precision is >= 5.
    // Required because round() only works if the rounded value can be represented in a double.
    // Numbers like 27.1084 can't -- they are represented as 27.108399999999999

    while( precision > 0 )
    {
        fraction *= 10.0;
        uint32_t wholePart = (int32_t) fraction;
        fraction -= wholePart;

        if ( precision > 1 )
        {
            if ( 9 != wholePart )
                return false;
        }
        else if ( wholePart <= 4 )
            return false;

        precision--;
    }

    return true;
} //round_up

static void print_double( double d, int precision, void (*putc)(char) )
{
    if ( get_d_sign( d ) )
    {
        (*putc)( '-' );
        d = set_d_sign( d, false );
    }
    
    if ( isnan( d ) )
    {
        (*putc)( 'n' );
        (*putc)( 'a' );
        (*putc)( 'n' );
        return;
    }
    
    if ( isinf( d ) )
    {
        (*putc)( 'i' );
        (*putc)( 'n' );
        (*putc)( 'f' );
        return;
    }

    double multiplier = pow( 10.0, precision );
    d = round( d * multiplier ) / multiplier;

    uint64_t wholePart = (uint64_t) d; // large double values will be above 18,446,744,073,709,551,615
    print_ui64( wholePart, 10, putc );

    if ( precision > 0 )
    {
        (*putc)( '.' );
        double fraction = d - wholePart;

        while ( precision > 0 )
        {
            fraction *= 10.0;
            wholePart = (int64_t) fraction;
            fraction -= wholePart;

            if ( round_up( fraction, precision ) )
            {
                wholePart++;
                precision = 0;
            }

            (*putc)( '0' + wholePart );
            precision--;
        }
    }
} //print_double

static void printfloat( float f, int precision, void (*putc)(char) )
{
    return print_double( f, precision, putc );
} //print_float

const static bool _doprnt_truncates = false;

static void _doprnt(
        const char     *fmt,
        va_list        *argp,
        void           (*putc)(char),
        int            radix)          /* default radix - for '%r' */
{
        int      length;
        volatile int      prec; // work around gcc bug with O1 not updating prec when it should
        bool     ladjust;
        char     padc;
        int64_t  n;
        uint64_t u;
        volatile uint64_t tmp; // work around gcc bug with O1 not working with 64-bit return values
        int      plus_sign;
        int      sign_char;
        bool     altfmt, truncate;
        int      base;
        char     c;
        int      capitals;
        int      num_width = 4;

        while ((c = *fmt) != '\0') {
            if (c != '%') {
                (*putc)(c);
                fmt++;
                continue;
            }

            fmt++;

            length = 0;
            prec = -1;
            ladjust = false;
            padc = ' ';
            plus_sign = 0;
            sign_char = 0;
            altfmt = false;

            while (true) {
                c = *fmt;
                if (c == '#') {
                    altfmt = true;
                }
                else if (c == '-') {
                    ladjust = true;
                }
                else if (c == '+') {
                    plus_sign = '+';
                }
                else if (c == ' ') {
                    if (plus_sign == 0)
                        plus_sign = ' ';
                }
                else
                    break;
                fmt++;
            }

            if (c == '0') {
                padc = '0';
                c = *++fmt;
            }

            if (isdigit(c)) {
                while(isdigit(c)) {
                    length = 10 * length + Ctod(c);
                    c = *++fmt;
                }
            }
            else if (c == '*') {
                length = va_arg(*argp, int);
                c = *++fmt;
                if (length < 0) {
                    ladjust = !ladjust;
                    length = -length;
                }
            }

            if (c == '.') {
                c = *++fmt;
                if (isdigit(c)) {
                    prec = 0;
                    while(isdigit(c)) {
                        prec = 10 * prec + Ctod(c);
                        c = *++fmt;
                    }
                }
                else if (c == '*') {
                    prec = va_arg(*argp, int);
                    c = *++fmt;
                }
                else
                    prec = 0;
            }

            if (c == 'l')
            {
                c = *++fmt;     /* need it if sizeof(int) < sizeof(long) */
                if ( c == 'l' )
                {
                    c = *++fmt;
                    num_width = 8;
                }
            }

            if (c == 'z')
                c = *++fmt;     /* size_t %zd type -- ignore */

            truncate = false;
            capitals=0;         /* Assume lower case printing */

            switch(c) {
                case 'b':
                case 'B':
                {
                    char *p;
                    bool     any;
                    int  i;

                    if ( 4 == num_width )
                        u = va_arg(*argp, uint32_t );
                    else
                        u = va_arg(*argp, uint64_t );
                    p = va_arg(*argp, char *);
                    base = *p++;
                    print_ui64(u, base, putc);

                    if (u == 0)
                        break;

                    any = false;
                    while ((i = *p++) != '\0') {
                        if (*fmt == 'B')
                            i = 33 - i;
                        if (*p <= 32) {
                            /*
                             * Bit field
                             */
                            int j;
                            if (any)
                                (*putc)(',');
                            else {
                                (*putc)('<');
                                any = true;
                            }
                            j = *p++;
                            if (*fmt == 'B')
                                j = 32 - j;
                            for (; (c = *p) > 32; p++)
                                (*putc)(c);
                            print_ui64((unsigned)( (u>>(j-1)) & ((2<<(i-j))-1)),
                                        base, putc);
                        }
                        else if (u & (1<<(i-1))) {
                            if (any)
                                (*putc)(',');
                            else {
                                (*putc)('<');
                                any = true;
                            }
                            for (; (c = *p) > 32; p++)
                                (*putc)(c);
                        }
                        else {
                            for (; *p > 32; p++)
                                continue;
                        }
                    }
                    if (any)
                        (*putc)('>');
                    break;
                }

                case 'c':
                    c = va_arg(*argp, int);
                    (*putc)(c);
                    break;

                case 's':
                {
                    char *p;
                    char *p2;
                    int in;

                    if (prec == -1)
                        prec = 0x7fffffff;      /* MAXINT */

                    p = va_arg(*argp, char *);

                    if (p == (char *)0)
                        p = (char *) "";

                    if (length > 0 && !ladjust) {
                        in = 0;
                        p2 = p;

                        for (; *p != '\0' && in < prec; p++)
                            in++;

                        p = p2;

                        while (in < length) {
                            (*putc)(' ');
                            in++;
                        }
                    }

                    in = 0;

                    while (*p != '\0') {
                        if (++in > prec || (length > 0 && in > length))
                            break;

                        (*putc)(*p++);
                    }

                    if (in < length && ladjust) {
                        while (in < length) {
                            (*putc)(' ');
                            in++;
                        }
                    }

                    break;
                }

                case 'o':
                    truncate = _doprnt_truncates;
                case 'O':
                    base = 8;
                    goto print_unsigned;

                case 'd':
                case 'i':
                    truncate = _doprnt_truncates;
                case 'D':
                case 'I':
                    base = 10;
                    goto print_signed;

                case 'f':
                case 'g': // 'e' isn't supported yet, so just use %f
                    goto print_float;

                case 'u':
                    truncate = _doprnt_truncates;
                case 'U':
                    base = 10;
                    goto print_unsigned;

                case 'p':
                    altfmt = true;
                case 'x':
                    truncate = _doprnt_truncates;
                    base = 16;
                    goto print_unsigned;

                case 'X':
                    base = 16;
                    capitals=16;        /* Print in upper case */
                    goto print_unsigned;

                case 'z':
                    truncate = _doprnt_truncates;
                    base = 16;
                    goto print_signed;
                        
                case 'Z':
                    base = 16;
                    capitals=16;        /* Print in upper case */
                    goto print_signed;

                case 'r':
                    truncate = _doprnt_truncates;
                case 'R':
                    base = radix;
                    goto print_signed;

                case 'n':
                    truncate = _doprnt_truncates;
                case 'N':
                    base = radix;
                    goto print_unsigned;

                print_signed:
                    if ( 4 == num_width )
                        n = va_arg(*argp, int32_t );
                    else
                        n = va_arg(*argp, int64_t );
                    if (n >= 0) {
                        u = n;
                        sign_char = plus_sign;
                    }
                    else {
                        u = -n;
                        sign_char = '-';
                    }
                    goto print_num;

                print_unsigned:
                    if ( 4 == num_width )
                        u = va_arg(*argp, uint32_t );
                    else
                        u = va_arg(*argp, uint64_t );
                    goto print_num;

                print_num:
                {
                    char        buf[MAXBUF];    /* build number here */
                    char *     p = &buf[MAXBUF-1];
                    static char digits[] = "0123456789abcdef0123456789ABCDEF";
                    char *prefix = 0;

                    if (truncate) u = (long)((int)(u));

                    if (u != 0 && altfmt) {
                        if (base == 8)
                            prefix = (char *) "0";
                        else if (base == 16)
                            prefix = (char *) "0x";
                    }

                    do {
                        /* Print in the correct case */
                        *p-- = digits[(u % base)+capitals];
                        tmp = u / base; // work around gcc -O1 bug with 64-bit return values being ignored
                        if ( 0 == tmp )
                            break;
                        u = tmp;
                    } while (u != 0);

                    length -= (&buf[MAXBUF-1] - p);
                    if (sign_char)
                        length--;
                    if (prefix)
                        length -= strlen((const char *) prefix);

                    if (padc == ' ' && !ladjust) {
                        /* blank padding goes before prefix */
                        while (--length >= 0)
                            (*putc)(' ');
                    }
                    if (sign_char)
                        (*putc)(sign_char);
                    if (prefix)
                        while (*prefix)
                            (*putc)(*prefix++);
                    if (padc == '0') {
                        /* zero padding goes after sign and prefix */
                        while (--length >= 0)
                            (*putc)('0');
                    }
                    while (++p != &buf[MAXBUF])
                        (*putc)(*p);

                    if (ladjust) {
                        while (--length >= 0)
                            (*putc)(' ');
                    }
                    break;
                }
                print_float: // only 4-byte floats are supported
                {
                    // varargs promotes floats to doubles in va_arg

                    double d = va_arg( *argp, double );
                    print_double( d, ( -1 == prec ? 6 : prec ), putc );
                    break;
                }

                case '\0':
                    fmt--;
                    break;

                default:
                    (*putc)(c);
            }
        fmt++;
        }
} //_doprnt

// override the default implementations of printf and sprintf in newlib because those
// versions don't support 64-bit integers, %z for size_t, and floating point

extern int printf( const char *fmt, ... )
{
    va_list listp;
    va_start( listp, fmt );
    printf_full_len = 0;
    _doprnt( fmt, &listp, printf_putc, 16 );
    va_end( listp );
    return printf_full_len;
} //printf

static int copybyte_buf_len = 0;
static int copybyte_full_len = 0;
static char *copybyte_str;

static void copybyte( char byte )
{
    copybyte_full_len++;

    if ( 0 == copybyte_buf_len || copybyte_full_len < copybyte_buf_len )
    {
        *copybyte_str++ = byte;
        *copybyte_str = 0;
    }
} //copybyte

extern int sprintf( char *buf, const char *fmt, ... )
{
    va_list listp;
    va_start( listp, fmt );
    copybyte_buf_len = 0;
    copybyte_full_len = 0;
    copybyte_str = buf;
    _doprnt( fmt, &listp, copybyte, 16 );
    va_end( listp );
    return copybyte_full_len;
} //sprintf

extern int snprintf( char *buf, size_t n, const char *fmt, ... )
{
    va_list listp;
    va_start( listp, fmt );
    copybyte_buf_len = (int) n;
    copybyte_full_len = 0;
    copybyte_str = buf;
    _doprnt( fmt, &listp, copybyte, 16 );
    va_end( listp );
    return copybyte_full_len;
} //sprintf

extern int fprintf( FILE * fp, const char *fmt, ... )
{
    g_fprintf_FILE = fp;
    va_list listp;
    va_start( listp, fmt );
    fprintf_full_len = 0;
    _doprnt( fmt, &listp, fprintf_putc, 16 );
    va_end( listp );
    return fprintf_full_len;
} //fprintf

extern int vfprintf( FILE * fp, const char * fmt, va_list args )
{
    g_fprintf_FILE = fp;
    fprintf_full_len = 0;
    _doprnt( fmt, &args, fprintf_putc, 16 );
    return fprintf_full_len;
} //vfprintf

extern char * floattoa( char * buffer, float f, int precision )
{
    copybyte_str = buffer;
    copybyte_buf_len = 0;
    copybyte_full_len = 0;
    printfloat( f, 6, copybyte );
    return buffer;
} //floattoa

extern "C" int puts( const char * str )
{
    while ( *str )
    {
        printf_putc( *str );
        str++;
    }

    printf_putc( 10 );
    return 0;
} //puts

extern "C" int putchar( int x )
{
    printf_putc( x & 0xff );
    return 0;
} //putchar

extern "C" bool _setbinarymode( bool binmode )
{
    bool oldstate = lf_to_crlf;
    lf_to_crlf = !binmode;
    return !oldstate;
} //_setbinarymode

