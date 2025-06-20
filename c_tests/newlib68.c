#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdnoreturn.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <math.h>
#include <dirent.h>
#include <fcntl.h>

#include <linuxem.h>

#define LINUX_AT_FDCWD -100

extern "C" void __attribute__((noreturn)) exit_emulator( int status );
extern "C" long syscall6( long number, long arg0, long arg1, long arg2, long arg3, long arg4, long arg5 );

extern "C" long syscall( long number, ... )
{
    va_list ap;
    va_start( ap, number );

    long arg0 = va_arg( ap, long );
    long arg1 = va_arg( ap, long );
    long arg2 = va_arg( ap, long );
    long arg3 = va_arg( ap, long );
    long arg4 = va_arg( ap, long );
    long arg5 = va_arg( ap, long );

    long result = syscall6( number, arg0, arg1, arg2, arg3, arg4, arg5 );
    if ( ( result < 0 ) && ( result > -4096 ) )
    {
        errno = -result;
        result = -1;
    }

    va_end( ap );
    return result;
} //syscall

int kill( pid_t pid, int sig ) { exit_emulator( 0 ); }
pid_t getpid( void ) { return 0x4955; } // IU is the best
extern "C" void _exit( int code ) { exit_emulator( code ); }

int close( int fd )
{
    return (int) syscall( SYS_close, fd );
}

extern "C" int open( const char * pathname, int flags, ... )
{
    int mode = 0;

    if ( 0 != ( flags & O_CREAT ) )
    {
        va_list ap;
        va_start( ap, flags );
        mode = va_arg( ap, int );
        va_end( ap );
    }

    return (int) syscall( SYS_openat, LINUX_AT_FDCWD, pathname, flags, mode ); // openat
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

    return (int) syscall( SYS_openat, dirfd, pathname, flags, mode ); // openat
} //openat

extern "C" int clock_gettime( clockid_t id, struct timespec * res )
{
    timespec_syscall tsc = { 0 };
    int result = (int) syscall( SYS_clock_gettime, 0 /*realtime*/, & tsc );
    if ( 0 == result )
    {
        res->tv_sec = tsc.tv_sec;
        res->tv_nsec = (uint32_t) tsc.tv_nsec;
    }
    return result;
} //clock_gettime

int usleep( useconds_t usec )
{
    // usleep is obsolete but used by this old GCC compiler. Use nanosleep instead

    timespec_syscall ns = {0};
    ns.tv_sec = usec / 1000000;
    ns.tv_nsec = ( usec % 1000000 ) * 1000;
    timespec_syscall remaining = {0};
    return (int) syscall( SYS_clock_nanosleep, 0 /* no clock id */, 0 /* no flags */, & ns, & remaining );
} //usleep

extern "C" int nanosleep( const struct timespec * duration, struct timespec * rem )
{
    timespec_syscall request; // two 64 bit values for all platforms for this emulator
    request.tv_sec = duration->tv_sec;
    request.tv_nsec = duration->tv_nsec;
    timespec_syscall remaining = {0};
    // printf( "sleeping for %llu sec", request.tv_sec ); printf( "  ... and %llu nsec\n", request.tv_nsec );
    int result = (int) syscall( SYS_clock_nanosleep, 0 /* no clock id */, 0 /* no flags */, & request, & remaining );

    if ( rem )
    {
        rem->tv_sec = remaining.tv_sec;
        rem->tv_nsec = remaining.tv_nsec;
    }

    return result;
} //nanosleep

int fstatat( int fd, const char * path, struct stat * statbuf, int flag )
{
    struct stat_linux_syscall sls = {0};
    int result = (int) syscall( SYS_newfstatat, fd, path, &sls, flag );
    if ( -1 == result )
        return result;

    statbuf->st_dev = sls.st_dev;
    statbuf->st_ino = sls.st_ino;
    statbuf->st_mode = sls.st_mode;
    statbuf->st_nlink = sls.st_nlink;
    statbuf->st_uid = sls.st_uid;
    statbuf->st_gid = sls.st_gid;
    statbuf->st_rdev = sls.st_rdev;
    statbuf->st_size = sls.st_size;
    statbuf->st_blksize = sls.st_blksize;
    statbuf->st_blocks = sls.st_blocks;
    statbuf->st_atime = sls.st_atim.tv_sec;
    statbuf->st_mtime = sls.st_mtim.tv_sec;
    statbuf->st_ctime = sls.st_ctim.tv_sec;
    return result;
} //fstatat

