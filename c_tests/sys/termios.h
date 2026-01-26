// these values are typical for Linux on modern ISAs, but probably different from older 68000 Linux distributions

#define NCCS 0x20

typedef uint32_t tcflag_t;
typedef uint8_t cc_t;

struct termios {
    tcflag_t c_iflag;   /* input modes */
    tcflag_t c_oflag;   /* output modes */
    tcflag_t c_cflag;   /* control modes */
    tcflag_t c_lflag;   /* local modes */
    cc_t     c_cc[NCCS];/* special characters */
};

// lflag
#define ICANON 0x2
#define ECHONL 0x40
#define ECHOK 0x20
#define ECHOKE 0x800
#define ECHOE 0x10
#define ECHO 8
#define EXTPROC 0x10000
#define ECHOPRT 0x400
#define ECONL 0x40
#define ISIG 1
#define IEXTEN 0x8000
#define ECHOCTL 0x200
#define TOSTOP 0x100

// oflag
#define OPOST 0x1
#define ONCLCR 0x4
#define ONRNL 0x8
#define ONOCR 0x10
#define ONLRET 0x20

// iflag
#define IXON 0x400
#define IXOFF 0x1000
#define ICRNL 0x100
#define INLCR 0x40
#define IGNCR 0x80
#define IUCLC 0x200
#define IMAXBEL 0x2000
#define BRKINT 0x2
#define INPCK 0x10
#define ISTRIP 0x20
#define IGNBRK 0x1
#define IGNPAR 0x4
#define PARMRK 0x8
#define IXANY 0x800
#define IUTF8 0x4000

// cflag
#define CS5 0
#define CS6 0x10
#define CS7 0x20
#define CSIZE 0x30
#define CSTOPB 0x40
#define CREAD 0x80
#define PARENB 0x100
#define CS8 0x30
#define HUPCL 0x400
#define CLOCAL 0x800
#define PARODD 0x200
#define CMSPAR 0x40000000
#define CRTSCTS 0x80000000

// c_cc

#define VMIN            0x6 
#define VTIME           0x5 
#define VINTR           0   
#define VQUIT           0x1 
#define VERASE          0x2 
#define VKILL           0x3 
#define VEOF            0x4 
#define VSWTC           0x7 
#define VSTART          0x8 
#define VSTOP           0x9 
#define VSUSP           0xa 
#define VEOL            0xb 
#define VEOL2           0x10
#define VREPRINT        0xc 
#define VWERASE         0xe 
#define VLNEXT          0xf 
#define VDISCARD        0xd 
// #define VSTATUS macOS only?

struct winsize
{
    uint16_t ws_row;    /* rows, in characters */
    uint16_t ws_col;    /* columns, in characters */
    uint16_t ws_xpixel; /* horizontal size, in pixels */
    uint16_t ws_ypixel; /* vertical size, in pixels */
};

extern "C" int tcgetattr( int fd, struct termios * termios_p );

#define TCSANOW 0
#define TCSADRAIN 1
#define TCSAFLUSH 2

extern "C" int tcsetattr( int fd, int optional_actions, struct termios * termios_p );


