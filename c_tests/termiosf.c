#include <stdio.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cerrno>

struct TermRaw
{
    termios orig{};
    bool enabled = false;

    int enable()
    {
        if (enabled)
            return 0;

        if (tcgetattr(STDIN_FILENO, &orig) == -1)
        {
            printf( "tcgetattr failed\n" );
            return -1;
        }
    
        termios raw = orig;
        raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        raw.c_oflag &= ~(OPOST);
        raw.c_cflag |= (CS8);
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 1;
    
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        {
            printf( "tcsetattr failed\n" );
            return -1;
        }
    
        enabled = true;
        return 0;
    }

    int disable()
    {
        if (!enabled)
            return 0;

        enabled = false;
        return tcsetattr( STDIN_FILENO, TCSAFLUSH, &orig );
    }

  ~TermRaw() { disable(); }
};

static int getWindowSize(int& rows, int& cols)
{
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) return -1;
    rows = (int)ws.ws_row;
    cols = (int)ws.ws_col;
    return 0;
} //getWindowSize

int main()
{
    printf( "lflag:\n" );
    printf( " icanon %#x\n", ICANON );
    printf( " echonl %#x\n", ECHONL );
    printf( " echok %#x\n", ECHOK );
    printf( " echoke %#x\n", ECHOKE );
    printf( " echoe %#x\n", ECHOE );
    printf( " echo %#x\n", ECHO ); 
    #ifdef EXTPROC    
    printf( " extproc %#x\n", EXTPROC );
    #endif
    printf( " echoprt %#x\n", ECHOPRT );
    printf( " econl %#x\n", ECHONL );
    printf( " isig %#x\n", ISIG );
    printf( " iexten %#x\n", IEXTEN );
    printf( " echoctl %#x\n", ECHOCTL );
#ifdef TOSTOP
    printf( " tostop %#x\n", TOSTOP );
#endif

    printf( "oflag\n" );
    printf( " opost %#x\n", OPOST );
#ifdef ONLCRL
    printf( " onlcr %#x\n", ONLCR );
#endif
#ifdef OCRNL
    printf( " ocrnl %#x\n", OCRNL );
#endif
    printf( " onocr %#x\n", ONOCR );
    printf( " onlret %#x\n", ONLRET );

    printf( "iflag\n" );
    printf( " ixon %#x\n", IXON ); 
    printf( " ixoff %#x\n", IXOFF );
    printf( " icrnl %#x\n", ICRNL );
    printf( " inlcr %#x\n", INLCR );
    printf( " igncr %#x\n", IGNCR );
#ifdef IUCLC
    printf( " iuclc %#x\n", IUCLC );
#endif
    printf( " imaxbel %#x\n", IMAXBEL );
    printf( " brkint %#x\n", BRKINT );
    printf( " inpck %#x\n", INPCK );
    printf( " istrip %#x\n", ISTRIP );
#ifdef IGNBRK
    printf( " ignbrk %#x\n", IGNBRK );
#endif
#ifdef IGNPAR
    printf( " ignpar %#x\n", IGNPAR );
#endif
#ifdef PARMRK
    printf( " parmrk %#x\n", PARMRK );
#endif
#ifdef IXANY
    printf( " ixany %#x\n", IXANY );
#endif
#ifdef IUTF8
    printf( " iutf8 %#x\n", IUTF8 );
#endif
    
    printf( "cflag\n" );
    printf( " cs5 %#x\n", CS5 );
    printf( " cs6 %#x\n", CS6 );
    printf( " cs7 %#x\n", CS7 );
    printf( " csize %#x\n", CSIZE );
    printf( " cstopb %#x\n", CSTOPB );
    printf( " cread %#x\n", CREAD );
    printf( " parenb %#x\n", PARENB );
    printf( " cs8 %#x\n", CS8 );
    printf( " hupcl %#x\n", HUPCL );
    printf( " clocal %#x\n", CLOCAL );
    printf( " parodd %#x\n", PARODD );
#ifdef CMSPAR
    printf( " cmspar %#x\n", CMSPAR );
#endif
    printf( " crtscts %#x\n", CRTSCTS );

    printf( "cc_c\n" );
    printf( " VMIN %#x\n", VMIN );
    printf( " VTIME %#x\n", VTIME );
    printf( " VINTR %#x\n", VINTR );
    printf( " VQUIT %#x\n", VQUIT );
    printf( " VERASE %#x\n", VERASE ); 
    printf( " VKILL %#x\n", VKILL );
    printf( " VEOF %#x\n", VEOF ); 
#ifdef VSWTC
    printf( " VSWTC %#x\n", VSWTC );
#endif
    printf( " VSTART %#x\n", VSTART );
    printf( " VSTOP %#x\n", VSTOP );
    printf( " VSUSP %#x\n", VSUSP );
    printf( " VEOL %#x\n", VEOL );
    printf( " VEOL2 %#x\n", VEOL2 );
    printf( " VREPRINT %#x\n", VREPRINT );
    printf( " VWERASE %#x\n", VWERASE );
    printf( " VLNEXT %#x\n", VLNEXT );
    printf( " VDISCARD %#x\n", VDISCARD );
#ifdef VSTATUS
    printf( " VSTATUS %#x\n", VSTATUS );
#endif

    printf( "tcsetattr\n" );
    printf( " TCSANOW %#x\n", TCSANOW );
    printf( " TCSADRAIN %#x\n", TCSADRAIN );    
    printf( " TCSAFLUSH %#x\n", TCSAFLUSH );

    printf( "TIOCGWINSZ %#lx\n", (long) TIOCGWINSZ );
    struct termios t;
    printf( "c_cc elements: %#x\n", (int) sizeof( t.c_cc ) );

    TermRaw tr;
    int result = tr.enable();
    if ( -1 == result )
    {
         printf( "error: termraw.enable failed, errno %d\n", errno );
         return 1;
    }

    int rows = 0, cols = 0;
    result = getWindowSize( rows, cols );
    if ( -1 == result )
    {
         // this will fail when output is redirected, as it is in the test suite. ignore. at least we get code coverage
         //printf( "error: getWindowSize failed, errno %d\n", errno );
         //return 1;
    }

    if ( rows < 5 || cols < 5 || rows > 200 || cols > 200 )
    {
         // this will fail when output is redirected, as it is in the test suite. ignore. at least we get code coverage
        //printf( "error rows %u, columns %u look odd\n", rows, cols );
        //return 1;
    }

    result = tr.disable();
    if ( -1 == result )
    {
         printf( "error: termraw.disable failed, errno %d\n", errno );
         return 1;
    }

    printf( "termiosf completed with great success\n" );
    return 0;
} //main