int getrusage( int who, struct rusage *usage )
{
    return (int) syscall( SYS_getrusage, who, usage );
}

extern "C" clock_t times( struct tms * buf )
{
    return (int) syscall( SYS_times, buf );
}

int rename( const char * oldpath, const char * newpath )
{
    return (int) syscall( SYS_renameat, LINUX_AT_FDCWD, oldpath, LINUX_AT_FDCWD, newpath, 0 );
}

int chdir( const char * path )
{
    return (int) syscall( SYS_chdir, path );
}

int mkdirat( int dirfd, const char * path, mode_t mode )
{
    return (int) syscall( SYS_mkdirat, dirfd, path, mode );
}

int unlinkat( int dirfd, const char * path, int flags )
{
    return (int) syscall( SYS_unlinkat, dirfd, path, flags );
}

int fdatasync( int fd )
{
    return (int) syscall( SYS_fdatasync, fd );
}

char * getcwd( char * buf, size_t size )
{
    return (char *) syscall( SYS_getcwd, buf, size );
}

int select( int nfds, fd_set * readfds, fd_set * writefds, fd_set * exceptfds, struct timeval * timeout )
{
    // map to pselect6 which means timespec not timeval and 0 sigset
    return (int) syscall( SYS_pselect6, nfds, readfds, writefds, exceptfds, 0, 0 );
}

int unlink( const char * path )
{
    return (int) syscall( SYS_unlinkat, LINUX_AT_FDCWD, path, 0 );
}

int fstat( int fd, struct stat * statbuf )
{
    struct stat_linux_syscall sls = {0};
    int result = (int) syscall( SYS_newfstat, fd, &sls );
    if ( -1 == result )
        return result;

    statbuf->st_dev = sls.st_dev;
    statbuf->st_ino = sls.st_ino;
    statbuf->st_mode = sls.st_mode;
    statbuf->st_nlink = sls.st_nlink;
    statbuf->st_uid = sls.st_uid;
    statbuf->st_gid = sls.st_gid;
    statbuf->st_rdev = sls.st_rdev;
    statbuf->st_size = sls.st_size;
    statbuf->st_blksize = sls.st_blksize;
    statbuf->st_blocks = sls.st_blocks;
    statbuf->st_atime = sls.st_atim.tv_sec;
    statbuf->st_mtime = sls.st_mtim.tv_sec;
    statbuf->st_ctime = sls.st_ctim.tv_sec;
    return result;
} //fstat

int gettimeofday( struct timeval *tv, void *tz )
{
    struct linux_timeval_syscall ltsc = {0};
    int result = (int) syscall( SYS_gettimeofday, &ltsc, tz );
    //printf( "gettimeofday result %d, sec %llx, usec %llx\n", result, ltsc.tv_sec, ltsc.tv_usec );
    if ( -1 != result )
    {
        tv->tv_sec = ltsc.tv_sec;
        tv->tv_usec = (uint32_t) ltsc.tv_usec;
    }
    return result;
} //gettimeofday

int isatty( int fd )
{
    local_kernel_termios term;
    int result = (int) syscall( SYS_ioctl, fd, 0x5401 /* TCGETS */, & term );
    return ( 0 == result );
} //isatty

_READ_WRITE_RETURN_TYPE read( int fd, void * buf, size_t count )
{
    return _READ_WRITE_RETURN_TYPE( syscall( SYS_read, fd, buf, count ) );
}

_READ_WRITE_RETURN_TYPE write( int fd, const void * buf, size_t count )
{
    return (_READ_WRITE_RETURN_TYPE) syscall( SYS_write, fd, (long) buf, count );;
}

extern "C" void exit( int status )
{
    exit_emulator( status );
}

off_t lseek( int fd, off_t offset, int whence )
{
    return (off_t) syscall( SYS_lseek, fd, offset, whence );
}

