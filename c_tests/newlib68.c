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

#include <linuxem.h>

#define O_CREAT 0x200
#define AT_FDCWD -100

extern "C" void __attribute__((noreturn)) exit_emulator( int status );

//extern "C" long syscall1( long number, long arg0 );
//extern "C" long syscall3( long number, long arg0, long arg1, long arg2 );
//extern "C" long syscall4( long number, long arg0, long arg1, long arg2, long arg3 );
//extern "C" long syscall5( long number, long arg0, long arg1, long arg2, long arg3, long arg4 );
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

int kill( pid_t pid, int sig ) { return -1; }
pid_t getpid( void ) { return 0x69; }
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

    return (int) syscall( SYS_openat, AT_FDCWD, pathname, flags, mode ); // openat
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

int fstatat( int fd, const char * path, struct stat * buf, int flag )
{
    return (int) syscall( SYS_newfstatat, fd, path, buf, flag );
}

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
    return (int) syscall( SYS_renameat, AT_FDCWD, oldpath, newpath );
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
    return (int) syscall( SYS_unlinkat, AT_FDCWD, path );
}

int fstat( int fd, struct stat * buf )
{
    return -1;
}

int gettimeofday( struct timeval *tv, void *tz )
{
    return (int) syscall( SYS_gettimeofday, tv, tz );
}

int isatty( int fd ) { return -1; }

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
    // the newlib I'm using doesn't support calling atexit() registered functions including destructors for global objects.
    exit_emulator( status );
}

off_t lseek( int fd, off_t offset, int whence )
{
    return (off_t) syscall( SYS_lseek, fd, offset, whence );
}

void * sbrk( intptr_t increment )
{
    char * current_brk = (char *) syscall( 214, 0 );
    long result = syscall( 214, (intptr_t) ( current_brk + increment ) );
    if ( -1 == result )
    {
        errno = ENOMEM;
        return (void *) -1;
    }

    return current_brk;
}

/***********************************************************************************/
/* the newlib with this compiler doesn't support printing floating point numbers   */
/* so this ancient code is used instead */

static int g_printf_fd = 1;

static void printf_putc( char c )
{
    write( g_printf_fd, &c, 1 );
}

#define isdigit(d) ((d) >= '0' && (d) <= '9')
#define Ctod(c) ((c) - '0')

#define MAXBUF ( sizeof( uint64_t ) * 8 )  // enough for binary

static void printnum( uint64_t u, int base, void (*putc)(char) )
{
    char buf[MAXBUF];
    char * p = &buf[MAXBUF-1];
    static char digs[] = "0123456789abcdef";

    do
    {
        *p-- = digs[u % base];
        u /= base;
    } while (u != 0);

    while (++p != &buf[MAXBUF])
        (*putc)(*p);
} //printnum

double set_d_sign( double d, bool sign )
{
    uint64_t val = sign ? ( ( * (uint64_t *) &d ) | 0x8000000000000000 ) : ( ( * (uint64_t *) &d ) & 0x7fffffffffffffff );
    return * (double *) &val;
} //set_d_sign

void printdouble( double d, int precision, void (*putc)(char) )
{
    if ( signbit( d ) )
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
// versions don't support 64-bit integers and floating point

extern int printf( const char *fmt, ... )
{
    g_printf_fd = 1; // stdout
    va_list listp;
    va_start( listp, fmt );
    _doprnt( fmt, &listp, printf_putc, 16 );
    va_end(listp);
    return 0;
} //printf

static char *copybyte_str;

static void copybyte( char byte )
{
  *copybyte_str++ = byte;
  *copybyte_str = '\0';
} //copybyte

extern int sprintf( char *buf, const char *fmt, ... )
{
    va_list listp;
    va_start( listp, fmt );
    copybyte_str = buf;
    _doprnt( fmt, &listp, copybyte, 16 );
    va_end( listp );
    return strlen(buf);
} //sprintf

extern int fprintf( FILE * fp, const char *fmt, ... )
{
    g_printf_fd = fileno( fp );
    va_list listp;
    va_start( listp, fmt );
    _doprnt( fmt, &listp, printf_putc, 16 );
    va_end( listp );
    return 0;
} //fprintf

extern char * floattoa( char * buffer, float f, int precision )
{
    copybyte_str = buffer;
    printfloat( f, 6, copybyte );
    return buffer;
} //floattoa