void * sbrk( intptr_t increment )
{
    char * current_brk = (char *) syscall( 214, 0 );
    long result = syscall( SYS_brk, (intptr_t) ( current_brk + increment ) );
    if ( -1 == result )
    {
        errno = ENOMEM;
        return (void *) -1;
    }

    if ( ( 0 != increment ) && ( (char *) result == current_brk ) )
    {
        errno = ENOMEM;
        return (void *) -1;
    }

    return current_brk;
}

long sysconf( int name )
{
    if ( _SC_CLK_TCK == name )
        return 100;

    return -1;
} //sysconf

int fsync( int fd )
{
    return syscall( SYS_fsync, fd );
} //fsync

int stat( const char * pathname, struct stat * statbuf )
{
    return fstatat( LINUX_AT_FDCWD, pathname, statbuf, 0 );
} //stat

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
    int fd = open( name, O_DIRECTORY );
    if ( -1 == fd )
        return 0;

    return fdopendir( fd );
} //opendir

struct dirent * readdir( DIR * dir )
{
    if ( 0 == dir )
        return 0;

    // m68 returns one file at a time as a simplification.

    static uint8_t ui8_buf[ 300 ];
    struct linux_dirent64_syscall * pdesc = (struct linux_dirent64_syscall *) & ui8_buf;

    int result = syscall( SYS_getdents64, dir->dd_fd, pdesc, sizeof( ui8_buf ) );
    if ( 0 == result || -1 == result )
        return 0;

    static struct dirent de = { 0 };
    de.d_ino = pdesc->d_ino;
    de.d_off = pdesc->d_off;
    de.d_reclen = pdesc->d_reclen;
    de.d_type = pdesc->d_type;
    strcpy( de.d_name, pdesc->d_name );

    return &de;
} //readdir

int closedir( DIR * dir )
{
    if ( 0 == dir )
        return -1;

    close( dir->dd_fd );
    free( dir->dd_buf );
    free( dir );
    return 0;
} //closedir


/***********************************************************************************/
/* the newlib with this compiler doesn't support printing floating point numbers,  */
/* 64-bit integers, or size_t %zd.                                                 */
/* so this ancient code from Apple is used instead with minor revisions            */

static FILE * g_fprintf_FILE = 0;
static int printf_full_len = 0;
static int fprintf_full_len = 0;

static void printf_putc( char c )
{
    printf_full_len++;
    write( 1, &c, 1 );
}

static void fprintf_putc( char c )
{
    fprintf_full_len++;
    fwrite( &c, 1, 1, g_fprintf_FILE );
}

#define isdigit(d) ( (d) >= '0' && (d) <= '9' )
#define Ctod(c) ( (c) - '0' )

#define MAXBUF ( sizeof( uint64_t ) * 8 )  // enough for binary

static void printnum( uint64_t u, int base, void (*putc)(char) )
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
} //printnum

static double set_d_sign( double d, bool sign )
{
    uint64_t val = sign ? ( ( * (uint64_t *) &d ) | 0x8000000000000000 ) : ( ( * (uint64_t *) &d ) & 0x7fffffffffffffff );
    return * (double *) &val;
} //set_d_sign

static bool get_d_sign( double d )
{
    return ( 0ull != ( ( * (uint64_t *) &d ) & 0x8000000000000000 ) );
} //get_d_sign

static void printdouble( double d, int precision, void (*putc)(char) )
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

    uint64_t wholePart = (int64_t) d; // large double values will be above 18,446,744,073,709,551,615
    printnum( wholePart, 10, putc );

    if ( precision > 0 )
    {
        (*putc)( '.' );
        double fraction = d - wholePart;

        while ( precision > 0 )
        {
            fraction *= 10;
            wholePart = (int64_t) fraction;
            (*putc)( '0' + wholePart );
            fraction -= wholePart;
            precision--;
        }
    }
} //printdouble

static void printfloat( float f, int precision, void (*putc)(char) )
{
    return printdouble( f, precision, putc );
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
                    printnum(u, base, putc);

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
                            printnum((unsigned)( (u>>(j-1)) & ((2<<(i-j))-1)),
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
                    truncate = _doprnt_truncates;
                case 'D':
                    base = 10;
                    goto print_signed;

                case 'f':
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
                    printdouble( d, ( -1 == prec ? 6 : prec ), putc );
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

