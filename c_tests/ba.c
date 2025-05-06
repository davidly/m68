// A very basic BASIC interpreter.
// Also, compilers for x64 on Windows, arm64 on Apple Silicon and Windows, 8080 on cp/m 2.2,
// Arm32 for Linux (tested on Raspberry PI 3B and 4), 6502 for the Apple 1, 8086 for DOS,
// 32-bit x86 for Windows, and 64-bit RISC-V for the Maixduino SiPeed Kendryte K210.
// implements a small subset of gw-basic; just enough to run a tic-tac-toe proof of failure app.
// a few of the many limitations:
//    -- based on TRS-80 Model 100 gw-basic. Equivalent to MBasic on CP/M.
//    -- only integer variables (4 byte) (2 bytes for 8080 and 6502 compilers) are supported
//    -- for loop start and end values must be constants
//    -- variables can only be two characters long plus a mandatory %
//    -- string values work in PRINT statements and nowhere else
//    -- a new token ELAP$ for PRINT that shows elapsed time milliseconds or microseconds
//    -- keywords supported: (see "Operators" below).
//    -- Not supported: DEF, PLAY, OPEN, INKEY$, DATA, READ, and a very long list of others.
//    -- only arrays of 1 or 2 dimensions are supported
// Notes about 8086/DOS WATCOM builds
//    -- The watcom compiler generates code when it sees "if (false) something;". A bunch of
//       the tracing and range checking code assumes this is optimized away like other compilers do.
//    -- No codegen on DOS because the binary gets too big.
//    -- no execution time tracking on DOS
//
//  The expression grammar for assigment and IF statements: (parens are literal and square brackets are to show options)
//
//    expression = term [additive]*
//    additive = [ + | - ] term
//    factor = ( expression ) | variable | constant
//    constant = (0-9)+
//    variable = varname | varname( expression )
//    varname = (a-z)+%
//    term = factor [multiplicative]*
//    multiplicative = [ * | / ] factor
//    relationalexpression = expression [relational]*
//    relational = [ < | > | <= | >= | = ] expression
//    logicalexpression = relationalexpression [logical]*
//    logical = [ AND | OR | XOR ] relationalexpression

#include <stdio.h>
#include <assert.h>

#include <algorithm>
#include <string>
#include <cstring>
#include <sstream>
#include <cctype>
#include <map>
#include <cstdint>
#include <vector>
#include <chrono>

using namespace std;

#ifndef WATCOM
    #define BA_ENABLE_COMPILER // this makes the app too big for WATCOM
    using namespace std::chrono;
#endif

bool g_Tracing = false;
bool g_ExpressionOptimization = true;
bool g_Quiet = false;
bool g_GenerateAppleDollar = false;
int g_pc = 0;
int g_argc = 0;
char ** g_argv = 0;

// this does not work with WATCOM / 8086 builds
//#define BA_ENABLE_INTERPRETER_EXECUTION_TIME

#ifdef DEBUG
    const bool RangeCheckArrays = true;
    const bool EnableTracing = true;

    #define __makeinline __declspec(noinline)
    //#define __makeinline
#else
    const bool RangeCheckArrays = false;  // oh how I wish C# supported turning off bounds checking
    const bool EnableTracing = false;     // makes eveything 10% slower

    #define __makeinline __forceinline
    //#define __makeinline
#endif

#ifdef __APPLE__
    // On an M1 Mac, this yields much faster/better results than on Windows and Linux x64 machines.
 
    uint64_t __rdtsc( void )
    {
        uint64_t val;
        asm volatile("mrs %0, cntvct_el0" : "=r" (val ));
        return val;
    }
#endif

#ifdef _MSC_VER  
    #include <intrin.h>

    #ifdef __GNUC__
        #define __assume( x )
    #endif
#else // g++, clang++
    #define __assume( x )
    #undef __makeinline
    #define __makeinline inline
#ifdef WATCOM
    #include <dos.h>

    uint32_t DosTimeInMS()
    {
        struct dostime_t tNow;
        _dos_gettime( &tNow );
        uint32_t t = (uint32_t) tNow.hour * 60 * 60 * 100;
        t += (uint32_t) tNow.minute * 60 * 100;
        t += (uint32_t) tNow.second * 100;
        t += (uint32_t) tNow.hsecond;
        return t * 10;
    } //DosTimeInMS

    int _strnicmp( const char * a, const char * b, int len )
    {
        for ( int i = 0; i < len; i++ )
        {
            char la = tolower( a[i] );
            char lb = tolower( b[i] );
            if ( !la && !lb )
                return 0;
            if ( !la )
                return -1;
            if ( !lb )
                return 1;
            if ( la != lb )
                return la - lb;
        }
        return 0;
    }
#else
    #define _strnicmp strncasecmp
    #define _stricmp strcasecmp
#endif

#ifndef _countof
#ifdef WATCOM
        #define _countof( x ) ( sizeof( x ) / sizeof( x[0] ) )
#else
        template < typename T, size_t N > size_t _countof( T ( & arr )[ N ] ) { return std::extent< T[ N ] >::value; }
#endif
#endif

    void strcpy_s( char * pto, size_t size, const char * pfrom  )
    {
        strcpy( pto, pfrom );
    }
#endif

int stcmp( const string & a, const string & b )
{
    return strcmp( a.c_str(), b.c_str() );
} //stcmp

enum AssemblyTarget { x86Win, x64Win, arm64Mac, arm64Win, i8080CPM, arm32Linux, mos6502Apple1, i8086DOS, riscv64, oiOS, m68kCPM };
AssemblyTarget g_AssemblyTarget = x64Win;
bool g_i386Target686 = true; // true for Pentium 686 cmovX instructions, false for generic 386

enum Token_Enum {
    Token_VARIABLE, Token_GOSUB, Token_GOTO, Token_PRINT, Token_RETURN, Token_END,                     // statements
    Token_REM, Token_DIM, Token_CONSTANT, Token_OPENPAREN, Token_CLOSEPAREN,
    Token_MULT, Token_DIV, Token_PLUS, Token_MINUS, Token_EQ, Token_NE, Token_LE, Token_GE, Token_LT, Token_GT, Token_AND, Token_OR, Token_XOR,  // operators in order of precedence
    Token_FOR, Token_NEXT, Token_IF, Token_THEN, Token_ELSE, Token_LINENUM, Token_STRING, Token_TO, Token_COMMA,
    Token_COLON, Token_SEMICOLON, Token_EXPRESSION, Token_TIME, Token_ELAP, Token_TRON, Token_TROFF,
    Token_ATOMIC, Token_INC, Token_DEC, Token_NOT, Token_INVALID };

#define Token int

const char * Tokens[] = { 
    "VARIABLE", "GOSUB", "GOTO", "PRINT", "RETURN", "END",
    "REM", "DIM", "CONSTANT", "OPENPAREN", "CLOSEPAREN",
    "MULT", "DIV", "PLUS", "MINUS", "EQ", "NE", "LE", "GE", "LT", "GT", "AND", "OR", "XOR",
    "FOR", "NEXT", "IF", "THEN", "ELSE", "LINENUM", "STRING", "TO", "COMMA",
    "COLON", "SEMICOLON", "EXPRESSION", "TIME$", "ELAP$", "TRON", "TROFF",
    "ATOMIC", "INC", "DEC", "NOT", "INVALID" };

const char * Operators[] = { 
    "VARIABLE", "GOSUB", "GOTO", "PRINT", "RETURN", "END",
    "REM", "DIM", "CONSTANT", "(", ")",
    "*", "/", "+", "-", "=", "<>", "<=", ">=", "<", ">", "&", "|", "^", 
    "FOR", "NEXT", "IF", "THEN", "ELSE", "LINENUM", "STRING", "TO", "COMMA",
    "COLON", "SEMICOLON", "EXPRESSION", "TIME$", "ELAP$", "TRON", "TROFF",
    "ATOMIC", "INC", "DEC", "NOT", "INVALID" };

#ifdef BA_ENABLE_COMPILER

const char * OperatorInstructionX64[] = { 
    0, 0, 0, 0, 0, 0,                          // filler
    0, 0, 0, 0, 0,                             // filler
    "imul", "idiv", "add", "sub", "sete", "setne", "setle", "setge", "setl", "setg", "and", "or", "xor", };

// most only used and work on Arm64

const char * OperatorInstructionArm[] = {
    0, 0, 0, 0, 0, 0,                          // filler
    0, 0, 0, 0, 0,                             // filler
    "mul", "sdiv", "add", "sub", "sete", "setne", "setle", "setge", "setl", "setg", "and", "orr", "eor", };

// only the last 3 are used

const char * OperatorInstructioni8080[] = {
    0, 0, 0, 0, 0, 0,                          // filler
    0, 0, 0, 0, 0,                             // filler
    "mul", "sdiv", "add", "sub", "sete", "setne", "setle", "setge", "setl", "setg", "ana", "ora", "xra", };

// only the last 3 are used

const char * OperatorInstruction6502[] = {
    0, 0, 0, 0, 0, 0,                          // filler
    0, 0, 0, 0, 0,                             // filler
    "mul", "sdiv", "add", "sub", "sete", "setne", "setle", "setge", "setl", "setg", "and", "ora", "eor", };

// only the last 3 are used

const char * OperatorInstructionRiscV64[] = {
    0, 0, 0, 0, 0, 0,                          // filler
    0, 0, 0, 0, 0,                             // filler
    "mul", "sdiv", "add", "sub", "sete", "setne", "setle", "setge", "setl", "setg", "and", "or", "xor", };

const char * OperatorInstructionm68kCPM[] = { 
    0, 0, 0, 0, 0, 0,                          // filler
    0, 0, 0, 0, 0,                             // filler
    "muls", "divs", "add.l", "sub.l", "seq", "sne", "sle", "sge", "slt", "sgt", "and.l", "or.l", "eor.l", };

const char * ConditionsArm[] = { 
    0, 0, 0, 0, 0, 0,                          // filler
    0, 0, 0, 0, 0,                             // filler
    0, 0, 0, 0, "eq", "ne", "le", "ge", "lt", "gt", 0, 0, 0 };

const char * ConditionsNotArm[] = { 
    0, 0, 0, 0, 0, 0,                          // filler
    0, 0, 0, 0, 0,                             // filler
    0, 0, 0, 0, "ne", "eq", "gt", "lt", "ge", "le", 0, 0, 0 };

const char * ConditionsRiscV[] = {
    0, 0, 0, 0, 0, 0,                          // filler
    0, 0, 0, 0, 0,                             // filler
    0, 0, 0, 0, "eq", "ne", "le", "ge", "lt", "gt", 0, 0, 0 };

const char * Conditionsm68kCPM[] = {
    0, 0, 0, 0, 0, 0,                          // filler
    0, 0, 0, 0, 0,                             // filler
    0, 0, 0, 0, "eq", "ne", "le", "ge", "lt", "gt", 0, 0, 0 };

const char * ConditionsNotm68kCPM[] = {
    0, 0, 0, 0, 0, 0,                          // filler
    0, 0, 0, 0, 0,                             // filler
    0, 0, 0, 0, "ne", "eq", "gt", "lt", "ge", "le", 0, 0, 0 };

// jump instruction if the condition is true

const char * RelationalInstructionX64[] = { 
    0, 0, 0, 0, 0, 0,                          // filler
    0, 0, 0, 0, 0,                             // filler
    0, 0, 0, 0, "je", "jne", "jle", "jge", "jl", "jg", 0, 0, 0, };

// jump instruction if the condition is false

const char * RelationalNotInstructionX64[] = { 
    0, 0, 0, 0, 0, 0,                          // filler
    0, 0, 0, 0, 0,                             // filler
    0, 0, 0, 0, "jne", "je", "jg", "jl", "jge", "jle", 0, 0, 0, };

const char * CMovInstructionX64[] = { 
    0, 0, 0, 0, 0, 0,                          // filler
    0, 0, 0, 0, 0,                             // filler
    0, 0, 0, 0, "cmove", "cmovne", "cmovle", "cmovge", "cmovl", "cmovg", 0, 0, 0, };

// the most frequently used variables are mapped to these registers

const char * MappedRegistersX64[] = {    "esi", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d" };
const char * MappedRegistersX64_64[] = { "rsi",  "r9",  "r10",  "r11",  "r12",  "r13",  "r14",  "r15" };

// Use of x10-x15 is dangerous since these aren't preserved during function calls.
// Whenever a call happens (to printf, time, etc.) use the macros save_volatile_registers and restore_volatile_registers.
// The macros are slow, but calling out is very uncommon and it's generally to slow functions.
// Use of the extra registers results in about a 3% overall benefit for tp.bas.
// Note that x16, x17, x18, x29, and x30 are reserved.

const char * MappedRegistersArm64[] = {    "w10", "w11", "w12", "w13", "w14", "w15", "w19", "w20", "w21", "w22", "w23", "w24", "w25", "w26", "w27", "w28" };
const char * MappedRegistersArm64_64[] = { "x10", "x11", "x12", "x13", "x14", "x15", "x19", "x20", "x21", "x22", "x23", "x24", "x25", "x26", "x27", "x28" };

// r9 may or not be volatile depending on the Linux ABI
// r3 is volatile; use save_volatile_registers on external calls. To reduce the probability of bugs, I'm leaving r3 out.

const char * MappedRegistersArm32[] = { /*"r3",*/ "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11" };

// x86 registers are a sparse commodity. Make sure no generated code uses these aside from variables.

const char * MappedRegistersX86[] = { "ecx", "esi", "edi" };

const char * MappedRegistersOIOS[] = { "rframe", "rarg1", "rarg2" };

const char * MappedRegistersm68kCPM[] = { "d3", "d4", "d5", "d6", "d7", "a3", "a4", "a5", "a6" };

const char * MappedRegistersRiscV64[] = { "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11" };

void RiscVPush( FILE * fp, char const * pcreg )
{
    // stacks must be 16-byte aligned

    fprintf( fp, "    addi     sp, sp, -16\n" );
    fprintf( fp, "    sd       %s, 0(sp)\n", pcreg );
} //RiscVPush

void RiscVPop( FILE * fp, char const * pcreg )
{
    // stacks must be 16-byte aligned

    fprintf( fp, "    ld       %s, 0(sp)\n", pcreg );
    fprintf( fp, "    addi     sp, sp, 16\n" );
} //RiscVPop

// For 6502 there are no mapped registers, but there are variables allocated on the Zero Page. Each of these consumes 2 bytes.

const int Max6502ZeroPageVariables = 16;

#endif //BA_ENABLE_COMPILER

__makeinline const char * TokenStr( Token i )
{
    if ( i < 0 || i > Token_INVALID )
    {
        printf( "token %d is malformed\n", i );
        return Tokens[ _countof( Tokens ) - 1 ];
    }

    return Tokens[ i ];
} //TokenStr

__makeinline bool isTokenOperator( Token t )
{
    return ( t >= Token_MULT && t <= Token_XOR );
} //isTokenOperator

__makeinline bool isTokenSimpleValue( Token t )
{
    return ( Token_CONSTANT == t || Token_VARIABLE == t );
} //isTokenSimpleValue

__makeinline bool isTokenStatement( Token t )
{
    return ( t >= Token_VARIABLE && t <= Token_END );
} //isTokenStatement

__makeinline bool isOperatorRelational( Token t )
{
    return ( t >= Token_EQ && t <= Token_GT );
} //isOperatorRelational

__makeinline bool isOperatorLogical( Token t )
{
    return ( t >= Token_AND && t <= Token_XOR );
} //isOperatorLogical

__makeinline bool isOperatorAdditive( Token t )
{
    return ( Token_PLUS == t || Token_MINUS == t );
} //isOperatorAdditive

__makeinline bool isOperatorMultiplicative( Token t )
{
    return ( Token_MULT == t || Token_DIV == t );
} //isOperatorMultiplicative

__makeinline bool FailsRangeCheck( int offset, size_t high )
{
    // check if an array access is outside the array. BASIC arrays are 0-based.

    return ( ( offset < 0 ) || ( offset >= high ) );
} //FailsRangeCheck

char * my_strlwr( char * str )
{
    unsigned char *p = (unsigned char *) str;

    while ( *p )
    {
        *p = (unsigned char) tolower( *p );
        p++;
    }
    return str;
}//my_strlwr

string UnescapeBASICString( string & str )
{
    // change two consecutive quotes to one

    string result;

    for ( const char * p = str.c_str(); *p; p++ )
    {
        result += *p;

        if ( '"' == *p && '"' == * (p + 1) )
            p++;
    }

    return result;
} //UnescapeBASICString

struct Variable
{
    Variable( const char * v )
    {
        memset( this, 0, sizeof *this );
        assert( strlen( v ) <= 3 );
        strcpy_s( name, _countof( name ), v );
        my_strlwr( name );
    }

    int value;            // when a scalar
    char name[4];         // variables can only be 2 chars + type (%) + null
    int dimensions;       // 0 for scalar. 1 or 2 for arrays.
    int dims[ 2 ];        // only support up to 2 dimensional arrays.
    vector<int> array;    // actual array values
    int references;       // when generating assembler: how many references in the basic app?
    string reg;           // when generating assembler: register mapped to this variable, if any
    bool mos6502ZeroPage; // true if allocated in the zero page for 6502
};

struct TokenValue
{
    void Clear()
    {
        token = Token_INVALID;
        pVariable = 0;
        value = 0;
        strValue = "";
        dimensions = 0;
        dims[ 0 ] = 0;
        dims[ 1 ] = 0;
        extra = 0;
    }

    TokenValue( Token t )
    {
        Clear();
        token = t;
    } //TokenValue

#ifdef WATCOM
    TokenValue()
    {
        Clear();
    } //TokenValue
#endif

    // note: 64 bytes in size, which is good because the compiler can use shl 6 for array lookups

    Token token;
    int value;             // value's definition varies depending on the token. 
    int dimensions;        // 0 for scalar or 1-2 if an array. Only non-0 for DIM statements
    int dims[ 2 ];         // only support up to 2 dimensional arrays. Only used for DIM statements
    int extra;             // filler for now. unused.
    Variable * pVariable;  // pointer to the actual variable where the value is stored
    string strValue;       // strValue's definition varies depending on the token.
#ifdef __APPLE__           // make structure size 64 bytes on mac/clang. Not needed for linux or Windows
    size_t extra2;
#endif
};

int stcmp( TokenValue const & a, TokenValue const & b )
{
    return stcmp( a.strValue, b.strValue );
} //stcmp

// maps to a line of BASIC

struct LineOfCode
{
    LineOfCode( int line, const char * code ) : 
        lineNumber( line ), firstToken( Token_INVALID ), sourceCode( code ), goTarget( false )

    #ifdef BA_ENABLE_INTERPRETER_EXECUTION_TIME
        , timesExecuted( 0 ), duration( 0 )
    #endif

    {
        tokenValues.reserve( 8 );
    }

#ifdef WATCOM
    LineOfCode() {}
#endif

    // These tokens will be scattered through memory. I tried making them all contiguous
    // and there was no performance benefit

    Token firstToken;                  // optimization: first token in tokenValues.
    vector<TokenValue> tokenValues;    // vector of tokens on the line of code
    string sourceCode;                 // the original BASIC line of code
    int lineNumber;                    // line number in BASIC
    bool goTarget;                     // true if a goto/gosub points at this line.

    #ifdef BA_ENABLE_INTERPRETER_EXECUTION_TIME
        uint64_t timesExecuted;       // # of times this line is executed
        uint64_t duration;            // execution time so far on this line of code
    #endif
};

vector<LineOfCode> g_linesOfCode;
#define g_lineno ( g_linesOfCode[ g_pc ].lineNumber )

struct ForGosubItem
{
    ForGosubItem( int f, int p )
    {
        isFor = f;
        pcReturn = p;
    }

#ifdef WATCOM
    ForGosubItem() {}
#endif

    int isFor;     // true if FOR, false if GOSUB
    int pcReturn;  // where to return in a NEXT or RETURN statment
};

// this is faster than both <stack> and Stack using <vector> to implement a stack because there are no memory allocations.

const int maxStack = 60;

template <class T> class Stack
{
    int current;
#ifdef WATCOM
    T items[ maxStack ]; 
#else
    union { T items[ maxStack ]; };  // avoid constructors and destructors on each T by using a union
#endif

    public:
        __makeinline Stack() : current( 0 ) {}
        __makeinline void push( T const & x ) { assert( current < maxStack ); items[ current++ ] = x; }
        __makeinline size_t size() { return current; }
        __makeinline void pop() { assert( current > 0 ); current--; }
        __makeinline T & top() { assert( current > 0 ); return items[ current - 1 ]; }
        __makeinline T & operator[] ( size_t i ) { return items[ i ]; }
};

class CFile
{
    private:
        FILE * fp;

    public:
        CFile( FILE * file ) : fp( file ) {}
        ~CFile() { Close(); }
        FILE * get() { return fp; }
        void Close()
        {
            if ( NULL != fp )
            {
                fclose( fp );
                fp = NULL;
            }
        }
};

static void Usage()
{
    printf( "command line: " );
    for ( int i = 0; i < g_argc; i++ )
        printf( "%s ", g_argv[ i ] );
    printf( "\n" );
    printf( "Usage: ba [-a] [-e] [-l] [-m] [-p] [-t] [-x] [-8] filename.bas [argvalue]\n" );
    printf( "  Basic interpreter\n" );
    printf( "  Arguments:     filename.bas     Subset of TRS-80 compatible BASIC\n" );
    printf( "                 argvalue         One optional integer argument to the app referenced in basic as av%%\n" );
#ifdef BA_ENABLE_COMPILER
    printf( "                 -a:X             Generate assembly code, where X is one of:\n" );
    printf( "                                  6 -- Generate 8-bit Apple 1 'sbasm30306\\sbasm.py' compatible assembler code to filename.s\n" );
    printf( "                                  8 -- Generate 8-bit CP/M 2.2 i8080 'asm' compatible assembler code to filename.asm\n" );
    printf( "                                  a -- Generate 64-bit arm64 Windows armasm64 compatible assembler code to filename.asm\n" );
    printf( "                                  c -- Generate 32-bit 68000 CP/M 68K as68.68k compatible assembler code to filename.s\n" );
    printf( "                                  d -- Generate 16-bit 8086 DOS ml /AT /omf /c compatible assembler code to filename.asm\n" );
    printf( "                                  3 -- Generate 32-bit Linux arm32 armv8 'gcc / as' compatible assembler code to filename.s\n" );
    printf( "                                  i -- Generate 32-bit i386 (686) Windows x86 'ml' compatible assembler code to filename.asm\n" );
    printf( "                                  I -- Generate 32-bit i386 (386) Windows 98 'ml' compatible assembler code to filename.asm\n" );
    printf( "                                  m -- Generate 64-bit MacOS 'as -arch arm64' compatible assembler code to filename.s\n" );
    printf( "                                  o -- Generate OneImage 'oia' compatible assembler code to filename.s\n" );
    printf( "                                  r -- Generate 64-bit RISC-V 64-bit GNU 'as' compatible assembler code to filename.s\n" );
    printf( "                                  x -- Generate 64-bit Windows x64 'ml64' compatible assembler code to filename.asm\n" );
    printf( "                 -d               Generate a dollar sign $ at the end of execution for Apple 1 apps\n" );
#endif
#ifdef BA_ENABLE_INTERPRETER_EXECUTION_TIME
    printf( "                 -e               Show interpreter execution count and time for each line\n" );
#endif
    printf( "                 -l               Show tokenized listing\n" );
#ifdef BA_ENABLE_COMPILER
    printf( "                 -o               Don't do expression optimization for assembly code\n" );
#endif
    printf( "                 -p               Show parse time for input file\n" );
    printf( "                 -q               Quiet. Don't show start and end messages in interpreter or compiled code\n" );
#ifdef BA_ENABLE_COMPILER
    printf( "                 -r               Don't use registers for variables in assembly code\n" );
#endif
    printf( "                 -t               Show debug tracing\n" );
    printf( "                 -x               Parse only; don't execute the code\n" );
#ifdef BA_ENABLE_COMPILER
    printf( "  notes:  --  Assembly instructions are located at the top of generated files\n" );
#endif
    exit( 1 );
} //Usage

const char * YesNo( bool f )
{
    return f ? "yes" : "no";
} //YesNo

long portable_filelen( FILE * fp )
{
    long current = ftell( fp );
    fseek( fp, 0, SEEK_END );
    long len = ftell( fp );
    fseek( fp, current, SEEK_SET );
    return len;
} //portable_filelen

bool isDigit( char c ) { return c >= '0' && c <= '9'; }
bool isAlpha( char c ) { return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ); }
bool isWhite( char c ) { return ' ' == c || 9 /* tab */ == c; }
bool isToken( char c ) { return isAlpha( c ) || ( '%' == c ); }
bool isOperator( char c ) { return '<' == c || '>' == c || '=' == c; }

__makeinline const char * pastNum( const char * p )
{
    while ( isDigit( *p ) )
        p++;
    return p;
} //pastNum

__makeinline void makelower( string & sl )
{
#ifdef WATCOM
    char * p = (char *) sl.data();
    size_t len = sl.length();
    for ( size_t i = 0; i < len; i++ )
        p[ i ] = tolower( p[ i ] );
#else
    std::transform( sl.begin(), sl.end(), sl.begin(),
                    [](unsigned char c){ return std::tolower(c); });
#endif
} //makelower

Token readTokenInner( const char * p, int & len )
{
    if ( 0 == *p )
    {
        len = 0;
        return Token_INVALID;
    }

    if ( '(' == *p )
    {
        len = 1;
        return Token_OPENPAREN;
    }

    if ( ')' == *p )
    {
        len = 1;
        return Token_CLOSEPAREN;
    }

    if ( ',' == *p )
    {
        len = 1;
        return Token_COMMA;
    }

    if ( ':' == *p )
    {
        len = 1;
        return Token_COLON;
    }

    if ( ';' == *p )
    {
        len = 1;
        return Token_SEMICOLON;
    }

    if ( '*' == *p )
    {
        len = 1;
        return Token_MULT;
    }

    if ( '/' == *p )
    {
        len = 1;
        return Token_DIV;
    }

    if ( '+' == *p )
    {
        len = 1;
        return Token_PLUS;
    }

    if ( '-' == *p )
    {
        len = 1;
        return Token_MINUS;
    }

    if ( '^' == *p )
    {
        len = 1;
        return Token_XOR;
    }

    if ( isDigit( *p ) )
    {
        len = (int) ( pastNum( p ) - p );
        return Token_CONSTANT;
    }

    if ( isOperator( *p ) )
    {
        if ( isOperator( * ( p + 1 ) ) )
        {
            len = 2;
            char c1 = *p;
            char c2 = * ( p + 1 );

            if ( c1 == '<' && c2 == '=' )
                return Token_LE;
            if ( c1 == '>' && c2 == '=' )
                return Token_GE;
            if ( c1 == '<' && c2 == '>' )
                return Token_NE;

            return Token_INVALID;
        }
        else
        {
            len = 1;

            if ( '<' == *p )
                return Token_LT;
            if ( '=' == *p )
                return Token_EQ;
            if ( '>' == *p )
                return Token_GT;

            return Token_INVALID;
        }
    }

    if ( *p == '"' )
    {
        const char * pend = strchr( p + 1, '"' );

        while ( pend && '"' == * ( pend + 1 ) )
            pend = strchr( pend + 2, '"' );

        if ( pend )
        {
            len = 1 + (int) ( pend - p );
            return Token_STRING;
        }

        return Token_INVALID;
    }

    if ( !_strnicmp( p, "TIME$", 5 ) )
    {
       len = 5;
       return Token_TIME;
    }

    if ( !_strnicmp( p, "ELAP$", 5 ) )
    {
        len = 5;
        return Token_ELAP;
    }

    len = 0;
    while ( ( isToken( * ( p + len ) ) ) && len < 10 )
        len++;

    if ( 1 == len && isAlpha( *p ) )
        return Token_VARIABLE; // in the future, this will be true

    if ( 2 == len )
    {
        if ( !_strnicmp( p, "OR", 2 ) )
            return Token_OR;

        if ( !_strnicmp( p, "IF", 2 ) )
            return Token_IF;

        if ( !_strnicmp( p, "TO", 2 ) )
            return Token_TO;

        if ( isAlpha( *p ) && ( '%' == * ( p + 1 ) ) )
            return Token_VARIABLE;
    }
    else if ( 3 == len )
    {
        if ( !_strnicmp( p, "REM", 3 ) )
            return Token_REM;

        if ( !_strnicmp( p, "DIM", 3 ) )
           return Token_DIM;

        if ( !_strnicmp( p, "AND", 3 ) )
           return Token_AND;

        if ( !_strnicmp( p, "FOR", 3 ) )
           return Token_FOR;

        if ( !_strnicmp( p, "END", 3 ) )
           return Token_END;

        if ( isAlpha( *p ) && isAlpha( * ( p + 1 ) ) && ( '%' == * ( p + 2 ) ) )
           return Token_VARIABLE;
    }
    else if ( 4 == len )
    {
        if ( !_strnicmp( p, "GOTO", 4 ) )
           return Token_GOTO;

        if ( !_strnicmp( p, "NEXT", 4 ) )
           return Token_NEXT;

        if ( !_strnicmp( p, "THEN", 4 ) )
           return Token_THEN;

        if ( !_strnicmp( p, "ELSE", 4 ) )
           return Token_ELSE;

        if ( !_strnicmp( p, "TRON", 4 ) )
           return Token_TRON;
    }
    else if ( 5 == len )
    {
        if ( !_strnicmp( p, "GOSUB", 5 ) )
           return Token_GOSUB;

        if ( !_strnicmp( p, "PRINT", 5 ) )
           return Token_PRINT;

        if ( !_strnicmp( p, "TROFF", 5 ) )
           return Token_TROFF;
    }

    else if ( 6 == len )
    {
        if ( !_strnicmp( p, "RETURN", 5 ) )
           return Token_RETURN;

        if ( !_strnicmp( p, "SYSTEM", 5 ) ) // system is the same as end; both exit execution
           return Token_END;
    }

    return Token_INVALID;
} //readTokenInner

__makeinline Token readToken( const char * p, int & len )
{
    Token t = readTokenInner( p, len );

    if ( EnableTracing && g_Tracing )
        printf( "  read token %s from string '%s', length %d\n", TokenStr( t ), p, len );

    return t;
} //readToken

__makeinline int readNum( const char * p )
{
    if ( !isDigit( *p ) )
        return -1;

    return atoi( p );
} //readNum

void Fail( const char * error, size_t line, size_t column, const char * code )
{
    printf( "Error: %s at line %zd column %zd: %s\n", error, line, column, code );
    exit( 1 );
} //Fail

void RuntimeFail( const char * error, size_t line )
{
    printf( "Runtime Error: %s at line %zd\n", error, line );
    exit( 1 );
} //RuntimeFail

__makeinline const char * pastWhite( const char * p )
{
    while ( isWhite( *p ) )
        p++;
    return p;
} //PastWhite

const char * ParseExpression( vector<TokenValue> & lineTokens, const char * pline, const char * line, int fileLine )
{
    if ( EnableTracing && g_Tracing )
        printf( "  parsing expression from '%s'\n", pline );

    bool first = true;
    int parens = 0;
    int tokenCount = 0;
    TokenValue expToken( Token_EXPRESSION );
    expToken.value = 666;
    lineTokens.push_back( expToken );
    size_t exp = lineTokens.size() - 1;
    bool isNegative = false;
    Token prevToken = Token_INVALID;

    do
    {
        int tokenLen = 0;
        pline = pastWhite( pline );
        Token token = readToken( pline, tokenLen );
        Token firstToken = token;
        TokenValue tokenValue( token );
        tokenCount++;
        bool resetFirst = false;

        if ( Token_MINUS == token && first )
        {
            isNegative = true;
            pline += tokenLen;
        }
        else if ( Token_CONSTANT == token )
        {
            tokenValue.value = atoi( pline );
            if ( isNegative )
            {
                tokenValue.value = -tokenValue.value;
                tokenCount--;
                isNegative = false;
            }

            if ( Token_CONSTANT == prevToken )
                Fail( "consecutive constants are a syntax error", fileLine, pline - line, line );

            lineTokens.push_back( tokenValue );
            pline += tokenLen;
        }
        else if ( Token_VARIABLE == token )
        {
            if ( isNegative )
            {
                TokenValue neg( Token_MINUS );
                lineTokens.push_back( neg );
                isNegative = false;
            }

            if ( Token_VARIABLE == prevToken )
                Fail( "consecutive variables are a syntax error", fileLine, pline - line, line );

            tokenValue.strValue.insert( 0, pline, tokenLen );

            if ( '%' != tokenValue.strValue[ tokenValue.strValue.length() - 1 ] )
                Fail( "integer variables must end with a % symbol", fileLine, pline - line, line );

            makelower( tokenValue.strValue );
            lineTokens.push_back( tokenValue );
            pline = pastWhite( pline + tokenLen );
            token = readToken( pline, tokenLen );
            if ( Token_OPENPAREN == token )
            {
                size_t iVarToken = lineTokens.size() - 1;
                lineTokens[ iVarToken ].dimensions = 1;

                tokenCount++;
                tokenValue.Clear();
                tokenValue.token = token;
                lineTokens.push_back( tokenValue );
                pline += tokenLen;

                size_t expression = lineTokens.size();

                pline = ParseExpression( lineTokens, pline, line, fileLine );
                tokenCount += lineTokens[ expression ].value;

                token = readToken( pline, tokenLen );
                if ( Token_COMMA == token )
                {
                    lineTokens[ iVarToken ].dimensions = 2;
                    tokenCount++;
                    tokenValue.Clear();
                    tokenValue.token = token;
                    lineTokens.push_back( tokenValue );
                    pline = pastWhite( pline + tokenLen );

                    size_t subexpression = lineTokens.size();
                    pline = ParseExpression( lineTokens, pline, line, fileLine );
                    tokenCount += lineTokens[ subexpression ].value;

                    pline = pastWhite( pline );
                    token = readToken( pline, tokenLen );
                }

                if ( Token_CLOSEPAREN != token )
                    Fail( "close parenthesis expected", fileLine, pline - line, line );

                tokenCount++;

                tokenValue.Clear();
                tokenValue.token = token;
                lineTokens.push_back( tokenValue );
                pline += tokenLen;
            }
        }
        else if ( Token_STRING == token )
        {
            if ( 1 != tokenCount )
                Fail( "string not expected", fileLine, 0, line );

            tokenValue.strValue.insert( 0, pline + 1, tokenLen - 2 );
            tokenValue.strValue = UnescapeBASICString( tokenValue.strValue );
            lineTokens.push_back( tokenValue );
            pline += tokenLen;
        }
        else if ( isTokenOperator( token ) )
        {
            if ( isTokenOperator( prevToken ) )
            {
                printf( "previous token %s, current token %s\n", Tokens[ prevToken ], Tokens[ token ] );
                Fail( "consecutive operators are a syntax error", fileLine, pline - line, line );
            }

            lineTokens.push_back( tokenValue );
            pline += tokenLen;
            resetFirst = true;
        }
        else if ( Token_OPENPAREN == token )
        {
            if ( isNegative )
            {
                TokenValue neg( Token_MINUS );
                lineTokens.push_back( neg );
                isNegative = false;
            }

            parens++;
            lineTokens.push_back( tokenValue );
            pline += tokenLen;
            resetFirst = true;
        }
        else if ( Token_CLOSEPAREN == token )
        {
            if ( 0 == parens )
                break;

            parens--;
            lineTokens.push_back( tokenValue );
            pline += tokenLen;
            resetFirst = true;
            isNegative = false;
        }
        else if ( Token_TIME == token )
        {
            lineTokens.push_back( tokenValue );
            pline += tokenLen;
        }
        else if ( Token_ELAP == token )
        {
            lineTokens.push_back( tokenValue );
            pline += tokenLen;
        }
        else if ( Token_INVALID == token && 0 != tokenLen )
        {
            Fail( "invalid token", fileLine, pline - line, line );
        }
        else
        {
            break;
        }

        pline = pastWhite( pline );

        first = resetFirst;
        prevToken = firstToken;
    } while( true );

    if ( 0 != parens )
        Fail( "unbalanced parenthesis count", fileLine, 0, line );

    // Don't create empty expressions. Well-formed basic programs won't do this,
    // but ancient versions of the interpreter allow it, so I do too.

    if ( 1 == tokenCount )
    {
        TokenValue tokenValue( Token_CONSTANT );
        lineTokens.push_back( tokenValue );
        tokenCount++;
    }

    lineTokens[ exp ].value = tokenCount;

    return pline;
} //ParseExpression

const char * ParseStatements( Token token, vector<TokenValue> & lineTokens, const char * pline, const char * line, int fileLine )
{
    if ( EnableTracing && g_Tracing )
        printf( "  parsing statements from '%s' token %s\n", pline, TokenStr( token ) );

    do
    {
        if ( EnableTracing && g_Tracing )
            printf( "  top of ParseStatements, token %s\n", TokenStr( token ) );

        if ( !isTokenStatement( token ) )
            Fail( "expected statement", fileLine, 1 + pline - line , line );

        TokenValue tokenValue( token );
        int tokenLen = 0;
        token = readToken( pline, tokenLen ); // redundant read to get length

        if ( EnableTracing && g_Tracing )
            printf( "  ParseStatements loop read top-level token %s\n", TokenStr( token ) );

        if ( Token_VARIABLE == token )
        {
            tokenValue.strValue.insert( 0, pline, tokenLen );

            if ( '%' != tokenValue.strValue[ tokenValue.strValue.length() - 1 ] )
                Fail( "integer variables must end with a % symbol", fileLine, 0, line );

            makelower( tokenValue.strValue );
            lineTokens.push_back( tokenValue );
            size_t iVarToken = lineTokens.size() - 1;

            pline = pastWhite( pline + tokenLen );
            token = readToken( pline, tokenLen );

            if ( Token_OPENPAREN == token )
            {
                lineTokens[ iVarToken ].dimensions++;

                tokenValue.Clear();
                tokenValue.token = token;
                lineTokens.push_back( tokenValue );

                pline = pastWhite( pline + tokenLen );
                pline = ParseExpression( lineTokens, pline, line, fileLine );

                token = readToken( pline, tokenLen );
                if ( Token_CLOSEPAREN == token )
                {
                    tokenValue.Clear();
                    tokenValue.token = token;
                    lineTokens.push_back( tokenValue );
                }
                else if ( Token_COMMA == token )
                {
                    lineTokens[ iVarToken ].dimensions++;

                    tokenValue.Clear();
                    tokenValue.token = token;
                    lineTokens.push_back( tokenValue );

                    pline = pastWhite( pline + tokenLen );
                    pline = ParseExpression( lineTokens, pline, line, fileLine );

                    pline = pastWhite( pline );
                    token = readToken( pline, tokenLen );

                    if ( Token_CLOSEPAREN == token )
                    {
                        tokenValue.Clear();
                        tokenValue.token = token;
                        lineTokens.push_back( tokenValue );
                    }
                    else
                        Fail( "expected ')' in array access", fileLine, 1 + pline - line , line );
                }
                else
                    Fail( "expected ')' or ',' in array access", fileLine, 1 + pline - line , line );

                pline = pastWhite( pline + tokenLen );
                token = readToken( pline, tokenLen );
            }

            if ( Token_EQ == token )
            {
                tokenValue.Clear();
                tokenValue.token = token;
                lineTokens.push_back( tokenValue );

                pline = pastWhite( pline + tokenLen );
                pline = ParseExpression( lineTokens, pline, line, fileLine );
            }
            else
                Fail( "expected '=' after a variable reference", fileLine, 1 + pline - line , line );
        }
        else if ( Token_GOSUB == token )
        {
            pline = pastWhite( pline + tokenLen );
            token = readToken( pline, tokenLen );
            if ( Token_CONSTANT == token )
            {
                tokenValue.value = atoi( pline );
                lineTokens.push_back( tokenValue );
            }
            else
                Fail( "expected a line number constant with GOSUB", fileLine, 1 + pline - line, line );

            pline += tokenLen;
        }
        else if ( Token_GOTO == token )
        {
            pline = pastWhite( pline + tokenLen );
            token = readToken( pline, tokenLen );
            if ( Token_CONSTANT == token )
            {
                tokenValue.value = atoi( pline );
                lineTokens.push_back( tokenValue );
            }
            else
                Fail( "expected a line number constant with GOTO", fileLine, 1 + pline - line, line );

            pline += tokenLen;
        }
        else if ( Token_END == token )
        {
            lineTokens.push_back( tokenValue );
            pline += tokenLen;
        }
        else if ( Token_RETURN == token )
        {
            lineTokens.push_back( tokenValue );
            pline += tokenLen;
        }
        else if ( Token_PRINT == token )
        {
            lineTokens.push_back( tokenValue );
            size_t iPrintToken = lineTokens.size() - 1;

            pline = pastWhite( pline + tokenLen );

            do
            {
                pline = ParseExpression( lineTokens, pline, line, fileLine );
                pline = pastWhite( pline );
                token = readToken( pline, tokenLen );

                if ( EnableTracing && g_Tracing )
                    printf( "  in Token_PRINT processing read token %s\n", TokenStr( token ) );

                if ( Token_SEMICOLON == token )
                {
                    pline = pastWhite( pline + tokenLen );
                    if ( 0 == *pline )
                    {
                        lineTokens[ iPrintToken ].extra = 1; // indicate that no CR/LF should be printed since PRINT ended in semicolon
                        break;
                    }
                    continue;
                }
                else if ( Token_ELSE == token )
                    break;
                else if ( Token_INVALID != token )
                    Fail( "unexpected PRINT arguments", fileLine, 1 + pline - line, line );
                else
                    break;
            } while( true );
        }

        pline = pastWhite( pline );
        token = readToken( pline, tokenLen );

        if ( Token_COLON == token )
        {
            pline = pastWhite( pline + tokenLen );
            token = readToken( pline, tokenLen );
        }
        else
            break;
    } while( true );

    return pline;
} //ParseStatements

__makeinline Variable * FindVariable( map<string, Variable> const & varmap, string const & name )
{
    // cast away const because the iterator requires non-const. yuck.

    map<string, Variable> &vm = ( map<string, Variable> & ) varmap;

    map<string,Variable>::iterator it;
    it = vm.find( name );
    if ( it == vm.end() )
        return 0;

    return & it->second;
} //FindVariable

__makeinline Variable * GetVariablePerhapsCreate( TokenValue & val, map<string, Variable>  & varmap )
{
    Variable *pvar = val.pVariable;
    if ( pvar )
        return pvar;

    if ( !pvar )
    {
        pvar = FindVariable( varmap, val.strValue );
        val.pVariable = pvar;
    }

    if ( !pvar )
    {
        Variable var( val.strValue.c_str() );
#ifdef WATCOM
        varmap.insert( std::pair<string, Variable> ( var.name, var ) );
#else
        varmap.emplace( var.name, var );
#endif
        pvar = FindVariable( varmap, var.name );
        val.pVariable = pvar;
    }

    return pvar;
} //GetVariablePerhapsCreate

__makeinline int GetSimpleValue( TokenValue const & val )
{
    assert( isTokenSimpleValue( val.token ) );

    if ( Token_CONSTANT == val.token )
        return val.value;

    assert( 0 != val.pVariable );
    return val.pVariable->value;
} //GetSimpleValue

__makeinline int run_operator( int a, Token t, int b )
{
    // in order of actual usage when running ttt

    switch( t )
    {
        case Token_EQ    : return ( a == b );
        case Token_AND   : return ( a & b );
        case Token_LT    : return ( a < b );
        case Token_GT    : return ( a > b );
        case Token_GE    : return ( a >= b );
        case Token_MINUS : return ( a - b );
        case Token_LE    : return ( a <= b );
        case Token_OR    : return ( a | b );
        case Token_PLUS  : return ( a + b );
        case Token_NE    : return ( a != b );
        case Token_MULT  : return ( a * b );
        case Token_DIV   : return ( a / b );
        case Token_XOR   : return ( a ^ b );
        default: __assume( false );
    }

    assert( !"invalid operator token" );
    return 0;
} //run_operator

__makeinline int run_operator_logical( int a, Token t, int b )
{
    switch( t )
    {
        case Token_AND   : return ( a & b );
        case Token_OR    : return ( a | b );
        case Token_XOR   : return ( a ^ b );
        default: __assume( false );
    }

    assert( !"invalid logical operator token" );
    return 0;
} //run_operator_p3

__makeinline int run_operator_relational( int a, Token t, int b )
{
    switch( t )
    {
        case Token_EQ    : return ( a == b );
        case Token_LT    : return ( a < b );
        case Token_NE    : return ( a != b );
        case Token_GT    : return ( a > b );
        case Token_GE    : return ( a >= b );
        case Token_LE    : return ( a <= b );
        default: __assume( false );
    }

    assert( !"invalid relational operator token" );
    return 0;
} //run_operator_relational

__makeinline int run_operator_additive( int a, Token t, int b )
{
    switch( t )
    {
        case Token_PLUS  : return ( a + b );
        case Token_MINUS : return ( a - b );
        default: __assume( false );
    }

    assert( !"invalid additive operator token" );
    return 0;
} //run_operator_additive

__makeinline int run_operator_multiplicative( int a, Token t, int b )
{
    switch( t )
    {
        case Token_MULT  : return ( a * b );
        case Token_DIV   : return ( a / b );
        default: __assume( false );
    }

    assert( !"invalid multiplicative operator token" );
    return 0;
} //run_operator_multiplicative

int EvaluateExpression( int & iToken, int beyond, vector<TokenValue> const & vals );
int EvaluateFactor( int & iToken, int beyond, vector<TokenValue> const & vals );
int EvaluateTerm( int & iToken, int beyond, vector<TokenValue> const & vals );

__makeinline int EvaluateMultiplicative( int & iToken, int beyond, vector<TokenValue> const & vals, int leftValue )
{
    assert( iToken < beyond );
    Token op = vals[ iToken ].token;
    iToken++;

    int rightValue = EvaluateFactor( iToken, beyond, vals );

    return run_operator_multiplicative( leftValue, op, rightValue );
} //EvaluateMultiplicative

int EvaluateTerm( int & iToken, int beyond, vector<TokenValue> const & vals )
{
    assert( iToken < beyond );
    assert( iToken < vals.size() );

    if ( EnableTracing && g_Tracing )
        printf( "Evaluate term # %d, %s\n", iToken, TokenStr( vals[ iToken ].token ) );

    int value = EvaluateFactor( iToken, beyond, vals );

    if ( iToken >= beyond )
        return value;

    Token t = vals[ iToken ].token;

    while ( isOperatorMultiplicative( t ) )
    {
        value = EvaluateMultiplicative( iToken, beyond, vals, value );

        if ( iToken >= beyond )
            break;

        t = vals[ iToken ].token;

        if ( EnableTracing && g_Tracing )
            printf( "next token  %d in EvaluateTerm: %d\n", iToken, t );
    }

    if ( EnableTracing && g_Tracing )
        printf( "Evaluate term returning %d\n", value );
    return value;
} //EvaluateTerm

int EvaluateFactor( int & iToken, int beyond, vector<TokenValue> const & vals )
{
    if ( EnableTracing && g_Tracing )
        printf( " Evaluate factor # %d, %s\n", iToken, TokenStr( vals[ iToken ].token ) );

    size_t limit = iToken + vals.size();
    int value = 0;

    if ( iToken < beyond )
    {
        Token t = vals[ iToken ].token;

        if ( Token_EXPRESSION == t )
        {
            iToken++;
            t = vals[ iToken ].token;
        }

        if ( Token_OPENPAREN == t )
        {
            iToken++;
            value = EvaluateExpression( iToken, beyond, vals );
            if ( EnableTracing && g_Tracing )
                printf( " after evaluateexpression, iToken: %d\n", iToken );

            assert( Token_CLOSEPAREN == vals[ iToken ].token );
            iToken++;
        }
        else if ( Token_VARIABLE == t )
        {
            Variable *pvar = vals[ iToken ].pVariable;

            if ( 0 == pvar->dimensions )
            {
                value = pvar->value;
                iToken++;

                if ( iToken < vals.size() && Token_OPENPAREN == vals[ iToken ].token )
                    RuntimeFail( "scalar variable used as an array", g_lineno );
            }
            else if ( 1 == pvar->dimensions )
            {
                iToken++; // variable

                if ( Token_OPENPAREN != vals[ iToken ].token )
                    RuntimeFail( "open parenthesis expected", g_lineno );

                iToken++; // open paren

                assert( Token_EXPRESSION == vals[ iToken ].token );

                int offset;
                if ( 2 == vals[ iToken ].value && Token_CONSTANT == vals[ iToken + 1 ].token ) // save recursion
                {
                    offset = vals[ iToken + 1 ].value;
                    iToken += vals[ iToken ].value;
                }
                else
                    offset = EvaluateExpression( iToken, iToken + vals[ iToken ].value, vals );

                if ( RangeCheckArrays && FailsRangeCheck( offset, pvar->array.size() ) )
                    RuntimeFail( "access of array beyond end", g_lineno );

                if ( RangeCheckArrays && iToken < limit && Token_COMMA == vals[ t ].token )
                    RuntimeFail( "accessed 1-dimensional array with 2 dimensions", g_lineno );

                value = pvar->array[ offset ];
                iToken++; // closing paren
            }
            else if ( 2 == pvar->dimensions )
            {
                iToken++; // variable

                if ( Token_OPENPAREN != vals[ iToken ].token )
                    RuntimeFail( "open parenthesis expected", g_lineno );

                iToken++; // open paren

                assert( Token_EXPRESSION == vals[ iToken ].token );
                int offset1 = EvaluateExpression( iToken, iToken + vals[ iToken ].value, vals );

                if ( RangeCheckArrays && FailsRangeCheck( offset1, pvar->dims[ 0 ] ) )
                    RuntimeFail( "access of first dimension in 2-dimensional array beyond end", g_lineno );

                if ( Token_COMMA != vals[ iToken ].token )
                    RuntimeFail( "comma expected for 2-dimensional array", g_lineno );

                iToken++; // comma

                assert( Token_EXPRESSION == vals[ iToken ].token );
                int offset2 = EvaluateExpression( iToken, iToken + vals[ iToken ].value, vals );

                if ( RangeCheckArrays && FailsRangeCheck( offset2, pvar->dims[ 1 ] ) )
                    RuntimeFail( "access of second dimension in 2-dimensional array beyond end", g_lineno );

                int arrayoffset = offset1 * pvar->dims[ 1 ] + offset2;
                assert( arrayoffset < pvar->array.size() );

                value = pvar->array[ arrayoffset ];

                iToken++; // closing paren
            }
        }
        else if ( Token_CONSTANT == t )
        {
            value = vals[ iToken ].value;
            iToken++;
        }
        else if ( Token_CLOSEPAREN == t )
        {
            assert( false && "why is there a close paren?" );
            iToken++;
        }
        else if ( Token_NOT == t )
        {
            iToken++;

            assert( Token_VARIABLE == vals[ iToken ].token );

            Variable *pvar = vals[ iToken ].pVariable;
            value = ! ( pvar->value );

            iToken ++;
        }
        else
        {
            printf( "unexpected token in EvaluateFactor %d %s\n", t, TokenStr( t ) );
            RuntimeFail( "unexpected token", g_lineno );
        }
    }

    if ( EnableTracing && g_Tracing )
        printf( " leaving EvaluateFactor, value %d\n", value );
    return value;
} //EvaluateFactor

__makeinline int EvaluateAdditive( int & iToken, int beyond, vector<TokenValue> const & vals, int valueLeft )
{
    if ( EnableTracing && g_Tracing )
        printf( "in Evaluate add, iToken %d\n", iToken );

    Token op = vals[ iToken ].token;
    iToken++;

    int valueRight = EvaluateTerm( iToken, beyond, vals );

    return run_operator_additive( valueLeft, op, valueRight );
} //EvaluateAdditive

int EvaluateExpression( int & iToken, int beyond, vector<TokenValue> const & vals )
{
    assert( iToken < beyond );
    assert( iToken < vals.size() );

    if ( EnableTracing && g_Tracing )
    {
        printf( "Evaluate expression for line %d token # %d %s, beyond %d\n", g_lineno, iToken, TokenStr( vals[ iToken ].token ), beyond );

        for ( int i = iToken; i < vals.size(); i++ )
            printf( "    %d:    %s\n", i, TokenStr( vals[ i ].token ) );
    }

    if ( Token_EXPRESSION == vals[ iToken ].token )
        iToken++;

    // look for a unary + or -
    int value = 0;

    if ( isOperatorAdditive( vals[ iToken ].token ) )
    {
        // make the left side of the operation 0
    }
    else
    {
        value = EvaluateTerm( iToken, beyond, vals );

        if ( iToken >= beyond )
            return value;
    }

    Token t = vals[ iToken ].token;

    while ( isOperatorAdditive( t ) )
    {
        value = EvaluateAdditive( iToken, beyond, vals, value );

        if ( iToken >= beyond )
            break;

        t = vals[ iToken ].token;
    }

    if ( EnableTracing && g_Tracing )
        printf( " leaving EvaluateExpression, value %d\n", value );
    return value;
} //EvaluateExpression

__makeinline int EvaluateRelational( int & iToken, int beyond, vector<TokenValue> const & vals, int leftValue )
{
    assert( iToken < beyond );
    assert( iToken < vals.size() );

    if ( EnableTracing && g_Tracing )
        printf( "in Evaluate relational, iToken %d\n", iToken );

    Token op = vals[ iToken ].token;
    iToken++;

    int rightValue = EvaluateExpression( iToken, beyond, vals );

    int value = run_operator_relational( leftValue, op, rightValue );

    if ( EnableTracing && g_Tracing )
        printf( " leaving EvaluateRelational, value %d\n", value );
    return value;
} //EvaluateRelational

__makeinline int EvaluateRelationalExpression( int & iToken, int beyond, vector<TokenValue> const & vals )
{
    assert( iToken < beyond );
    assert( iToken < vals.size() );

    if ( EnableTracing && g_Tracing )
    {
        printf( "Evaluate relational expression for line %d token # %d %s\n", g_lineno, iToken, TokenStr( vals[ iToken ].token ) );

        for ( int i = iToken; i < beyond; i++ )
            printf( "    %d:    %s\n", i, TokenStr( vals[ i ].token ) );
    }

    // This won't be an EXPRESSION for cases like x = x + ...
    // But it will be EXPRESSION when called from EvaluateLogicalExpression

    if ( Token_EXPRESSION == vals[ iToken ].token )
        iToken++;

    int value = EvaluateExpression( iToken, beyond, vals );

    if ( iToken >= vals.size() )
        return value;

    Token t = vals[ iToken ].token;

    while ( isOperatorRelational( t ) )
    {
        value = EvaluateRelational( iToken, beyond, vals, value );

        if ( iToken >= beyond )
            break;

        t = vals[ iToken ].token;
    }

    if ( EnableTracing && g_Tracing )
        printf( " leaving EvaluateRelationalExpression, value %d\n", value );
    return value;
} //EvaluateRelationalExpression

__makeinline int EvaluateLogical( int & iToken, int beyond, vector<TokenValue> const & vals, int leftValue )
{
    assert( iToken < beyond );
    assert( iToken < vals.size() );

    if ( EnableTracing && g_Tracing )
        printf( "in Evaluate logical, iToken %d\n", iToken );

    Token op = vals[ iToken ].token;
    iToken++;

    int rightValue = EvaluateRelationalExpression( iToken, beyond, vals );

    assert( isOperatorLogical( op ) );

    int value = run_operator_logical( leftValue, op, rightValue );

    if ( EnableTracing && g_Tracing )
        printf( " leaving EvaluateLogical, value %d\n", value );
    return value;
} //EvaluateLogical

__makeinline int EvaluateLogicalExpression( int & iToken, vector<TokenValue> const & vals )
{
    int beyond = iToken + vals[ iToken ].value;

    assert( iToken < beyond );
    assert( beyond <= vals.size() );
    assert( iToken < vals.size() );

    if ( EnableTracing && g_Tracing )
    {
        printf( "Evaluate logical expression for line %d token # %d %s\n", g_lineno, iToken, TokenStr( vals[ iToken ].token ) );

        for ( int i = iToken; i < beyond; i++ )
            printf( "    %d:    %s\n", i, TokenStr( vals[ i ].token ) );
    }

    assert( Token_EXPRESSION == vals[ iToken ].token );

    int value = EvaluateRelationalExpression( iToken, beyond, vals );

    if ( iToken >= beyond )
        return value;

    Token t = vals[ iToken ].token;

    while ( isOperatorLogical( t ) )
    {
        value = EvaluateLogical( iToken, beyond, vals, value );

        if ( iToken >= beyond )
            break;

        t = vals[ iToken ].token;
    }

    if ( EnableTracing && g_Tracing )
        printf( " leaving EvaluateLogicalExpression, value %d\n", value );
    return value;
} //EvaluateLogicalExpression

__makeinline int EvaluateExpressionOptimized( int & iToken, vector<TokenValue> const & vals )
{
    if ( EnableTracing && g_Tracing )
        printf( "EvaluateExpressionOptimized starting at line %d, token %d, which is %s, length %d\n",
                g_lineno, iToken, TokenStr( vals[ iToken ].token ), vals[ iToken ].value );

    assert( Token_EXPRESSION == vals[ iToken ].token );

    int value;
    int tokenCount = vals[ iToken ].value;

    #ifdef DEBUG
        int beyond = iToken + tokenCount;
    #endif

    // implement a few specialized/optimized cases in order of usage

    if ( 2 == tokenCount )
    {
        value = GetSimpleValue( vals[ iToken + 1 ] );
        iToken += tokenCount;
    }
    else if ( 6 == tokenCount &&
              Token_VARIABLE == vals[ iToken + 1 ].token &&
              Token_OPENPAREN == vals[ iToken + 2 ].token )
    {
        // 0 EXPRESSION, value 6, strValue ''
        // 1 VARIABLE, value 0, strValue 'sa%'
        // 2 OPENPAREN, value 0, strValue ''
        // 3 EXPRESSION, value 2, strValue ''
        // 4 VARIABLE, value 0, strValue 'st%'   (this can optionally be a constant)
        // 5 CLOSEPAREN, value 0, strValue ''

        Variable *pvar = vals[ iToken + 1 ].pVariable;
        assert( pvar && "array variable doesn't exist yet somehow" );

        if ( 1 != pvar->dimensions ) // can't be > 1 or tokenCount would be greater
            RuntimeFail( "scalar variable used as an array", g_lineno );

        int offset = GetSimpleValue( vals[ iToken + 4 ] );
        if ( RangeCheckArrays && FailsRangeCheck( offset, pvar->dims[ 0 ] ) )
            RuntimeFail( "index beyond the bounds of an array", g_lineno );

        value = pvar->array[ offset ];
        iToken += tokenCount;
    }
    else if ( 4 == tokenCount )
    {
        assert( isTokenSimpleValue( vals[ iToken + 1 ].token ) );
        assert( isTokenOperator( vals[ iToken + 2 ].token ) );
        assert( isTokenSimpleValue( vals[ iToken + 3 ].token ) );
    
        value = run_operator( GetSimpleValue( vals[ iToken + 1 ] ),
                              vals[ iToken + 2 ].token,
                              GetSimpleValue( vals[ iToken + 3 ] ) );
        iToken += tokenCount;
    }
    else if ( 16 == tokenCount &&
              Token_VARIABLE == vals[ iToken + 1 ].token &&
              Token_OPENPAREN == vals[ iToken + 4 ].token &&
              Token_CONSTANT == vals[ iToken + 6 ].token &&
              Token_VARIABLE == vals[ iToken + 9 ].token &&
              Token_OPENPAREN == vals[ iToken + 12 ].token &&
              Token_CONSTANT == vals[ iToken + 14 ].token &&
              isOperatorLogical( vals[ iToken + 8 ].token ) &&
              isOperatorRelational( vals[ iToken + 2 ].token ) &&
              isOperatorRelational( vals[ iToken + 10 ].token ) )
    {
        //  0 EXPRESSION, value 16, strValue ''
        //  1 VARIABLE, value 0, strValue 'wi%'
        //  2 EQ, value 0, strValue ''
        //  3 VARIABLE, value 0, strValue 'b%'
        //  4 OPENPAREN, value 0, strValue ''
        //  5 EXPRESSION, value 2, strValue ''
        //  6 CONSTANT, value 5, strValue ''
        //  7 CLOSEPAREN, value 0, strValue ''
        //  8 AND, value 0, strValue ''
        //  9 VARIABLE, value 0, strValue 'wi%'
        // 10 EQ, value 0, strValue ''
        // 11 VARIABLE, value 0, strValue 'b%'
        // 12 OPENPAREN, value 0, strValue ''
        // 13 EXPRESSION, value 2, strValue ''
        // 14 CONSTANT, value 8, strValue ''
        // 15 CLOSEPAREN, value 0, strValue ''

        // crazy optimization just for ttt that yields a 10% overall win.
        // this is unlikely to help any other BASIC app.

        if ( RangeCheckArrays )
        {
            if ( FailsRangeCheck( vals[ iToken + 6 ].value, vals[ iToken + 3 ].pVariable->array.size() ) ||
                 FailsRangeCheck( vals[ iToken + 14 ].value, vals[ iToken + 11 ].pVariable->array.size() ) )
                RuntimeFail( "index beyond the bounds of an array", g_lineno );

            if ( ( 1 != vals[ iToken + 3 ].pVariable->dimensions ) ||
                 ( 1 != vals[ iToken + 11 ].pVariable->dimensions ) )
                RuntimeFail( "variable used as if it has one array dimension when it does not", g_lineno );
        }

        value = run_operator_logical( run_operator_relational( vals[ iToken + 1 ].pVariable->value,
                                                               vals[ iToken + 2 ].token,
                                                               vals[ iToken + 3 ].pVariable->array[ vals[ iToken + 6 ].value ] ),
                                      vals[ iToken + 8 ].token,
                                      run_operator_relational( vals[ iToken + 9 ].pVariable->value,
                                                               vals[ iToken + 10 ].token,
                                                               vals[ iToken + 11 ].pVariable->array[ vals[ iToken + 14 ].value ] ) );
        iToken += tokenCount;
    }
    else if ( 3 == tokenCount )
    {
        if ( Token_NOT == vals[ iToken + 1 ].token )
            value = ! ( vals[ iToken + 2 ].pVariable->value );
        else
        {
            assert( Token_MINUS == vals[ iToken + 1 ].token );
            value = - GetSimpleValue( vals[ iToken + 2 ] );
        }
        iToken += tokenCount;
    }
    else
    {
        // for anything not optimized, fall through to the generic implementation

        value = EvaluateLogicalExpression( iToken, vals );
    }

    if ( EnableTracing && g_Tracing )
        printf( "returning expression value %d\n", value );

    #ifdef DEBUG
    assert( iToken == beyond );
    #endif

    return value;
} //EvaluateExpressionOptimized

void PrintNumberWithCommas( char *pchars, long long n )
{
    if ( n < 0 )
    {
        sprintf( pchars, "-" );
        PrintNumberWithCommas( pchars, -n );
        return;
    }

    if ( n < 1000 )
    {
        sprintf( pchars + strlen( pchars ), "%lld", n );
        return;
    }

    PrintNumberWithCommas( pchars, n / 1000 );
    sprintf( pchars + strlen( pchars ), ",%03lld", n % 1000 );
} //PrintNumberWithCommas

void ShowLocListing( LineOfCode & loc )
{
    printf( "line %d has %zd tokens  ====>> %s\n", loc.lineNumber, loc.tokenValues.size(), loc.sourceCode.c_str() );
    
    for ( size_t t = 0; t < loc.tokenValues.size(); t++ )
    {
        TokenValue & tv = loc.tokenValues[ t ];
        printf( "  token %3zd %s, value %d, strValue '%s'",
                t, TokenStr( tv.token ), tv.value, tv.strValue.c_str() );
    
        if ( Token_DIM == tv.token )
        {
            printf( " dimensions: %d, length: ", tv.dimensions );
            for ( int d = 0; d < tv.dimensions; d++ )
                printf( " %d", tv.dims[ d ] );
        }
        else if ( Token_FOR == tv.token && 1 == loc.tokenValues[ t ].extra )
            printf( " simple for loop" );
    
        printf( "\n" );
    }
} //ShowLocListing

void RemoveREMStatements()
{
    // Also, remove lines with no statements
    // 1st pass: move goto/gosub targets to the first following non-REM statement 

    for ( size_t l = 0; l < g_linesOfCode.size(); l++ )
    {
        LineOfCode & loc = g_linesOfCode[ l ];

        for ( size_t t = 0; t < loc.tokenValues.size(); t++ )
        {
            TokenValue & tv = loc.tokenValues[ t ];

            if ( Token_GOTO == tv.token || Token_GOSUB == tv.token )
            {
                for ( size_t lo = 0; lo < g_linesOfCode.size(); lo++ )
                {
                    if ( ( g_linesOfCode[ lo ].lineNumber == tv.value ) &&
                         ( ( 0 == g_linesOfCode[ lo ].tokenValues.size() ) ||
                           ( Token_REM == g_linesOfCode[ lo ].tokenValues[ 0 ].token ) ) )
                    {
                        // look for the next statement that's not REM

                        bool foundOne = false;
                        for ( size_t h = lo + 1; h < g_linesOfCode.size(); h++ )
                        {
                            if ( Token_REM != g_linesOfCode[ h ].tokenValues[ 0 ].token )
                            {
                                foundOne = true;
                                tv.value = g_linesOfCode[ h ].lineNumber;
                                break;
                            }
                        }

                        // There is always an END statement, so we'll find one

                        assert( foundOne );
                        break;
                    }
                }
            }
        }
    }

    // 2nd pass: remove all REM statements

    size_t endloc = g_linesOfCode.size();
    size_t curloc = 0;

    while ( curloc < endloc )
    {
        LineOfCode & loc = g_linesOfCode[ curloc ];

        if ( ( 0 == loc.tokenValues.size() ) || ( Token_REM == loc.tokenValues[ 0 ].token ) )
        {
            g_linesOfCode.erase( g_linesOfCode.begin() + curloc );
            endloc--;
        }
        else
            curloc++;
    }
} //RemoveREMStatements

void AddENDStatement()
{
    bool addEnd = true;

    if ( g_linesOfCode.size() && Token_END == g_linesOfCode[ g_linesOfCode.size() - 1 ].tokenValues[ 0 ].token )
        addEnd = false;

    if ( addEnd )
    {
        int linenumber = 1 + g_linesOfCode[ g_linesOfCode.size() - 1 ].lineNumber;
        LineOfCode loc( linenumber, "2000000000 end" );
        g_linesOfCode.push_back( loc );
        TokenValue tokenValue( Token_END );
        g_linesOfCode[ g_linesOfCode.size() - 1 ].tokenValues.push_back( tokenValue );
    }
} //AddENDStatement

void PatchGotoAndGosubNumbers()
{
    // patch goto/gosub line numbers with actual offsets to remove runtime searches
    // also, pull out the first token for better memory locality

    for ( size_t l = 0; l < g_linesOfCode.size(); l++ )
    {
        LineOfCode & loc = g_linesOfCode[ l ];
    
        for ( size_t t = 0; t < loc.tokenValues.size(); t++ )
        {
            TokenValue & tv = loc.tokenValues[ t ];
            bool found = false;

            if ( Token_GOTO == tv.token || Token_GOSUB == tv.token )
            {
                for ( size_t lo = 0; lo < g_linesOfCode.size(); lo++ )
                {
                    if ( g_linesOfCode[ lo ].lineNumber == tv.value )
                    {
                        tv.value = (int) lo;
                        found = true;
                        g_linesOfCode[ lo ].goTarget = true;
                        break;
                    }
                }

                if ( !found )
                {
                    printf( "Error: statement %s referenced undefined line number %d\n", TokenStr( tv.token ), tv.value );
                    exit( 1 );
                }
            }
        }
    }
} //PatchGotoAndGosubNumbers

void OptimizeWithRewrites( bool showListing )
{
    for ( size_t l = 0; l < g_linesOfCode.size(); l++ )
    {
        LineOfCode & loc = g_linesOfCode[ l ];
        vector<TokenValue> & vals = loc.tokenValues;
        bool rewritten = false;

        if ( 0 == vals.size() )
            continue;

        // line 150 has 5 tokens  ====>> 150 for x% = 2 to n%
        //   token   0 FOR, value 0, strValue 'x%'
        //   token   1 EXPRESSION, value 2, strValue ''
        //   token   2 CONSTANT, value 2, strValue ''
        //   token   3 EXPRESSION, value 2, strValue ''
        //   token   4 VARIABLE, value 0, strValue 'n%'
        // line 160 has 8 tokens  ====>> 160 a%( x% ) = 1
        //   token   0 VARIABLE, value 0, strValue 'a%'
        //   token   1 OPENPAREN, value 0, strValue ''
        //   token   2 EXPRESSION, value 2, strValue ''
        //   token   3 VARIABLE, value 0, strValue 'x%'
        //   token   4 CLOSEPAREN, value 0, strValue ''
        //   token   5 EQ, value 0, strValue ''
        //   token   6 EXPRESSION, value 2, strValue ''
        //   token   7 CONSTANT, value 1, strValue ''
        // line 170 has 1 tokens  ====>> 170 next x%
        //   token   0 NEXT, value 0, strValue 'x%'

        if ( Token_FOR == vals[ 0 ].token &&
             5 == vals.size() &&
             Token_CONSTANT == vals[ 2 ].token &&
             ( Token_VARIABLE == vals[ 4 ].token || Token_CONSTANT == vals[ 4 ].token ) )
        {
            LineOfCode & loc1 = g_linesOfCode[ l + 1 ];
            vector<TokenValue> & vals1 = loc1.tokenValues;

            if ( 8 == vals1.size() &&
                 Token_VARIABLE == vals1[ 0 ].token &&
                 Token_OPENPAREN == vals1[ 1 ].token &&
                 Token_EQ == vals1[ 5 ].token &&
                 Token_CONSTANT == vals1[ 7 ].token )
            {
                LineOfCode & loc2 = g_linesOfCode[ l + 2 ];
                vector<TokenValue> & vals2 = loc2.tokenValues;

                if ( Token_NEXT == vals2[ 0 ].token )
                {
                    rewritten = true;
                    vals[ 0 ].extra = 1;  // indicate that the for loop can optionally optimally be a memfill
                }
            }
        }

        // if 0 <> EXPRESSION   ========>>>>>>>>  if EXPRESSION
        // 4180 has 11 tokens
        //   token   0 IF, value 0, strValue ''
        //   token   1 EXPRESSION, value 8, strValue ''
        //   token   2 CONSTANT, value 0, strValue ''
        //   token   3 NE, value 0, strValue ''
        //   token   4 VARIABLE, value 0, strValue 'b%'
        //   token   5 OPENPAREN, value 0, strValue ''
        //   token   6 EXPRESSION, value 2, strValue ''
        //   token   7 VARIABLE, value 0, strValue 'p%'
        //   token   8 CLOSEPAREN, value 0, strValue ''
        //   token   9 THEN, value 0, strValue ''
        //   token  10 GOTO, value 4500, strValue ''

        if ( Token_IF == vals[ 0 ].token &&
             Token_EXPRESSION == vals[ 1 ].token &&
             Token_CONSTANT == vals[ 2 ].token &&
             0 == vals[ 2 ].value &&
             Token_NE == vals[ 3 ].token )
        {
            vals.erase( vals.begin() + 2 );
            vals.erase( vals.begin() + 2 );
            vals[ 1 ].value -= 2;

            rewritten = true;
        }

        // VARIABLE = VARIABLE + 1  =============>  ATOMIC INC VARIABLE
        // 4500 has 6 tokens
        //   token   0 VARIABLE, value 0, strValue 'p%'
        //   token   1 EQ, value 0, strValue ''
        //   token   2 EXPRESSION, value 4, strValue ''
        //   token   3 VARIABLE, value 0, strValue 'p%'
        //   token   4 PLUS, value 0, strValue ''
        //   token   5 CONSTANT, value 1, strValue ''

        else if ( 6 == vals.size() &&
                  Token_VARIABLE == vals[ 0 ].token &&
                  Token_EQ == vals[ 1 ].token &&
                  Token_VARIABLE == vals[ 3 ].token &&
                  !stcmp( vals[ 0 ], vals[ 3 ] ) &&
                  Token_PLUS == vals[ 4 ].token &&
                  Token_CONSTANT == vals[ 5 ].token &&
                  1 == vals[ 5 ].value )
        {
            string varname = vals[ 3 ].strValue;
            vals.clear();

            TokenValue tval( Token_ATOMIC );
            vals.push_back( tval );

            tval.token = Token_INC;
            tval.strValue = varname;
            vals.push_back( tval );

            rewritten = true;
        }

        // VARIABLE = VARIABLE - 1  =============>  ATOMIC DEC VARIABLE
        // 4500 has 6 tokens
        //   token   0 VARIABLE, value 0, strValue 'p%'
        //   token   1 EQ, value 0, strValue ''
        //   token   2 EXPRESSION, value 4, strValue ''
        //   token   3 VARIABLE, value 0, strValue 'p%'
        //   token   4 MINUS, value 0, strValue ''
        //   token   5 CONSTANT, value 1, strValue ''

        else if ( 6 == vals.size() &&
                  Token_VARIABLE == vals[ 0 ].token &&
                  Token_EQ == vals[ 1 ].token &&
                  Token_VARIABLE == vals[ 3 ].token &&
                  !stcmp( vals[ 0 ], vals[ 3 ] ) &&
                  Token_MINUS == vals[ 4 ].token &&
                  Token_CONSTANT == vals[ 5 ].token &&
                  1 == vals[ 5 ].value )
        {
            string varname = vals[ 3 ].strValue;
            vals.clear();

            TokenValue tval( Token_ATOMIC );
            vals.push_back( tval );

            tval.token = Token_DEC;
            tval.strValue = varname;
            vals.push_back( tval );

            rewritten = true;
        }

        // IF 0 = VARIABLE  =============>  IF NOT VARIABLE
        // 2410 has 7 tokens
        //   token   0 IF, value 0, strValue ''
        //   token   1 EXPRESSION, value 4, strValue ''
        //   token   2 CONSTANT, value 0, strValue ''
        //   token   3 EQ, value 0, strValue ''
        //   token   4 VARIABLE, value 0, strValue 'wi%'
        //   token   5 THEN, value 0, strValue ''
        //   token   6 GOTO, value 2500, strValue ''

        else if ( 7 == vals.size() &&
                  Token_IF == vals[ 0 ].token &&
                  Token_EXPRESSION == vals[ 1 ].token &&
                  4 == vals[ 1 ].value &&
                  Token_CONSTANT == vals[ 2 ].token &&
                  0 == vals[ 2 ].value &&
                  Token_EQ == vals[ 3 ].token &&
                  Token_VARIABLE == vals[ 4 ].token )
        {
            vals.erase( vals.begin() + 2 );
            vals[ 2 ].token = Token_NOT;
            vals[ 1 ].value = 3;

            rewritten = true;
        }

        // IF VARIABLE = 0  =============>  IF NOT VARIABLE
        // 2410 has 7 tokens
        //   token   0 IF, value 0, strValue ''
        //   token   1 EXPRESSION, value 4, strValue ''
        //   token   2 VARIABLE, value 0, strValue 'wi%'
        //   token   3 EQ, value 0, strValue ''
        //   token   4 CONSTANT, value 0, strValue ''
        //   token   5 THEN, value 0, strValue ''
        //   token   6 GOTO, value 2500, strValue ''

        else if ( 7 == vals.size() &&
                  Token_IF == vals[ 0 ].token &&
                  Token_EXPRESSION == vals[ 1 ].token &&
                  4 == vals[ 1 ].value &&
                  Token_VARIABLE == vals[ 2 ].token &&
                  Token_EQ == vals[ 3 ].token &&
                  Token_CONSTANT == vals[ 4 ].token &&
                  0 == vals[ 4 ].value )
        {
            vals[ 3 ] = vals[ 2 ];
            vals[ 2 ].token = Token_NOT;
            vals[ 2 ].strValue = "";
            vals[ 1 ].value = 3;
            vals.erase( vals.begin() + 4 );

            rewritten = true;
        }

        if ( showListing && rewritten )
        {
            printf( "line rewritten as:\n" );
            ShowLocListing( loc );
        }
    }
} //OptimizeWithRewrites

void SetFirstTokens()
{
    for ( size_t l = 0; l < g_linesOfCode.size(); l++ )
    {
        LineOfCode & loc = g_linesOfCode[ l ];
        loc.firstToken = loc.tokenValues[ 0 ].token;
    }
} //SetFirstTokens

void CreateVariables( map<string, Variable> & varmap )
{
    for ( size_t l = 0; l < g_linesOfCode.size(); l++ )
    {
        LineOfCode & loc = g_linesOfCode[ l ];
    
        for ( size_t t = 0; t < loc.tokenValues.size(); t++ )
        {
            TokenValue & tv = loc.tokenValues[ t ];

            if ( ( Token_INC == tv.token ) ||
                 ( Token_DEC == tv.token ) ||
                 ( Token_VARIABLE == tv.token ) || // create arrays as singletons until a DIM statement
                 ( Token_FOR == tv.token ) )
            {
                Variable * pvar = GetVariablePerhapsCreate( tv, varmap );
                pvar->references++;
            }
        }
    }
} //CreateVariables

#ifdef BA_ENABLE_COMPILER

const char * GenVariableName( string const & s )
{
    static char acName[ 100 ];

    assert( s.length() < _countof( acName ) );

    if ( i8080CPM == g_AssemblyTarget )
        strcpy_s( acName, _countof( acName ), "var$" );
    else
        strcpy_s( acName, _countof( acName ), "var_" );

    strcpy_s( acName + strlen( acName ), _countof( acName ) - 4, s.c_str() );
    acName[ strlen( acName ) - 1 ] = 0;
    return acName;
} //GenVariableName

const char * GenVariableReg( map<string, Variable> const & varmap, string const & s )
{
    Variable * pvar = FindVariable( varmap, s );
    assert( pvar && "variable must exist in GenVariableReg" );

    return pvar->reg.c_str();
} //GenVariableReg

const char * GenVariableReg64( map<string, Variable> const & varmap, string const & s )
{
    Variable * pvar = FindVariable( varmap, s );
    assert( pvar && "variable must exist in GenVariableReg" );

    const char * r = pvar->reg.c_str();

    if ( x64Win == g_AssemblyTarget )
    {
        for ( int i = 0; i < _countof( MappedRegistersX64 ); i++ )
            if ( !_stricmp( r, MappedRegistersX64[ i ] ) )
                return MappedRegistersX64_64[ i ];
    }
    else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
    {
        for ( int i = 0; i < _countof( MappedRegistersArm64 ); i++ )
            if ( !_stricmp( r, MappedRegistersArm64[ i ] ) )
                return MappedRegistersArm64_64[ i ];
    }
  
    assert( false && "why is there no 64 bit mapping to a register?" );
    return 0;
} //GenVariableReg64

bool IsVariableInReg( map<string, Variable> const & varmap, string const & s )
{
    Variable * pvar = FindVariable( varmap, s );
    assert( pvar && "variable must exist in IsVariableInReg" );

    return ( 0 != pvar->reg.length() );
} //IsVariableInReg

bool fitsIn12Bits( int x )
{
    return ( x >= 0 && x < 4096 );
} //fitsIn12Bits

bool fitsIn8Bits( int x )
{
    return (x >= 0 && x < 256 );
} //fitsIn8Bits

int g_lohCount = 0;

void LoadArm64Label( FILE * fp, const char * reg, const char * labelname )
{
    if ( arm64Mac == g_AssemblyTarget )
    {
        fprintf( fp, "Lloh%d:\n", g_lohCount++ );
        fprintf( fp, "    adrp     %s, %s@PAGE\n", reg, labelname );
        fprintf( fp, "Lloh%d:\n", g_lohCount++ );
        fprintf( fp, "    add      %s, %s, %s@PAGEOFF\n", reg, reg, labelname );
    }
    else if ( arm64Win == g_AssemblyTarget )
    {
        fprintf( fp, "    adrp     %s, %s\n", reg, labelname );
        fprintf( fp, "    add      %s, %s, %s\n", reg, reg, labelname );
    }
} //LoadArm64Label

void LoadArm64Address( FILE * fp, const char * reg, string const & varname )
{
    LoadArm64Label( fp, reg, GenVariableName( varname ) );
} //LoadArm64Address

void LoadArm64Address( FILE * fp, const char * reg, map<string, Variable> const varmap, string const & varname )
{
    if ( IsVariableInReg( varmap, varname ) )
        fprintf( fp, "    mov      %s, %s\n", reg, GenVariableReg64( varmap, varname ) );
    else
        LoadArm64Address( fp, reg, varname );
} //LoadArm64Address

void LoadArm64Constant( FILE * fp, const char * reg, int i )
{
    // mov is 1 instruction. The assembler will create multiple instructions for ldr as required

    if ( 0 == ( i & 0xffffff00 ) )
        fprintf( fp, "    mov      %s, %d\n", reg, i );
    else
        fprintf( fp, "    ldr      %s, =%#x\n", reg, i );
} //LoadArm64Constant

void LoadArm32Label( FILE * fp, const char * reg, const char * labelname )
{
    // movw and movt are only available on armv8 and later
    // not supported on Raspberry PI 3 CPU unless gcc is passed -march=armv8-a

    fprintf( fp, "    movw     %s, #:lower16:%s\n", reg, labelname );
    fprintf( fp, "    movt     %s, #:upper16:%s\n", reg, labelname );
} //LoadArm32Label

void LoadArm32LineNumber( FILE * fp, const char * reg, int linenumber )
{
    char aclabel[ 100 ];
    sprintf( aclabel, "line_number_%d", linenumber );
    LoadArm32Label( fp, reg, aclabel );
} //LoadArm32LineNumber

void LoadArm32Address( FILE * fp, const char * reg, string const & varname )
{
    LoadArm32Label( fp, reg, GenVariableName( varname ) );
} //LoadArm32Address

void LoadArm32Address( FILE * fp, const char * reg, map<string, Variable> const varmap, string const & varname )
{
    if ( IsVariableInReg( varmap, varname ) )
        fprintf( fp, "    mov      %s, %s\n", reg, GenVariableReg( varmap, varname ) );
    else
        LoadArm32Address( fp, reg, varname );
} //LoadArm32Address

void LoadArm32Constant( FILE * fp, const char * reg, int i )
{
    // mov is 1 instruction. 

    if ( 0 == ( i & 0xffffff00 ) )
        fprintf( fp, "    mov      %s, #%d\n", reg, i );
    else if ( 0 == ( i & 0xfffff000 ) )
        fprintf( fp, "    ldr      %s, =%#x\n", reg, i );
    else
    {
        fprintf( fp, "    movw     %s, #:lower16:%d\n", reg, i );
        fprintf( fp, "    movt     %s, #:upper16:%d\n", reg, i );
    }
} //LoadArm32Constant

void GenerateOp( FILE * fp, map<string, Variable> const & varmap, vector<TokenValue> const & vals,
                 int left, int right, Token op, int leftArray = 0, int rightArray = 0 )
{
    // optimize for wi% = b%( 0 )

    if ( Token_VARIABLE == vals[ left ].token &&
         IsVariableInReg( varmap, vals[ left ].strValue ) &&
         0 == vals[ left ].dimensions &&
         isOperatorRelational( op ) &&
         Token_VARIABLE == vals[ right ].token &&
         0 != vals[ right ].dimensions )
    {
        if ( x64Win == g_AssemblyTarget )
        {
            fprintf( fp, "    cmp      %s, DWORD PTR [%s + %d]\n", GenVariableReg( varmap, vals[ left ].strValue ),
                     GenVariableName( vals[ right ].strValue ), 4 * vals[ rightArray ].value );

            fprintf( fp, "    %-6s   al\n", OperatorInstructionX64[ op ] );
            fprintf( fp, "    movzx    rax, al\n" );
        }
        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
        {
            LoadArm64Address( fp, "x1", vals[ right ].strValue );

            int offset = 4 * vals[ rightArray ].value;

            if ( fitsIn8Bits( offset ) )
                fprintf( fp, "    ldr      w0, [x1, %d]\n", offset );
            else
            {
                LoadArm64Constant( fp, "x0", 4 * vals[ rightArray ].value );
                fprintf( fp, "    add      x1, x1, x0\n" );
                fprintf( fp, "    ldr      w0, [x1]\n" );
            }

            fprintf( fp, "    cmp      %s, w0\n", GenVariableReg( varmap, vals[ left ].strValue ) );
            fprintf( fp, "    cset     x0, %s\n", ConditionsArm[ op ] );
        }
        return;
    }

    // optimize this typical case to save a mov: x% relop CONSTANT

    if ( Token_VARIABLE == vals[ left ].token &&
         0 == vals[ left ].dimensions &&
         isOperatorRelational( op ) &&
         Token_CONSTANT == vals[ right ].token )
    {
        string const & varname = vals[ left ].strValue;

        if ( x64Win == g_AssemblyTarget )
        {
            if ( IsVariableInReg( varmap, varname ) )
                fprintf( fp, "    cmp      %s, %d\n", GenVariableReg( varmap, varname ), vals[ right ].value );
            else
                fprintf( fp, "    cmp      DWORD PTR [%s], %d\n", GenVariableName( varname ), vals[ right ].value );

            fprintf( fp, "    %-6s   al\n", OperatorInstructionX64[ op ] );
            fprintf( fp, "    movzx    rax, al\n" );
        }
        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
        {
            LoadArm64Constant( fp, "x1", vals[ right ].value );

            if ( IsVariableInReg( varmap, varname ) )
                fprintf( fp, "    cmp      %s, w1\n", GenVariableReg( varmap, varname ) );
            else
            {
                LoadArm64Address( fp, "x2", varname );
                fprintf( fp, "    ldr      w0, [x2]\n" );
                fprintf( fp, "    cmp      w0, w1\n" );
            }

            fprintf( fp, "    cset     x0, %s\n", ConditionsArm[ op ] );            
        }
        return;
    }

    // general case: left operator right
    // first: load left

    if ( Token_CONSTANT == vals[ left ].token )
    {
        if ( x64Win == g_AssemblyTarget )
            fprintf( fp, "    mov      eax, %d\n", vals[ left ].value );
        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
            LoadArm64Constant( fp, "x0", vals[ left ].value );
    }
    else if ( 0 == vals[ left ].dimensions )
    {
        string const & varname = vals[ left ].strValue;

        if ( IsVariableInReg( varmap, varname ) )
        {
            if ( x64Win == g_AssemblyTarget )
                fprintf( fp, "    mov      eax, %s\n", GenVariableReg( varmap, varname ) );
            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                fprintf( fp, "    mov      w0, %s\n", GenVariableReg( varmap, varname ) );
        }
        else
        {
            if ( x64Win == g_AssemblyTarget )
                fprintf( fp, "    mov      eax, DWORD PTR [%s]\n", GenVariableName( varname ) );
            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
            {
                LoadArm64Address( fp, "x1", varname );
                fprintf( fp, "    ldr      w0, [x1]\n" );
            }
        }
    }
    else
    {
        if ( x64Win == g_AssemblyTarget )
            fprintf( fp, "    mov      eax, DWORD PTR [%s + %d]\n", GenVariableName( vals[ left ].strValue ),
                     4 * vals[ leftArray ].value );
        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
        {
            LoadArm64Address( fp, "x1", vals[ left ].strValue );

            int offset = 4 * vals[ leftArray ].value;

            if ( fitsIn8Bits( offset ) )
                fprintf( fp, "    ldr      w0, [x1 + %d]\n", offset );
            else
            {
                LoadArm64Constant( fp, "x0", offset );
                fprintf( fp, "    add      x1, x1, x0\n" );
                fprintf( fp, "    ldr      w0, [x1]\n" );
            }
        }
    }

    if ( isOperatorRelational( op ) )
    {
        // relational

        if ( Token_CONSTANT == vals[ right ].token )
        {
            if ( x64Win == g_AssemblyTarget )
                fprintf( fp, "    cmp      eax, %d\n", vals[ right ].value );
            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
            {
                LoadArm64Constant( fp, "x1", vals[ right ].value );
                fprintf( fp, "    cmp      w1, w1\n" );
            }
        }
        else if ( 0 == vals[ right ].dimensions )
        {
            string const & varname = vals[ right ].strValue;
            if ( IsVariableInReg( varmap, varname ) )
            {
                if ( x64Win == g_AssemblyTarget )
                    fprintf( fp, "    cmp      eax, %s\n", GenVariableReg( varmap, varname ) );
                else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                    fprintf( fp, "    cmp      w0, %s\n", GenVariableReg( varmap, varname ) );
            }
            else
            {
                if ( x64Win == g_AssemblyTarget )
                    fprintf( fp, "    cmp      eax, DWORD PTR [%s]\n", GenVariableName( varname ) );
                else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                {
                    LoadArm64Address( fp, "x2", varname );
                    fprintf( fp, "    ldr      w1, [x2]\n" );
                    fprintf( fp, "    cmp      w0, w1\n" );
                }
            }
        }
        else
        {
            if ( x64Win == g_AssemblyTarget )
            {
                fprintf( fp, "    cmp      eax, DWORD PTR [%s + %d]\n", GenVariableName( vals[ right ].strValue ),
                        4 * vals[ rightArray ].value );
            }
            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
            {
                LoadArm64Address( fp, "x1", vals[ right ].strValue );

                int offset = 4 * vals[ rightArray ].value;

                if ( fitsIn8Bits( offset ) )
                    fprintf( fp, "    ldr      w1, [x1, %d]\n", offset );
                else
                {
                    LoadArm64Constant( fp, "x3", offset );
                    fprintf( fp, "    add      x1, x1, x3\n" );
                    fprintf( fp, "    ldr      w1, [x1]\n" );
                }

                fprintf( fp, "    cmp      w0, w1\n" );
            }
        }

        if ( x64Win == g_AssemblyTarget )
        {
            fprintf( fp, "    %-6s   al\n", OperatorInstructionX64[ op ] );
            fprintf( fp, "    movzx    rax, al\n" );
        }
        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
        {
            fprintf( fp, "    cset     x0, %s\n", ConditionsArm[ op ] );            
        }
    }
    else
    {
        // arithmetic and logical (which in BASIC is both arithmetic and logical)

        if ( x64Win == g_AssemblyTarget && Token_DIV == op )
        {
            if ( Token_CONSTANT == vals[ right ].token )
                fprintf( fp, "    mov      rbx, %d\n", vals[ right ].value );
            else if ( 0 == vals[ right ].dimensions )
            {
                string const & varname = vals[ right ].strValue;
                if ( IsVariableInReg( varmap, varname ) )
                    fprintf( fp, "    mov      ebx, %s\n", GenVariableReg( varmap, varname ) );
                else
                    fprintf( fp, "    mov      ebx, DWORD PTR [%s]\n", GenVariableName( varname ) );
            }
            else
                fprintf( fp, "    mov      ebx, DWRD PTR [%s + %d]\n", GenVariableName( vals[ right ].strValue ),
                         4 * vals[ rightArray ].value );

            fprintf( fp, "    cdq\n" );
            fprintf( fp, "    idiv     ebx\n" );
        }
        else
        {
            if ( Token_CONSTANT == vals[ right ].token )
            {
                if ( x64Win == g_AssemblyTarget )
                    fprintf( fp, "    %-6s   eax, %d\n", OperatorInstructionX64[ op ], vals[ right ].value );
                else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                {
                    LoadArm64Constant( fp, "x1", vals[ right ].value );
                    fprintf( fp, "    %-6s   w0, w0, w1\n", OperatorInstructionArm[ op ] );
                }
            }
            else if ( 0 == vals[ right ].dimensions )
            {
                string const & varname = vals[ right ].strValue;
                if ( IsVariableInReg( varmap, varname ) )
                {
                    if ( x64Win == g_AssemblyTarget )
                        fprintf( fp, "    %-6s   eax, %s\n", OperatorInstructionX64[ op ], GenVariableReg( varmap, varname ) );
                    else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                        fprintf( fp, "    %-6s   w0, w0, %s\n", OperatorInstructionArm[ op ], GenVariableReg( varmap, varname ) );
                }
                else
                {
                    if ( x64Win == g_AssemblyTarget )
                        fprintf( fp, "    %-6s   eax, DWORD PTR [%s]\n", OperatorInstructionX64[ op ], GenVariableName( varname ) );
                    else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                    {
                        LoadArm64Address( fp, "x2", varname );
                        fprintf( fp, "    ldr      w1, [x2]\n" );
                        fprintf( fp, "    %-6s     w0, w0, w1\n", OperatorInstructionArm[ op ] );
                    }
                }
            }
            else
            {
                if ( x64Win == g_AssemblyTarget )
                    fprintf( fp, "    %-6s   eax, DWORD PTR [%s + %d]\n", OperatorInstructionX64[ op ], GenVariableName( vals[ right ].strValue ),
                             4 * vals[ rightArray ].value );
                else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                {
                    LoadArm64Address( fp, "x1", vals[ right ].strValue );

                    int offset = 4 * vals[ rightArray ].value;

                    if ( fitsIn8Bits( offset ) )
                        fprintf( fp, "    ldr      w1, [x1, %d]\n", offset );
                    else
                    {
                        LoadArm64Constant( fp, "x3", offset );
                        fprintf( fp, "    add      x3, x1, x3\n" );
                        fprintf( fp, "    ldr      w1, [x3]\n" );
                    }

                    fprintf( fp, "    %-6s     w0, w0, w1\n", OperatorInstructionArm[ op ] );
                }
            }
        }
    }
} //GenerateOp

void GenerateExpression( FILE * fp, map<string, Variable> const & varmap, int & iToken, int beyond, vector<TokenValue> const & vals );
void GenerateFactor( FILE * fp, map<string, Variable> const & varmap, int & iToken, int beyond, vector<TokenValue> const & vals );
void GenerateTerm( FILE * fp, map<string, Variable> const & varmap, int & iToken, int beyond, vector<TokenValue> const & vals );

void GenerateMultiply( FILE * fp, map<string, Variable> const & varmap, int & iToken, int beyond, vector<TokenValue> const & vals )
{
    assert( iToken < beyond );
    iToken++;

    GenerateFactor( fp, varmap, iToken, beyond, vals );

    if ( x64Win == g_AssemblyTarget )
    {
        fprintf( fp, "    pop      rbx\n" );
        fprintf( fp, "    imul     rax, rbx\n" );
    }
    else if ( arm32Linux == g_AssemblyTarget )
    {
        fprintf( fp, "    pop      {r1}\n" );
        fprintf( fp, "    mul      r0, r0, r1\n" );
    }
    else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
    {
        fprintf( fp, "    ldr      x1, [sp], #16\n" );
        fprintf( fp, "    mul      w0, w0, w1\n" );
    }
    else if ( i8080CPM == g_AssemblyTarget )
    {
        fprintf( fp, "    pop      d\n" );
        fprintf( fp, "    call     imul\n" );
    }
    else if ( mos6502Apple1 == g_AssemblyTarget )
    {
        fprintf( fp, "    pla\n" );
        fprintf( fp, "    sta      otherOperand\n" );
        fprintf( fp, "    pla\n" );
        fprintf( fp, "    sta      otherOperand+1\n" );
        fprintf( fp, "    jsr      imul\n" );
    }
    else if ( i8086DOS == g_AssemblyTarget )
    {
        fprintf( fp, "    pop      bx\n" );
        fprintf( fp, "    xor      dx, dx\n" );
        fprintf( fp, "    imul     bx\n" );
    }
    else if ( x86Win == g_AssemblyTarget )
    {
        fprintf( fp, "    pop      ebx\n" );
        fprintf( fp, "    imul     eax, ebx\n" );
    }
    else if ( riscv64 == g_AssemblyTarget )
    {
        RiscVPop( fp, "t0" );
        fprintf( fp, "    mul      a0, a0, t0\n" );
    }
    else if ( m68kCPM == g_AssemblyTarget )
    {
        fprintf( fp, "    move.l    (a7)+, d1\n" );
        fprintf( fp, "    muls      d1, d0\n" ); // bug: only 16 bits of d0 used
    }
    else if ( oiOS == g_AssemblyTarget )
        fprintf( fp, "    imulst\n" );
} //GenerateMultiply

void GenerateDivide( FILE * fp, map<string, Variable> const & varmap, int & iToken, int beyond, vector<TokenValue> const & vals )
{
    assert( iToken < beyond );
    iToken++;

    GenerateFactor( fp, varmap, iToken, beyond, vals );

    if ( x64Win == g_AssemblyTarget )
    {
        fprintf( fp, "    mov      rbx, rax\n" );
        fprintf( fp, "    pop      rax\n" );
        fprintf( fp, "    cdq\n" );
        fprintf( fp, "    idiv     ebx\n" );
    }
    else if ( arm32Linux == g_AssemblyTarget )
    {
        fprintf( fp, "    mov      r1, r0\n" );
        fprintf( fp, "    pop      {r0}\n" );
        fprintf( fp, "    save_volatile_registers\n" );
        fprintf( fp, "    bl       __aeabi_idiv\n" ); // GNU C runtime: r0 = r0 / r1
        fprintf( fp, "    restore_volatile_registers\n" );
    }
    else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
    {
        fprintf( fp, "    ldr      x1, [sp], #16\n" );
        fprintf( fp, "    sdiv     w0, w1, w0\n" );
    }
    else if ( i8080CPM == g_AssemblyTarget )
    {
        fprintf( fp, "    pop      d\n" );
        fprintf( fp, "    call     idiv\n" );
    }
    else if ( mos6502Apple1 == g_AssemblyTarget )
    {
        fprintf( fp, "    pla\n" );
        fprintf( fp, "    sta      otherOperand\n" );
        fprintf( fp, "    pla\n" );
        fprintf( fp, "    sta      otherOperand+1\n" );
        fprintf( fp, "    jsr      idiv\n" );
    }
    else if ( i8086DOS == g_AssemblyTarget )
    {
        fprintf( fp, "    mov      bx, ax\n" );
        fprintf( fp, "    pop      ax\n" );
        fprintf( fp, "    cwd\n" );      // sign-extend ax to dx, so it's dx:ax / bx => ax
        fprintf( fp, "    idiv     bx\n" );
    }
    else if ( x86Win == g_AssemblyTarget )
    {
        fprintf( fp, "    mov      ebx, eax\n" );
        fprintf( fp, "    pop      eax\n" );
        fprintf( fp, "    cdq\n" );
        fprintf( fp, "    idiv     ebx\n" );
    }
    else if ( riscv64 == g_AssemblyTarget )
    {
        fprintf( fp, "    mv       t0, a0\n" );
        RiscVPop( fp, "a0" );
        fprintf( fp, "    div      a0, a0, t0\n" );
    }
    else if ( m68kCPM == g_AssemblyTarget )
    {
        fprintf( fp, "    move.l    d0, d1\n" );
        fprintf( fp, "    move.l    (a7)+, d0\n" );
        fprintf( fp, "    divs      d1, d0\n" ); // bug: only 16 bits of d0 used and only 16-bit result
        fprintf( fp, "    ext.l     d0\n" ); // sign extend and overwrite remainder in the upper word with the sign
    }
    else if ( oiOS == g_AssemblyTarget )
        fprintf( fp, "    idivst\n" );
} //GenerateDivide

void PushAccumulator( FILE * fp )
{
    if ( x64Win == g_AssemblyTarget )
        fprintf( fp, "    push     rax\n" );
    else if ( arm32Linux == g_AssemblyTarget )
        fprintf( fp, "    push     {r0}\n" );
    else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
        fprintf( fp, "    str      x0, [sp, #-16]!\n" ); // save 8 bytes in a 16-byte spot
    else if ( i8080CPM == g_AssemblyTarget )
        fprintf( fp, "    push     h\n" );
    else if ( mos6502Apple1 == g_AssemblyTarget )
    {
        fprintf( fp, "    lda      curOperand+1\n" );
        fprintf( fp, "    pha\n" );
        fprintf( fp, "    lda      curOperand\n" );
        fprintf( fp, "    pha\n" );
    }
    else if ( i8086DOS == g_AssemblyTarget )
        fprintf( fp, "    push     ax\n" );
    else if ( x86Win == g_AssemblyTarget )
        fprintf( fp, "    push     eax\n" );
    else if ( riscv64 == g_AssemblyTarget )
        RiscVPush( fp, "a0" );
    else if ( m68kCPM == g_AssemblyTarget )
        fprintf( fp, "    move.l    d0, -(a7)\n" );
    else if ( oiOS == g_AssemblyTarget )
        fprintf( fp, "    push     rres\n" );
} //PushAccumulator

void GenerateTerm( FILE * fp, map<string, Variable> const & varmap, int & iToken, int beyond, vector<TokenValue> const & vals )
{
    assert( iToken < beyond );
    assert( iToken < vals.size() );

    if ( EnableTracing && g_Tracing )
        printf( "generate term iToken %d, %s\n", iToken, TokenStr( vals[ iToken ].token ) );

    GenerateFactor( fp, varmap, iToken, beyond, vals );

    if ( iToken >= beyond )
        return;

    Token t = vals[ iToken ].token;

    while ( isOperatorMultiplicative( t ) )
    {
        PushAccumulator( fp );

        if ( Token_MULT == t )
            GenerateMultiply( fp, varmap, iToken, beyond, vals );
        else
            GenerateDivide( fp, varmap, iToken, beyond, vals );

        if ( iToken >= beyond )
            break;

        t = vals[ iToken ].token;

        if ( EnableTracing && g_Tracing )
            printf( "next token  %d in GenerateTerm: %d\n", iToken, t );
    }
} //GenerateTerm

void GenerateFactor( FILE * fp, map<string, Variable> const & varmap, int & iToken, int beyond,vector<TokenValue> const & vals )
{
    if ( EnableTracing && g_Tracing )
        printf( "GenerateFactor iToken %d, beyond %d, %s\n", iToken, beyond, TokenStr( vals[ iToken ].token ) );

    if ( iToken < beyond )
    {
        Token t = vals[ iToken ].token;

        if ( Token_EXPRESSION == t )
        {
            iToken++;
            t = vals[ iToken ].token;
        }

        if ( Token_OPENPAREN == t )
        {
            iToken++;
            GenerateExpression( fp, varmap, iToken, beyond, vals );
            assert( Token_CLOSEPAREN == vals[ iToken ].token );
            iToken++;
        }
        else if ( Token_VARIABLE == t )
        {
            string const & varname = vals[ iToken ].strValue;

            if ( 0 == vals[ iToken ].dimensions )
            {
                if ( x64Win == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                        fprintf( fp, "    movsxd   rax, %s\n", GenVariableReg( varmap, varname ) );
                    else
                        fprintf( fp, "    movsxd   rax, DWORD PTR [%s]\n", GenVariableName( varname ) );
                }
                else if ( arm32Linux == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                        fprintf( fp, "    mov      r0, %s\n", GenVariableReg( varmap, varname ) );
                    else
                    {
                        LoadArm32Address( fp, "r1", varname );
                        fprintf( fp, "    ldr      r0, [r1]\n" );
                    }
                }
                else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                        fprintf( fp, "    mov      w0, %s\n", GenVariableReg( varmap, varname ) );
                    else
                    {
                        LoadArm64Address( fp, "x1", varname );
                        fprintf( fp, "    ldr      w0, [x1]\n" );
                    }
                }
                else if ( i8080CPM == g_AssemblyTarget )
                {
                    fprintf( fp, "    lhld     %s\n", GenVariableName( varname ) );
                }
                else if ( mos6502Apple1 == g_AssemblyTarget )
                {
                    fprintf( fp, "    lda      %s\n", GenVariableName( varname ) );
                    fprintf( fp, "    sta      curOperand\n" );
                    fprintf( fp, "    lda      %s+1\n", GenVariableName( varname ) );
                    fprintf( fp, "    sta      curOperand+1\n" );
                }
                else if ( i8086DOS == g_AssemblyTarget )
                {
                    fprintf( fp, "    mov      ax, ds: [ %s ]\n", GenVariableName( varname ) );
                }
                else if ( x86Win == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                        fprintf( fp, "    mov      eax, %s\n", GenVariableReg( varmap, varname ) );
                    else
                        fprintf( fp, "    mov      eax, DWORD PTR [%s]\n", GenVariableName( varname ) );
                }
                else if ( riscv64 == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                        fprintf( fp, "    mv       a0, %s\n", GenVariableReg( varmap, varname ) );
                    else
                    {
                        fprintf( fp, "    lla      t0, %s\n", GenVariableName( varname ) );
                        fprintf( fp, "    lw       a0, (t0)\n" );
                    }
                }
                else if ( m68kCPM == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                        fprintf( fp, "    move.l   %s, d0\n", GenVariableReg( varmap, varname ) );
                    else
                        fprintf( fp, "    move.l   %s, d0\n", GenVariableName( varname ) );
                }
                else if ( oiOS == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                        fprintf( fp, "    mov      rres, %s\n", GenVariableReg( varmap, varname ) );
                    else
                        fprintf( fp, "    ld       rres, [ %s ]\n", GenVariableName( varname ) );
                }
            }
            else if ( 1 == vals[ iToken ].dimensions )
            {
                iToken++; // variable

                if ( Token_OPENPAREN != vals[ iToken ].token )
                    RuntimeFail( "open parenthesis expected", g_lineno );

                iToken++; // open paren

                assert( Token_EXPRESSION == vals[ iToken ].token );
                GenerateExpression( fp, varmap, iToken, iToken + vals[ iToken ].value, vals );

                if ( x64Win == g_AssemblyTarget )
                {
                    fprintf( fp, "    shl      rax, 2\n" );
                    fprintf( fp, "    lea      rbx, [ %s ]\n", GenVariableName( varname ) );
                    fprintf( fp, "    add      rbx, rax\n" );
                    fprintf( fp, "    mov      eax, [ rbx ]\n" );
                }
                else if ( arm32Linux == g_AssemblyTarget )
                {
                    LoadArm32Address( fp, "r1", varname );
                    fprintf( fp, "    add      r1, r1, r0, lsl #2\n" );
                    fprintf( fp, "    ldr      r0, [r1], #0\n" );
                }
                else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                        fprintf( fp, "    mov      x1, %s\n", GenVariableReg64( varmap, varname ) );
                    else
                        LoadArm64Address( fp, "x1", varname );
                    fprintf( fp, "    add      x1, x1, x0, lsl #2\n" );
                    fprintf( fp, "    ldr      w0, [x1], 0\n" );
                }
                else if ( i8080CPM == g_AssemblyTarget )
                {
                    // double h because each variable in the array is 2 bytes

                    fprintf( fp, "    dad      h\n" );
                    fprintf( fp, "    lxi      d, %s\n", GenVariableName( varname ) );
                    fprintf( fp, "    dad      d\n" );

                    fprintf( fp, "    mov      e, m\n" );
                    fprintf( fp, "    inx      h\n" );
                    fprintf( fp, "    mov      d, m\n" );
                    fprintf( fp, "    xchg\n" );
                }
                else if ( mos6502Apple1 == g_AssemblyTarget )
                {
                    fprintf( fp, "    asl      curOperand\n" );   // shift left into carry
                    fprintf( fp, "    rol      curOperand+1\n" ); // shift left from carry
                    fprintf( fp, "    lda      #%s\n", GenVariableName( varname ) );
                    fprintf( fp, "    clc\n" );
                    fprintf( fp, "    adc      curOperand\n" );
                    fprintf( fp, "    sta      curOperand\n" );
                    fprintf( fp, "    lda      /%s\n", GenVariableName( varname ) );
                    fprintf( fp, "    adc      curOperand+1\n" );
                    fprintf( fp, "    sta      curOperand+1\n" );
                    fprintf( fp, "    ldy      #0\n" );
                    fprintf( fp, "    lda      (curOperand), y\n" ); // there is no ldx (zp), y instruction
                    fprintf( fp, "    tax\n" );
                    fprintf( fp, "    iny\n" );
                    fprintf( fp, "    lda      (curOperand), y\n" );
                    fprintf( fp, "    sta      curOperand+1\n" );
                    fprintf( fp, "    stx      curOperand\n" );
                }
                else if ( i8086DOS == g_AssemblyTarget )
                {
                    fprintf( fp, "    shl      ax, 1\n" );
                    fprintf( fp, "    lea      si, %s\n", GenVariableName( varname ) );
                    fprintf( fp, "    add      si, ax\n" );
                    fprintf( fp, "    mov      ax, [ si ]\n" );
                }
                else if ( x86Win == g_AssemblyTarget )
                {
                    fprintf( fp, "    shl      eax, 2\n" );
                    fprintf( fp, "    lea      ebx, [ %s ]\n", GenVariableName( varname ) );
                    fprintf( fp, "    add      ebx, eax\n" );
                    fprintf( fp, "    mov      eax, [ ebx ]\n" );
                }
                else if ( riscv64 == g_AssemblyTarget )
                {
                    fprintf( fp, "    lla      t0, %s\n", GenVariableName( varname ) );
                    fprintf( fp, "    slli     a0, a0, 2\n" );
                    fprintf( fp, "    add      t0, t0, a0\n" );
                    fprintf( fp, "    lw       a0, (t0)\n" );
                }
                else if ( m68kCPM == g_AssemblyTarget )
                {
                    fprintf( fp, "    lea      %s, a0\n", GenVariableName( varname ) );
                    fprintf( fp, "    lsl.l    #2, d0\n" );
                    fprintf( fp, "    move.l   (a0,d0.l), d0\n" );
                }
                else if ( oiOS == g_AssemblyTarget )
                    fprintf( fp, "    ldae     %s[ rres ]\n", GenVariableName( varname )  );
            }
            else if ( 2 == vals[ iToken ].dimensions )
            {
                iToken++; // variable

                if ( Token_OPENPAREN != vals[ iToken ].token )
                    RuntimeFail( "open parenthesis expected", g_lineno );

                iToken++; // open paren

                assert( Token_EXPRESSION == vals[ iToken ].token );
                GenerateExpression( fp, varmap, iToken, iToken + vals[ iToken ].value, vals );
                PushAccumulator( fp );

                if ( Token_COMMA != vals[ iToken ].token )
                    RuntimeFail( "expected a 2-dimensional array", g_lineno );
                iToken++; // comma

                assert( Token_EXPRESSION == vals[ iToken ].token );
                GenerateExpression( fp, varmap, iToken, iToken + vals[ iToken ].value, vals );

                Variable * pvar = FindVariable( varmap, varname );
                assert( pvar );

                if ( x64Win == g_AssemblyTarget )
                {
                    fprintf( fp, "    pop      rbx\n" );
                    fprintf( fp, "    imul     rbx, %d\n", pvar->dims[ 1 ] );
                    fprintf( fp, "    add      rax, rbx\n" );
                    fprintf( fp, "    shl      rax, 2\n" );
                    fprintf( fp, "    lea      rbx, [ %s ]\n", GenVariableName( varname ) );
                    fprintf( fp, "    add      rbx, rax\n" );
                    fprintf( fp, "    mov      eax, [ rbx ]\n" );
                }
                else if ( arm32Linux == g_AssemblyTarget )
                {
                    fprintf( fp, "    pop      {r1}\n" );
                    LoadArm32Constant( fp, "r2", pvar->dims[ 1 ] );
                    fprintf( fp, "    mul      r1, r1, r2\n" );
                    fprintf( fp, "    add      r0, r0, r1\n" );
                    LoadArm32Address( fp, "r1", varname );
                    fprintf( fp, "    add      r1, r1, r0, lsl #2\n" );
                    fprintf( fp, "    ldr      r0, [r1], #0\n" );
                }
                else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                {
                    fprintf( fp, "    ldr      x1, [sp], #16\n" );
                    LoadArm64Constant( fp, "x2", pvar->dims[ 1 ] );
                    fprintf( fp, "    mul      x1, x1, x2\n" );
                    fprintf( fp, "    add      x0, x0, x1\n" );
                    if ( IsVariableInReg( varmap, varname ) )
                        fprintf( fp, "    mov      x1, %s\n", GenVariableReg64( varmap, varname ) );
                    else
                        LoadArm64Address( fp, "x1", varname );
                    fprintf( fp, "    add      x1, x1, x0, lsl #2\n" );
                    fprintf( fp, "    ldr      w0, [x1], 0\n" );
                }
                else if ( i8080CPM == g_AssemblyTarget )
                {
                    fprintf( fp, "    pop      d\n" );
                    fprintf( fp, "    push     h\n" );
                    fprintf( fp, "    lxi      h, %d\n", pvar->dims[ 1 ] );
                    fprintf( fp, "    call     imul\n" );
                    fprintf( fp, "    pop      d\n" );
                    fprintf( fp, "    dad      d\n" );
                    fprintf( fp, "    dad      h\n" );
                    fprintf( fp, "    lxi      d, %s\n", GenVariableName( varname ) );
                    fprintf( fp, "    dad      d\n" );
                    fprintf( fp, "    mov      e, m\n" );
                    fprintf( fp, "    inx      h\n" );
                    fprintf( fp, "    mov      d, m\n" );
                    fprintf( fp, "    xchg\n" );
                }
                else if ( mos6502Apple1 == g_AssemblyTarget )
                {
                    // stash away the most recent expression

                    fprintf( fp, "    lda      curOperand\n" );
                    fprintf( fp, "    sta      arrayOffset\n" );
                    fprintf( fp, "    lda      curOperand+1\n" );
                    fprintf( fp, "    sta      arrayOffset+1\n" );

                    // multiply the first dimension by the size of the [1] dimension

                    fprintf( fp, "    pla\n" );
                    fprintf( fp, "    sta      curOperand\n" );
                    fprintf( fp, "    pla\n" );
                    fprintf( fp, "    sta      curOperand+1\n" );
                    fprintf( fp, "    lda      #%d\n", pvar->dims[ 1 ] );
                    fprintf( fp, "    sta      otherOperand\n" );
                    fprintf( fp, "    lda      /%d\n", pvar->dims[ 1 ] );
                    fprintf( fp, "    sta      otherOperand+1\n" );
                    fprintf( fp, "    jsr      imul\n" );

                    // add the two amounts above

                    fprintf( fp, "    lda      curOperand\n" );
                    fprintf( fp, "    clc\n" );
                    fprintf( fp, "    adc      arrayOffset\n" );
                    fprintf( fp, "    sta      arrayOffset\n" );
                    fprintf( fp, "    lda      curOperand+1\n" );
                    fprintf( fp, "    adc      arrayOffset+1\n" );
                    fprintf( fp, "    sta      arrayOffset+1\n" );

                    // double it to get the final array offset

                    fprintf( fp, "    lda      arrayOffset\n" );
                    fprintf( fp, "    clc\n" );
                    fprintf( fp, "    adc      arrayOffset\n" );
                    fprintf( fp, "    sta      arrayOffset\n" );
                    fprintf( fp, "    lda      arrayOffset+1\n" );
                    fprintf( fp, "    adc      arrayOffset+1\n" );
                    fprintf( fp, "    sta      arrayOffset+1\n" );

                    // add the address of the array to the offset

                    fprintf( fp, "    lda      #%s\n", GenVariableName( varname ) );
                    fprintf( fp, "    clc\n" );
                    fprintf( fp, "    adc      arrayOffset\n" );
                    fprintf( fp, "    sta      arrayOffset\n" );
                    fprintf( fp, "    lda      /%s\n", GenVariableName( varname ) );
                    fprintf( fp, "    adc      arrayOffset+1\n" );
                    fprintf( fp, "    sta      arrayOffset+1\n" );

                    // read the value into curOperand

                    fprintf( fp, "    ldy      #0\n" );
                    fprintf( fp, "    lda      (arrayOffset), y\n" );
                    fprintf( fp, "    sta      curOperand\n" );
                    fprintf( fp, "    iny\n" );
                    fprintf( fp, "    lda      (arrayOffset), y\n" );
                    fprintf( fp, "    sta      curOperand+1\n" );
                }
                else if ( i8086DOS == g_AssemblyTarget )
                {
                    fprintf( fp, "    mov      cx, ax\n" );
                    fprintf( fp, "    pop      ax\n" );
                    fprintf( fp, "    mov      bx, %d\n", pvar->dims[ 1 ] );
                    fprintf( fp, "    imul     bx\n" );
                    fprintf( fp, "    add      ax, cx\n" );
                    fprintf( fp, "    shl      ax, 1\n" );
                    fprintf( fp, "    lea      si, %s\n", GenVariableName( varname ) );
                    fprintf( fp, "    add      si, ax\n" );
                    fprintf( fp, "    mov      ax, [ si ]\n" );
                }
                else if ( x86Win == g_AssemblyTarget )
                {
                    fprintf( fp, "    pop      ebx\n" );
                    fprintf( fp, "    imul     ebx, %d\n", pvar->dims[ 1 ] );
                    fprintf( fp, "    add      eax, ebx\n" );
                    fprintf( fp, "    shl      eax, 2\n" );
                    fprintf( fp, "    lea      ebx, [ %s ]\n", GenVariableName( varname ) );
                    fprintf( fp, "    add      ebx, eax\n" );
                    fprintf( fp, "    mov      eax, [ ebx ]\n" );
                }
                else if ( riscv64 == g_AssemblyTarget )
                {
                    RiscVPop( fp, "t1" );
                    fprintf( fp, "    li       t2, %d\n", pvar->dims[ 1 ] );
                    fprintf( fp, "    mul      t1, t1, t2\n" );
                    fprintf( fp, "    add      a0, a0, t1\n" );
                    fprintf( fp, "    lla      t0, %s\n", GenVariableName( varname ) );
                    fprintf( fp, "    slli     a0, a0, 2\n" );
                    fprintf( fp, "    add      t0, t0, a0\n" );
                    fprintf( fp, "    lw       a0, (t0)\n" );
                }
                else if ( m68kCPM == g_AssemblyTarget )
                {
                    fprintf( fp, "    move.l   (a7)+, d1\n" );
                    fprintf( fp, "    move.l   #%d, d2\n", pvar->dims[ 1 ] );
                    fprintf( fp, "    muls     d1, d2\n" );
                    fprintf( fp, "    add.l    d0, d2\n" );
                    fprintf( fp, "    move.l   #%s, a0\n", GenVariableName( varname ) );
                    fprintf( fp, "    lsl.l    #2, d2\n" );
                    fprintf( fp, "    move.l   (a0,d2), d0\n" );
                }
                else if ( oiOS == g_AssemblyTarget )
                {
                    fprintf( fp, "    ldi      rtmp, %u\n", pvar->dims[ 1 ] );
                    fprintf( fp, "    mathst   rtmp, rtmp, imul\n" );
                    fprintf( fp, "    add      rres, rtmp\n" );
                    fprintf( fp, "    ldae     %s[ rres ]\n", GenVariableName( varname )  );
                }
            }

            iToken++;
        }
        else if ( Token_CONSTANT == t )
        {
            if ( x64Win == g_AssemblyTarget )
                fprintf( fp, "    mov      rax, %d\n", vals[ iToken ].value );
            else if ( arm32Linux == g_AssemblyTarget )
                LoadArm32Constant( fp, "r0", vals[ iToken ].value );
            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                LoadArm64Constant( fp, "x0", vals[ iToken ].value );
            else if ( i8080CPM == g_AssemblyTarget )
                fprintf( fp, "    lxi      h, %d\n", vals[ iToken ].value );
            else if ( mos6502Apple1 == g_AssemblyTarget )
            {
                fprintf( fp, "    lda      #%d\n", vals[ iToken ].value );
                fprintf( fp, "    sta      curOperand\n" );
                fprintf( fp, "    lda      /%d\n", vals[ iToken ].value );
                fprintf( fp, "    sta      curOperand+1\n" );
            }
            else if ( i8086DOS == g_AssemblyTarget )
                fprintf( fp, "    mov      ax, %d\n", vals[ iToken ].value );
            else if ( x86Win == g_AssemblyTarget )
                fprintf( fp, "    mov      eax, %d\n", vals[ iToken ].value );
            else if ( riscv64 == g_AssemblyTarget )
                fprintf( fp, "    li       a0, %d\n", vals[ iToken ].value ); // will this work for large constants?
            else if ( m68kCPM == g_AssemblyTarget )
                fprintf( fp, "    move.l   #%d, d0\n", vals[ iToken ].value );
            else if ( oiOS == g_AssemblyTarget )
            {
                if ( 0 == vals[ iToken ].value )
                    fprintf( fp, "    zero     rres\n" );
                else
                    fprintf( fp, "    ldi      rres, %d\n", vals[ iToken ].value );
            }

            iToken++;
        }
        else if ( Token_CLOSEPAREN == t )
        {
            assert( false && "why is there a close paren?" );
            iToken++;
        }
        else if ( Token_NOT == t )
        {
            iToken++;

            assert( Token_VARIABLE == vals[ iToken ].token );

            string const & varname = vals[ iToken ].strValue;

            if ( x64Win == g_AssemblyTarget )
            {
                if ( IsVariableInReg( varmap, varname ) )
                    fprintf( fp, "    cmp      %s, 0\n", GenVariableReg( varmap, varname ) );
                else
                    fprintf( fp, "    cmp      DWORD PTR [%s], 0\n", GenVariableName( varname ) );

                fprintf( fp, "    sete     al\n" );
                fprintf( fp, "    movzx    rax, al\n" );
            }
            else if ( arm32Linux == g_AssemblyTarget )
            {
                fprintf( fp, "    mov      r0, #0\n" );

                if ( IsVariableInReg( varmap, varname ) )
                    fprintf( fp, "    cmp      %s, #0\n", GenVariableReg( varmap, varname ) );
                else
                {
                    LoadArm32Address( fp, "r1", varname );
                    fprintf( fp, "    ldr      r1, [r1]\n" );
                    fprintf( fp, "    cmp      r1, #0\n" );
                }

                fprintf( fp, "    moveq    r0, #1\n" );
            }
            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
            {
                if ( IsVariableInReg( varmap, varname ) )
                    fprintf( fp, "    cmp      %s, 0\n", GenVariableReg( varmap, varname ) );
                else
                {
                    LoadArm64Address( fp, "x1", varname );
                    fprintf( fp, "    ldr      x0, [x1]\n" );
                    fprintf( fp, "    cmp      x0, 0\n" );
                }

                fprintf( fp, "    cset     x0, eq\n" );
            }
            else if ( i8080CPM == g_AssemblyTarget )
            {
                static int s_notLabel = 0;

                fprintf( fp, "    lhld     %s\n", GenVariableName( varname ) );
                fprintf( fp, "    mov      a, h\n" );
                fprintf( fp, "    mvi      h, 0\n" );
                fprintf( fp, "    ora      l\n" );
                fprintf( fp, "    jz       nl$%d\n", s_notLabel );
                fprintf( fp, "    mvi      l, 0\n" );
                fprintf( fp, "    jmp      nl2$%d\n", s_notLabel );
                fprintf( fp, "  nl$%d:\n", s_notLabel );
                fprintf( fp, "    mvi      l, 1\n" );
                fprintf( fp, "  nl2$%d\n", s_notLabel );

                s_notLabel++;
            }
            else if ( mos6502Apple1 == g_AssemblyTarget )
            {
                static int s_notLabel = 0;

                fprintf( fp, "    lda      #0\n" );
                fprintf( fp, "    sta      curOperand+1\n" );
                fprintf( fp, "    cmp      %s\n", GenVariableName( varname ) );
                fprintf( fp, "    bne      _not_done_%d\n", s_notLabel );
                fprintf( fp, "    cmp      %s+1\n", GenVariableName( varname ) );
                fprintf( fp, "    bne      _not_done_%d\n", s_notLabel );
                fprintf( fp, "    lda      #1\n" );
                fprintf( fp, "_not_done_%d\n", s_notLabel );
                fprintf( fp, "    sta      curOperand\n" );

                s_notLabel++;
            }
            else if ( i8086DOS == g_AssemblyTarget )
            {
                static int s_labelVal = 0;

                fprintf( fp, "    cmp      WORD PTR ds: [%s], 0\n", GenVariableName( varname ) );
                fprintf( fp, "    je       _not_true_%d\n", s_labelVal );
                fprintf( fp, "    mov      ax, 0\n" );
                fprintf( fp, "    jmp      _not_done_%d\n", s_labelVal );
                fprintf( fp, "  _not_true_%d:\n", s_labelVal );
                fprintf( fp, "    mov      ax, 1\n" );
                fprintf( fp, "  _not_done_%d:\n", s_labelVal );
        
                s_labelVal++;
            }
            else if ( x86Win == g_AssemblyTarget )
            {
                if ( IsVariableInReg( varmap, varname ) )
                    fprintf( fp, "    cmp      %s, 0\n", GenVariableReg( varmap, varname ) );
                else
                    fprintf( fp, "    cmp      DWORD PTR [%s], 0\n", GenVariableName( varname ) );

                fprintf( fp, "    sete     al\n" );
                fprintf( fp, "    movzx    eax, al\n" );
            }
            else if ( riscv64 == g_AssemblyTarget )
            {
                if ( IsVariableInReg( varmap, varname ) )
                    fprintf( fp, "    sltiu    a0, %s, 1\n", GenVariableReg( varmap, varname ) );
                else
                {
                    fprintf( fp, "    lla      a0, %s\n", GenVariableName( varname ) );
                    fprintf( fp, "    ld       a0, (a0)\n" );
                    fprintf( fp, "    sltiu    a0, a0, 1\n" );
                }
            }
            else if ( m68kCPM == g_AssemblyTarget )
            {
                fprintf( fp, "    clr.l    d0\n" ); // make sure high 3 bytes are 0
                if ( IsVariableInReg( varmap, varname ) )
                    fprintf( fp, "    cmp.l    #0, %s\n", GenVariableReg( varmap, varname ) );
                else
                    fprintf( fp, "    cmp.l    #0, (%s)\n", GenVariableName( varname ) );
                fprintf( fp, "    seq      d0\n" );
            }
            else if ( oiOS == g_AssemblyTarget )
            {
                if ( IsVariableInReg( varmap, varname ) )
                    fprintf( fp, "    cmp     rres, %s, rzero, eq\n", GenVariableReg( varmap, varname ) );
                else
                {
                    fprintf( fp, "    ld      rres, [%s]\n", GenVariableName( varname ) );
                    fprintf( fp, "    inv     rres\n" );
                }
            }

            iToken++;
        }
        else
        {
            printf( "unexpected token in GenerateFactor %d %s\n", t, TokenStr( t ) );
            RuntimeFail( "unexpected token", g_lineno );
        }
    }

    if ( EnableTracing && g_Tracing )
        printf( " leaving GenerateFactor, iToken %d\n", iToken );
} //GenerateFactor

void GenerateAdd( FILE * fp, map<string, Variable> const & varmap, int & iToken, int beyond, vector<TokenValue> const & vals )
{
    if ( EnableTracing && g_Tracing )
        printf( "in generate add, iToken %d\n", iToken );

    iToken++;

    GenerateTerm( fp, varmap, iToken, beyond, vals );

    if ( x64Win == g_AssemblyTarget )
    {
        fprintf( fp, "    pop      rbx\n" );
        fprintf( fp, "    add      rax, rbx\n" );
    }
    else if ( arm32Linux == g_AssemblyTarget )
    {
        fprintf( fp, "    pop      {r1}\n" );
        fprintf( fp, "    add      r0, r0, r1\n" );
    }
    else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
    {
        fprintf( fp, "    ldr      x1, [sp], #16\n" );
        fprintf( fp, "    add      w0, w0, w1\n" );
    }
    else if ( i8080CPM == g_AssemblyTarget )
    {
        fprintf( fp, "    pop      d\n" );
        fprintf( fp, "    dad      d\n" );
    }
    else if ( mos6502Apple1 == g_AssemblyTarget )
    {
        fprintf( fp, "    clc\n" );
        fprintf( fp, "    pla\n" );
        fprintf( fp, "    adc      curOperand\n" );
        fprintf( fp, "    sta      curOperand\n" );
        fprintf( fp, "    pla\n" );
        fprintf( fp, "    adc      curOperand+1\n" );
        fprintf( fp, "    sta      curOperand+1\n" );
    }
    else if ( i8086DOS == g_AssemblyTarget )
    {
        fprintf( fp, "    pop      bx\n" );
        fprintf( fp, "    add      ax, bx\n" );
    }
    else if ( x86Win == g_AssemblyTarget )
    {
        fprintf( fp, "    pop      ebx\n" );
        fprintf( fp, "    add      eax, ebx\n" );
    }
    else if ( riscv64 == g_AssemblyTarget )
    {
        RiscVPop( fp, "t0" );
        fprintf( fp, "    add      a0, a0, t0\n" );
    }
    else if ( m68kCPM == g_AssemblyTarget )
    {
        fprintf( fp, "    move.l   (a7)+, d1\n" );
        fprintf( fp, "    add.l    d1, d0\n" );
    }
    else if ( oiOS == g_AssemblyTarget )
        fprintf( fp, "    addst\n" );
} //GenerateAdd

void GenerateSubtract( FILE * fp, map<string, Variable> const & varmap, int & iToken, int beyond, vector<TokenValue> const & vals )
{
    if ( EnableTracing && g_Tracing )
        printf( "in generate subtract, iToken %d\n", iToken );

    iToken++;

    GenerateTerm( fp, varmap, iToken, beyond,  vals );

    if ( x64Win == g_AssemblyTarget )
    {
        fprintf( fp, "    mov      rbx, rax\n" );
        fprintf( fp, "    pop      rax\n" );
        fprintf( fp, "    sub      rax, rbx\n" );
    }
    else if ( arm32Linux == g_AssemblyTarget )
    {
        fprintf( fp, "    pop      {r1}\n" );
        fprintf( fp, "    sub      r0, r1, r0\n" );
    }
    else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
    {
        fprintf( fp, "    ldr      x1, [sp], #16\n" );
        fprintf( fp, "    sub      w0, w1, w0\n" );
    }
    else if ( i8080CPM == g_AssemblyTarget )
    {
        fprintf( fp, "    pop      d\n" );
        fprintf( fp, "    mov      a, e\n" );
        fprintf( fp, "    sub      l\n" );
        fprintf( fp, "    mov      l, a\n" );
        fprintf( fp, "    mov      a, d\n" );
        fprintf( fp, "    sbb      h\n" );
        fprintf( fp, "    mov      h, a\n" );
    }
    else if ( mos6502Apple1 == g_AssemblyTarget )
    {
        // the carry flag acts as a reverse borrow

        fprintf( fp, "    sec\n" );
        fprintf( fp, "    pla\n" );
        fprintf( fp, "    sbc      curOperand\n" );
        fprintf( fp, "    sta      curOperand\n" );
        fprintf( fp, "    pla\n" );
        fprintf( fp, "    sbc      curOperand+1\n" );
        fprintf( fp, "    sta      curOperand+1\n" );
    }
    else if ( i8086DOS == g_AssemblyTarget )
    {
        fprintf( fp, "    mov      bx, ax\n" );
        fprintf( fp, "    pop      ax\n" );
        fprintf( fp, "    sub      ax, bx\n" );
    }
    else if ( x86Win == g_AssemblyTarget )
    {
        fprintf( fp, "    mov      ebx, eax\n" );
        fprintf( fp, "    pop      eax\n" );
        fprintf( fp, "    sub      eax, ebx\n" );
    }
    else if ( riscv64 == g_AssemblyTarget )
    {
        fprintf( fp, "    mv       t0, a0\n" );
        RiscVPop( fp, "a0" );
        fprintf( fp, "    sub      a0, a0, t0\n" );
    }
    else if ( m68kCPM == g_AssemblyTarget )
    {
        fprintf( fp, "    move.l   d0, d1\n" );
        fprintf( fp, "    move.l   (a7)+, d0\n" );
        fprintf( fp, "    sub.l    d1, d0\n" );
    }
    else if ( oiOS == g_AssemblyTarget )
        fprintf( fp, "    subst\n" );
} //GenerateSubtract

void GenerateExpression( FILE * fp, map<string, Variable> const & varmap, int & iToken, int beyond, vector<TokenValue> const & vals )
{
    assert( iToken < beyond );
    assert( iToken < vals.size() );

    if ( EnableTracing && g_Tracing )
    {
        printf( "generate expression for line %d iToken %d %s\n", g_lineno, iToken, TokenStr( vals[ iToken ].token ) );

        for ( int i = iToken; i < vals.size(); i++ )
            printf( "    %d:    %s\n", i, TokenStr( vals[ i ].token ) );
    }

    // this will be called after an open paren, for example: ( 3 + 4 ) and not be an EXPRESSION in that case.

    if ( Token_EXPRESSION == vals[ iToken ].token )
        iToken++;

    // look for a unary + or -

    if ( isOperatorAdditive( vals[ iToken ].token ) )
    {
        // make the left side of the operation 0

        if ( x64Win == g_AssemblyTarget )
            fprintf( fp, "    xor      rax, rax\n" );
        else if ( arm32Linux == g_AssemblyTarget )
            fprintf( fp, "    mov      r0, #0\n" );
        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
            fprintf( fp, "    mov      x0, 0\n" );
        else if ( i8080CPM == g_AssemblyTarget )
            fprintf( fp, "    lxi      h, 0\n" );
        else if ( mos6502Apple1 == g_AssemblyTarget )
        {
            fprintf( fp, "    lda      #0\n" );
            fprintf( fp, "    sta      curOperand\n" );
            fprintf( fp, "    sta      curOperand+1\n" );
        }
        else if ( i8086DOS == g_AssemblyTarget )
            fprintf( fp, "    xor      ax, ax\n" );
        else if ( x86Win == g_AssemblyTarget )
            fprintf( fp, "    xor      eax, eax\n" );
        else if ( riscv64 == g_AssemblyTarget )
            fprintf( fp, "    mv       a0, zero\n" );
        else if ( m68kCPM == g_AssemblyTarget )
            fprintf( fp, "    clr.l    d0\n" );
        else if ( oiOS == g_AssemblyTarget )
            fprintf( fp, "    zero     rres\n" );
    }
    else
    {
        GenerateTerm( fp, varmap, iToken, beyond, vals );

        if ( iToken >= beyond )
            return;
    }

    Token t = vals[ iToken ].token;

    while ( isOperatorAdditive( t ) )
    {
        PushAccumulator( fp );

        if ( Token_PLUS == t )
            GenerateAdd( fp, varmap, iToken, beyond, vals );
        else
            GenerateSubtract( fp, varmap, iToken, beyond, vals );

        if ( iToken >= beyond )
            break;

        t = vals[ iToken ].token;
    }
} //GenerateExpression

template <class T>void Swap( T & a, T & b )
{
    T temp = a;
    a = b;
    b = temp;
} //Swap

void Generate6502Relation( FILE * fp, const char * lhs, const char * rhs, Token op, const char * truename, int truenumber )
{
    assert( isOperatorRelational( op ) );

    if ( Token_GE == op || Token_GT == op )
    {
        Swap( lhs, rhs );
    
        if ( Token_GE == op )
            op = Token_LE;
        else
            op = Token_LT;
    }
    
    // now only EQ, NE, LE, or LT

    static int gen6502Relation = 0;

    if ( Token_EQ == op )
    {
        fprintf( fp, "    lda      %s\n", lhs );
        fprintf( fp, "    cmp      %s\n", rhs );
        fprintf( fp, "    bne      _false_relation_%d\n", gen6502Relation );
        fprintf( fp, "    lda      %s+1\n", lhs );
        fprintf( fp, "    cmp      %s+1\n", rhs );
        fprintf( fp, "    beq      %s%d\n", truename, truenumber );
    }
    else if ( Token_NE == op )
    {
        fprintf( fp, "    lda      %s\n", lhs );
        fprintf( fp, "    cmp      %s\n", rhs );
        fprintf( fp, "    bne      %s%d\n", truename, truenumber );
        fprintf( fp, "    lda      %s+1\n", lhs );
        fprintf( fp, "    cmp      %s+1\n", rhs );
        fprintf( fp, "    bne      %s%d\n", truename, truenumber );
    }
    else if ( Token_LT == op || Token_LE == op )
    {
        fprintf( fp, "    sec\n" );
        fprintf( fp, "    lda      %s+1\n", lhs );
        fprintf( fp, "    sbc      %s+1\n", rhs );

        if ( Token_LE == op )
            fprintf( fp, "    beq      _label3_%d\n", gen6502Relation );

        fprintf( fp, "    bvc      _label1_%d\n", gen6502Relation );
        fprintf( fp, "    eor      #$80\n" );

        fprintf( fp, "_label1_%d:\n", gen6502Relation );
        fprintf( fp, "    bmi      %s%d\n", truename, truenumber );
        fprintf( fp, "    bvc      _label2_%d\n", gen6502Relation );
        fprintf( fp, "    eor      #$80\n" );

        fprintf( fp, "_label2_%d:\n", gen6502Relation );
        fprintf( fp, "    bne      _false_relation_%d\n", gen6502Relation );

        fprintf( fp, "_label3_%d:\n", gen6502Relation );
        fprintf( fp, "    lda      %s\n", lhs );
        fprintf( fp, "    sbc      %s\n", rhs );

        if ( Token_LE == op )
            fprintf( fp, "    beq      %s%d\n", truename, truenumber );

        fprintf( fp, "    bcc      %s%d\n", truename, truenumber );
    }
    else
    {
        assert( false && "unrecognized relational token\n" );
    }

    fprintf( fp, "_false_relation_%d:\n", gen6502Relation );    

    gen6502Relation++;
} //Generate6502Relation

void Generate8080Relation( FILE * fp, Token op, const char * truename, int truenumber )
{
    assert( isOperatorRelational( op ) );

    // ( de == lhs ) op ( hl == rhs )
    // de and hl are not guaranteed to survive

    if ( Token_GE == op || Token_GT == op )
    {
        fprintf( fp, "    xchg\n" );  // swap de and hl
    
        if ( Token_GE == op )
            op = Token_LE;
        else
            op = Token_LT;
    }
    
    // now only EQ, NE, LE, or LT

    static int gen8080Relation = 0;

    if ( Token_EQ == op )
    {
        fprintf( fp, "    mov      a, e\n" );
        fprintf( fp, "    cmp      l\n" );
        fprintf( fp, "    jnz      fRE%d\n", gen8080Relation );
        fprintf( fp, "    mov      a, d\n" );
        fprintf( fp, "    cmp      h\n" );
        fprintf( fp, "    jz       %s%d\n", truename, truenumber );
    }
    else if ( Token_NE == op )
    {
        fprintf( fp, "    mov      a, e\n" );
        fprintf( fp, "    cmp      l\n" );
        fprintf( fp, "    jnz      %s%d\n", truename, truenumber );
        fprintf( fp, "    mov      a, d\n" );
        fprintf( fp, "    cmp      h\n" );
        fprintf( fp, "    jnz      %s%d\n", truename, truenumber );
    }
    else if ( Token_LT == op || Token_LE == op )
    {
        if ( Token_LE == op )
        {
            // check for equality

            fprintf( fp, "    mov      a, e\n" );
            fprintf( fp, "    cmp      l\n" );
            fprintf( fp, "    jnz      ltRE%d\n", gen8080Relation );
            fprintf( fp, "    mov      a, d\n" );
            fprintf( fp, "    cmp      h\n" );
            fprintf( fp, "    jz       %s%d\n", truename, truenumber );
        }

        fprintf( fp, "  ltRE%d:\n", gen8080Relation ); // check for less than

        // are the high bits the same and so the same signs?

        fprintf( fp, "    mov      a, d\n" );
        fprintf( fp, "    xra      h\n" );
        fprintf( fp, "    jp       ssRE%d\n", gen8080Relation );

        fprintf( fp, "    xra      d\n" );
        fprintf( fp, "    jm       fRE%d\n", gen8080Relation );
        fprintf( fp, "    jmp      %s%d\n", truename, truenumber );

        fprintf( fp, "  ssRE%d:\n", gen8080Relation ); // same sign

        fprintf( fp, "    mov      a, e\n" );
        fprintf( fp, "    sub      l\n" );
        fprintf( fp, "    mov      a, d\n" );
        fprintf( fp, "    sbb      h\n" );

        fprintf( fp, "    jc       %s%d\n", truename, truenumber );
    }
    else
    {
        assert( false && "unrecognized relational token\n" );
    }

    fprintf( fp, "  fRE%d:\n", gen8080Relation );    // false -- fall through

    gen8080Relation++;
} //Generate8080Relation

void GenerateRelational( FILE * fp, map<string, Variable> const & varmap, int & iToken, int beyond, vector<TokenValue> const & vals )
{
    assert( iToken < beyond );
    assert( iToken < vals.size() );

    if ( EnableTracing && g_Tracing )
        printf( "in generate relational, iToken %d\n", iToken );

    Token op = vals[ iToken ].token;
    iToken++;

    GenerateExpression( fp, varmap, iToken, beyond, vals );

    assert( isOperatorRelational( op ) );

    if ( x64Win == g_AssemblyTarget )
    {
        fprintf( fp, "    mov      rbx, rax\n" );
        fprintf( fp, "    pop      rax\n" );
        fprintf( fp, "    cmp      eax, ebx\n" );
        fprintf( fp, "    %-6s   al\n", OperatorInstructionX64[ op ] );
        fprintf( fp, "    movzx    rax, al\n" );
    }
    else if ( arm32Linux == g_AssemblyTarget )
    {
        fprintf( fp, "    pop      {r1}\n" );
        fprintf( fp, "    cmp      r1, r0\n" );
        fprintf( fp, "    mov      r0, #0\n" );
        fprintf( fp, "    mov%s    r0, #1\n", ConditionsArm[ op ] );
    }
    else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
    {
        fprintf( fp, "    mov      x2, 1\n" );
        fprintf( fp, "    ldr      x1, [sp], #16\n" );
        fprintf( fp, "    cmp      w1, w0\n" );
        fprintf( fp, "    csel     x0, x2, xzr, %s\n", ConditionsArm[ op ] );
    }
    else if ( i8080CPM == g_AssemblyTarget )
    {
        static int s_labelVal = 0;

        fprintf( fp, "    pop      d\n" );

        Generate8080Relation( fp, op, "tRE", s_labelVal );
        fprintf( fp, "    lxi      h, 0\n" );           // false
        fprintf( fp, "    jmp      dRE%d\n", s_labelVal );

        fprintf( fp, "  tRE%d:\n", s_labelVal );        // t = true
        fprintf( fp, "    lxi      h, 1\n" );

        fprintf( fp, "  dRE%d:\n", s_labelVal );        // d = done

        s_labelVal++;
    }
    else if ( mos6502Apple1 == g_AssemblyTarget )
    {
        static int s_labelVal = 0;

        // "EQ", "NE", "LE", "GE", "LT", "GT"
        // stack == lhs, curOperand = rhs

        fprintf( fp, "    pla\n" );
        fprintf( fp, "    sta      otherOperand\n" );
        fprintf( fp, "    pla\n" );
        fprintf( fp, "    sta      otherOperand+1\n" );
        fprintf( fp, "    ldy      #1\n" );      // default is true

        Generate6502Relation( fp, "otherOperand", "curOperand", op, "_relational_true_", s_labelVal );
        fprintf( fp, "    ldy      #0\n" );      // relation is false

        fprintf( fp, "_relational_true_%d:\n", s_labelVal );
        fprintf( fp, "    sty      curOperand\n" );
        fprintf( fp, "    lda      #0\n" );
        fprintf( fp, "    sta      curOperand+1\n" );

        s_labelVal++;
    }
    else if ( i8086DOS == g_AssemblyTarget )
    {
        static int s_labelVal = 0;

        fprintf( fp, "    mov      bx, ax\n" );
        fprintf( fp, "    pop      ax\n" );
        fprintf( fp, "    cmp      ax, bx\n" );
        fprintf( fp, "    %-6s   _relational_true_%d\n", RelationalInstructionX64[ op ], s_labelVal ); // same as x64
        fprintf( fp, "    mov      ax, 0\n" );
        fprintf( fp, "    jmp      _relational_done_%d\n", s_labelVal );
        fprintf( fp, "_relational_true_%d:\n", s_labelVal );
        fprintf( fp, "    mov      ax, 1\n" );
        fprintf( fp, "_relational_done_%d:\n", s_labelVal );

        s_labelVal++;
    }
    else if ( x86Win == g_AssemblyTarget )
    {
        fprintf( fp, "    mov      ebx, eax\n" );
        fprintf( fp, "    pop      eax\n" );
        fprintf( fp, "    cmp      eax, ebx\n" );
        fprintf( fp, "    %-6s   al\n", OperatorInstructionX64[ op ] );
        fprintf( fp, "    movzx    eax, al\n" );
    }
    else if ( m68kCPM == g_AssemblyTarget )
    {
        fprintf( fp, "    move.l   d0, d1\n" );
        fprintf( fp, "    move.l   (a7)+, d0\n" );
        fprintf( fp, "    cmp.l    d1, d0\n" );
        fprintf( fp, "    %-6s     d0\n", OperatorInstructionm68kCPM[ op ] );
        fprintf( fp, "    andi.l   #1, d0\n" );
    }
    else if ( riscv64 == g_AssemblyTarget )
    {
        // Helpful: https://stackoverflow.com/questions/64547741/how-is-risc-v-neg-instruction-imeplemented
        // yes, the typo in the url is killing me

        fprintf( fp, "    mv       t0, a0\n" );
        RiscVPop( fp, "a0" );

        fprintf( fp, "    sub      t0, a0, t0\n" );
        if ( Token_EQ == op )
            fprintf( fp, "    sltiu    a0, t0, 1\n" );
        else if ( Token_NE == op )
            fprintf( fp, "    sltu     a0, zero, t0\n" );
        else if ( Token_LT == op )
            fprintf( fp, "    slt      a0, t0, zero\n" );
        else if ( Token_GT == op )
            fprintf( fp, "    slt      a0, zero, t0\n" );
        else // LE or GE
        {
            fprintf( fp, "    sltiu    a0, t0, 1\n" ); // check for ==

            if ( Token_LE == op )
                fprintf( fp, "    slt      t1, t0, zero\n" );
            else // GE
                fprintf( fp, "    slt      t1, zero, t0\n" );

            fprintf( fp, "    or       a0, a0, t1\n" ); // either == or the L/G
        }
    }
    else if ( oiOS == g_AssemblyTarget )
        fprintf( fp, "    cmpst    rres, rres, %s\n", ConditionsArm[ op ] ); // rres = ( top_of_stack OP rrres )
} //GenerateRelational

void GenerateRelationalExpression( FILE * fp, map<string, Variable> const & varmap, int & iToken, int beyond, vector<TokenValue> const & vals )
{
    assert( iToken < beyond );
    assert( iToken < vals.size() );

    if ( EnableTracing && g_Tracing )
    {
        printf( "generate relational expression for line %d iToken %d %s\n", g_lineno, iToken, TokenStr( vals[ iToken ].token ) );

        for ( int i = iToken; i < beyond; i++ )
            printf( "    %d:    %s\n", i, TokenStr( vals[ i ].token ) );
    }

    // This won't be an EXPRESSION for cases like x = x + ...
    // But it will be EXPRESSION when called from GenerateLogicalExpression

    if ( Token_EXPRESSION == vals[ iToken ].token )
        iToken++;

    GenerateExpression( fp, varmap, iToken, beyond, vals );

    if ( iToken >= beyond )
        return;

    Token t = vals[ iToken ].token;

    while ( isOperatorRelational( t ) )
    {
        PushAccumulator( fp );

        GenerateRelational( fp, varmap, iToken, beyond, vals );

        if ( iToken >= beyond )
            break;

        t = vals[ iToken ].token;
    }
} //GenerateRelationalExpression

void GenerateLogical( FILE * fp, map<string, Variable> const & varmap, int & iToken, int beyond, vector<TokenValue> const & vals )
{
    assert( iToken < beyond );
    assert( iToken < vals.size() );

    if ( EnableTracing && g_Tracing )
        printf( "in generate logical, iToken %d\n", iToken );

    Token op = vals[ iToken ].token;
    iToken++;

    GenerateRelationalExpression( fp, varmap, iToken, beyond, vals );

    assert( isOperatorLogical( op ) ); // AND, OR, XOR

    if ( x64Win == g_AssemblyTarget )
    {
        fprintf( fp, "    pop      rbx\n" );
        fprintf( fp, "    %-6s   rax, rbx\n", OperatorInstructionX64[ op ] );
    }
    else if ( arm32Linux == g_AssemblyTarget )
    {
        fprintf( fp, "    pop      {r1}\n" );
        fprintf( fp, "    %-6s   r0, r1, r0\n", OperatorInstructionArm[ op ] );
    }
    else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
    {
        fprintf( fp, "    ldr      x1, [sp], #16\n" );
        fprintf( fp, "    %-6s   x0, x1, x0\n", OperatorInstructionArm[ op ] );
    }   
    else if ( i8080CPM == g_AssemblyTarget )
    {
        // lhs in de, rhs in hl

        fprintf( fp, "    pop      d\n" );
        fprintf( fp, "    mov      a, h\n" );
        fprintf( fp, "    %-6s   d\n", OperatorInstructioni8080[ op ] );
        fprintf( fp, "    mov      h, a\n" );
        fprintf( fp, "    mov      a, l\n" );
        fprintf( fp, "    %-6s   e\n", OperatorInstructioni8080[ op ] );
        fprintf( fp, "    mov      l, a\n" );
    }
    else if ( mos6502Apple1 == g_AssemblyTarget )
    {
        fprintf( fp, "    pla\n" );
        fprintf( fp, "    %-6s   curOperand\n", OperatorInstruction6502[ op ] );
        fprintf( fp, "    sta      curOperand\n" );
        fprintf( fp, "    pla\n" );
        fprintf( fp, "    %-6s   curOperand+1\n", OperatorInstruction6502[ op ] );
        fprintf( fp, "    sta      curOperand+1\n" );
    }
    else if ( i8086DOS == g_AssemblyTarget )
    {
        fprintf( fp, "    pop      bx\n" );
        fprintf( fp, "    %-6s   ax, bx\n", OperatorInstructionX64[ op ] ); // same as x64 instructions
    }
    else if ( x86Win == g_AssemblyTarget )
    {
        fprintf( fp, "    pop      ebx\n" );
        fprintf( fp, "    %-6s   eax, ebx\n", OperatorInstructionX64[ op ] ); // same as x64 instructions
    }
    else if ( riscv64 == g_AssemblyTarget )
    {
        RiscVPop( fp, "t0" );
        fprintf( fp, "    %-6s   a0, a0, t0\n", OperatorInstructionRiscV64[ op ] );
    }
    else if ( m68kCPM == g_AssemblyTarget )
    {
        fprintf( fp, "    move.l (a7)+, d1\n" );
        fprintf( fp, "    %-6s   d1, d0\n", OperatorInstructionm68kCPM[ op ] );
    }
    else if ( oiOS == g_AssemblyTarget )
        fprintf( fp, "    mathst   rres, rres, %-6s\n", OperatorInstructionX64[ op ] ); // same as x64 instructions
} //GenerateLogical

void GenerateLogicalExpression( FILE * fp, map<string, Variable> const & varmap, int & iToken, vector<TokenValue> const & vals )
{
    int beyond = iToken + vals[ iToken ].value;

    assert( iToken < beyond );
    assert( beyond <= vals.size() );
    assert( iToken < vals.size() );

    if ( EnableTracing && g_Tracing )
    {
        printf( "generate logical expression for line %d iToken %d %s\n", g_lineno, iToken, TokenStr( vals[ iToken ].token ) );

        for ( int i = iToken; i < beyond; i++ )
            printf( "    %d:    %s\n", i, TokenStr( vals[ i ].token ) );
    }

    assert( Token_EXPRESSION == vals[ iToken ].token );

    GenerateRelationalExpression( fp, varmap, iToken, beyond, vals );

    if ( iToken >= beyond )
        return;

    Token t = vals[ iToken ].token;

    while ( isOperatorLogical( t ) )
    {
        PushAccumulator( fp );

        GenerateLogical( fp, varmap, iToken, beyond, vals );

        if ( iToken >= beyond )
            break;

        t = vals[ iToken ].token;
    }
} //GenereateLogicalExpression

void GenerateOptimizedExpression( FILE * fp, map<string, Variable> const & varmap, int & iToken, vector<TokenValue> const & vals )
{
    // On x64, result is in rax. only modifies rax, rbx, and rdx (without saving them)
    // On arm32, result is in r0
    // On arm64, result is in w0
    // On i8080, result is in hl
    // On mos6502, result is in curOperand
    // On i8086, result is in ax
    // On i386, result is in eax
    // On riscv64, result is in a0
    // On oiOS, result is in rres
    // On m68kCPM, result is in d0

    assert( Token_EXPRESSION == vals[ iToken ].token );
    int tokenCount = vals[ iToken ].value;

    #ifdef DEBUG
        int firstToken = iToken;
    #endif

    if ( EnableTracing && g_Tracing )
        printf( "  GenerateOptimizedExpression token %d, which is %s, length %d\n",
                iToken, TokenStr( vals[ iToken ].token ), vals[ iToken ].value );

    if ( i8080CPM == g_AssemblyTarget || arm32Linux == g_AssemblyTarget ||
         i8086DOS == g_AssemblyTarget || x86Win == g_AssemblyTarget ||
         riscv64 == g_AssemblyTarget || oiOS == g_AssemblyTarget ||
         m68kCPM == g_AssemblyTarget || !g_ExpressionOptimization )
        goto label_no_expression_optimization;

    if ( 2 == tokenCount )
    {
        if ( Token_CONSTANT == vals[ iToken + 1 ].token )
        {
            if ( x64Win == g_AssemblyTarget )
                fprintf( fp, "    mov      eax, %d\n", vals[ iToken + 1 ].value );
            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                LoadArm64Constant( fp, "x0", vals[ iToken + 1 ].value );
            else if ( mos6502Apple1 == g_AssemblyTarget )
            {
                fprintf( fp, "    lda      #%d\n", vals[ iToken + 1 ].value );
                fprintf( fp, "    sta      curOperand\n" );
                fprintf( fp, "    lda      /%d\n", vals[ iToken + 1 ].value );
                fprintf( fp, "    sta      curOperand+1\n" );
            }
        }
        else if ( Token_VARIABLE == vals[ iToken + 1 ].token )
        {
            string const & varname = vals[ iToken + 1 ].strValue;

            if ( x64Win == g_AssemblyTarget )
            {   
                if ( IsVariableInReg( varmap, varname ) )
                    fprintf( fp, "    mov      eax, %s\n", GenVariableReg( varmap, varname ) );
                else
                    fprintf( fp, "    mov      eax, [%s]\n", GenVariableName( varname ) );
            }
            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
            {
                if ( IsVariableInReg( varmap, varname ) )
                    fprintf( fp, "    mov      w0, %s\n", GenVariableReg( varmap, varname ) );
                else
                {
                    LoadArm64Address( fp, "x1", varname );
                    fprintf( fp, "    ldr      w0, [x1]\n" );
                }
            }
            else if ( mos6502Apple1 == g_AssemblyTarget )
            {
                fprintf( fp, "    lda      %s\n", GenVariableName( varname ) );
                fprintf( fp, "    sta      curOperand\n" );
                fprintf( fp, "    lda      %s+1\n", GenVariableName( varname ) );
                fprintf( fp, "    sta      curOperand+1\n" );
            }
        }

        iToken += tokenCount;
    }
    else if ( 6 == tokenCount &&
              Token_VARIABLE == vals[ iToken + 1 ].token &&
              Token_OPENPAREN == vals[ iToken + 2 ].token )
    {
        // sa%( st% ) or sa%( 10 )
        // 0 EXPRESSION, value 6, strValue ''
        // 1 VARIABLE, value 0, strValue 'sa%'
        // 2 OPENPAREN, value 0, strValue ''
        // 3 EXPRESSION, value 2, strValue ''
        // 4 VARIABLE, value 0, strValue 'st%'   (this can optionally be a constant)
        // 5 CLOSEPAREN, value 0, strValue ''

        if ( 1 != vals[ iToken + 1 ].dimensions ) // can't be > 1 or tokenCount would be greater
            RuntimeFail( "scalar variable used as an array", g_lineno );

        if ( Token_CONSTANT == vals[ iToken + 4 ].token )
        {
            string const & varname = vals[ iToken + 1 ].strValue;

            if ( x64Win == g_AssemblyTarget )
                fprintf( fp, "    mov      eax, DWORD PTR[ %s + %d ]\n", GenVariableName( varname ),
                         4 * vals[ iToken + 4 ].value );
            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
            {
                LoadArm64Address( fp, "x1", varname );

                int offset = 4 * vals[ iToken + 4 ].value;

                if ( fitsIn8Bits( offset ) )
                    fprintf( fp, "    ldr      w0, [x1, %d]\n", offset );
                else
                {
                    LoadArm64Constant( fp, "x0", offset );
                    fprintf( fp, "    add      x1, x1, x0\n" );
                    fprintf( fp, "    ldr      w0, [x1]\n" );
                }
            }
            else if ( mos6502Apple1 == g_AssemblyTarget )
            {
                fprintf( fp, "    lda      #%d\n", 2 * vals[ iToken + 4 ].value );
                fprintf( fp, "    clc\n" );
                fprintf( fp, "    adc      #%s\n", GenVariableName( varname ) );
                fprintf( fp, "    sta      arrayOffset\n" );
                fprintf( fp, "    lda      /%s\n", GenVariableName( varname ) );
                fprintf( fp, "    adc      /%d\n", 2 * vals[ iToken + 4 ].value );
                fprintf( fp, "    sta      arrayOffset+1\n" );
                fprintf( fp, "    ldy      #0\n" );
                fprintf( fp, "    lda      (arrayOffset), y\n" );
                fprintf( fp, "    sta      curOperand\n" );
                fprintf( fp, "    iny\n" );
                fprintf( fp, "    lda      (arrayOffset), y\n" );
                fprintf( fp, "    sta      curOperand+1\n" );
            }
        }
        else
        {
            int iStart = iToken + 3;
            GenerateOptimizedExpression( fp, varmap, iStart, vals );
    
            string const & varname = vals[ iToken + 1 ].strValue;

            if ( x64Win == g_AssemblyTarget )
            {
                fprintf( fp, "    lea      rdx, [%s]\n", GenVariableName( varname ) );
                fprintf( fp, "    shl      rax, 2\n" );
                fprintf( fp, "    mov      eax, [rax + rdx]\n" );
            }
            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
            {
                LoadArm64Address( fp, "x1", varname );
                fprintf( fp, "    add      x1, x1, x0, lsl #2\n" );
                fprintf( fp, "    ldr      w0, [x1]\n" );
            }
            else if ( mos6502Apple1 == g_AssemblyTarget )
            {
                fprintf( fp, "    asl      curOperand\n" );   // shift left into carry
                fprintf( fp, "    rol      curOperand+1\n" ); // shift left from carry
                fprintf( fp, "    lda      #%s\n", GenVariableName( varname ) );
                fprintf( fp, "    clc\n" );
                fprintf( fp, "    adc      curOperand\n" );
                fprintf( fp, "    sta      curOperand\n" );
                fprintf( fp, "    lda      /%s\n", GenVariableName( varname ) );
                fprintf( fp, "    adc      curOperand+1\n" );
                fprintf( fp, "    sta      curOperand+1\n" );

                fprintf( fp, "    ldy      #0\n" );
                fprintf( fp, "    lda      (curOperand), y\n" );
                fprintf( fp, "    tax\n" );
                fprintf( fp, "    iny\n" );
                fprintf( fp, "    lda      (curOperand), y\n" );
                fprintf( fp, "    sta      curOperand+1\n" );
                fprintf( fp, "    stx      curOperand\n" );
            }
        }

        iToken += tokenCount;
    }
    else if ( mos6502Apple1 != g_AssemblyTarget &&
              4 == tokenCount )
    {
        assert( isTokenSimpleValue( vals[ iToken + 1 ].token ) );
        assert( isTokenOperator( vals[ iToken + 2 ].token ) );
        assert( isTokenSimpleValue( vals[ iToken + 3 ].token ) );

        GenerateOp( fp, varmap, vals, iToken + 1, iToken + 3, vals[ iToken + 2 ].token );

        iToken += tokenCount;
    }
    else if ( x64Win == g_AssemblyTarget && 3 == tokenCount )
    {
        if ( Token_NOT == vals[ iToken + 1 ].token )
        {
            string const & varname = vals[ iToken + 2 ].strValue;
            
            if ( IsVariableInReg( varmap, varname ) )
                fprintf( fp, "    cmp      %s, 0\n", GenVariableReg( varmap, varname ) );
            else
                fprintf( fp, "    cmp      DWORD PTR [%s], 0\n", GenVariableName( varname ) );

            fprintf( fp, "    sete     al\n" );
            fprintf( fp, "    movzx    rax, al\n" );
        }
        else
        {
            assert( Token_MINUS == vals[ iToken + 1 ].token );

            string const & varname = vals[ iToken + 2 ].strValue;
            if ( IsVariableInReg( varmap, varname ) )
                fprintf( fp, "    mov      eax, %s\n", GenVariableReg( varmap, varname ) );
            else
                fprintf( fp, "    mov      eax, [%s]\n", GenVariableName( varname ) );

            fprintf( fp, "    neg      rax\n" );
        }

        iToken += tokenCount;
    }
    else if ( mos6502Apple1 != g_AssemblyTarget &&
              16 == tokenCount &&
              Token_VARIABLE == vals[ iToken + 1 ].token &&
              Token_OPENPAREN == vals[ iToken + 4 ].token &&
              Token_CONSTANT == vals[ iToken + 6 ].token &&
              Token_VARIABLE == vals[ iToken + 9 ].token &&
              Token_OPENPAREN == vals[ iToken + 12 ].token &&
              Token_CONSTANT == vals[ iToken + 14 ].token &&
              isOperatorRelational( vals[ iToken + 2 ].token ) &&
              isOperatorRelational( vals[ iToken + 10 ].token ) )
    {
        //  0 EXPRESSION, value 16, strValue ''
        //  1 VARIABLE, value 0, strValue 'wi%'
        //  2 EQ, value 0, strValue ''
        //  3 VARIABLE, value 0, strValue 'b%'
        //  4 OPENPAREN, value 0, strValue ''
        //  5 EXPRESSION, value 2, strValue ''
        //  6 CONSTANT, value 5, strValue ''
        //  7 CLOSEPAREN, value 0, strValue ''
        //  8 AND, value 0, strValue ''
        //  9 VARIABLE, value 0, strValue 'wi%'
        // 10 EQ, value 0, strValue ''
        // 11 VARIABLE, value 0, strValue 'b%'
        // 12 OPENPAREN, value 0, strValue ''
        // 13 EXPRESSION, value 2, strValue ''
        // 14 CONSTANT, value 8, strValue ''
        // 15 CLOSEPAREN, value 0, strValue ''

        //value = run_operator( run_operator( vals[ iToken + 1 ].pVariable->value,
        //                                    vals[ iToken + 2 ].token,
        //                                    vals[ iToken + 3 ].pVariable->array[ vals[ iToken + 6 ].value ] ),
        //                      vals[ iToken + 8 ].token,
        //                      run_operator( vals[ iToken + 9 ].pVariable->value,
        //                                    vals[ iToken + 10 ].token,
        //                                    vals[ iToken + 11 ].pVariable->array[ vals[ iToken + 14 ].value ] ) );

        GenerateOp( fp, varmap, vals, iToken + 1, iToken + 3, vals[ iToken + 2 ].token, 0, iToken + 6 );

        if ( Token_AND == vals[ iToken + 8 ].token )
        {
            if ( x64Win == g_AssemblyTarget )
            {
                fprintf( fp, "    test     rax, rax\n" );
                fprintf( fp, "    jz       label_early_out_%d\n", g_pc );
            }
            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                fprintf( fp, "    cbz      w0, label_early_out_%d\n", g_pc );
        }

        if ( x64Win == g_AssemblyTarget )
            fprintf( fp, "    mov      rdx, rax\n" );
        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
            fprintf( fp, "    mov      x5, x0\n" );
    
        GenerateOp( fp, varmap, vals, iToken + 9, iToken + 11, vals[ iToken + 10 ].token, 0, iToken + 14 );

        Token finalOp = vals[ iToken + 8 ].token;
        if ( isOperatorRelational( finalOp ) )
        {
            if ( x64Win == g_AssemblyTarget )
            {
                fprintf( fp, "    cmp      rax, rdx\n" );
                fprintf( fp, "    %-6s   al\n", OperatorInstructionX64[ finalOp ] );
                fprintf( fp, "    movzx    rax, al\n" );
            }
            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
            {
                fprintf( fp, "    cmp      w0, w5\n" );
                fprintf( fp, "    cset     x0, %s\n", ConditionsArm[ finalOp ] );
            }
        }
        else
        {
            if ( x64Win == g_AssemblyTarget )
                fprintf( fp, "    %-6s   rax, rdx\n", OperatorInstructionX64[ finalOp ] );
            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                fprintf( fp, "    %-6s   w0, w0, w5\n", OperatorInstructionArm[ finalOp ] );

            if ( Token_AND == vals[ iToken + 8 ].token )
            {
                if ( arm64Mac == g_AssemblyTarget )
                    fprintf( fp, "  .p2align 3\n" );

                if ( arm64Win == g_AssemblyTarget )
                    fprintf( fp, "label_early_out_%d\n", g_pc );
                else
                    fprintf( fp, "  label_early_out_%d:\n", g_pc );
            }
        }

        iToken += tokenCount;
    }
    else
    {
label_no_expression_optimization:

        GenerateLogicalExpression( fp, varmap, iToken, vals );
    }

    #ifdef DEBUG
    assert( iToken == ( firstToken + tokenCount ) );
    #endif
} //GenerateOptimizedExpression

struct VarCount
{
    VarCount() : name( 0 ), refcount( 0 ) {};
    const char * name;
    int refcount;
};

static int CompareVarCount( const void * a, const void * b )
{
    // sort by # of refcounts high to low, so we know which variables are referenced most frequently

    VarCount const * pa = (VarCount const *) a;
    VarCount const * pb = (VarCount const *) b;

    return pb->refcount - pa->refcount;
} //CompareVarCount

std::string replace(const std::string& str, const std::string& searchStr,const std::string& replacer)
{
    if (searchStr == "")
        return str;

    std::string result = "";
    size_t pos = 0;
    size_t pos2 = str.find(searchStr, pos);

    while (pos2 != std::string::npos)
    {
        result += str.substr(pos, pos2-pos) + replacer;
        pos = pos2 + searchStr.length();
        pos2 = str.find(searchStr, pos);
    }

    result += str.substr(pos, str.length()-pos);
    return result;
} //replace

string singleQuoteEscape( string & str )
{
    string result;

    for ( const char * p = str.c_str(); *p; p++ )
    {
        if ( '\'' == *p )
            result += *p;

        result += *p;
    }

    return result;
} //singleQuoteEscape

string arm64WinEscape( string & str )
{
    string result;

    for ( const char * p = str.c_str(); *p; p++ )
    {
        result += *p;

        if ( '"' == *p || '$' == *p )
            result += *p;
    }

    return result;
} //arm64WinEscape

string arm64MacEscape( string & str )
{
    string result;

    for ( const char * p = str.c_str(); *p; p++ )
    {
        if ( '"' == *p )
            result += '\\';

        result += *p;
    }

    return result;
} //arm64MacEscape

string mos6502Escape( string & str )
{
    // escape characters in a 6502 string constant. I think just a single-quote is the only one

    string result;

    for ( const char * p = str.c_str(); *p; p++ )
    {
        if ( '\'' == *p )
            result += '\\';

        result += *p;
    }

    return result;
} //mos6502Escape

string cpm68kEscape( string & str )
{
    // single quotes must be called out as bytes

    string result;

    result += '\'';

    for ( const char * p = str.c_str(); *p; p++ )
    {
        if ( '\'' == *p )
        {
            result += '\'';
            result += ',';
            result += '3';
            result += '9';
            result += ',';
            result += '\'';
        }
        else
            result += *p;
    }

    result += '\'';
    return result;
} //cpm68kEscape

const char * RemoveExclamations( const char * pc )
{
    // cp/m comments begin with ';' but end with an exclamation mark. Remove them.

    static char line[ 300 ];
    strcpy( line, pc );

    for ( char * p = line; *p; p++ )
        if ( '!' == *p )
            *p = '.';

    return line;
} //RemoveExclamations

void GenerateASM( const char * outputfile, map<string, Variable> & varmap, bool useRegistersInASM )
{
    // some C implemenations don't truncate with "w"
    remove( outputfile );

    CFile fileOut( fopen( outputfile, "w" ) );
    FILE * fp = fileOut.get();
    if ( NULL == fileOut.get() )
    {
        printf( "can't open output file %s\n", outputfile );
        Usage();
    }

    int mos6502FirstZeroPageVariable = 0x40;

    if ( x64Win == g_AssemblyTarget )
    {
        fprintf( fp, "; Build on Windows in a Visual Studio vcvars64.bat cmd window using a .bat script like this:\n" );
        fprintf( fp, "; ml64 /nologo %%1.asm /Zd /Zf /Zi /link /OPT:REF /nologo ^\n" );
        fprintf( fp, ";      /subsystem:console ^\n" );
        fprintf( fp, ";      /defaultlib:kernel32.lib ^\n" );
        fprintf( fp, ";      /defaultlib:user32.lib ^\n" );
        fprintf( fp, ";      /defaultlib:libucrt.lib ^\n" );
        fprintf( fp, ";      /defaultlib:libcmt.lib ^\n" );
        fprintf( fp, ";      /entry:mainCRTStartup\n" );
        fprintf( fp, ";\n" );
        fprintf( fp, "; BA flags: use registers: %s, expression optimization: %s\n", YesNo( useRegistersInASM ), YesNo( g_ExpressionOptimization ) );

        fprintf( fp, "extern printf: PROC\n" );
        fprintf( fp, "extern exit: PROC\n" );
        fprintf( fp, "extern atoi: PROC\n" );
        fprintf( fp, "extern QueryPerformanceCounter: PROC\n" );
        fprintf( fp, "extern QueryPerformanceFrequency: PROC\n" );
        fprintf( fp, "extern GetLocalTime: PROC\n" );
        fprintf( fp, "data_segment SEGMENT ALIGN( 4096 ) 'DATA'\n" );
    }
    else if ( arm32Linux == g_AssemblyTarget )
    {
        fprintf( fp, "@  Build on an arm32 Linux machine using this command (tested on Raspberry PI 3):\n" );
        fprintf( fp, "@     gcc -o sample sample.s -march=armv8-a\n" );
        fprintf( fp, "@\n" );
        fprintf( fp, "@ BA flags: use registers: %s, expression optimization: %s\n", YesNo( useRegistersInASM ), YesNo( g_ExpressionOptimization ) );
        fprintf( fp, ".global main\n" );
        fprintf( fp, ".code 32\n" );
        fprintf( fp, ".macro save_volatile_registers\n" );
        fprintf( fp, "    push     {r3, r9}\n" ); // not using r3 anymore and r9 may not be volatile
        fprintf( fp, ".endm\n" );
        fprintf( fp, ".macro restore_volatile_registers\n" );
        fprintf( fp, "    pop      {r3, r9}\n" );
        fprintf( fp, ".endm\n" );
        fprintf( fp, ".data\n" );
    }
    else if ( arm64Mac == g_AssemblyTarget )
    {
        fprintf( fp, "; Build on an Apple Silicon Mac using a shell script like this:\n" );
        fprintf( fp, ";    as -arch arm64 $1.s -o $1.o\n" );
        fprintf( fp, ";    ld $1.o -o $1 -syslibroot 'xcrun -sdk macos --show-sdk-path' -e _start -L /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib -lSystem\n" );
        fprintf( fp, ";\n" );
        fprintf( fp, "; BA flags: use registers: %s, expression optimization: %s\n", YesNo( useRegistersInASM ), YesNo( g_ExpressionOptimization ) );

        fprintf( fp, ".global _start\n" );
        fprintf( fp, ".macro save_volatile_registers\n" );
        fprintf( fp, "    stp      x10, x11, [sp, #-16]!\n" ); 
        fprintf( fp, "    stp      x12, x13, [sp, #-16]!\n" ); 
        fprintf( fp, "    stp      x14, x15, [sp, #-16]!\n" );
        fprintf( fp, "    sub      sp, sp, #32\n" ); // save room for locals and arguments 
        fprintf( fp, ".endmacro\n" );
        fprintf( fp, ".macro restore_volatile_registers\n" );
        fprintf( fp, "    add      sp, sp, #32\n" );
        fprintf( fp, "    ldp      x14, x15, [sp], #16\n" );
        fprintf( fp, "    ldp      x12, x13, [sp], #16\n" );
        fprintf( fp, "    ldp      x10, x11, [sp], #16\n" );
        fprintf( fp, ".endmacro\n" );
        fprintf( fp, ".data\n" );
    }
    else if ( arm64Win == g_AssemblyTarget )
    {
        fprintf( fp, "; Build on an Arm64 Windows machine like this:\n" );
        fprintf( fp, ";    armasm64 -nologo sample.asm -o sample.obj -g\n" );
        fprintf( fp, ";    link sample.obj /nologo /defaultlib:libucrt.lib /defaultlib:libcmt.lib /defaultlib:kernel32.lib ^\n" );
        fprintf( fp, ";        /defaultlib:legacy_stdio_definitions.lib /entry:mainCRTStartup /subsystem:console\n" );
        fprintf( fp, ";\n" );
        fprintf( fp, "; BA flags: use registers: %s, expression optimization: %s\n", YesNo( useRegistersInASM ), YesNo( g_ExpressionOptimization ) );

        fprintf( fp, "  IMPORT |printf|\n" );
        fprintf( fp, "  IMPORT |exit|\n" );
        fprintf( fp, "  IMPORT |atoi|\n" );
        fprintf( fp, "  IMPORT |GetLocalTime|\n" );
        fprintf( fp, "  EXPORT |main|\n" );
        fprintf( fp, "  MACRO\n" );
        fprintf( fp, "    save_volatile_registers\n" );
        fprintf( fp, "    stp      x10, x11, [sp, #-16]!\n" ); 
        fprintf( fp, "    stp      x12, x13, [sp, #-16]!\n" ); 
        fprintf( fp, "    stp      x14, x15, [sp, #-16]!\n" );
        fprintf( fp, "    sub      sp, sp, #32\n" ); // save room for locals and arguments 
        fprintf( fp, "  MEND\n" );
        fprintf( fp, "  MACRO\n" );
        fprintf( fp, "    restore_volatile_registers\n" );
        fprintf( fp, "    add      sp, sp, #32\n" );
        fprintf( fp, "    ldp      x14, x15, [sp], #16\n" );
        fprintf( fp, "    ldp      x12, x13, [sp], #16\n" );
        fprintf( fp, "    ldp      x10, x11, [sp], #16\n" );
        fprintf( fp, "  MEND\n" );
        fprintf( fp, "  AREA |.data|, DATA, align=6, codealign\n" );
    }
    else if ( i8080CPM == g_AssemblyTarget )
    {
        fprintf( fp, "; assemble, load, and run on 8080/Z80 CP/M 2.2 using the following for test.asm:\n" );
        fprintf( fp, ";   asm test\n" );
        fprintf( fp, ";   load test\n" );
        fprintf( fp, ";   test\n" );
        fprintf( fp, ";\n" );
        fprintf( fp, "; BA flags: use registers: %s, expression optimization: %s\n", YesNo( useRegistersInASM ), YesNo( g_ExpressionOptimization ) );
        fprintf( fp, "BDOS equ 5\n" );
        fprintf( fp, "WCONF equ 2\n" );
        fprintf( fp, "PRSTR equ 9\n" );
        fprintf( fp, "    org      100h\n" );
        fprintf( fp, "    jmp      start\n" ); // jump over data
    }
    else if ( mos6502Apple1 == g_AssemblyTarget )
    {
        // The Apple 1 reserves locations 0x0024 through 0x002b in the 0 page for the monitor
        // Also, 0x0200 through 0x027f are reserved in non-0-page RAM.
        // Generated code starts apps at 0x1000 and uses 0 page starting with 0x0030

        vector<char> justFile( strlen( outputfile ) + 1 );
        strcpy( justFile.data(), outputfile );
        char * pcDot = strrchr( justFile.data(), '.' );
        if ( 0 != pcDot )
            *pcDot = 0;

        fprintf( fp, "; assemble for an Apple 1 using the following for %s:\n", outputfile );
        fprintf( fp, ";   sbasm30306\\sbasm.py %s\n", justFile.data() );
        fprintf( fp, "; sbasm.py can be found here: https://www.sbprojects.net/sbasm/\n" );
        fprintf( fp, "; this creates a %s.hex hex text file with 'address: bytes' lines that can be loaded on an Apple 1\n", justFile.data() );
        fprintf( fp, ";\n" );
        fprintf( fp, "; BA flags: use registers: %s, expression optimization: %s\n", YesNo( useRegistersInASM ), YesNo( g_ExpressionOptimization ) );
        fprintf( fp, "    .cr       6502\n" );
        fprintf( fp, "    .tf       %s.hex, AP1, 8\n", justFile.data() );
        fprintf( fp, "    .or       $1000\n" );
        fprintf( fp, "echo          .eq     $ffef\n" );  // prints character in a
        fprintf( fp, "prbyte        .eq     $ffdc\n" );  // prints hex value of a 
        fprintf( fp, "exitapp       .eq     $ff1f\n" );  // returns to Apple 1 monitor
        fprintf( fp, "printString   .eq     $30\n" ); // temporary string for prstr function, 0x30 and 0x31 are reserved
        fprintf( fp, "curOperand    .eq     $32\n" ); // most recent expression. occupies 32 and 33
        fprintf( fp, "otherOperand  .eq     $34\n" ); // secondary operand. occupies 34 and 35
        fprintf( fp, "arrayOffset   .eq     $36\n" ); // array offset temporary. occupies 36 and 37

        // if global variables above go to this constant, raise this constant

        mos6502FirstZeroPageVariable = 0x40;

        fprintf( fp, "    jmp      start\n" );
    }
    else if ( i8086DOS == g_AssemblyTarget )
    {
        fprintf( fp, "; build ttt.asm using masm v5 like this:\n" );
        fprintf( fp, ";    ntvdm -h masm5\\bin\\masm /Zi /Zd /z /l ttt,,,;\n" );
        fprintf( fp, ";    ntvdm -h masm5\\bin\\link /CP:1 ttt,,ttt,,nul.def\n" );
        fprintf( fp, ";\n" );
        fprintf( fp, "; BA flags: use registers: %s, expression optimization: %s\n", YesNo( useRegistersInASM ), YesNo( g_ExpressionOptimization ) );
        fprintf( fp, "\n" );
        fprintf( fp, ".model small\n" );
        fprintf( fp, ".stack 100h\n" );
        fprintf( fp, "; DOS constants\n" );
        fprintf( fp, "\n" );
        fprintf( fp, "dos_write_char     equ   2h\n" );
        fprintf( fp, "dos_get_systemtime equ   1ah\n" );
        fprintf( fp, "dos_exit           equ   4ch\n" );
        fprintf( fp, "\n" );
        fprintf( fp, "the_dataseg segment para public 'data'\n" );
        fprintf( fp, "    assume ds: the_dataseg\n" );
    }
    else if ( x86Win == g_AssemblyTarget )
    {
        if ( g_i386Target686 )
        {
            fprintf( fp, "; Build on Windows in a Visual Studio vcvars32.bat cmd window using a .bat script like this:\n" );
            fprintf( fp, "; ml /nologo %%1.asm /Fl /Zd /Zf /Zi /link /OPT:REF /nologo ^\n" );
            fprintf( fp, ";        /subsystem:console ^\n" );
            fprintf( fp, ";        /defaultlib:kernel32.lib ^\n" );
            fprintf( fp, ";        /defaultlib:user32.lib ^\n" );
            fprintf( fp, ";        /defaultlib:libucrt.lib ^\n" );
            fprintf( fp, ";        /defaultlib:libcmt.lib ^\n" );
            fprintf( fp, ";        /defaultlib:legacy_stdio_definitions.lib ^\n" );
            fprintf( fp, ";        /entry:mainCRTStartup\n" );
            fprintf( fp, ";\n" );
            fprintf( fp, "; BA flags: use registers: %s, expression optimization: %s\n", YesNo( useRegistersInASM ), YesNo( g_ExpressionOptimization ) );
            fprintf( fp, ".686       ; released by Intel in 1995. First Intel cpu to have cmovX instructions\n" );
        }
        else
        {
            fprintf( fp, "; Build on Windows 98 using ml 7.x like this:\n" );
            fprintf( fp, ";   ml /c sample.asm\n" );
            fprintf( fp, ";   link sample.obj /OPT:REF /defaultlib:msvcrt.lib /subsystem:console,3.10 /entry:mainCRTStartup\n" );
            fprintf( fp, "; If you try to build on modern Windows, _printf will be unresolved.\n" );
            fprintf( fp, ";    workaround: add /defaultlib:legacy_stdio_definitions.lib\n" );
            fprintf( fp, ";\n" );
            fprintf( fp, "; BA flags: use registers: %s, expression optimization: %s\n", YesNo( useRegistersInASM ), YesNo( g_ExpressionOptimization ) );
            fprintf( fp, ".386\n" );
        }

        fprintf( fp, ".model flat, c\n" );
        fprintf( fp, "\n" );
        fprintf( fp, "extern QueryPerformanceCounter@4: PROC\n" );
        fprintf( fp, "extern QueryPerformanceFrequency@4: PROC\n" );
        fprintf( fp, "extern GetLocalTime@4: PROC\n" );
        fprintf( fp, "\n" );

        fprintf( fp, "extern printf: proc\n" );
        fprintf( fp, "extern exit: proc\n" );
        fprintf( fp, "extern atoi: proc\n" );
        fprintf( fp, "data_segment SEGMENT 'DATA'\n" );
    }
    else if ( riscv64 == g_AssemblyTarget )
    {
        fprintf( fp, "# Instructions for a Kendryte K210 RISC-V Sipeed Maixduino Board\n" );
        fprintf( fp, "# Build on Windows using Gnu tools from a Sipeed Maixduino Board configuration of Arduino IDE 2.0.3\n" );
        fprintf( fp, "# (Substitute bamain.* for your app name)\n" );
        fprintf( fp, "#   as -mabi=lp64f -march=rv64imafc -fpic bamain.s -o bamain.o\n" );
        fprintf( fp, "# Edit platform.txt to add bamain.o to the list of linked object files in the '## Link gc-sections, archives, and objects' line\n" );
        fprintf( fp, "#   c:\\users\\david\\appdata\\local\\arduino15\\packages\\maixduino\\hardware\\k210\\0.3.11\\platform.txt\n" );
        fprintf( fp, "# In Arduino, create a simple app that looks like this: \n" );
        fprintf( fp, "#    #include <Sipeed_ST7789.h>\n" );
        fprintf( fp, "#    \n" );
        fprintf( fp, "#    SPIClass spi_(SPI0); // MUST be SPI0 for Maix series on board LCD\n" );
        fprintf( fp, "#    Sipeed_ST7789 lcd(320, 240, spi_);\n" );
        fprintf( fp, "#    \n" );
        fprintf( fp, "#    extern \"C\" void bamain( void );\n" );
        fprintf( fp, "#    \n" );
        fprintf( fp, "#    extern \"C\" void rvos_print_text( const char * pc )\n" );
        fprintf( fp, "#    {\n" );
        fprintf( fp, "#        lcd.printf( \"%%s\", pc );\n" );
        fprintf( fp, "#    }\n" );
        fprintf( fp, "#    \n" );
        fprintf( fp, "#    void setup()\n" );
        fprintf( fp, "#    {\n" );
        fprintf( fp, "#        lcd.begin( 15000000, COLOR_BLACK ); // frequency and fill with red\n" );
        fprintf( fp, "#        bamain();\n" );
        fprintf( fp, "#    }\n" );
        fprintf( fp, "#    \n" );
        fprintf( fp, "#    void loop()\n" );
        fprintf( fp, "#    {\n" );
        fprintf( fp, "#        while ( true );\n" );
        fprintf( fp, "#    }\n" );
        fprintf( fp, "#\n" );
        fprintf( fp, "# BA flags: use registers: %s, expression optimization: %s\n", YesNo( useRegistersInASM ), YesNo( g_ExpressionOptimization ) );

        fprintf( fp, ".section        .sbss,\"aw\",@nobits\n" );
        fprintf( fp, "  .align 3\n" );
        fprintf( fp, "  print_buffer:\n    .zero 256\n" );
    }
    else if ( m68kCPM == g_AssemblyTarget )
    {
        fprintf( fp, "* build for CP/M 68K using the assembler and linker from that OS:\n" );
        fprintf( fp, "*   as68 -l -u <filename>.S\n" );
        fprintf( fp, "*   lo68 -l -r -u_nofloat -o <filename>.68K S.O, <filename>.O CLIB\n" );
        fprintf( fp, "* \n" );
        fprintf( fp, "* BA flags: use registers: %s, expression optimization: %s\n", YesNo( useRegistersInASM ), YesNo( g_ExpressionOptimization ) );
        fprintf( fp, ".data\n" );
    }
    else if ( oiOS == g_AssemblyTarget )
    {
        fprintf( fp, "; Build using oia like this: oia <filename.s>\n" );
        fprintf( fp, "; Then run the binary like this: oios <filename.oi>\n" );
        fprintf( fp, ".data\n" );
    }

    bool elapReferenced = false;
    bool timeReferenced = false;

    for ( size_t l = 0; l < g_linesOfCode.size(); l++ )
    {
        LineOfCode & loc = g_linesOfCode[ l ];
        vector<TokenValue> & vals = loc.tokenValues;

        if ( Token_DIM == vals[ 0 ].token )
        {
            int cdwords = vals[ 0 ].dims[ 0 ];
            if ( 2 == vals[ 0 ].dimensions )
                cdwords *= vals[ 0 ].dims[ 1 ];

            Variable * pvar = FindVariable( varmap, vals[ 0 ].strValue );

            // If an array is declared but never referenced later (and so not in varmap), ignore it

            if ( 0 != pvar )
            {
                pvar->dimensions = vals[ 0 ].dimensions;
                pvar->dims[ 0 ] = vals[ 0 ].dims[ 0 ];
                pvar->dims[ 1 ] = vals[ 0 ].dims[ 1 ];

                if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                {
                    fprintf( fp, "  align 16\n" );
                    fprintf( fp, "    %8s DD %d DUP (0)\n", GenVariableName( vals[ 0 ].strValue ), cdwords );
                }
                else if ( arm64Mac == g_AssemblyTarget || arm32Linux == g_AssemblyTarget )
                {
                    fprintf( fp, "  .p2align 4\n" );
                    fprintf( fp, "    %8s: .space %d\n", GenVariableName( vals[ 0 ].strValue ), cdwords * 4 );
                }
                else if ( arm64Win == g_AssemblyTarget )
                {
                    fprintf( fp, "  align 16\n" );
                    fprintf( fp, "%s space %d\n", GenVariableName( vals[ 0 ].strValue ), cdwords * 4 );
                }
                else if ( i8080CPM == g_AssemblyTarget )
                    fprintf( fp, "    %8s: DS %d\n", GenVariableName( vals[ 0 ].strValue ), cdwords * 2 ); 
                else if ( mos6502Apple1 == g_AssemblyTarget )
                {
                    // n.b. don't generate 6502 arrays here; put them at the end so they can be deleted
                    // from transfers via a serial port to actualy Apple 1 machines.

                    //fprintf( fp, "%s:\n", GenVariableName( vals[ 0 ].strValue ) ); 
                    //fprintf( fp, "    .rf %d\n", cdwords * 2 ); 
                }
                else if ( i8086DOS == g_AssemblyTarget )
                    fprintf( fp, "    %8s dw %d DUP (0)\n", GenVariableName( vals[ 0 ].strValue ), cdwords );
                else if ( riscv64 == g_AssemblyTarget )
                {
                    fprintf( fp, "  .align 3\n" );
                    fprintf( fp, "  %8s:\n    .zero %d\n", GenVariableName( vals[ 0 ].strValue ), cdwords * 4 );
                }
                else if ( m68kCPM == g_AssemblyTarget )
                {
                    fprintf( fp, "%s:\n", GenVariableName( vals[ 0 ].strValue ) );
                    fprintf( fp, ".ds %d\n", cdwords * 4 );
                }
                else if ( oiOS == g_AssemblyTarget )
                {
                    // don't generate iois arrays here -- do it after scalar variables to improve relative references
                    //fprintf( fp, "    align\n" );
                    //fprintf( fp, "    image_t %8s[%d]\n", GenVariableName( vals[ 0 ].strValue ), cdwords );
                }
            }
        }
        else if ( Token_PRINT == vals[ 0 ].token || Token_IF == vals[ 0 ].token )
        {
            for ( int t = 0; t < vals.size(); t++ )
            {
                if ( Token_STRING == vals[ t ].token )
                {
                    string strEscaped = singleQuoteEscape( vals[ t ].strValue );
                    if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                        fprintf( fp, "    str_%zd_%d   db  '%s', 0\n", l, t , strEscaped.c_str() );
                    else if ( arm64Mac == g_AssemblyTarget || arm32Linux == g_AssemblyTarget )
                    {
                        string e = arm64MacEscape( vals[ t ].strValue );
                        fprintf( fp, "    str_%zd_%d: .asciz \"%s\"\n", l, t, e.c_str() );
                    }
                    else if ( arm64Win == g_AssemblyTarget )
                    {
                        string e = arm64WinEscape( vals[ t ].strValue );
                        fprintf( fp, "str_%zd_%d dcb \"%s\", 0\n", l, t, e.c_str() );
                    }
                    else if ( i8080CPM == g_AssemblyTarget )
                        fprintf( fp, "      s$%zd$%d: db '%s', 0\n", l, t, strEscaped.c_str() );
                    else if ( mos6502Apple1 == g_AssemblyTarget )
                    {
                        string str6502Escaped = mos6502Escape( vals[ t ].strValue );
                        fprintf( fp, "str_%zd_%d .az '%s'\n", l, t, str6502Escaped.c_str() );
                    }
                    else if ( i8086DOS == g_AssemblyTarget )
                        fprintf( fp, "    str_%zd_%d   db  '%s', 0\n", l, t, strEscaped.c_str() );
                    else if ( riscv64 == g_AssemblyTarget )
                    {
                        // risc-v strings get declared below in the non-writeable section
                    }
                    else if ( m68kCPM == g_AssemblyTarget )
                    {
                        string str68k = cpm68kEscape( vals[ t ].strValue );
                        fprintf( fp, "s$%zd$%d: .dc.b %s,0\n", l, t, str68k.c_str() );
                    }
                    else if ( oiOS == g_AssemblyTarget )
                        fprintf( fp, "    string str_%zd_%d \"%s\"\n", l, t, vals[ t ].strValue.c_str() );
                }
                else if ( Token_ELAP == vals[ t ].token )
                    elapReferenced = true;
                else if ( Token_TIME == vals[ t ].token )
                    timeReferenced = true;
            }
        }
    }

    vector<VarCount> varscount;

    for ( std::map<string, Variable>::iterator  it = varmap.begin(); it != varmap.end(); it++ )
    {
        // enable registers for arm64 arrays since loading addresses via code is slow.
        // it's 5% overall faster this way

        if ( 0 == it->second.dimensions || arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
        {
            VarCount vc;
            vc.name = it->first.c_str();
            vc.refcount = it->second.references;
            varscount.push_back( vc );
        }
    }

    qsort( varscount.data(), varscount.size(), sizeof( VarCount ), CompareVarCount );

    int availableRegisters = 0;
    if ( useRegistersInASM )
        availableRegisters = ( x64Win == g_AssemblyTarget) ? _countof( MappedRegistersX64 ) : 
                             ( arm64Mac == g_AssemblyTarget ) ? _countof( MappedRegistersArm64 ) :
                             ( arm64Win == g_AssemblyTarget ) ? _countof( MappedRegistersArm64 ) :
                             ( arm32Linux == g_AssemblyTarget ) ? _countof( MappedRegistersArm32 ) :
                             ( x86Win == g_AssemblyTarget ) ? _countof( MappedRegistersX86 ) :
                             ( riscv64 == g_AssemblyTarget ) ? _countof( MappedRegistersRiscV64 ) :
                             ( m68kCPM == g_AssemblyTarget ) ? _countof( MappedRegistersm68kCPM ) :
                             ( oiOS == g_AssemblyTarget ) ? _countof( MappedRegistersOIOS ) :
                             0;

    for ( size_t i = 0; i < varscount.size() && 0 != availableRegisters; i++ )
    {
        Variable * pvar = FindVariable( varmap, varscount[ i ].name );
        assert( pvar );

        if ( !strcmp( pvar->name, "av%" ) ) // never assign av% to a register
            continue;

        availableRegisters--;
        if ( x64Win == g_AssemblyTarget )
            pvar->reg = MappedRegistersX64[ availableRegisters ];
        else if ( arm32Linux == g_AssemblyTarget )
            pvar->reg = MappedRegistersArm32[ availableRegisters ];
        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
            pvar->reg = MappedRegistersArm64[ availableRegisters ];
        else if ( x86Win == g_AssemblyTarget )
            pvar->reg = MappedRegistersX86[ availableRegisters ];
        else if ( riscv64 == g_AssemblyTarget )
            pvar->reg = MappedRegistersRiscV64[ availableRegisters ];
        else if ( m68kCPM == g_AssemblyTarget )
            pvar->reg = MappedRegistersm68kCPM[ availableRegisters ];
        else if ( oiOS == g_AssemblyTarget )
            pvar->reg = MappedRegistersOIOS[ availableRegisters ];

        if ( EnableTracing && g_Tracing )
            printf( "variable %s has %d references and is mapped to register %s\n",
                    varscount[ i ].name, varscount[ i ].refcount, pvar->reg.c_str() );

        if ( arm32Linux == g_AssemblyTarget )
            fprintf( fp, "   @ " );
        else if ( riscv64 == g_AssemblyTarget )
            fprintf( fp, "   # " );
        else if ( m68kCPM == g_AssemblyTarget )
            fprintf( fp, "* " );
        else
            fprintf( fp, "   ; " );

        fprintf( fp, "variable %s (referenced %d times) will use register %s\n", pvar->name,
                 varscount[ i ].refcount, pvar->reg.c_str() );
    }

    if ( x64Win == g_AssemblyTarget )
        fprintf( fp, "  align 16\n" );
    else if ( arm32Linux == g_AssemblyTarget )
        fprintf( fp, "  .p2align 2\n" );
    else if ( arm64Mac == g_AssemblyTarget )
        fprintf( fp, "  .p2align 4\n" );
    else if ( riscv64 == g_AssemblyTarget )
        fprintf( fp, "  .align 3\n" );
    else if ( oiOS == g_AssemblyTarget )
        fprintf( fp, "    align\n" );

    int num6502ZeroPageVariables = 0;

    if ( mos6502Apple1 == g_AssemblyTarget )
    {
        for ( size_t i = 0; i < varscount.size() && num6502ZeroPageVariables != Max6502ZeroPageVariables; i++ )
        {
            Variable * pvar = FindVariable( varmap, varscount[ i ].name );
            assert( pvar );
            pvar->mos6502ZeroPage = true;
            fprintf( fp, "%s  .eq $%x\n", GenVariableName( varscount[ i ].name ),
                     ( 2 * num6502ZeroPageVariables ) + mos6502FirstZeroPageVariable );
            num6502ZeroPageVariables++;
        }
    }

    for ( std::map<string, Variable>::iterator it = varmap.begin(); it != varmap.end(); it++ )
    {
        if ( ( 0 == it->second.dimensions ) && ( 0 == it->second.reg.length() )  )
        {
            if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                fprintf( fp, "    %8s DD   0\n", GenVariableName( it->first ) );
            else if ( arm32Linux == g_AssemblyTarget )
                fprintf( fp, "    %8s: .int 0\n", GenVariableName( it->first ) );
            else if ( arm64Mac == g_AssemblyTarget )
                fprintf( fp, "    %8s: .quad 0\n", GenVariableName( it->first ) );
            else if ( arm64Win == g_AssemblyTarget )
                fprintf( fp, "%s dcq 0\n", GenVariableName( it->first ) );
            else if ( i8080CPM == g_AssemblyTarget )
                fprintf( fp, "    %8s: dw  0\n", GenVariableName( it->first ) );
            else if ( mos6502Apple1 == g_AssemblyTarget && ! it->second.mos6502ZeroPage )
                fprintf( fp, "%s  .dw  0\n", GenVariableName( it->first ) );
            else if ( i8086DOS == g_AssemblyTarget )
                fprintf( fp, "    %8s dw   0\n", GenVariableName( it->first ) );
            else if ( riscv64 == g_AssemblyTarget )
                fprintf( fp, "    %8s:\n    .zero   8\n", GenVariableName( it->first ) );
            else if ( m68kCPM == g_AssemblyTarget )
                fprintf( fp, "    %8s:\n    .dc.l 0\n", GenVariableName( it->first ) );
            else if ( oiOS == g_AssemblyTarget )
                fprintf( fp, "    image_t  %s\n", GenVariableName( it->first ) );
        }
    }

    varscount.clear();

    if ( x64Win == g_AssemblyTarget )
    {
        fprintf( fp, "  align 16\n" );
        fprintf( fp, "    gosubCount     dq    0\n" ); // count of active gosub calls
        fprintf( fp, "    startTicks     dq    0\n" );
        fprintf( fp, "    perfFrequency  dq    0\n" );
        fprintf( fp, "    currentTicks   dq    0\n" );
        fprintf( fp, "    currentTime    dq 2  DUP(0)\n" ); // SYSTEMTIME is 8 WORDS

        fprintf( fp, "    errorString    db    'internal error', 10, 0\n" );
        if ( !g_Quiet )
        {
            fprintf( fp, "    startString    db    'running basic', 10, 0\n" );
            fprintf( fp, "    stopString     db    'done running basic', 10, 0\n" );
        }
        fprintf( fp, "    newlineString  db    10, 0\n" );

        if ( elapReferenced )
            fprintf( fp, "    elapString     db    '%%lld microseconds (-6)', 0\n" );
        if ( timeReferenced )
            fprintf( fp, "    timeString     db    '%%02d:%%02d:%%02d:%%03d', 0\n" );

        fprintf( fp, "    intString      db    '%%d', 0\n" );
        fprintf( fp, "    strString      db    '%%s', 0\n" );

        fprintf( fp, "data_segment ENDS\n" );

        fprintf( fp, "code_segment SEGMENT ALIGN( 4096 ) 'CODE'\n" );
        fprintf( fp, "main PROC\n" );
        fprintf( fp, "    push     rbp\n" );
        fprintf( fp, "    mov      rbp, rsp\n" );
        fprintf( fp, "    sub      rsp, 32 + 8 * 4\n" );

        // set BASIC variable av% to the integer value of the first app argument (if any)

        if ( FindVariable( varmap, "av%" ) )
        {
            fprintf( fp, "    cmp      rcx, 2\n" );
            fprintf( fp, "    jl       no_arguments\n" );
            fprintf( fp, "    mov      rcx, [ rdx + 8 ]\n" );
            fprintf( fp, "    call     atoi\n" );
            fprintf( fp, "    mov      DWORD PTR [%s], eax\n", GenVariableName( "av%" ) );
            fprintf( fp, "  no_arguments:\n" );
        }

        if ( !g_Quiet )
        {
            fprintf( fp, "    lea      rcx, [startString]\n" );
            fprintf( fp, "    call     printf\n" );
        }

        if ( elapReferenced )
        {
            fprintf( fp, "    lea      rcx, [startTicks]\n" );
            fprintf( fp, "    call     QueryPerformanceCounter\n" );
            fprintf( fp, "    lea      rcx, [perfFrequency]\n" );
            fprintf( fp, "    call     QueryPerformanceFrequency\n" );
        }
    }
    else if ( arm32Linux == g_AssemblyTarget )
    {
        fprintf( fp, "  .p2align 4\n" );
        fprintf( fp, "    gosubCount:    .int 0\n" );
        fprintf( fp, "    startTicks:    .quad 0\n" );
        fprintf( fp, "    rawTime:       .quad 0\n" ); // time_t
        fprintf( fp, "    errorString:   .asciz \"internal error\\n\"\n" );

        if ( !g_Quiet )
        {
            fprintf( fp, "    startString:   .asciz \"running basic\\n\"\n" );
            fprintf( fp, "    stopString:    .asciz \"done running basic\\n\"\n" );
        }
        fprintf( fp, "    newlineString: .asciz \"\\n\"\n" );

        if ( timeReferenced )
            fprintf( fp, "    timeString:    .asciz \"%%02d:%%02d:%%02d\"\n" );

        if ( elapReferenced )
            fprintf( fp, "    elapString:    .asciz \"%%d microseconds (-6)\"\n" );

        fprintf( fp, "    intString:     .asciz \"%%d\"\n" );
        fprintf( fp, "    strString:     .asciz \"%%s\"\n" );

        fprintf( fp, ".p2align 4\n" );
        fprintf( fp, ".text\n" );
        fprintf( fp, "main:\n" );

        fprintf( fp, "    push     {ip, lr}\n" );

        // set BASIC variable av% to the integer value of the first app argument (if any)

        if ( FindVariable( varmap, "av%" ) )
        {
            LoadArm32Constant( fp, "r2", 2 );
            fprintf( fp, "    cmp      r0, r2\n" );
            fprintf( fp, "    blt      no_arguments\n" );
            fprintf( fp, "    add      r1, r1, #4\n" );
            fprintf( fp, "    ldr      r0, [r1]\n" );
            fprintf( fp, "    bl       atoi\n" );
            LoadArm32Address( fp, "r1", "av%" );
            fprintf( fp, "    str      r0, [r1]\n" );
            fprintf( fp, "no_arguments:\n" );
        }

        if ( !g_Quiet )
        {
            LoadArm32Label( fp, "r0", "startString" );
            fprintf( fp, "    bl       call_printf\n" );
        }

        if ( elapReferenced )
        {
            fprintf( fp, "    bl       clock\n" );
            LoadArm32Label( fp, "r1", "startTicks" );
            fprintf( fp, "    str      r0, [r1]\n" );
        }
    }
    else if ( arm64Mac == g_AssemblyTarget )
    {
        fprintf( fp, "  .p2align 4\n" );
        fprintf( fp, "    gosubCount:    .quad 0\n" );
        fprintf( fp, "    startTicks:    .quad 0\n" );
        fprintf( fp, "    rawTime:       .quad 0\n" ); // time_t
        fprintf( fp, "    errorString:   .asciz \"internal error\\n\"\n" );
        if ( !g_Quiet )
        {
            fprintf( fp, "    startString:   .asciz \"running basic\\n\"\n" );
            fprintf( fp, "    stopString:    .asciz \"done running basic\\n\"\n" );
        }
        fprintf( fp, "    newlineString: .asciz \"\\n\"\n" );

        if ( timeReferenced )
            fprintf( fp, "    timeString:    .asciz \"%%02d:%%02d:%%02d\"\n" );
        if ( elapReferenced )
            fprintf( fp, "    elapString:    .asciz \"%%lld microseconds (-6)\"\n" );

        fprintf( fp, "    intString:     .asciz \"%%d\"\n" );
        fprintf( fp, "    strString:     .asciz \"%%s\"\n" );

        fprintf( fp, ".p2align 4\n" );
        fprintf( fp, ".text\n" );
        fprintf( fp, "_start:\n" );

        fprintf( fp, "    sub      sp, sp, #32\n" );
        fprintf( fp, "    stp      x29, x30, [sp, #16]\n" );
        fprintf( fp, "    add      x29, sp, #16\n" );

        // set BASIC variable av% to the integer value of the first app argument (if any)

        if ( FindVariable( varmap, "av%" ) )
        {
            LoadArm64Constant( fp, "x2", 2 );
            fprintf( fp, "    cmp      x0, x2\n" );
            fprintf( fp, "    b.lt     no_arguments\n" );
            fprintf( fp, "    add      x1, x1, 8\n" );
            fprintf( fp, "    ldr      x0, [x1]\n" );
            fprintf( fp, "    bl       _atoi\n" );
            LoadArm64Address( fp, "x1", "av%" );
            fprintf( fp, "    str      w0, [x1]\n" );
            fprintf( fp, "no_arguments:\n" );
        }

        if ( !g_Quiet )
        {
            LoadArm64Label( fp, "x0", "startString" );
            fprintf( fp, "    bl       call_printf\n" );
        }

        if ( elapReferenced )
        {
            LoadArm64Label( fp, "x1", "startTicks" );
            fprintf( fp, "    mrs      x0, cntvct_el0\n" );
            fprintf( fp, "    str      x0, [x1]\n" );
        }
    }
    else if ( arm64Win == g_AssemblyTarget )
    {
        fprintf( fp, "  align 16\n" );
        fprintf( fp, "currentTime   space 16\n" ); // SYSTEMTIME is 8 WORDS
        fprintf( fp, "gosubCount    dcq 0\n" );
        fprintf( fp, "startTicks    dcq 0\n" );
        fprintf( fp, "errorString   dcb \"internal error\\n\", 0\n" );
        if ( !g_Quiet )
        {
            fprintf( fp, "startString   dcb \"running basic\\n\", 0\n" );
            fprintf( fp, "stopString    dcb \"done running basic\\n\", 0\n" );
        }
        fprintf( fp, "newlineString dcb \"\\n\", 0\n" );

        if ( timeReferenced )
            fprintf( fp, "timeString    dcb \"%%02d:%%02d:%%02d:%%03d\", 0\n" );
        if ( elapReferenced )
            fprintf( fp, "elapString    dcb \"%%lld microseconds (-6)\", 0\n" );

        fprintf( fp, "intString     dcb \"%%d\", 0\n" );
        fprintf( fp, "strString     dcb \"%%s\", 0\n" );

        fprintf( fp, "  area .code, code, align=4, codealign\n" );
        fprintf( fp, "  align 16\n" );
        fprintf( fp, "main PROC\n" );

        fprintf( fp, "    sub      sp, sp, #32\n" );
        fprintf( fp, "    stp      x29, x30, [sp, #16]\n" );
        fprintf( fp, "    add      x29, sp, #16\n" );

        // set BASIC variable av% to the integer value of the first app argument (if any)

        if ( FindVariable( varmap, "av%" ) )
        {
            LoadArm64Constant( fp, "x2", 2 );
            fprintf( fp, "    cmp      x0, x2\n" );
            fprintf( fp, "    b.lt     no_arguments\n" );
            fprintf( fp, "    add      x1, x1, 8\n" );
            fprintf( fp, "    ldr      x0, [x1]\n" );
            fprintf( fp, "    bl       atoi\n" );
            LoadArm64Address( fp, "x1", "av%" );
            fprintf( fp, "    str      w0, [x1]\n" );
            fprintf( fp, "no_arguments\n" );
        }

        if ( !g_Quiet )
        {
            LoadArm64Label( fp, "x0", "startString" );
            fprintf( fp, "    bl       call_printf\n" );
        }

        if ( elapReferenced )
        {
            LoadArm64Label( fp, "x1", "startTicks" );
            fprintf( fp, "    mrs      x0, cntvct_el0\n" );
            fprintf( fp, "    str      x0, [x1]\n" );
        }
    }
    else if ( i8080CPM == g_AssemblyTarget )
    {
        fprintf( fp, "    errorString:    db    'internal error', 13, 10, 0\n" );
        if ( !g_Quiet )
        {
            fprintf( fp, "    startString:    db    'running basic', 13, 10, 0\n" );
            fprintf( fp, "    stopString:     db    'done running basic', 13, 10, 0\n" );
        }
        fprintf( fp, "    newlineString:  db    13, 10, 0\n" );
        fprintf( fp, "    mulTmp:         dw    0\n" ); // temporary for imul and idiv functions
        fprintf( fp, "    divRem:         dw    0\n" ); // idiv remainder

        fprintf( fp, "start:\n" );
        fprintf( fp, "    push     b\n" );
        fprintf( fp, "    push     d\n" );
        fprintf( fp, "    push     h\n" );

        for ( size_t l = 0; l < g_linesOfCode.size(); l++ )
        {
            LineOfCode & loc = g_linesOfCode[ l ];
            vector<TokenValue> & vals = loc.tokenValues;
    
            if ( Token_DIM == vals[ 0 ].token )
            {
                int cdwords = vals[ 0 ].dims[ 0 ];
                if ( 2 == vals[ 0 ].dimensions )
                    cdwords *= vals[ 0 ].dims[ 1 ];
    
                Variable * pvar = FindVariable( varmap, vals[ 0 ].strValue );
    
                if ( 0 != pvar )
                {
                    fprintf( fp, "    lxi      d, %d\n", cdwords * 2 );
                    fprintf( fp, "    lxi      b, %s\n", GenVariableName( vals[ 0 ].strValue ) );
                    fprintf( fp, "    call     zeromem\n" );
                }
            }
        }

        // set BASIC variable av% to the integer value of the first app argument (if any)

        if ( FindVariable( varmap, "av%" ) )
        {
            fprintf( fp, "    lda     128\n" );
            fprintf( fp, "    cpi     0\n" );
            fprintf( fp, "    jz      noargument\n" );
            fprintf( fp, "    mvi     d, 0\n" );
            fprintf( fp, "    mov     e, a\n" );
            fprintf( fp, "    lxi     h, 129\n" );
            fprintf( fp, "    dad     d\n" );
            fprintf( fp, "    mvi     m, 0\n" );
            fprintf( fp, "    lxi     h, 129\n" );
            fprintf( fp, "    call    atou\n" );
            fprintf( fp, "    shld     %s\n", GenVariableName( "av%" ) );
            fprintf( fp, "  noargument:\n" );
        }

        if ( !g_Quiet )
        {
            fprintf( fp, "    lxi      h, startString\n" );
            fprintf( fp, "    call     DISPLAY\n" );
        }
    }
    else if ( mos6502Apple1 == g_AssemblyTarget )
    {
        fprintf( fp, "intString      .az    '32768'\n" ); // big enough for 5 digits and no minus sign, plus 0.
        fprintf( fp, "errorString    .az    'internal error', #13, #10\n" );
        if ( !g_Quiet )
        {
            fprintf( fp, "startString    .az    #13, #10, 'running basic', #13, #10\n" );
            if ( g_GenerateAppleDollar )
                fprintf( fp, "stopString     .az    'done running basic$', #13, #10\n" ); // end with $ for timing app hack
            else
                fprintf( fp, "stopString     .az    'done running basic', #13, #10\n" );
        }
        fprintf( fp, "newlineString  .az    #13, #10\n" );
        fprintf( fp, "divRem         .dw    0\n" ); // idiv remainder
        fprintf( fp, "mulResult      .dl    0\n" ); // multiplication result
        fprintf( fp, "tempWord       .dw    0\n" ); // temporary storage

        fprintf( fp, "start\n" );

        for ( size_t l = 0; l < g_linesOfCode.size(); l++ )
        {
            LineOfCode & loc = g_linesOfCode[ l ];
            vector<TokenValue> & vals = loc.tokenValues;
    
            if ( Token_DIM == vals[ 0 ].token )
            {
                int cdwords = vals[ 0 ].dims[ 0 ];
                if ( 2 == vals[ 0 ].dimensions )
                    cdwords *= vals[ 0 ].dims[ 1 ];
    
                Variable * pvar = FindVariable( varmap, vals[ 0 ].strValue );
    
                if ( 0 != pvar )
                {
                    fprintf( fp, "    lda      #%d\n", cdwords * 2 );
                    fprintf( fp, "    sta      curOperand\n" );
                    fprintf( fp, "    lda      /%d\n", cdwords * 2 );
                    fprintf( fp, "    sta      curOperand+1\n" );
                    fprintf( fp, "    lda      #%s\n", GenVariableName( vals[ 0 ].strValue ) );
                    fprintf( fp, "    sta      otherOperand\n" );
                    fprintf( fp, "    lda      /%s\n", GenVariableName( vals[ 0 ].strValue ) );
                    fprintf( fp, "    sta      otherOperand+1\n" );
                    fprintf( fp, "    jsr      zeromem\n" );
                }
            }
        }

        // zero the values of the Zero Page variables

        if ( 0 != num6502ZeroPageVariables )
        {
            int zeroPageBytes = 2 * num6502ZeroPageVariables;

            fprintf( fp, "    lda      #%d\n", zeroPageBytes );
            fprintf( fp, "    sta      curOperand\n" );
            fprintf( fp, "    lda      /%d\n", zeroPageBytes );
            fprintf( fp, "    sta      curOperand+1\n" );
            fprintf( fp, "    lda      #%d\n", mos6502FirstZeroPageVariable );
            fprintf( fp, "    sta      otherOperand\n" );
            fprintf( fp, "    lda      /%d\n", mos6502FirstZeroPageVariable );
            fprintf( fp, "    sta      otherOperand+1\n" );
            fprintf( fp, "    jsr      zeromem\n" );
        }

        if ( !g_Quiet )
        {
            fprintf( fp, "    lda      #startString\n" );
            fprintf( fp, "    sta      printString\n" );
            fprintf( fp, "    lda      /startString\n" );
            fprintf( fp, "    sta      printString+1\n" );
            fprintf( fp, "    jsr      prstr\n" );
        }
    }
    else if ( i8086DOS == g_AssemblyTarget )
    {
        fprintf( fp, "crlfmsg        db      13,10,0\n" );
        if ( elapReferenced )
            fprintf( fp, "elapString     db      ' seconds',0\n" );
        if ( !g_Quiet )
        {
            fprintf( fp, "startString    db      'running basic',13,10,0\n" );
            fprintf( fp, "stopString     db      'done running basic',13,10,0\n" );
        }
        fprintf( fp, "errorString    db      'internal error',13,10,0\n" );
        fprintf( fp, "starttime      dd      0\n" );
        fprintf( fp, "scratchpad     dd      0\n" );
        fprintf( fp, "result         dd      0\n" );
        fprintf( fp, "the_dataseg ends\n" );
        fprintf( fp, "\n" );

        fprintf( fp, ".code\n" );
        fprintf( fp, "start:\n" );
        fprintf( fp, "    mov      ax, the_dataseg\n" );
        fprintf( fp, "    mov      ds, ax\n" );

        // set BASIC variable av% to the integer value of the first app argument (if any)

        if ( FindVariable( varmap, "av%" ) )
        {
            fprintf( fp, "    push     ds\n" );
            fprintf( fp, "    mov      ax, es\n" );
            fprintf( fp, "    mov      ds, ax\n" );
            fprintf( fp, "    mov      di, 0\n" );
            fprintf( fp, "    xor      ax, ax\n" );
            fprintf( fp, "    cmp      al, byte ptr [ di + 128 ]\n" );
            fprintf( fp, "    jz       no_arguments\n" );
            fprintf( fp, "    mov      cx, 129\n" );
            fprintf( fp, "    call     atou\n" );
            fprintf( fp, "    pop      ds\n" );
            fprintf( fp, "    push     ds\n" );
            fprintf( fp, "    mov      WORD PTR ds: [%s], ax\n", GenVariableName( "av%" ) );
            fprintf( fp, "no_arguments:\n" );
            fprintf( fp, "    pop      ds\n" );
        }

        if ( elapReferenced )
        {
            fprintf( fp, "    xor      ax, ax\n" );
            fprintf( fp, "    int      1ah\n" );
            fprintf( fp, "    mov      WORD PTR ds: [ starttime ], dx\n" ); // low word
            fprintf( fp, "    mov      WORD PTR ds: [ starttime + 2 ], cx\n" ); // high word
        }

        if ( !g_Quiet )
        {
            fprintf( fp, "    mov      dx, offset startString\n" );
            fprintf( fp, "    call     printstring\n" );
        }
    }
    else if ( x86Win == g_AssemblyTarget )
    {
        fprintf( fp, "  align 16\n" );
        fprintf( fp, "    gosubCount     dq    0\n" ); // count of active gosub calls
        fprintf( fp, "    startTicks     dq    0\n" );
        fprintf( fp, "    perfFrequency  dq    0\n" );
        fprintf( fp, "    currentTicks   dq    0\n" );
        fprintf( fp, "    currentTime    dq 2  DUP(0)\n" ); // SYSTEMTIME is 8 WORDS

        fprintf( fp, "    errorString    db    'internal error', 10, 0\n" );
        if ( !g_Quiet )
        {
            fprintf( fp, "    startString    db    'running basic', 10, 0\n" );
            fprintf( fp, "    stopString     db    'done running basic', 10, 0\n" );
        }
        fprintf( fp, "    newlineString  db    10, 0\n" );

        if ( elapReferenced )
            fprintf( fp, "    elapString     db    '%%d milliseconds', 0\n" );
        if ( timeReferenced )
            fprintf( fp, "    timeString     db    '%%02d:%%02d:%%02d:%%03d', 0\n" );

        fprintf( fp, "    intString      db    '%%d', 0\n" );
        fprintf( fp, "    strString      db    '%%s', 0\n" );

        fprintf( fp, "data_segment ENDS\n" );

        fprintf( fp, "code_segment SEGMENT 'CODE'\n" );
        fprintf( fp, "main PROC\n" );
        fprintf( fp, "    push     ebp\n" );
        fprintf( fp, "    mov      ebp, esp\n" );
        fprintf( fp, "    push     edi\n" );
        fprintf( fp, "    push     esi\n" );

        // set BASIC variable av% to the integer value of the first app argument (if any)

        if ( FindVariable( varmap, "av%" ) )
        {
            fprintf( fp, "    cmp      DWORD PTR [ ebp + 8 ], 2\n" );
            fprintf( fp, "    jl       no_arguments\n" );
            fprintf( fp, "    mov      ecx, [ ebp + 12 ]\n" );
            fprintf( fp, "    mov      ecx, [ ecx + 4 ]\n" );
            fprintf( fp, "    push     ecx\n" );
            fprintf( fp, "    call     atoi\n" );
            fprintf( fp, "    mov      DWORD PTR [%s], eax\n", GenVariableName( "av%" ) );
            fprintf( fp, "  no_arguments:\n" );
        }

        if ( !g_Quiet )
        {
            fprintf( fp, "    push     offset startString\n" );
            fprintf( fp, "    call     printf\n" );
        }

        if ( elapReferenced )
        {
            fprintf( fp, "    add      esp, 4\n" );
            fprintf( fp, "    push     offset startTicks\n" );
            fprintf( fp, "    call     QueryPerformanceCounter@4\n" );
            fprintf( fp, "    push     offset perfFrequency\n" );
            fprintf( fp, "    call     QueryPerformanceFrequency@4\n" );
            fprintf( fp, "    mov      eax, DWORD PTR [perfFrequency]\n" );
            fprintf( fp, "    mov      ebx, 1000000\n" ); // get it down to milliseconds
            fprintf( fp, "    xor      edx, edx\n" );
            fprintf( fp, "    div      ebx\n" );
            fprintf( fp, "    mov      DWORD PTR [perfFrequency], eax\n" );
        }
    }
    else if ( riscv64 == g_AssemblyTarget )
    {
        fprintf( fp, "  startTicks:\n   .zero 8\n" );
        fprintf( fp, "  currentTime:\n    .zero 16\n" );

        fprintf( fp, ".section .rodata\n" );
        if ( !g_Quiet )
        {
            fprintf( fp, "  startString:\n    .string \"running basic\\n\"\n" );
            fprintf( fp, "  stopString:\n    .string \"done running basic\\n\"\n" );
        }
        fprintf( fp, "  newlineString:\n   .string \"\\n\"\n" );
        fprintf( fp, "  errorString:\n   .string \"internal error\\n\"\n" );

        if ( elapReferenced )
            fprintf( fp, "  elapString:\n   .string \" microseconds\"\n" );
        if ( timeReferenced )
            fprintf( fp, "  timeString:\n   .string \"%%02d:%%02d:%%02d:%%03d\"\n" );

        // risc-v strings have to go here

        for ( size_t l = 0; l < g_linesOfCode.size(); l++ )
        {
            LineOfCode & loc = g_linesOfCode[ l ];
            vector<TokenValue> & vals = loc.tokenValues;
    
            if ( Token_PRINT == vals[ 0 ].token || Token_IF == vals[ 0 ].token )
            {
                for ( int t = 0; t < vals.size(); t++ )
                {
                    if ( Token_STRING == vals[ t ].token )
                    {
                        string e = arm64MacEscape( vals[ t ].strValue );
                        fprintf( fp, "  str_%zd_%d:\n    .string \"%s\"\n", l, t, e.c_str() );
                    }
                }
            }
        }

        fprintf( fp, ".text\n" );
        fprintf( fp, ".ifdef MAIXDUINO\n" );
        fprintf( fp, "  .globl bamain\n" );
        fprintf( fp, "  .type bamain, @function\n" );
        fprintf( fp, "  bamain:\n" );
        fprintf( fp, ".else\n" );
        fprintf( fp, "  .globl main\n" );
        fprintf( fp, "  .type main, @function\n" );
        fprintf( fp, "  main:\n" );
        fprintf( fp, ".endif\n" );

        fprintf( fp, "    .cfi_startproc\n" );
        fprintf( fp, "    addi     sp, sp, -128\n" );
        fprintf( fp, "    sd       ra, 16(sp)\n" );
        fprintf( fp, "    sd       s0, 24(sp)\n" );
        fprintf( fp, "    sd       s1, 32(sp)\n" );
        fprintf( fp, "    sd       s2, 40(sp)\n" );
        fprintf( fp, "    sd       s3, 48(sp)\n" );
        fprintf( fp, "    sd       s4, 56(sp)\n" );
        fprintf( fp, "    sd       s5, 64(sp)\n" );
        fprintf( fp, "    sd       s6, 72(sp)\n" );
        fprintf( fp, "    sd       s7, 80(sp)\n" );
        fprintf( fp, "    sd       s8, 88(sp)\n" );
        fprintf( fp, "    sd       s9, 96(sp)\n" );
        fprintf( fp, "    sd       s10, 104(sp)\n" );
        fprintf( fp, "    sd       s11, 112(sp)\n" );

        // set BASIC variable av% to the integer value of the first app argument (if any)

        if ( FindVariable( varmap, "av%" ) )
        {
            fprintf( fp, "    li       a2, 2\n" );
            fprintf( fp, "    blt      a0, a2, .no_arguments\n" );
            fprintf( fp, "    slli     a2, a2, 2\n" );
            fprintf( fp, "    add      a1, a1, a2\n" );
            fprintf( fp, "    ld       a0, (a1)\n" );
            fprintf( fp, "    jal      a_to_uint64\n" );
            fprintf( fp, "    lla      t1, %s\n", GenVariableName( "av%" ) );
            fprintf( fp, "    sw       a0, (t1)\n" );
            fprintf( fp, "  .no_arguments:\n" );
        }

        if ( !g_Quiet )
        {
            fprintf( fp, "    lla      a0, startString\n" );
            fprintf( fp, "    jal      rvos_print_text\n" );
        }

        fprintf( fp, ".ifdef MAIXDUINO\n" );
        fprintf( fp, "    rdcycle  a0  # rdtime doesn't work on the K210 CPU\n" );
        fprintf( fp, ".else\n" );
        fprintf( fp, "    rdtime   a0  # time in nanoseconds\n" );
        fprintf( fp, ".endif\n" );
        fprintf( fp, "    lla      t0, startTicks\n" );
        fprintf( fp, "    sd       a0, (t0)\n" );
    }
    else if ( m68kCPM == g_AssemblyTarget )
    {
        if ( elapReferenced )
            fprintf( fp, "elapString: .dc.b 'seconds',0\n" );
        if ( !g_Quiet )
        {
            fprintf( fp, "startString: .dc.b 'running basic',13,10,0\n" );
            fprintf( fp, "stopString: .dc.b 'done running basic',13,10,0\n" );
        }
        fprintf( fp, "errorString: .dc.b 'internal error',13,10,0\n" );
        fprintf( fp, "strString:   .dc.b '%%s',0\n" );
        fprintf( fp, "intString:   .dc.b '%%ld',0\n" );
        fprintf( fp, "newlineString: .dc.b 13,10,0\n" );

        fprintf( fp, ".globl _main\n" );
        fprintf( fp, ".text\n" );
        fprintf( fp, "_main:\n" );
        if ( !g_Quiet )
        {
            fprintf( fp, "    lea        startString, a0\n" );
            fprintf( fp, "    move.l     a0, -(a7)\n" );
            fprintf( fp, "    jsr        _printf\n" );
            fprintf( fp, "    add.l      #4, a7\n" );
        }
    }
    else if ( oiOS == g_AssemblyTarget )
    {
        if ( elapReferenced )
            fprintf( fp, "    string elapString \" seconds\"\n" );
        if ( !g_Quiet )
        {
            fprintf( fp, "    string startString \"running basic\\n\"\n" );
            fprintf( fp, "    string stopString \"done running basic\\n\"\n" );
        }
        fprintf( fp, "    string errorString \"internal error\\n\"\n" );
        fprintf( fp, "    string newlineString \"\\n\"\n" );

        for ( size_t l = 0; l < g_linesOfCode.size(); l++ )
        {
            LineOfCode & loc = g_linesOfCode[ l ];
            vector<TokenValue> & vals = loc.tokenValues;
    
            if ( Token_DIM == vals[ 0 ].token )
            {
                int cdwords = vals[ 0 ].dims[ 0 ];
                if ( 2 == vals[ 0 ].dimensions )
                    cdwords *= vals[ 0 ].dims[ 1 ];
    
                Variable * pvar = FindVariable( varmap, vals[ 0 ].strValue );
    
                if ( 0 != pvar )
                {
                    fprintf( fp, "    align\n" );
                    fprintf( fp, "    image_t %8s[%d]\n", GenVariableName( vals[ 0 ].strValue ), cdwords );
                }
            }
        }

        fprintf( fp, ".dataend\n" );
        fprintf( fp, "\n" );

        fprintf( fp, ".code\n" );
        fprintf( fp, "start:\n" );
        if ( !g_Quiet )
        {
            fprintf( fp, "    ldi      rarg1, startString\n" );
            fprintf( fp, "    syscall  1\n" );
        }
    }

    if ( useRegistersInASM )
    {
        if ( x64Win == g_AssemblyTarget )
            for ( size_t i = 0; i < _countof( MappedRegistersX64 ); i++ )
                fprintf( fp, "    xor      %s, %s\n", MappedRegistersX64[ i ], MappedRegistersX64[ i ] );
        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
            for ( size_t i = 0; i < _countof( MappedRegistersArm64 ); i++ )
                fprintf( fp, "    mov      %s, 0\n", MappedRegistersArm64[ i ] );
        else if ( arm32Linux == g_AssemblyTarget )
            for ( size_t i = 0; i < _countof( MappedRegistersArm32 ); i++ )
                fprintf( fp, "    mov      %s, #0\n", MappedRegistersArm32[ i ] );
        else if ( x86Win == g_AssemblyTarget )
            for ( size_t i = 0; i < _countof( MappedRegistersX86 ); i++ )
                fprintf( fp, "    xor      %s, %s\n", MappedRegistersX86[ i ], MappedRegistersX86[ i ] );
        else if ( riscv64 == g_AssemblyTarget )
            for ( size_t i = 0; i < _countof( MappedRegistersRiscV64 ); i++ )
                fprintf( fp, "    mv       %s, zero\n", MappedRegistersRiscV64[ i ] );
        else if ( m68kCPM == g_AssemblyTarget )
            for ( size_t i = 0; i < _countof( MappedRegistersm68kCPM ); i++ )
                fprintf( fp, "    clr.l       %s\n", MappedRegistersm68kCPM[ i ] );
        else if ( oiOS == g_AssemblyTarget )
            for ( size_t i = 0; i < _countof( MappedRegistersOIOS ); i++ )
                fprintf( fp, "    zero     %s\n", MappedRegistersOIOS[ i ] );

        for ( std::map<string, Variable>::iterator it = varmap.begin(); it != varmap.end(); it++ )
        {
            if ( ( 0 != it->second.dimensions ) && ( 0 != it->second.reg.length() ) )
            {
                string const & varname = it->first;

                if ( x64Win == g_AssemblyTarget )
                    fprintf( fp, "    mov      %s, %s\n", GenVariableReg( varmap, varname ), varname.c_str() );
                else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                    LoadArm64Address( fp, GenVariableReg64( varmap, varname ), varname );
            }
        }
    }

    static int s_uniqueLabel = 0;
    static Stack<ForGosubItem> forGosubStack;
    size_t activeIf = -1;

    for ( size_t l = 0; l < g_linesOfCode.size(); l++ )
    {
        LineOfCode & loc = g_linesOfCode[ l ];
        g_pc = (int) l;
        vector<TokenValue> & vals = loc.tokenValues;
        Token token = loc.firstToken;
        int t = 0;

        if ( EnableTracing && g_Tracing )
            printf( "generating code for line %zd ====> %s\n", l, loc.sourceCode.c_str() );

        if ( loc.goTarget )
        {
            if ( arm64Mac == g_AssemblyTarget  )
                fprintf( fp, ".p2align 2\n" ); // arm64 branch targets must be 4-byte aligned
            else if ( arm64Win == g_AssemblyTarget )
                fprintf( fp, "  align 4\n" ); // arm64 branch targets must be 4-byte aligned
        }

        if ( i8080CPM == g_AssemblyTarget )
            fprintf( fp, "  ln$%zd:   ; ===>>> %s\n", l, RemoveExclamations( loc.sourceCode.c_str() ) );
        else if ( mos6502Apple1 == g_AssemblyTarget )
            fprintf( fp, "line_number_%zd   ; ===>>> %s\n", l, loc.sourceCode.c_str() );
        else if ( arm32Linux == g_AssemblyTarget )
            fprintf( fp, "  line_number_%zd:   @ ===>>> %s\n", l, loc.sourceCode.c_str() );
        else if ( arm64Win == g_AssemblyTarget )
            fprintf( fp, "line_number_%zd   ; ===>>> %s\n", l, loc.sourceCode.c_str() );
        else if ( riscv64 == g_AssemblyTarget )
            fprintf( fp, "  line_number_%zd:   # ===>>> %s\n", l, loc.sourceCode.c_str() );
        else if ( m68kCPM == g_AssemblyTarget )
            fprintf( fp, "ln$%zd:    * ===>>> %s\n", l, loc.sourceCode.c_str() );
        else
            fprintf( fp, "  line_number_%zd:   ; ===>>> %s\n", l, loc.sourceCode.c_str() );

        do  // all tokens in the line
        {
            if ( EnableTracing && g_Tracing )
                printf( "generating code for line %zd, token %d %s, valsize %zd\n", l, t, TokenStr( vals[ t ].token ), vals.size() );

            if ( Token_VARIABLE == token )
            {
                int variableToken = t;
                t++;
    
                if ( Token_EQ == vals[ t ].token )
                {
                    t++;
    
                    assert( Token_EXPRESSION == vals[ t ].token );

                    if ( !g_ExpressionOptimization )
                        goto label_no_eq_optimization;

                    if ( i8080CPM != g_AssemblyTarget &&
                         Token_CONSTANT == vals[ t + 1 ].token &&
                         2 == vals[ t ].value  )
                    {
                        // e.g.: x% = 3
                        // note: testing for 0 and generating xor resulted in slower generated code. Not sure why.

                        string & varname = vals[ variableToken ].strValue;
                        int val = vals[ t + 1 ].value;

                        if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                        {
                            if ( IsVariableInReg( varmap, varname ) )
                                fprintf( fp, "    mov      %s, %d\n", GenVariableReg( varmap, varname ), val );
                            else
                                fprintf( fp, "    mov      DWORD PTR [%s], %d\n", GenVariableName( varname ), val );
                        }
                        else if ( arm32Linux == g_AssemblyTarget )
                        {
                            if ( IsVariableInReg( varmap, varname ) )
                                LoadArm32Constant( fp, GenVariableReg( varmap, varname ), val );
                            else
                            {
                                LoadArm32Constant( fp, "r0", val );
                                LoadArm32Address( fp, "r1", varname );
                                fprintf( fp, "    str      r0, [r1]\n" );
                            }
                        }
                        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                        {
                            if ( IsVariableInReg( varmap, varname ) )
                                LoadArm64Constant( fp, GenVariableReg( varmap, varname ), val );
                            else
                            {
                                LoadArm64Constant( fp, "x0", val );
                                LoadArm64Address( fp, "x1", varname );
                                fprintf( fp, "    str      w0, [x1]\n" );
                            }
                        }
                        else if ( mos6502Apple1 == g_AssemblyTarget )
                        {
                            fprintf( fp, "    lda      #%d\n", val );
                            fprintf( fp, "    sta      %s\n", GenVariableName( varname ) );
                            fprintf( fp, "    lda      /%d\n", val );
                            fprintf( fp, "    sta      %s+1\n", GenVariableName( varname ) );
                        }
                        else if ( i8086DOS == g_AssemblyTarget )
                            fprintf( fp, "    mov      WORD PTR ds: [%s], %d\n", GenVariableName( varname ), val );
                        else if ( riscv64 == g_AssemblyTarget )
                        {
                            if ( IsVariableInReg( varmap, varname ) )
                                fprintf( fp, "    li       %s, %d\n", GenVariableReg( varmap, varname ), val );
                            else
                            {
                                fprintf( fp, "    lla      t1, %s\n", GenVariableName( varname ) );
                                fprintf( fp, "    li       t0, %d\n", val );
                                fprintf( fp, "    sw       t0, (t1)\n" );
                            }
                        }
                        else if ( m68kCPM == g_AssemblyTarget )
                        {
                            if ( IsVariableInReg( varmap, varname ) )
                                fprintf( fp, "    move.l   #%d, %s\n", val, GenVariableReg( varmap, varname ) );
                            else
                            {
                                fprintf( fp, "    move.l   #%d, d0\n", val );
                                fprintf( fp, "    move.l   d0, %s\n", GenVariableName( varname ) );
                            }
                        }
                        else if ( oiOS == g_AssemblyTarget )
                        {
                            if ( IsVariableInReg( varmap, varname ) )
                            {
                                if ( 0 == val )
                                    fprintf( fp, "    zero     %s\n", GenVariableReg( varmap, varname ) );
                                else
                                    fprintf( fp, "    ldi      %s, %d\n", GenVariableReg( varmap, varname ), val );
                            }
                            else
                            {
                                if ( 0 == val )
                                    fprintf( fp, "    st       [%s], rzero\n", GenVariableName( varname ) );
                                else
                                {
                                    if ( val < -32 || val > 31 )
                                    {
                                        fprintf( fp, "    ldi      rres, %d\n", val );
                                        fprintf( fp, "    st       [%s], rres\n", GenVariableName( varname ) );
                                    }
                                    else
                                        fprintf( fp, "    sti      [%s], %d\n", GenVariableName( varname ), val );
                                }
                            }
                        }

                        t += vals[ t ].value;
                    }
                    else if ( Token_VARIABLE == vals[ t + 1 ].token && 2 == vals[ t ].value &&
                              IsVariableInReg( varmap, vals[ t + 1 ].strValue ) &&
                              IsVariableInReg( varmap, vals[ variableToken ].strValue ) )
                    {
                        // e.g.: x% = y%

                        if ( x64Win == g_AssemblyTarget || arm64Mac == g_AssemblyTarget || x86Win == g_AssemblyTarget ||
                             arm32Linux == g_AssemblyTarget || arm64Win == g_AssemblyTarget || oiOS == g_AssemblyTarget )
                            fprintf( fp, "    mov      %s, %s\n", GenVariableReg( varmap, vals[ variableToken ].strValue ),
                                                                  GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                        else if ( riscv64 == g_AssemblyTarget )
                            fprintf( fp, "    mv       %s, %s\n", GenVariableReg( varmap, vals[ variableToken ].strValue ),
                                                                  GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                        else if ( m68kCPM == g_AssemblyTarget )
                            fprintf( fp, "    move.l   %s, %s\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ),
                                                                  GenVariableReg( varmap, vals[ variableToken ].strValue ) );

                        t += vals[ t ].value;
                    }
                    else if ( oiOS == g_AssemblyTarget &&
                              Token_VARIABLE == vals[ t + 1 ].token && 2 == vals[ t ].value )
                    {
                        // e.g.: x% = y%
                        // note: the above case handles inreg = inreg.
                        string & varTo = vals[ variableToken ].strValue;
                        string & varFrom = vals[ t + 1 ].strValue;

                        if ( IsVariableInReg( varmap, varTo ) )
                            fprintf( fp, "    ld       %s, [ %s ]\n", GenVariableReg( varmap, varTo ), GenVariableName( varFrom ) );
                        else if ( IsVariableInReg( varmap, varFrom ) )
                            fprintf( fp, "    st       [%s], %s\n", GenVariableName( varTo ), GenVariableReg( varmap, varFrom ) );
                        else
                        {
                            fprintf( fp, "    ld       rres, [ %s ]\n", GenVariableName( varFrom ) );
                            fprintf( fp, "    st       [%s], rres\n", GenVariableName( varTo ) );
                        }

                        t += vals[ t ].value;
                    }
                    else if ( mos6502Apple1 == g_AssemblyTarget &&
                              Token_VARIABLE == vals[ t + 1 ].token &&
                              2 == vals[ t ].value )
                    {
                        // e.g.: x% = y%

                        fprintf( fp, "    lda      %s\n", GenVariableName( vals[ t + 1 ].strValue ) );
                        fprintf( fp, "    sta      %s\n", GenVariableName( vals[ variableToken ].strValue ) );
                        fprintf( fp, "    lda      %s+1\n", GenVariableName( vals[ t + 1 ].strValue ) );
                        fprintf( fp, "    sta      %s+1\n", GenVariableName( vals[ variableToken ].strValue ) );

                        t += vals[ t ].value;
                    }
                    else if ( oiOS == g_AssemblyTarget &&
                              6 == vals.size() &&
                              isTokenSimpleValue( vals[ t + 1 ].token ) &&
                              isTokenOperator( vals[ t + 2 ].token ) &&
                              isTokenSimpleValue( vals[ t + 3 ].token ) )
                    {
                        // line 120 has 6 tokens  ====>> 120 K% = I% + PR%
                        //   0 VARIABLE, value 0, strValue 'k%'
                        //   1 EQ, value 0, strValue ''
                        //   2 EXPRESSION, value 4, strValue ''
                        //   3 VARIABLE, value 0, strValue 'i%'
                        //   4 PLUS, value 0, strValue ''
                        //   5 VARIABLE, value 0, strValue 'pr%'

                        if ( Token_VARIABLE == vals[ t + 1 ].token &&
                             Token_VARIABLE == vals[ t + 3 ].token &&
                             IsVariableInReg( varmap, vals[ variableToken ].strValue ) &&
                             IsVariableInReg( varmap, vals[ t + 1 ].strValue ) &&
                             IsVariableInReg( varmap, vals[ t + 3 ].strValue ) &&
                             !stcmp( vals[ variableToken ].strValue, vals[ t + 1 ].strValue ) )
                        {
                            // K% = K% + PR%
                            fprintf( fp, "    add      %s, %s\n",
                                     GenVariableReg( varmap, vals[ variableToken ].strValue ),
                                     GenVariableReg( varmap, vals[ t + 3 ].strValue ) );
                        }
                        else
                        {
                            const char * pregtouse = "rres";
    
                            if ( Token_CONSTANT == vals[ t + 1 ].token )
                                fprintf( fp, "    ldi      rres, %d\n", vals[ t + 1 ].value );
                            else
                            {
                                if ( IsVariableInReg( varmap, vals[ t + 1 ].strValue ) )
                                    pregtouse = GenVariableReg( varmap, vals[ t + 1 ].strValue );
                                else
                                    fprintf( fp, "    ld       rres, [%s]\n", GenVariableName( vals[ t + 1 ].strValue ) );
                            }
    
                            const char * psecondreg = "rtmp";
    
                            if ( Token_CONSTANT == vals[ t + 3 ].token )
                                fprintf( fp, "    ldi      rtmp, %d\n", vals[ t + 3 ].value );
                            else
                            {
                                if ( IsVariableInReg( varmap, vals[ t + 3 ].strValue ) )
                                    psecondreg = GenVariableReg( varmap, vals[ t + 3 ].strValue );
                                else
                                    fprintf( fp, "    ld       rtmp, [%s]\n", GenVariableName( vals[ t + 3 ].strValue ) );
                            }
    
                            const char * presultreg = "rres";
                            if ( IsVariableInReg( varmap, vals[ variableToken ].strValue ) )
                                presultreg = GenVariableReg( varmap, vals[ variableToken ].strValue ) ;
    
                            if ( isOperatorRelational( vals[ t + 2 ].token )  )
                                fprintf( fp, "    cmp      %s, %s, %s, %s\n", presultreg, pregtouse, psecondreg, ConditionsArm[ vals[ t + 2 ].token ] );
                            else
                                fprintf( fp, "    math     %s, %s, %s, %s\n", presultreg, pregtouse, psecondreg, OperatorInstructionX64[ vals[ t + 2 ].token ] );
    
                            if ( !IsVariableInReg( varmap, vals[ variableToken ].strValue ) )
                                fprintf( fp, "    st       [%s], rres\n", GenVariableName( vals[ variableToken ].strValue ) );
                        }

                        t += vals[ t ].value;
                    }
                    else if ( 6 == vals[ t ].value &&
                              oiOS != g_AssemblyTarget &&
                              Token_VARIABLE == vals[ t + 1 ].token &&
                              IsVariableInReg( varmap, vals[ variableToken ].strValue ) &&
                              Token_OPENPAREN == vals[ t + 2 ].token &&
                              isTokenSimpleValue( vals[ t + 4 ].token ) &&
                              ( Token_CONSTANT == vals[ t + 4 ].token || IsVariableInReg( varmap, vals[ t + 4 ].strValue ) ) )
                    {
                        // e.g.: p% = sp%( st% ) 
                        //       p% = sp%( 4 )

                        // line 4290 has 8 tokens  ====>> 4290 p% = sp%(st%)
                        // token   0 VARIABLE, value 0, strValue 'p%'
                        // token   1 EQ, value 0, strValue ''
                        // token   2 EXPRESSION, value 6, strValue ''
                        // token   3 VARIABLE, value 0, strValue 'sp%'
                        // token   4 OPENPAREN, value 0, strValue ''
                        // token   5 EXPRESSION, value 2, strValue ''
                        // token   6 VARIABLE, value 0, strValue 'st%'
                        // token   7 CLOSEPAREN, value 0, strValue ''

                        if ( x64Win == g_AssemblyTarget )
                        {
                            if ( Token_CONSTANT == vals[ t + 4 ].token )
                            {
                                fprintf( fp, "    mov      %s, [ %s + %d ]\n", GenVariableReg( varmap, vals[ variableToken ].strValue ),
                                                                               GenVariableName( vals[ t + 1 ].strValue ),
                                                                               4 * vals[ t + 4 ].value );
                            }
                            else
                            {
                                fprintf( fp, "    mov      eax, %s\n", GenVariableReg( varmap, vals[ t + 4 ].strValue ) );
                                fprintf( fp, "    shl      rax, 2\n" );
                                fprintf( fp, "    lea      rbx, [%s]\n", GenVariableName( vals[ t + 1 ].strValue ) );
                                fprintf( fp, "    mov      %s, [ rax + rbx ]\n", GenVariableReg( varmap, vals[ variableToken ].strValue ) );
                            }
                        }
                        else if ( arm32Linux == g_AssemblyTarget )
                        {
                            string & vararray = vals[ t + 1 ].strValue;
                            string & varname = vals[ variableToken ].strValue;

                            if ( Token_CONSTANT == vals[ t + 4 ].token )
                            {
                                LoadArm32Address( fp, "r1", varmap, vararray );

                                if ( 0 != vals[ t + 4 ].value )
                                {
                                    int constant = 4 * vals[ t + 4 ].value;
                                    LoadArm32Constant( fp, "r2", constant );
                                    fprintf( fp, "    add      r1, r1, r2\n" );
                                }

                                fprintf( fp, "    ldr      %s, [r1]\n", GenVariableReg( varmap, varname ) );
                            }
                            else
                            {
                                LoadArm32Address( fp, "r1", varmap, vararray );
                                fprintf( fp, "    add      r1, r1, %s, lsl #2\n", GenVariableReg( varmap, vals[ t + 4 ].strValue ) );
                                fprintf( fp, "    ldr      %s, [r1]\n", GenVariableReg( varmap, varname ) );
                            }
                        }
                        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                        {
                            string & vararray = vals[ t + 1 ].strValue;
                            string & varname = vals[ variableToken ].strValue;

                            if ( Token_CONSTANT == vals[ t + 4 ].token )
                            {
                                if ( 0 != vals[ t + 4 ].value )
                                {
                                    int constant = 4 * vals[ t + 4 ].value;

                                    if ( fitsIn8Bits( constant ) )
                                    {
                                        if ( IsVariableInReg( varmap, vararray ) )
                                            fprintf( fp, "    ldr      %s, [%s, %d]\n", GenVariableReg( varmap, varname ),
                                                                                        GenVariableReg64( varmap, vararray ),
                                                                                        constant );
                                        else
                                        {
                                            LoadArm64Address( fp, "x1", varmap, vals[ t + 1 ].strValue );
                                            fprintf( fp, "    ldr      %s, [x1, %d]\n", GenVariableReg( varmap, varname ),
                                                                                        constant );
                                        }
                                    }
                                    else
                                    {
                                        if ( IsVariableInReg( varmap, vararray ) )
                                            fprintf( fp, "    mov      x1, %s\n", GenVariableReg( varmap, vararray ) );
                                        else
                                            LoadArm64Address( fp, "x1", varmap, vararray );
            
                                        if ( fitsIn12Bits( constant ) )
                                            fprintf( fp, "    add      x1, x1, %d\n", constant );
                                        else
                                        {
                                            LoadArm64Constant( fp, "x2", constant );
                                            fprintf( fp, "    add      x1, x1, x2\n" );
                                        }

                                        fprintf( fp, "    ldr      %s, [x1]\n", GenVariableReg( varmap, varname ) );
                                    }
                                }
                                else
                                {
                                   if ( IsVariableInReg( varmap, vararray ) )
                                       fprintf( fp, "    ldr      %s, [%s]\n", GenVariableReg( varmap, varname ),
                                                                               GenVariableReg64( varmap, vararray ) );
                                   else
                                   {
                                       LoadArm64Address( fp, "x1", varmap, vararray );
                                       fprintf( fp, "    ldr      %s, [x1]\n", GenVariableReg( varmap, varname ) );
                                   }
                                }
                            }
                            else
                            {
                                if ( IsVariableInReg( varmap, vararray ) )
                                {
                                    fprintf( fp, "    add      x1, %s, %s, lsl #2\n", GenVariableReg64( varmap, vararray ), GenVariableReg64( varmap, vals[ t + 4 ].strValue ) );
                                    fprintf( fp, "    ldr      %s, [x1]\n", GenVariableReg( varmap, varname ) );
                                }
                                else
                                {
                                    LoadArm64Address( fp, "x1", varmap, vararray );

                                    fprintf( fp, "    add      x1, x1, %s, lsl #2\n", GenVariableReg64( varmap, vals[ t + 4 ].strValue ) );
                                    fprintf( fp, "    ldr      %s, [x1]\n", GenVariableReg( varmap, varname ) );
                                }
                            }
                        }
                        else if ( x86Win == g_AssemblyTarget )
                        {
                            if ( Token_CONSTANT == vals[ t + 4 ].token )
                            {
                                fprintf( fp, "    mov      %s, [ %s + %d ]\n", GenVariableReg( varmap, vals[ variableToken ].strValue ),
                                                                               GenVariableName( vals[ t + 1 ].strValue ),
                                                                               4 * vals[ t + 4 ].value );
                            }
                            else
                            {
                                fprintf( fp, "    mov      eax, %s\n", GenVariableReg( varmap, vals[ t + 4 ].strValue ) );
                                fprintf( fp, "    shl      eax, 2\n" );
                                fprintf( fp, "    lea      ebx, [%s]\n", GenVariableName( vals[ t + 1 ].strValue ) );
                                fprintf( fp, "    mov      %s, [ eax + ebx ]\n", GenVariableReg( varmap, vals[ variableToken ].strValue ) );
                            }
                        }
                        else if ( riscv64 == g_AssemblyTarget )
                        {
                            fprintf( fp, "    lla      t0, %s\n", GenVariableName( vals[ t + 1 ].strValue ) );

                            if ( Token_CONSTANT == vals[ t + 4 ].token )
                                fprintf( fp, "    li       t1, %d\n", 4 * vals[ t + 4 ].value );
                            else
                            {
                                fprintf( fp, "    mv       t1, %s\n", GenVariableReg( varmap, vals[ t + 4 ].strValue ) );
                                fprintf( fp, "    slli     t1, t1, 2\n" );
                            }

                            fprintf( fp, "    add      t0, t0, t1\n" );
                            fprintf( fp, "    lw       %s, (t0)\n", GenVariableReg( varmap, vals[ variableToken ].strValue ) );
                        }
                        else if ( m68kCPM == g_AssemblyTarget )
                        {
                            fprintf( fp, "    lea      %s, a0\n", GenVariableName( vals[ t + 1 ].strValue ) );

                            if ( Token_CONSTANT == vals[ t + 4 ].token )
#if 0
                                fprintf( fp, "    move.l   #%d, d1\n", 4 * vals[ t + 4 ].value );
                                fprintf( fp, "    move.l   (a0, d1), %s\n", GenVariableReg( varmap, vals[ variableToken ].strValue ) );
#else
                                fprintf( fp, "    move.l   %d(a0), %s\n", 4 * vals[ t + 4 ].value, GenVariableReg( varmap, vals[ variableToken ].strValue ) );
#endif
                            else
                            {
                                fprintf( fp, "    move.l   %s, d1\n", GenVariableReg( varmap, vals[ t + 4 ].strValue ) );
                                fprintf( fp, "    lsl.l    #2, d1\n" );
                                fprintf( fp, "    move.l   (a0, d1), %s\n", GenVariableReg( varmap, vals[ variableToken ].strValue ) );
                            }
                        }

                        t += vals[ t ].value;
                    }
                    else if ( i8080CPM == g_AssemblyTarget &&
                              8 == vals.size() &&
                              Token_VARIABLE == vals[ t + 1 ].token &&
                              Token_PLUS == vals[ t + 2 ].token &&
                              Token_VARIABLE == vals[ t + 3 ].token &&
                              !stcmp( vals[ t + 1 ].strValue, vals[ t + 3 ].strValue ) &&
                              Token_PLUS == vals[ t + 4 ].token &&
                              Token_CONSTANT == vals[ t + 5 ].token )
                    {
                        // line 105 has 8 tokens  ====>> 105 PR% = I% + I% + 3
                        //   0 VARIABLE, value 0, strValue 'pr%'
                        //   1 EQ, value 0, strValue ''
                        //   2 EXPRESSION, value 6, strValue ''
                        //   3 VARIABLE, value 0, strValue 'i%'
                        //   4 PLUS, value 0, strValue ''
                        //   5 VARIABLE, value 0, strValue 'i%'
                        //   6 PLUS, value 0, strValue ''
                        //   7 CONSTANT, value 3, strValue ''

                        fprintf( fp, "    lhld     %s\n", GenVariableName( vals[ t + 1 ].strValue ) );
                        fprintf( fp, "    dad      h\n" );
                        fprintf( fp, "    lxi      d, %d\n", vals[ t + 5 ].value );
                        fprintf( fp, "    dad      d\n" );
                        fprintf( fp, "    shld     %s\n", GenVariableName( vals[ variableToken ].strValue ) );
                    }
                    else if ( oiOS == g_AssemblyTarget &&
                              8 == vals.size() &&
                              Token_VARIABLE == vals[ t + 1 ].token &&
                              Token_PLUS == vals[ t + 2 ].token &&
                              Token_VARIABLE == vals[ t + 3 ].token &&
                              !stcmp( vals[ t + 1 ].strValue, vals[ t + 3 ].strValue ) &&
                              Token_PLUS == vals[ t + 4 ].token &&
                              Token_CONSTANT == vals[ t + 5 ].token )
                    {
                        // line 105 has 8 tokens  ====>> 105 PR% = I% + I% + 3
                        //   0 VARIABLE, value 0, strValue 'pr%'
                        //   1 EQ, value 0, strValue ''
                        //   2 EXPRESSION, value 6, strValue ''
                        //   3 VARIABLE, value 0, strValue 'i%'
                        //   4 PLUS, value 0, strValue ''
                        //   5 VARIABLE, value 0, strValue 'i%'
                        //   6 PLUS, value 0, strValue ''
                        //   7 CONSTANT, value 3, strValue ''

                        if ( IsVariableInReg( varmap, vals[ t + 1 ].strValue ) &&
                             IsVariableInReg( varmap, vals[ variableToken ].strValue ) )
                        {
                            fprintf( fp, "    ldi      %s, %d\n", GenVariableReg( varmap, vals[ variableToken ].strValue ), vals[ t + 5 ].value );
                            fprintf( fp, "    add      %s, %s\n",
                                     GenVariableReg( varmap, vals[ variableToken ].strValue ),
                                     GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                            fprintf( fp, "    add      %s, %s\n",
                                     GenVariableReg( varmap, vals[ variableToken ].strValue ),
                                     GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                        }
                        else
                        {
                            if ( IsVariableInReg( varmap, vals[ t + 1 ].strValue ) )
                            {
                                const char * pvarval =GenVariableReg( varmap, vals[ t + 1 ].strValue );
                                fprintf( fp, "    math     rres, %s, %s, add\n", pvarval, pvarval );
                            }
                            else
                            {
                                fprintf( fp, "    ld      rres, [%s]\n", GenVariableName( vals[ t + 1 ].strValue ) );
                                fprintf( fp, "    add     rres, rres\n" );
                            }
    
                            fprintf( fp, "    ldi      rtmp, %d\n", vals[ t + 5 ].value );
    
                            if ( IsVariableInReg( varmap, vals[ variableToken ].strValue ) )
                                fprintf( fp, "    math     %s, rtmp, rres, add\n", GenVariableReg( varmap, vals[ variableToken ].strValue ) );
                            else
                            {
                                fprintf( fp, "    add      rres, rtmp\n" );
                                fprintf( fp, "    st       [%s], rres\n", GenVariableName( vals[ variableToken ].strValue ) );
                            }
                        }
                    }
                    else if ( mos6502Apple1 == g_AssemblyTarget &&
                              6 == vals[ t ].value &&
                              Token_VARIABLE == vals[ t + 1 ].token &&
                              Token_OPENPAREN == vals[ t + 2 ].token &&
                              isTokenSimpleValue( vals[ t + 4 ].token ) )
                    {
                        // e.g.: p% = sp%( st% ) 
                        //       p% = sp%( 4 )

                        // line 4290 has 8 tokens  ====>> 4290 p% = sp%(st%)
                        //   0 VARIABLE, value 0, strValue 'p%'
                        //   1 EQ, value 0, strValue ''
                        //   2 EXPRESSION, value 6, strValue ''
                        //   3 VARIABLE, value 0, strValue 'sp%'
                        //   4 OPENPAREN, value 0, strValue ''
                        //   5 EXPRESSION, value 2, strValue ''
                        //   6 VARIABLE, value 0, strValue 'st%'
                        //   7 CLOSEPAREN, value 0, strValue ''

                        string & vararray = vals[ t + 1 ].strValue;
                        string & varname = vals[ variableToken ].strValue;

                        if ( Token_CONSTANT == vals[ t + 4 ].token )
                        {
                            fprintf( fp, "    lda      #%d\n", 2 * vals[ t + 4 ].value );
                            fprintf( fp, "    clc\n" );
                            fprintf( fp, "    adc      #%s\n", GenVariableName( vararray ) );
                            fprintf( fp, "    sta      arrayOffset\n" );
                            fprintf( fp, "    lda      /%s\n", GenVariableName( vararray ) );
                            fprintf( fp, "    adc      /%d\n", 2 * vals[ t + 4 ].value );
                            fprintf( fp, "    sta      arrayOffset+1\n" );
                            fprintf( fp, "    ldy      #0\n" );
                            fprintf( fp, "    lda      (arrayOffset), y\n" );
                            fprintf( fp, "    sta      %s\n", GenVariableName( varname ) );
                            fprintf( fp, "    iny\n" );
                            fprintf( fp, "    lda      (arrayOffset), y\n" );
                            fprintf( fp, "    sta      %s+1\n", GenVariableName( varname ) );
                        }
                        else
                        {
                            fprintf( fp, "    lda      %s\n", GenVariableName( vals[ t + 4 ].strValue ) );
                            fprintf( fp, "    sta      curOperand\n" );
                            fprintf( fp, "    lda      %s+1\n", GenVariableName( vals[ t + 4 ].strValue ) );
                            fprintf( fp, "    sta      curOperand+1\n" );

                            fprintf( fp, "    asl      curOperand\n" );   // shift left into carry
                            fprintf( fp, "    rol      curOperand+1\n" ); // shift left from carry
                            fprintf( fp, "    lda      #%s\n", GenVariableName( vararray ) );
                            fprintf( fp, "    clc\n" );
                            fprintf( fp, "    adc      curOperand\n" );
                            fprintf( fp, "    sta      curOperand\n" );
                            fprintf( fp, "    lda      /%s\n", GenVariableName( vararray ) );
                            fprintf( fp, "    adc      curOperand+1\n" );
                            fprintf( fp, "    sta      curOperand+1\n" );
            
                            fprintf( fp, "    ldy      #0\n" );
                            fprintf( fp, "    lda      (curOperand), y\n" );
                            fprintf( fp, "    tax\n" );
                            fprintf( fp, "    iny\n" );
                            fprintf( fp, "    lda      (curOperand), y\n" );
                            fprintf( fp, "    sta      %s+1\n", GenVariableName( varname ) );
                            fprintf( fp, "    stx      %s\n", GenVariableName( varname ) );
                        }

                        t += vals[ t ].value;
                    }
                    else if ( i8086DOS == g_AssemblyTarget &&
                              6 == vals[ t ].value &&
                              Token_VARIABLE == vals[ t + 1 ].token &&
                              Token_OPENPAREN == vals[ t + 2 ].token &&
                              isTokenSimpleValue( vals[ t + 4 ].token ) )
                    {
                        // e.g.: p% = sp%( st% ) 
                        //       p% = sp%( 4 )

                        // line 4290 has 8 tokens  ====>> 4290 p% = sp%(st%)
                        //   0 VARIABLE, value 0, strValue 'p%'
                        //   1 EQ, value 0, strValue ''
                        //   2 EXPRESSION, value 6, strValue ''
                        //   3 VARIABLE, value 0, strValue 'sp%'
                        //   4 OPENPAREN, value 0, strValue ''
                        //   5 EXPRESSION, value 2, strValue ''
                        //   6 VARIABLE, value 0, strValue 'st%'
                        //   7 CLOSEPAREN, value 0, strValue ''

                        string & vararray = vals[ t + 1 ].strValue;
                        string & varname = vals[ variableToken ].strValue;

                        if ( Token_CONSTANT == vals[ t + 4 ].token )
                        {
                            fprintf( fp, "    mov      ax, ds: [ %s + %d ]\n", GenVariableName( vararray ), 2 * vals[ t + 4 ].value );
                        }
                        else
                        {
                            fprintf( fp, "    mov      si, ds: [ %s ]\n", GenVariableName( vals[ t + 4 ].strValue ) );
                            fprintf( fp, "    shl      si, 1\n" );
                            fprintf( fp, "    mov      ax, ds: [ offset %s + si ]\n", GenVariableName( vararray ) );
                        }

                        fprintf( fp, "    mov      WORD PTR ds: [ %s ], ax\n", GenVariableName( varname ) );

                        t += vals[ t ].value;
                    }
                    else if ( oiOS == g_AssemblyTarget &&
                              6 == vals[ t ].value &&
                              Token_VARIABLE == vals[ t + 1 ].token &&
                              Token_OPENPAREN == vals[ t + 2 ].token &&
                              isTokenSimpleValue( vals[ t + 4 ].token ) )
                    {
                        // e.g.: p% = sp%( st% ) 
                        //       p% = sp%( 4 )

                        // line 4290 has 8 tokens  ====>> 4290 p% = sp%(st%)
                        //   0 VARIABLE, value 0, strValue 'p%'
                        //   1 EQ, value 0, strValue ''
                        //   2 EXPRESSION, value 6, strValue ''
                        //   3 VARIABLE, value 0, strValue 'sp%'
                        //   4 OPENPAREN, value 0, strValue ''
                        //   5 EXPRESSION, value 2, strValue ''
                        //   6 VARIABLE, value 0, strValue 'st%'
                        //   7 CLOSEPAREN, value 0, strValue ''

                        string & vararray = vals[ t + 1 ].strValue;
                        string & varname = vals[ variableToken ].strValue;

                        if ( IsVariableInReg( varmap, varname ) )
                        {
                            if ( Token_CONSTANT == vals[ t + 4 ].token )
                            {
                                fprintf( fp, "    ldi      rres, %d\n", vals[ t + 4 ].value );
                                fprintf( fp, "    ldo      %s, %s[ rres ]\n", GenVariableReg( varmap, varname ), GenVariableName( vararray ) );
                            }
                            else
                            {
                                if ( IsVariableInReg( varmap, vals[ t + 4 ].strValue ) )
                                    fprintf( fp, "    ldo      %s, %s[ %s ]\n", GenVariableReg( varmap, varname ),
                                             GenVariableName( vararray ), GenVariableReg( varmap, vals[ t + 4 ].strValue ) );
                                else
                                {
                                    fprintf( fp, "    ld       rres, [%s]\n", GenVariableName( vals[ t + 4 ].strValue ) );
                                    fprintf( fp, "    ldo      %s, %s[ rres ]\n",  GenVariableReg( varmap, varname ), GenVariableName( vararray ) );
                                }
                            }
                        }
                        else
                        {
                            if ( Token_CONSTANT == vals[ t + 4 ].token )
                            {
                                fprintf( fp, "    ldi      rres, %d\n", vals[ t + 4 ].value );
                                fprintf( fp, "    ldae     %s[ rres ]\n", GenVariableName( vararray ) );
                            }
                            else
                            {
                                if ( IsVariableInReg( varmap, vals[ t + 4 ].strValue ) )
                                    fprintf( fp, "    ldae     %s[ %s ]\n", GenVariableName( vararray ), GenVariableReg( varmap, vals[ t + 4 ].strValue ) );
                                else
                                {
                                    fprintf( fp, "    ld       rres, [%s]\n", GenVariableName( vals[ t + 4 ].strValue ) );
                                    fprintf( fp, "    ldae     %s[ rres ]\n", GenVariableName( vararray ) );
                                }
                            }
    
                            fprintf( fp, "    st       [ %s ], rres\n", GenVariableName( varname ) );
                        }

                        t += vals[ t ].value;
                    }
                    else if ( oiOS == g_AssemblyTarget &&
                              12 == vals[ t ].value &&
                              IsVariableInReg( varmap, vals[ variableToken ].strValue ) &&
                              Token_OPENPAREN == vals[ t + 1 ].token &&
                              Token_OPENPAREN == vals[ t + 3 ].token &&
                              Token_CLOSEPAREN == vals[ t + 6 ].token &&
                              Token_CONSTANT == vals[ t + 8 ].token &&
                              Token_CLOSEPAREN == vals[ t + 9 ].token &&
                              Token_VARIABLE == vals[ t + 2 ].token &&
                              Token_VARIABLE == vals[ t + 5 ].token &&
                              IsVariableInReg( varmap, vals[ t + 5 ].strValue ) &&
                              Token_VARIABLE == vals[ t + 11 ].token &&
                              IsVariableInReg( varmap, vals[ t + 11 ].strValue ) &&
                              isTokenOperator( vals[ t + 7 ].token ) &&
                              isTokenOperator( vals[ t + 10 ].token ) )
                    {
                        // line 260 has 14 tokens  ====>> 260 x% = ( a%( n% ) * 10 ) + qu%
                        //    0 VARIABLE, value 0, strValue 'x%'
                        //    1 EQ, value 0, strValue ''
                        //    2 EXPRESSION, value 12, strValue ''
                        //    3 OPENPAREN, value 0, strValue ''
                        //    4 VARIABLE, value 0, strValue 'a%'
                        //    5 OPENPAREN, value 0, strValue ''
                        //    6 EXPRESSION, value 2, strValue ''
                        //    7 VARIABLE, value 0, strValue 'n%'
                        //    8 CLOSEPAREN, value 0, strValue ''
                        //    9 MULT, value 0, strValue ''
                        //   10 CONSTANT, value 10, strValue ''
                        //   11 CLOSEPAREN, value 0, strValue ''
                        //   12 PLUS, value 0, strValue ''
                        //   13 VARIABLE, value 0, strValue 'qu%'

                        fprintf( fp, "    ldo      rres, %s[%s]\n", GenVariableName( vals[ t + 2 ].strValue ), GenVariableReg( varmap, vals[ t + 5 ].strValue ) );
                        fprintf( fp, "    ldi      rtmp, %d\n", vals[ t + 8 ].value );
                        fprintf( fp, "    %-4s     rres, rtmp\n", OperatorInstructionX64[ vals[ t + 7 ].token ] );
                        fprintf( fp, "    math     %s, rres, %s, %s\n",
                                 GenVariableReg( varmap, vals[ variableToken ].strValue ),
                                 GenVariableReg( varmap, vals[ t + 11 ].strValue ),
                                 OperatorInstructionX64[ vals[ t + 10 ].token ] );

                        t += vals[ t ].value;
                    }
                    else
                    {
label_no_eq_optimization:
                        GenerateOptimizedExpression( fp, varmap, t, vals );
                        string const & varname = vals[ variableToken ].strValue;
                                                
                        if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                        {
                            if ( IsVariableInReg( varmap, varname ) )
                                fprintf( fp, "    mov      %s, eax\n", GenVariableReg( varmap, varname ) );
                            else
                                fprintf( fp, "    mov      DWORD PTR [%s], eax\n", GenVariableName( varname ) );
                        }
                        else if ( arm32Linux == g_AssemblyTarget )
                        {
                            if ( IsVariableInReg( varmap, varname ) )
                                fprintf( fp, "    mov      %s, r0\n", GenVariableReg( varmap, varname ) );
                            else
                            {
                                LoadArm32Address( fp, "r1", varname );
                                fprintf( fp, "    str      r0, [r1]\n" );
                            }
                        }
                        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                        {
                            if ( IsVariableInReg( varmap, varname ) )
                                fprintf( fp, "    mov      %s, w0\n", GenVariableReg( varmap, varname ) );
                            else
                            {
                                LoadArm64Address( fp, "x1", varname );
                                fprintf( fp, "    str      w0, [x1]\n" );
                            }
                        }
                        else if ( i8080CPM == g_AssemblyTarget )
                        {
                            fprintf( fp, "    shld     %s\n", GenVariableName( varname ) );
                        }
                        else if ( mos6502Apple1 == g_AssemblyTarget )
                        {
                            fprintf( fp, "    lda      curOperand\n" );
                            fprintf( fp, "    sta      %s\n", GenVariableName( varname ) );
                            fprintf( fp, "    lda      curOperand+1\n" );
                            fprintf( fp, "    sta      %s+1\n", GenVariableName( varname ) );
                        }
                        else if ( i8086DOS == g_AssemblyTarget )
                            fprintf( fp, "    mov      WORD PTR ds: [%s], ax\n", GenVariableName( varname ) );
                        else if ( riscv64 == g_AssemblyTarget )
                        {
                            if ( IsVariableInReg( varmap, varname ) )
                                fprintf( fp, "    mv       %s, a0\n", GenVariableReg( varmap, varname ) );
                            else
                            {
                                fprintf( fp, "    lla      t0, %s\n", GenVariableName( varname ) );
                                fprintf( fp, "    sw       a0, (t0)\n" );
                            }
                        }
                        else if ( m68kCPM == g_AssemblyTarget )
                        {
                            if ( IsVariableInReg( varmap, varname ) )
                                fprintf( fp, "    move.l   d0, %s\n", GenVariableReg( varmap, varname ) );
                            else
                                fprintf( fp, "    move.l   d0, %s\n", GenVariableName( varname ) );
                        }
                        else if ( oiOS == g_AssemblyTarget )
                        {
                            if ( IsVariableInReg( varmap, varname ) )
                                fprintf( fp, "    mov      %s, rres\n", GenVariableReg( varmap, varname ) );
                            else
                                fprintf( fp, "    st       [%s], rres\n", GenVariableName( varname ) );
                        }
                    }
                }
                else if ( Token_OPENPAREN == vals[ t ].token )
                {
                    t++;

                    assert( Token_EXPRESSION == vals[ t ].token );

                    if ( !g_ExpressionOptimization )
                        goto label_no_array_eq_optimization;

                    if ( x64Win == g_AssemblyTarget &&
                         false &&                        // This code is smaller on x64, but overall runtime is 10% slower!?!
                         8 == vals.size() &&
                         Token_CONSTANT == vals[ t + 1 ].token &&
                         Token_EQ == vals[ t + 3 ].token &&
                         Token_CONSTANT == vals[ t + 5 ].token )
                    {
                        // e.g.: b%( 0 ) = 0

                        // line 60 has 8 tokens  ====>> 60 b%(0) = 0
                        //    0 VARIABLE, value 0, strValue 'b%'
                        //    1 OPENPAREN, value 0, strValue ''
                        //    2 EXPRESSION, value 2, strValue ''
                        //    3 CONSTANT, value 0, strValue ''
                        //    4 CLOSEPAREN, value 0, strValue ''
                        //    5 EQ, value 0, strValue ''
                        //    6 EXPRESSION, value 2, strValue ''
                        //    7 CONSTANT, value 0, strValue ''

                        fprintf( fp, "    mov      DWORD PTR [ %s + %d ], %d\n", GenVariableName( vals[ variableToken ].strValue ),
                                                                                 4 * vals[ t + 1 ].value,
                                                                                 vals[ t + 5 ].value );
                        break;
                    }
                    else if ( ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget ) &&
                              8 == vals.size() &&
                              Token_CONSTANT == vals[ t + 1 ].token &&
                              Token_EQ == vals[ t + 3 ].token &&
                              Token_CONSTANT == vals[ t + 5 ].token )
                    {
                        // line 73 has 8 tokens  ====>> 73 b%(4) = 0
                        //   0 VARIABLE, value 0, strValue 'b%'
                        //   1 OPENPAREN, value 0, strValue ''
                        //   2 EXPRESSION, value 2, strValue ''
                        //   3 CONSTANT, value 4, strValue ''
                        //   4 CLOSEPAREN, value 0, strValue ''
                        //   5 EQ, value 0, strValue ''
                        //   6 EXPRESSION, value 2, strValue ''
                        //   7 CONSTANT, value 0, strValue ''

                        char const * arrayReg = "x2";
                        char const * writeReg = "x2";
                        if ( IsVariableInReg( varmap, vals[ variableToken ].strValue ) )
                        {
                            arrayReg = GenVariableReg64( varmap, vals[ variableToken ].strValue );
                            writeReg = arrayReg;
                        }
                        else    
                            LoadArm64Address( fp, "x2", varmap, vals[ variableToken ].strValue );

                        int offset = 4 * vals[ t + 1 ].value;
                        if ( !fitsIn8Bits( offset ) )
                        {
                            LoadArm64Constant( fp, "x1", offset );
                            fprintf( fp, "    add      x1, x1, %s\n", arrayReg );
                            writeReg = "x1";
                            offset = 0;
                        }

                        if ( 0 == vals[ t + 5 ].value )
                            fprintf( fp, "    str      wzr, [%s, %d]\n", writeReg, offset );
                        else
                        {
                            LoadArm64Constant( fp, "x0", vals[ t + 5 ].value );                            
                            fprintf( fp, "    str      w0, [%s, %d]\n", writeReg, offset );
                        }
                        break;
                    }
                    else if ( mos6502Apple1 == g_AssemblyTarget &&
                              8 == vals.size() &&
                              Token_CONSTANT == vals[ t + 1 ].token &&
                              Token_EQ == vals[ t + 3 ].token &&
                              Token_CONSTANT == vals[ t + 5 ].token &&
                              vals[ t + 1 ].value < 64 )
                    {
                        // line 73 has 8 tokens  ====>> 73 b%(4) = 0
                        //   0 VARIABLE, value 0, strValue 'b%'
                        //   1 OPENPAREN, value 0, strValue ''
                        //   2 EXPRESSION, value 2, strValue ''
                        //   3 CONSTANT, value 4, strValue ''
                        //   4 CLOSEPAREN, value 0, strValue ''
                        //   5 EQ, value 0, strValue ''
                        //   6 EXPRESSION, value 2, strValue ''
                        //   7 CONSTANT, value 0, strValue ''

                        fprintf( fp, "    lda      #%d\n", 2 * vals[ t + 1 ].value );
                        fprintf( fp, "    clc\n" );
                        fprintf( fp, "    adc      #%s\n", GenVariableName( vals[ variableToken ].strValue ) );
                        fprintf( fp, "    sta      arrayOffset\n" );
                        fprintf( fp, "    lda      /%s\n", GenVariableName( vals[ variableToken ].strValue ) );
                        fprintf( fp, "    adc      #0\n" );
                        fprintf( fp, "    sta      arrayOffset+1\n" );
                        fprintf( fp, "    lda      #%d\n", vals[ t + 5 ].value );
                        fprintf( fp, "    ldy      #0\n" );
                        fprintf( fp, "    sta      (arrayOffset), y\n" );
                        fprintf( fp, "    iny\n" );
                        fprintf( fp, "    lda      /%d\n", vals[ t + 5 ].value );
                        fprintf( fp, "    sta      (arrayOffset), y\n" );

                        break;
                    }
                    else if ( ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget ) &&
                              8 == vals.size() &&
                              Token_VARIABLE == vals[ t + 1 ].token &&
                              IsVariableInReg( varmap, vals[ t + 1 ].strValue ) &&
                              IsVariableInReg( varmap, vals[ variableToken ].strValue ) &&
                              Token_CONSTANT == vals[ t + 5 ].token )
                    {
                        // line 4328 has 8 tokens  ====>> 4328 b%(p%) = 0
                        //   0 VARIABLE, value 0, strValue 'b%'
                        //   1 OPENPAREN, value 0, strValue ''
                        //   2 EXPRESSION, value 2, strValue ''
                        //   3 VARIABLE, value 0, strValue 'p%'
                        //   4 CLOSEPAREN, value 0, strValue ''
                        //   5 EQ, value 0, strValue ''
                        //   6 EXPRESSION, value 2, strValue ''
                        //   7 CONSTANT, value 0, strValue ''

                        fprintf( fp, "    add      x1, %s, %s, lsl #2\n", GenVariableReg64( varmap, vals[ variableToken ].strValue ), GenVariableReg64( varmap, vals[ t + 1 ].strValue ) );
                        
                        if ( 0 == vals[ t + 5 ].value )
                            fprintf( fp, "    str      wzr, [x1]\n" );
                        else
                        {
                            LoadArm64Constant( fp, "x0", vals[ t + 5 ].value );                            
                            fprintf( fp, "    str      w0, [x1]\n" );
                        }
                        break;
                    }
                    else if ( m68kCPM == g_AssemblyTarget &&
                              8 == vals.size() &&
                              Token_VARIABLE == vals[ t + 1 ].token &&
                              IsVariableInReg( varmap, vals[ t + 1 ].strValue ) &&
                              Token_CONSTANT == vals[ t + 5 ].token )
                    {
                        // line 4328 has 8 tokens  ====>> 4328 b%(p%) = 0
                        //   0 VARIABLE, value 0, strValue 'b%'
                        //   1 OPENPAREN, value 0, strValue ''
                        //   2 EXPRESSION, value 2, strValue ''
                        //   3 VARIABLE, value 0, strValue 'p%'
                        //   4 CLOSEPAREN, value 0, strValue ''
                        //   5 EQ, value 0, strValue ''
                        //   6 EXPRESSION, value 2, strValue ''
                        //   7 CONSTANT, value 0, strValue ''

                        fprintf( fp, "    lea      %s, a0\n", GenVariableName( vals[ variableToken ].strValue ) );
                        fprintf( fp, "    move.l   %s, d1\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                        fprintf( fp, "    lsl.l    #2, d1\n" );
                        fprintf( fp, "    move.l   #%d, (a0,d1)\n", vals[ t + 5 ].value );
                        break;
                    }
                    else if ( oiOS == g_AssemblyTarget &&
                              8 == vals.size() &&
                              Token_VARIABLE == vals[ t + 1 ].token && 
                              Token_CONSTANT == vals[ t + 5 ].token )
                    {
                        // line 140 has 8 tokens  ====>> 140 FL%(K%) = 0
                        //   0 VARIABLE, value 0, strValue 'fl%'
                        //   1 OPENPAREN, value 0, strValue ''
                        //   2 EXPRESSION, value 2, strValue ''
                        //   3 VARIABLE, value 0, strValue 'k%'
                        //   4 CLOSEPAREN, value 0, strValue ''
                        //   5 EQ, value 0, strValue ''
                        //   6 EXPRESSION, value 2, strValue ''
                        //   7 CONSTANT, value 0, strValue ''

                        if ( IsVariableInReg( varmap, vals[ t + 1 ].strValue ) )
                        {
                            if ( 0 == vals[ t + 5 ].value )
                                fprintf( fp, "    sto       %s[ %s ], rzero\n", GenVariableName( vals[ variableToken ].strValue ),
                                                                                GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                            else
                            {
                                fprintf( fp, "    ldi       rtmp, %d\n", vals[ t + 5 ].value );
                                fprintf( fp, "    sto       %s[ %s ], rtmp\n", GenVariableName( vals[ variableToken ].strValue ),
                                                                               GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                            }
                        }
                        else
                        {
                            fprintf( fp, "    ld        rres, [%s]\n", GenVariableName( vals[ t + 1 ].strValue ) );
                            if ( 0 == vals[ t + 5 ].value )
                                fprintf( fp, "    sto       %s[ rres ], rzero\n", GenVariableName( vals[ variableToken ].strValue ) );
                            else
                            {
                                fprintf( fp, "    ldi       rtmp, %d\n", vals[ t + 5 ].value );
                                fprintf( fp, "    sto       %s[ rres ], rtmp\n", GenVariableName( vals[ variableToken ].strValue ) );
                            }
                        }

                        break;
                    }
                    else if ( ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget ) &&
                              8 == vals.size() &&
                              Token_VARIABLE == vals[ t + 1 ].token &&
                              IsVariableInReg( varmap, vals[ t + 1 ].strValue ) &&
                              Token_VARIABLE == vals[ t + 5 ].token &&
                              IsVariableInReg( varmap, vals[ t + 5 ].strValue ) )
                    {
                        // line 4230 has 8 tokens  ====>> 4230 sv%(st%) = v%
                        //   0 VARIABLE, value 0, strValue 'sv%'
                        //   1 OPENPAREN, value 0, strValue ''
                        //   2 EXPRESSION, value 2, strValue ''
                        //   3 VARIABLE, value 0, strValue 'st%'
                        //   4 CLOSEPAREN, value 0, strValue ''
                        //   5 EQ, value 0, strValue ''
                        //   6 EXPRESSION, value 2, strValue ''
                        //   7 VARIABLE, value 0, strValue 'v%'

                        string & vararray = vals[ variableToken ].strValue;

                        if ( IsVariableInReg( varmap, vararray ) )
                        {
                            fprintf( fp, "    add      x2, %s, %s, lsl #2\n", GenVariableReg64( varmap, vararray ), GenVariableReg64( varmap, vals[ t + 1 ].strValue ) );
                            fprintf( fp, "    str      %s, [x2]\n", GenVariableReg( varmap, vals[ t + 5 ].strValue ) );
                        }
                        else
                        {
                            LoadArm64Address( fp, "x2", varmap, vals[ variableToken ].strValue );
                            fprintf( fp, "    add      x2, x2, %s, lsl #2\n", GenVariableReg64( varmap, vals[ t + 1 ].strValue )  );
                            fprintf( fp, "    str      %s, [x2]\n", GenVariableReg( varmap, vals[ t + 5 ].strValue ) );
                        }

                        break;
                    }
                    else if ( mos6502Apple1 == g_AssemblyTarget &&
                              8 == vals.size() &&
                              Token_VARIABLE == vals[ t + 1 ].token &&
                              Token_VARIABLE == vals[ t + 5 ].token )
                    {
                        // line 4230 has 8 tokens  ====>> 4230 sv%(st%) = v%
                        //   0 VARIABLE, value 0, strValue 'sv%'
                        //   1 OPENPAREN, value 0, strValue ''
                        //   2 EXPRESSION, value 2, strValue ''
                        //   3 VARIABLE, value 0, strValue 'st%'
                        //   4 CLOSEPAREN, value 0, strValue ''
                        //   5 EQ, value 0, strValue ''
                        //   6 EXPRESSION, value 2, strValue ''
                        //   7 VARIABLE, value 0, strValue 'v%'

                        string & vararray = vals[ variableToken ].strValue;

                        fprintf( fp, "    lda      %s\n", GenVariableName( vals[ t + 1 ].strValue ) );
                        fprintf( fp, "    sta      arrayOffset\n" );
                        fprintf( fp, "    lda      %s+1\n", GenVariableName( vals[ t + 1 ].strValue ) );
                        fprintf( fp, "    sta      arrayOffset+1\n" );
                        fprintf( fp, "    asl      arrayOffset\n" );
                        fprintf( fp, "    rol      arrayOffset+1\n" );

                        fprintf( fp, "    lda      #%s\n", GenVariableName( vararray ) );
                        fprintf( fp, "    clc\n" );
                        fprintf( fp, "    adc      arrayOffset\n" );
                        fprintf( fp, "    sta      arrayOffset\n" );

                        fprintf( fp, "    lda      /%s\n", GenVariableName( vararray ) );
                        fprintf( fp, "    adc      arrayOffset+1\n" );
                        fprintf( fp, "    sta      arrayOffset+1\n" );

                        fprintf( fp, "    lda      %s\n", GenVariableName( vals[ t + 5 ].strValue ) );
                        fprintf( fp, "    ldy      #0\n" );
                        fprintf( fp, "    sta      (arrayOffset), y\n" );
                        fprintf( fp, "    lda      %s+1\n", GenVariableName( vals[ t + 5 ].strValue ) );
                        fprintf( fp, "    iny\n" );
                        fprintf( fp, "    sta      (arrayOffset), y\n" );

                        break;
                    }
                    else if ( i8086DOS == g_AssemblyTarget &&
                              8 == vals.size() &&
                              Token_VARIABLE == vals[ t + 1 ].token &&
                              Token_VARIABLE == vals[ t + 5 ].token )
                    {
                        // line 4230 has 8 tokens  ====>> 4230 sv%(st%) = v%
                        //   0 VARIABLE, value 0, strValue 'sv%'
                        //   1 OPENPAREN, value 0, strValue ''
                        //   2 EXPRESSION, value 2, strValue ''
                        //   3 VARIABLE, value 0, strValue 'st%'
                        //   4 CLOSEPAREN, value 0, strValue ''
                        //   5 EQ, value 0, strValue ''
                        //   6 EXPRESSION, value 2, strValue ''
                        //   7 VARIABLE, value 0, strValue 'v%'

                        string & vararray = vals[ variableToken ].strValue;

                        fprintf( fp, "    mov      bx, ds: [ %s ]\n", GenVariableName( vals[ t + 5 ].strValue ) );
                        fprintf( fp, "    mov      si, ds: [ %s ]\n", GenVariableName( vals[ t + 1 ].strValue ) );
                        fprintf( fp, "    shl      si, 1\n" );
                        fprintf( fp, "    mov      WORD PTR ds: [ offset %s + si ], bx\n", GenVariableName( vararray ) );

                        break;
                    }
                    else if ( oiOS == g_AssemblyTarget &&
                              8 == vals.size() &&
                              Token_VARIABLE == vals[ t + 1 ].token &&
                              Token_VARIABLE == vals[ t + 5 ].token )
                    {
                        // line 4230 has 8 tokens  ====>> 4230 sv%(st%) = v%
                        //   0 VARIABLE, value 0, strValue 'sv%'
                        //   1 OPENPAREN, value 0, strValue ''
                        //   2 EXPRESSION, value 2, strValue ''
                        //   3 VARIABLE, value 0, strValue 'st%'
                        //   4 CLOSEPAREN, value 0, strValue ''
                        //   5 EQ, value 0, strValue ''
                        //   6 EXPRESSION, value 2, strValue ''
                        //   7 VARIABLE, value 0, strValue 'v%'

                        string & vararray = vals[ variableToken ].strValue;
                        const char * presultreg = "rres";

                        if ( IsVariableInReg( varmap, vals[ t + 5 ].strValue ) )
                            presultreg = GenVariableReg( varmap, vals[ t + 5 ].strValue );
                        else
                            fprintf( fp, "    ld       rres, [ %s ]\n", GenVariableName( vals[ t + 5 ].strValue ) );

                        if ( IsVariableInReg( varmap, vals[ t + 1 ].strValue ) )
                            fprintf( fp, "    sto      %s[ %s ], %s\n", GenVariableName( vararray ), GenVariableReg( varmap, vals[ t + 1 ].strValue ), presultreg );
                        else
                        {
                            fprintf( fp, "    ld       rtmp, [ %s ]\n", GenVariableName( vals[ t + 1 ].strValue ) );
                            fprintf( fp, "    sto      %s[ rtmp ], %s\n", GenVariableName( vararray ), presultreg );
                        }

                        break;
                    }
                    else if ( x86Win == g_AssemblyTarget &&
                              8 == vals.size() &&
                              Token_VARIABLE == vals[ t + 1 ].token &&
                              IsVariableInReg( varmap, vals[ t + 1 ].strValue ) &&
                              Token_VARIABLE == vals[ t + 5 ].token )
                    {
                        // line 4230 has 8 tokens  ====>> 4230 sv%(st%) = v%
                        //   0 VARIABLE, value 0, strValue 'sv%'
                        //   1 OPENPAREN, value 0, strValue ''
                        //   2 EXPRESSION, value 2, strValue ''
                        //   3 VARIABLE, value 0, strValue 'st%'
                        //   4 CLOSEPAREN, value 0, strValue ''
                        //   5 EQ, value 0, strValue ''
                        //   6 EXPRESSION, value 2, strValue ''
                        //   7 VARIABLE, value 0, strValue 'v%'

                        string & vararray = vals[ variableToken ].strValue;
                        string & varrhs = vals[ t + 5 ].strValue;

                        fprintf( fp, "    mov      eax, %s\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                        fprintf( fp, "    shl      eax, 2\n" );

                        if ( IsVariableInReg( varmap, varrhs ) )
                        {
                            fprintf( fp, "    mov      DWORD PTR [ offset %s + eax ], %s\n", GenVariableName( vararray ),
                                                                                             GenVariableReg( varmap, varrhs ) );
                        }
                        else
                        {
                            fprintf( fp, "    mov      ebx, DWORD PTR [%s]\n", GenVariableName( varrhs ) );
                            fprintf( fp, "    mov      DWORD PTR [ offset %s + eax ], ebx\n", GenVariableName( vararray ) );
                        }
                        break;
                    }
                    else if ( m68kCPM == g_AssemblyTarget &&
                              8 == vals.size() &&
                              Token_VARIABLE == vals[ t + 1 ].token &&
                              IsVariableInReg( varmap, vals[ t + 1 ].strValue ) &&
                              Token_VARIABLE == vals[ t + 5 ].token )
                    {
                        // line 4230 has 8 tokens  ====>> 4230 sv%(st%) = v%
                        //   0 VARIABLE, value 0, strValue 'sv%'
                        //   1 OPENPAREN, value 0, strValue ''
                        //   2 EXPRESSION, value 2, strValue ''
                        //   3 VARIABLE, value 0, strValue 'st%'
                        //   4 CLOSEPAREN, value 0, strValue ''
                        //   5 EQ, value 0, strValue ''
                        //   6 EXPRESSION, value 2, strValue ''
                        //   7 VARIABLE, value 0, strValue 'v%'

                        string & vararray = vals[ variableToken ].strValue;
                        string & varrhs = vals[ t + 5 ].strValue;

                        fprintf( fp, "    move.l   %s, d0\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                        fprintf( fp, "    lsl.l    #2, d0\n" );
                        fprintf( fp, "    move.l   #%s, a0\n", GenVariableName( vararray ) );

                        if ( IsVariableInReg( varmap, varrhs ) )
                            fprintf( fp, "    move.l   %s, (a0,d0)\n", GenVariableReg( varmap, varrhs ) );
                        else
                            fprintf( fp, "    move.l   %s, (a0,d0)\n", GenVariableName( varrhs ) );
                        break;
                    }
                    else if ( oiOS == g_AssemblyTarget &&
                              14 == vals.size() &&
                              Token_VARIABLE == vals[ t + 1 ].token &&
                              IsVariableInReg( varmap, vals[ t + 1 ].strValue ) &&
                              Token_VARIABLE == vals[ t + 5 ].token &&
                              IsVariableInReg( varmap, vals[ t + 5 ].strValue ) &&
                              Token_VARIABLE == vals[ t + 8 ].token &&
                              IsVariableInReg( varmap, vals[ t + 8 ].strValue ) &&
                              Token_VARIABLE == vals[ t + 10 ].token &&
                              IsVariableInReg( varmap, vals[ t + 10 ].strValue ) &&
                              Token_OPENPAREN == vals[ t + 7 ].token &&
                              isTokenOperator( vals[ t + 6 ].token ) &&
                              isTokenOperator( vals[ t + 9 ].token ) )
                    {
                        // line 246 has 14 tokens  ====>> 246 a%( n% ) = x% - ( n% * qu% )
                        //    0 VARIABLE, value 0, strValue 'a%'
                        //    1 OPENPAREN, value 0, strValue ''
                        //    2 EXPRESSION, value 2, strValue ''
                        //    3 VARIABLE, value 0, strValue 'n%'
                        //    4 CLOSEPAREN, value 0, strValue ''
                        //    5 EQ, value 0, strValue ''
                        //    6 EXPRESSION, value 8, strValue ''
                        //    7 VARIABLE, value 0, strValue 'x%'
                        //    8 MINUS, value 0, strValue ''
                        //    9 OPENPAREN, value 0, strValue ''
                        //   10 VARIABLE, value 0, strValue 'n%'
                        //   11 MULT, value 0, strValue ''
                        //   12 VARIABLE, value 0, strValue 'qu%'
                        //   13 CLOSEPAREN, value 0, strValue ''

                        string & vararray = vals[ variableToken ].strValue;

                        fprintf( fp, "    math     rres, %s, %s, %s\n",
                                 GenVariableReg( varmap, vals[ t + 8 ].strValue ),
                                 GenVariableReg( varmap, vals[ t + 10 ].strValue ),
                                 OperatorInstructionX64[ vals[ t + 9 ].token ] );
                        fprintf( fp, "    math     rres, %s, rres, %s\n",
                                 GenVariableReg( varmap, vals[ t + 5 ].strValue ),
                                 OperatorInstructionX64[ vals[ t + 6 ].token ] );
                        fprintf( fp, "    sto      %s[%s], rres\n",
                                 GenVariableName( vararray ),
                                 GenVariableReg( varmap, vals[ t + 1 ].strValue ) );

                        t += vals[ t ].value;
                    }
                    else
                    {
label_no_array_eq_optimization:

                        GenerateOptimizedExpression( fp, varmap, t, vals );

                        if ( Token_COMMA == vals[ t ].token )
                        {
                            Variable * pvar = FindVariable( varmap, vals[ variableToken ].strValue );

                            if ( 2 != pvar->dimensions )
                                RuntimeFail( "using a variable as if it has 2 dimensions.", g_lineno );

                            t++; // comma

                            PushAccumulator( fp );
                            GenerateOptimizedExpression( fp, varmap, t, vals );

                            if ( x64Win == g_AssemblyTarget )
                            {
                                fprintf( fp, "    pop      rbx\n" );
                                fprintf( fp, "    imul     rbx, %d\n", pvar->dims[ 1 ] );
                                fprintf( fp, "    add      rax, rbx\n" );
                            }
                            else if ( arm32Linux == g_AssemblyTarget )
                            {
                                fprintf( fp, "    pop      {r1}\n" );
                                LoadArm32Constant( fp, "r2", pvar->dims[ 1 ] );
                                fprintf( fp, "    mul      r1, r1, r2\n" );
                                fprintf( fp, "    add      r0, r0, r1\n" );
                            }
                            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                            {
                                fprintf( fp, "    ldr      x1, [sp], #16\n" );
                                LoadArm64Constant( fp, "x2", pvar->dims[ 1 ] );
                                fprintf( fp, "    mul      w1, w1, w2\n" );
                                fprintf( fp, "    add      w0, w0, w1\n" );
                            }
                            else if ( i8080CPM == g_AssemblyTarget )
                            {
                                fprintf( fp, "    pop      d\n" );
                                fprintf( fp, "    push     h\n" );
                                fprintf( fp, "    lxi      h, %d\n", pvar->dims[ 1 ] );
                                fprintf( fp, "    call     imul\n" );
                                fprintf( fp, "    pop      d\n" );
                                fprintf( fp, "    dad      d\n" );
                            }
                            else if ( mos6502Apple1 == g_AssemblyTarget )
                            {
                                // stash away the most recent expression
            
                                fprintf( fp, "    lda      curOperand\n" );
                                fprintf( fp, "    sta      arrayOffset\n" );
                                fprintf( fp, "    lda      curOperand+1\n" );
                                fprintf( fp, "    sta      arrayOffset+1\n" );
            
                                // multiply the first dimension by the size of the [1] dimension
            
                                fprintf( fp, "    pla\n" );
                                fprintf( fp, "    sta      curOperand\n" );
                                fprintf( fp, "    pla\n" );
                                fprintf( fp, "    sta      curOperand+1\n" );
                                fprintf( fp, "    lda      #%d\n", pvar->dims[ 1 ] );
                                fprintf( fp, "    sta      otherOperand\n" );
                                fprintf( fp, "    lda      /%d\n", pvar->dims[ 1 ] );
                                fprintf( fp, "    sta      otherOperand+1\n" );
                                fprintf( fp, "    jsr      imul\n" );
            
                                // add the two amounts above
            
                                fprintf( fp, "    lda      curOperand\n" );
                                fprintf( fp, "    clc\n" );
                                fprintf( fp, "    adc      arrayOffset\n" );
                                fprintf( fp, "    sta      curOperand\n" );
                                fprintf( fp, "    lda      curOperand+1\n" );
                                fprintf( fp, "    adc      arrayOffset+1\n" );
                                fprintf( fp, "    sta      curOperand+1\n" );
                            }
                            else if ( i8086DOS == g_AssemblyTarget )
                            {
                                fprintf( fp, "    mov      cx, ax\n" );
                                fprintf( fp, "    pop      ax\n" );
                                fprintf( fp, "    mov      bx, %d\n", pvar->dims[ 1 ] );
                                fprintf( fp, "    imul     bx\n" );
                                fprintf( fp, "    add      ax, cx\n" );
                            }
                            else if ( x86Win == g_AssemblyTarget )
                            {
                                fprintf( fp, "    pop      ebx\n" );
                                fprintf( fp, "    imul     ebx, %d\n", pvar->dims[ 1 ] );
                                fprintf( fp, "    add      eax, ebx\n" );
                            }
                            else if ( riscv64 == g_AssemblyTarget )
                            {
                                RiscVPop( fp, "t1" );
                                fprintf( fp, "    li       t2, %d\n", pvar->dims[ 1 ] );
                                fprintf( fp, "    mul      t1, t1, t2\n" );
                                fprintf( fp, "    add      a0, a0, t1\n" );
                            }
                            else if ( m68kCPM == g_AssemblyTarget )
                            {
                                fprintf( fp, "    move.l   (a7)+, d1\n" );
                                fprintf( fp, "    move.l   #%d, d2\n", pvar->dims[ 1 ] );
                                fprintf( fp, "    muls     d2, d1\n" );
                                fprintf( fp, "    add      d1, d0\n" );
                            }
                            else if ( oiOS == g_AssemblyTarget )
                            {
                                fprintf( fp, "    ldi      rtmp, %u\n", pvar->dims[ 1 ] );
                                fprintf( fp, "    mathst   rtmp, rtmp, imul\n" );
                                fprintf( fp, "    add      rres, rtmp\n" );
                            }
                        }
        
                        t += 2; // ) =

                        string const & varname = vals[ variableToken ].strValue;
        
                        if ( x64Win == g_AssemblyTarget )
                        {
                            fprintf( fp, "    shl      rax, 2\n" );
                            fprintf( fp, "    lea      rbx, [%s]\n", GenVariableName( varname ) );
                        }
                        else if ( arm32Linux == g_AssemblyTarget )
                        {
                            LoadArm32Address( fp, "r1", varmap, varname );
                            fprintf( fp, "    add      r1, r1, r0, lsl #2\n" );
                        }
                        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                        {
                            LoadArm64Address( fp, "x1", varmap, varname );
                            fprintf( fp, "    add      x1, x1, x0, lsl #2\n" );
                        }
                        else if ( i8080CPM == g_AssemblyTarget )
                        {
                            fprintf( fp, "    dad      h\n" );
                            fprintf( fp, "    lxi      d, %s\n", GenVariableName( varname ) );
                            fprintf( fp, "    dad      d\n" );
                            fprintf( fp, "    xchg\n" );
                        }
                        else if ( mos6502Apple1 == g_AssemblyTarget )
                        {
                            fprintf( fp, "    asl      curOperand\n" );   // shift left into carry
                            fprintf( fp, "    rol      curOperand+1\n" ); // shift left from carry
                            fprintf( fp, "    lda      #%s\n", GenVariableName( varname ) );
                            fprintf( fp, "    clc\n" );
                            fprintf( fp, "    adc      curOperand\n" );
                            fprintf( fp, "    sta      arrayOffset\n" );
                            fprintf( fp, "    lda      /%s\n", GenVariableName( varname ) );
                            fprintf( fp, "    adc      curOperand+1\n" );
                            fprintf( fp, "    sta      arrayOffset+1\n" );
                        }
                        else if ( i8086DOS == g_AssemblyTarget )
                        {
                            fprintf( fp, "    shl      ax, 1\n" );
                            fprintf( fp, "    lea      si, %s\n", GenVariableName( varname ) );
                        }
                        else if ( x86Win == g_AssemblyTarget )
                        {
                            fprintf( fp, "    shl      eax, 2\n" );
                            fprintf( fp, "    lea      ebx, [%s]\n", GenVariableName( varname ) );
                        }
                        else if ( riscv64 == g_AssemblyTarget )
                        {
                            fprintf( fp, "    lla      t0, %s\n", GenVariableName( varname ) );
                            fprintf( fp, "    slli     a0, a0, 2\n" );
                            fprintf( fp, "    add      t0, t0, a0\n" );
                        }
                        else if ( m68kCPM == g_AssemblyTarget )
                        {
                            fprintf( fp, "    move.l   #%s, d1\n", GenVariableName( varname ) );
                            fprintf( fp, "    lsl.l    #2, d0\n" );
                            fprintf( fp, "    add.l    d1, d0\n" );
                        }
                        else if ( oiOS == g_AssemblyTarget )
                        {
                            fprintf( fp, "    shlimg   ; rres implied\n" );
                            fprintf( fp, "    ldi      rtmp, %s\n", GenVariableName( varname ) );
                            fprintf( fp, "    add      rres, rtmp\n" );
                        }

                        assert( Token_EXPRESSION == vals[ t ].token );
    
                        if ( Token_CONSTANT == vals[ t + 1 ].token && 2 == vals[ t ].value )
                        {
                            if ( x64Win == g_AssemblyTarget )
                                fprintf( fp, "    mov      DWORD PTR [rbx + rax], %d\n", vals[ t + 1 ].value );
                            else if ( arm32Linux == g_AssemblyTarget )
                            {
                                LoadArm32Constant( fp, "r0", vals[ t + 1 ].value );
                                fprintf( fp, "    str      r0, [r1]\n" );
                            }
                            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                            {
                                LoadArm64Constant( fp, "x0", vals[ t + 1 ].value );
                                fprintf( fp, "    str      w0, [x1]\n" );
                            }
                            else if ( i8080CPM == g_AssemblyTarget )
                            {
                                fprintf( fp, "    mvi      a, %d\n", ( vals[ t + 1 ].value ) & 0xff );
                                fprintf( fp, "    stax     d\n" );
                                fprintf( fp, "    inx      d\n" );
                                if ( 0 != vals[ t + 1 ].value )
                                    fprintf( fp, "    mvi      a, %d\n", ( ( vals[ t + 1 ].value ) >> 8 ) & 0xff );
                                fprintf( fp, "    stax     d\n" );
                            }
                            else if ( mos6502Apple1 == g_AssemblyTarget )
                            {
                                fprintf( fp, "    ldy      #0\n" );
                                fprintf( fp, "    lda      #%d\n", vals[ t + 1 ].value );
                                fprintf( fp, "    sta      (arrayOffset), y\n" );
                                fprintf( fp, "    iny\n" );
                                fprintf( fp, "    lda      /%d\n", vals[ t + 1 ].value );
                                fprintf( fp, "    sta      (arrayOffset), y\n" );
                            }
                            else if ( i8086DOS == g_AssemblyTarget )
                            {
                                fprintf( fp, "    mov      bx, ax\n" );
                                fprintf( fp, "    mov      WORD PTR [ si + bx ], %d\n", vals[ t + 1 ].value );
                            }
                            else if ( x86Win == g_AssemblyTarget )
                                fprintf( fp, "    mov      DWORD PTR [ebx + eax], %d\n", vals[ t + 1 ].value );
                            else if ( riscv64 == g_AssemblyTarget )
                            {
                                fprintf( fp, "    li       t1, %d\n", vals[ t + 1 ].value );
                                fprintf( fp, "    sw       t1, (t0)\n" );
                            }
                            else if ( m68kCPM == g_AssemblyTarget )
                            {
                                fprintf( fp, "    move.l   d0, a0\n" );
                                fprintf( fp, "    move.l   #%d, (a0)\n", vals[ t + 1 ].value );
                            }
                            else if ( oiOS == g_AssemblyTarget )
                            {
                                int val = vals[ t + 1 ].value;
                                if ( 0 == val )
                                    fprintf( fp, "    st       [ rres ], rzero\n" );
                                else
                                {
                                    fprintf( fp, "    ldi      rtmp, %d\n", val );
                                    fprintf( fp, "    st       [ rres ], rtmp\n" );
                                }
                            }

                            t += 2;
                        }
                        else if ( Token_VARIABLE == vals[ t + 1 ].token && 2 == vals[ t ].value &&
                                  IsVariableInReg( varmap, vals[ t + 1 ].strValue ) )
                        {
                            string & varone = vals[ t + 1 ].strValue;

                            if ( x64Win == g_AssemblyTarget )
                                fprintf( fp, "    mov      DWORD PTR [rbx + rax], %s\n", GenVariableReg( varmap, varone ) );
                            else if ( arm32Linux == g_AssemblyTarget )
                                fprintf( fp, "    str      %s, [r1]\n", GenVariableReg( varmap, varone ) );
                            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                                fprintf( fp, "    str      %s, [x1]\n", GenVariableReg( varmap, varone ) );
                            else if ( x86Win == g_AssemblyTarget )
                                fprintf( fp, "    mov      DWORD PTR [ebx + eax], %s\n", GenVariableReg( varmap, varone ) );
                            else if ( riscv64 == g_AssemblyTarget )
                                fprintf( fp, "    sw       %s, (t0)\n", GenVariableReg( varmap, varone ) );
                            else if ( m68kCPM == g_AssemblyTarget )
                            {
                                fprintf( fp, "    move.l   d0, a0\n" );
                                fprintf( fp, "    move.l   %s, (a0)\n", GenVariableReg( varmap, varone ) );
                            }
                            else if ( oiOS == g_AssemblyTarget )
                                fprintf( fp, "    st       [rres], %s\n", GenVariableReg( varmap, varone ) );

                            t += 2;
                        }
                        else
                        {
                            if ( x64Win == g_AssemblyTarget )
                            {
                                fprintf( fp, "    add      rbx, rax\n" );
                                fprintf( fp, "    push     rbx\n" );
                            }
                            else if ( arm32Linux == g_AssemblyTarget )
                                fprintf( fp, "    push     {r1}\n" );
                            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                                fprintf( fp, "    str      x1, [sp, #-16]!\n" );
                            else if ( i8080CPM == g_AssemblyTarget )
                                fprintf( fp, "    push     d\n" );
                            else if ( i8086DOS == g_AssemblyTarget )
                            {
                                fprintf( fp, "    add      si, ax\n" );
                                fprintf( fp, "    push     si\n" );
                            }
                            else if ( x86Win == g_AssemblyTarget )
                            {
                                fprintf( fp, "    add      ebx, eax\n" );
                                fprintf( fp, "    push     ebx\n" );
                            }
                            else if ( riscv64 == g_AssemblyTarget )
                                RiscVPush( fp, "t0" );
                            else if ( m68kCPM == g_AssemblyTarget )
                                fprintf( fp, "    move.l   d0, -(a7)\n" );
                            else if ( oiOS == g_AssemblyTarget )
                                fprintf( fp, "    push     rres\n" );
                            
                            GenerateOptimizedExpression( fp, varmap, t, vals );
                            
                            if ( x64Win == g_AssemblyTarget )
                            {
                                fprintf( fp, "    pop      rbx\n" );
                                fprintf( fp, "    mov      DWORD PTR [rbx], eax\n" );
                            }
                            else if ( arm32Linux == g_AssemblyTarget )
                            {
                                fprintf( fp, "    pop      {r1}\n" );
                                fprintf( fp, "    str      r0, [r1]\n" );
                            }
                            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                            {
                                fprintf( fp, "    ldr      x1, [sp], #16\n" );
                                fprintf( fp, "    str      w0, [x1]\n" );
                            }
                            else if ( i8080CPM == g_AssemblyTarget )
                            {
                                fprintf( fp, "    pop      d\n" );
                                fprintf( fp, "    mov      a, l\n" );
                                fprintf( fp, "    stax     d\n" );
                                fprintf( fp, "    inx      d\n" );
                                fprintf( fp, "    mov      a, h\n" );
                                fprintf( fp, "    stax     d\n" );
                            }
                            else if ( mos6502Apple1 == g_AssemblyTarget )
                            {
                                fprintf( fp, "    ldy      #0\n" );
                                fprintf( fp, "    lda      curOperand\n" );
                                fprintf( fp, "    sta      (arrayOffset), y\n" );
                                fprintf( fp, "    iny\n" );
                                fprintf( fp, "    lda      curOperand+1\n" );
                                fprintf( fp, "    sta      (arrayOffset), y\n" );
                            }
                            else if ( i8086DOS == g_AssemblyTarget )
                            {
                                fprintf( fp, "    pop      si\n" );
                                fprintf( fp, "    mov      WORD PTR [ si ], ax\n" );
                            }
                            else if ( x86Win == g_AssemblyTarget )
                            {
                                fprintf( fp, "    pop      ebx\n" );
                                fprintf( fp, "    mov      DWORD PTR [ebx], eax\n" );
                            }
                            else if ( riscv64 == g_AssemblyTarget )
                            {
                                RiscVPop( fp, "t0" );
                                fprintf( fp, "    sw       a0, (t0)\n" );
                            }
                            else if ( m68kCPM == g_AssemblyTarget )
                            {
                                fprintf( fp, "    move.l   (a7)+, a0\n" );
                                fprintf( fp, "    move.l   d0, (a0)\n" );
                            }
                            else if ( oiOS == g_AssemblyTarget )
                                fprintf( fp, "    stst     rres\n" );
                        }
                    }
                }

                if ( t == vals.size() )
                    break;
            }
            else if ( Token_END == token )
            {
                if ( x64Win == g_AssemblyTarget || mos6502Apple1 == g_AssemblyTarget || i8086DOS == g_AssemblyTarget ||
                     x86Win == g_AssemblyTarget || oiOS == g_AssemblyTarget )
                    fprintf( fp, "    jmp      end_execution\n" );
                else if ( arm64Mac == g_AssemblyTarget || arm32Linux == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                    fprintf( fp, "    b        end_execution\n" );
                else if ( i8080CPM == g_AssemblyTarget )
                    fprintf( fp, "    jmp      endExecution\n" );
                else if ( riscv64 == g_AssemblyTarget )
                    fprintf( fp, "    j        end_execution\n" );
                else if ( m68kCPM == g_AssemblyTarget )
                    fprintf( fp, "    bra      end_execution\n" );
                break;
            }
            else if ( Token_FOR == token )
            {
                string const & varname = vals[ t ].strValue;

                if ( 1 == vals[ t ].extra &&  // memfill optimization
                     oiOS == g_AssemblyTarget )
                {
                    int teval = t + 1;

                    fprintf( fp, "    push     rarg1\n" );
                    fprintf( fp, "    push     rarg2\n" );

                    if ( Token_VARIABLE == vals[ t + 4 ].token )
                    {
                        if ( IsVariableInReg( varmap, vals[ t + 4 ].strValue ) )
                            fprintf( fp, "    mov      rarg2, %s\n", GenVariableReg( varmap, vals[ t + 4 ].strValue ) ); // ending index
                        else
                            fprintf( fp, "    ld       rarg2, [%s]\n", GenVariableName( vals[ t + 4 ].strValue ) ); // ending index
                        fprintf( fp, "    inc      rarg2\n" );
                    }
                    else
                        fprintf( fp, "    ldi      rarg2, %u\n", 1 + vals[ t + 4 ].value ); // ending index

                    if ( 0 != vals[ t + 2 ].value ) // if the starting index isn't 0
                    {
                        fprintf( fp, "    ldi      rres, %d\n", vals[ t + 2 ].value ); // starting index
                        fprintf( fp, "    sub      rarg2, rres\n" );
                    }
                    else
                        fprintf( fp, "    zero     rres\n" );

                    LineOfCode & loc1 = g_linesOfCode[ l + 1 ];
                    vector<TokenValue> & vals1 = loc1.tokenValues;
                    fprintf( fp, "    ldi      rarg1, %s\n", GenVariableName( vals1[ 0 ].strValue ) );
                    fprintf( fp, "    ldi      rtmp, %d\n", vals1[ 7 ].value ); // fillValue
                    fprintf( fp, "    memf\n" );

                    fprintf( fp, "    pop      rarg2\n" );
                    fprintf( fp, "    pop      rarg1\n" );

                    l += 2; // point at next
                    break;
                }

                if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                        fprintf( fp, "    mov      %s, %d\n", GenVariableReg( varmap, varname ), vals[ t + 2 ].value );
                    else
                        fprintf( fp, "    mov      [%s], %d\n", GenVariableName( varname ), vals[ t + 2 ].value );
                }
                else if ( arm32Linux == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                        LoadArm32Constant( fp, GenVariableReg( varmap, varname ), vals[ t + 2 ].value );
                    else
                    {
                        LoadArm32Address( fp, "r0", varname );
                        LoadArm32Constant( fp, "r1", vals[ t + 2 ].value );
                        fprintf( fp, "    str      r1, [r0]\n" );
                    }
                }
                else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                        fprintf( fp, "    mov      %s, %d\n", GenVariableReg( varmap, varname ), vals[ t + 2 ].value );
                    else
                    {
                        LoadArm64Address( fp, "x0", varname );
                        LoadArm64Constant( fp, "w1", vals[ t + 2 ].value );
                        fprintf( fp, "    str      w1, [x0]\n" );
                    }
                }
                else if ( i8080CPM == g_AssemblyTarget )
                {
                    fprintf( fp, "    lxi      h, %d\n", vals[ t + 2 ].value );
                    fprintf( fp, "    shld     %s\n", GenVariableName( varname ) );
                }
                else if ( mos6502Apple1 == g_AssemblyTarget )
                {
                    fprintf( fp, "    lda      #%d\n", vals[ t + 2 ].value );
                    fprintf( fp, "    sta      %s\n", GenVariableName( varname ) );
                    fprintf( fp, "    lda      /%d\n", vals[ t + 2 ].value );
                    fprintf( fp, "    sta      %s+1\n", GenVariableName( varname ) );
                }
                else if ( i8086DOS == g_AssemblyTarget )
                {
                    fprintf( fp, "    mov      WORD PTR ds: [%s], %d\n", GenVariableName( varname ), vals[ t + 2 ].value );
                }
                else if ( riscv64 == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                        fprintf( fp, "    li       %s, %d\n", GenVariableReg( varmap, varname ), vals[ t + 2 ].value );
                    else
                    {
                        fprintf( fp, "    lla      t0, %s\n", GenVariableName( varname) );
                        fprintf( fp, "    li       t1, %d\n", vals[ t + 2 ].value );
                        fprintf( fp, "    sw       t1, (t0)\n" );
                    }
                }
                else if ( m68kCPM == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                        fprintf( fp, "    move.l   #%d, %s\n", vals[ t + 2 ].value, GenVariableReg( varmap, varname ) );
                    else
                    {
                        fprintf( fp, "    lea      %s, a0\n", GenVariableName( varname) );
                        fprintf( fp, "    move.l   #%d, (a0)\n", vals[ t + 2 ].value );
                    }
                }
                else if ( oiOS == g_AssemblyTarget )
                {
                    int val = vals[ t + 2 ].value;
                    if ( IsVariableInReg( varmap, varname ) )
                    {
                        if ( 0 == val )
                            fprintf( fp, "    zero     %s\n", GenVariableReg( varmap, varname ) );
                        else if ( val >= -32 && val <= 31 )
                            fprintf( fp, "    ldi      %s, %d\n", GenVariableReg( varmap, varname ), val );
                    }
                    else
                    {
                        if ( 0 == val )
                            fprintf( fp, "    st       [%s], rzero\n", GenVariableName( varname ) );
                        else if ( val >= -32 && val <= 31 )
                            fprintf( fp, "    sti      [%s], %d\n", GenVariableName( varname ), val );
                        else
                        {
                            fprintf( fp, "    ldi      rtmp, %d\n", val );
                            fprintf( fp, "    st       [%s], rtmp\n", GenVariableName( varname ) );
                        }
                    }
                }

                ForGosubItem item( true, (int) l );
                forGosubStack.push( item );
    
                if ( arm64Mac == g_AssemblyTarget )
                    fprintf( fp, ".p2align 2\n" );

                if ( i8080CPM == g_AssemblyTarget || m68kCPM == g_AssemblyTarget )
                    fprintf( fp, "  fl$%zd:\n", l ); // fl = for loop
                else if ( mos6502Apple1 == g_AssemblyTarget )
                    fprintf( fp, "for_loop_%zd:\n", l );
                else if ( arm64Win == g_AssemblyTarget )
                    fprintf( fp, "for_loop_%zd\n", l );
                else
                    fprintf( fp, "  for_loop_%zd:\n", l );

                int iStart = t + 3;
                GenerateOptimizedExpression( fp, varmap, iStart, vals );
                if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                        fprintf( fp, "    cmp      %s, eax\n", GenVariableReg( varmap, varname ) );
                    else
                        fprintf( fp, "    cmp      [%s], eax\n", GenVariableName( varname ) );
    
                    fprintf( fp, "    jg       after_for_loop_%zd\n", l );
                }
                else if ( arm32Linux == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                        fprintf( fp, "    cmp      %s, r0\n", GenVariableReg( varmap, varname ) );
                    else
                    {
                        LoadArm32Address( fp, "r1", varname );
                        fprintf( fp, "    ldr      r1, [r1]\n" );
                        fprintf( fp, "    cmp      r1, r0\n" );
                    }

                    fprintf( fp, "    bgt      after_for_loop_%zd\n", l );
                }
                else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                        fprintf( fp, "    cmp      %s, w0\n", GenVariableReg( varmap, varname ) );
                    else
                    {
                        LoadArm64Address( fp, "x1", varname );
                        fprintf( fp, "    ldr      w1, [x1]\n" );
                        fprintf( fp, "    cmp      w1, w0\n" );
                    }

                    fprintf( fp, "    b.gt       after_for_loop_%zd\n", l );
                }
                else if ( i8080CPM == g_AssemblyTarget )
                {
                    fprintf( fp, "    inx      h\n" );
                    fprintf( fp, "    xchg\n" );
                    fprintf( fp, "    lhld     %s\n", GenVariableName( varname ) );
                    Generate8080Relation( fp, Token_NE, "fc$", (int) l );
                    fprintf( fp, "    jmp      af$%zd\n", l ); // af == after for
                    fprintf( fp, "  fc$%zd:\n", l );  // fc == for code
                }
                else if ( mos6502Apple1 == g_AssemblyTarget )
                {
                    Generate6502Relation( fp, GenVariableName( varname ), "curOperand", Token_LE, "_for_continue_", (int) l  );
                    fprintf( fp, "    jmp      after_for_loop_%zd\n", l );
                    fprintf( fp, "_for_continue_%zd:\n", l );
                }
                else if ( i8086DOS == g_AssemblyTarget )
                {
                    fprintf( fp, "    cmp      WORD PTR ds: [%s], ax\n", GenVariableName( varname ) );
                    fprintf( fp, "    jle      _for_continue_%zd\n", l );
                    fprintf( fp, "    jmp      after_for_loop_%zd\n", l );
                    fprintf( fp, "_for_continue_%zd:\n", l );
                }
                else if ( riscv64 == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                        fprintf( fp, "    mv       t0, %s\n", GenVariableReg( varmap, varname ) );
                    else
                    {
                        fprintf( fp, "    lla      t0, %s\n", GenVariableName( varname) );
                        fprintf( fp, "    lw       t0, (t0)\n" );
                    }
                    fprintf( fp, "    bgt      t0, a0, after_for_loop_%zd\n", l );
                }
                else if ( m68kCPM == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                        fprintf( fp, "    move.l   %s, d1\n", GenVariableReg( varmap, varname ) );
                    else
                        fprintf( fp, "    move.l   %s, d1\n", GenVariableName( varname) );
                    fprintf( fp, "    cmp.l    d0, d1\n" );
                    fprintf( fp, "    bgt      af$%zd\n", l );
                }
                else if ( oiOS == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                        fprintf( fp, "    j        %s, rres, gt, after_for_loop_%zd\n", GenVariableReg( varmap, varname ), l );
                    else
                    {
                        fprintf( fp, "    ld       rtmp, [%s]\n", GenVariableName( varname ) );
                        fprintf( fp, "    j        rtmp, rres, gt, after_for_loop_%zd\n", l );
                    }
                }

                break;
            }
            else if ( Token_NEXT == token )
            {
                if ( 0 == forGosubStack.size() )
                    RuntimeFail( "next without for", l );
    
                ForGosubItem & item = forGosubStack.top();
                string const & loopVal = g_linesOfCode[ item.pcReturn ].tokenValues[ 0 ].strValue;

                if ( stcmp( loopVal, vals[ t ].strValue ) )
                    RuntimeFail( "NEXT statement variable doesn't match current FOR loop variable", l );

                if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, loopVal ) )
                        fprintf( fp, "    inc      %s\n", GenVariableReg( varmap, loopVal ) );
                    else
                        fprintf( fp, "    inc      DWORD PTR [%s]\n", GenVariableName( loopVal ) );

                    fprintf( fp, "    jmp      for_loop_%d\n", item.pcReturn );
                    fprintf( fp, "    align    16\n" );
                }
                else if ( arm32Linux == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, loopVal ) )
                    {
                        fprintf( fp, "    mov      r1, #1\n" );
                        fprintf( fp, "    add      %s, %s, r1\n", GenVariableReg( varmap, loopVal ),
                                                                  GenVariableReg( varmap, loopVal ) );
                    }
                    else
                    {
                        LoadArm32Address( fp, "r0", loopVal );
                        fprintf( fp, "    ldr      r1, [r0]\n" );
                        fprintf( fp, "    add      r1, r1, #1\n" );
                        fprintf( fp, "    str      r1, [r0]\n" );
                    }

                    fprintf( fp, "    b        for_loop_%d\n", item.pcReturn );
                    fprintf( fp, "    .p2align 2\n" );
                }
                else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, loopVal ) )
                        fprintf( fp, "    add      %s, %s, 1\n", GenVariableReg( varmap, loopVal ),
                                                                 GenVariableReg( varmap, loopVal ) );
                    else
                    {
                        LoadArm64Address( fp, "x0", loopVal );
                        fprintf( fp, "    ldr      w1, [x0]\n" );
                        fprintf( fp, "    add      x1, x1, 1\n" );
                        fprintf( fp, "    str      w1, [x0]\n" );
                    }

                    fprintf( fp, "    b        for_loop_%d\n", item.pcReturn );
                    if ( arm64Mac == g_AssemblyTarget )
                        fprintf( fp, "    .p2align 2\n" );
                }
                else if ( i8080CPM == g_AssemblyTarget )
                {
                    fprintf( fp, "    lhld     %s\n", GenVariableName( loopVal ) );
                    fprintf( fp, "    inx      h\n" );
                    fprintf( fp, "    shld     %s\n", GenVariableName( loopVal ) );
                    fprintf( fp, "    jmp      fl$%d\n", item.pcReturn ); // fl = for loop
                }
                else if ( mos6502Apple1 == g_AssemblyTarget )
                {
                    fprintf( fp, "    inc      %s\n", GenVariableName( loopVal ) );
                    fprintf( fp, "    bne      _next_no_hiinc_%zd\n", l );
                    fprintf( fp, "    inc      %s+1\n", GenVariableName( loopVal ) );
                    fprintf( fp, "_next_no_hiinc_%zd\n", l );
                    fprintf( fp, "    jmp      for_loop_%d\n", item.pcReturn );
                }
                else if ( i8086DOS == g_AssemblyTarget )
                {
                    fprintf( fp, "    inc      WORD PTR ds: [%s]\n", GenVariableName( loopVal ) );
                    fprintf( fp, "    jmp      for_loop_%d\n", item.pcReturn );
                }
                else if ( riscv64 == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, loopVal ) )
                        fprintf( fp, "    addi     %s, %s, 1\n", GenVariableReg( varmap, loopVal ),
                                                                 GenVariableReg( varmap, loopVal ) );
                    else
                    {
                        fprintf( fp, "    lla      t0, %s\n", GenVariableName( loopVal ) );
                        fprintf( fp, "    lw       t1, (t0)\n" );
                        fprintf( fp, "    addi     t1, t1, 1\n" );
                        fprintf( fp, "    sw       t1, (t0)\n" );
                    }
                    fprintf( fp, "    j        for_loop_%d\n", item.pcReturn );
                }
                else if ( m68kCPM == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, loopVal ) )
                        fprintf( fp, "    addi     #1, %s\n", GenVariableReg( varmap, loopVal ) );
                    else
                    {
                        fprintf( fp, "    lea      %s, a0\n", GenVariableName( loopVal ) );
                        fprintf( fp, "    addi.l   #1, (a0)\n" );
                    }
                    fprintf( fp, "    bra      fl$%d\n", item.pcReturn );
                }
                else if ( oiOS == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, loopVal ) )
                        fprintf( fp, "    inc      %s\n", GenVariableReg( varmap, loopVal ) );
                    else
                        fprintf( fp, "    inc      [%s]\n", GenVariableName( loopVal ) );
                    fprintf( fp, "    jmp      for_loop_%d\n", item.pcReturn );
                }

                if ( i8080CPM == g_AssemblyTarget || m68kCPM == g_AssemblyTarget )
                    fprintf( fp, "  af$%d:\n", item.pcReturn );
                else if ( mos6502Apple1 == g_AssemblyTarget )
                    fprintf( fp, "after_for_loop_%d:\n", item.pcReturn );
                else if ( arm64Win == g_AssemblyTarget )
                    fprintf( fp, "after_for_loop_%d\n", item.pcReturn );
                else
                    fprintf( fp, "  after_for_loop_%d:\n", item.pcReturn );

                forGosubStack.pop();
                break;
            }
            else if ( Token_GOSUB == token )
            {
                // not worth the runtime check for return without gosub?
                //fprintf( fp, "    inc      [gosubCount]\n" );

                if ( x64Win == g_AssemblyTarget )
                {
                    fprintf( fp, "    lea      rax, line_number_%d\n", vals[ t ].value );
                    fprintf( fp, "    call     label_gosub\n" );
                }
                else if ( x86Win == g_AssemblyTarget )
                {
                    fprintf( fp, "    call     line_number_%d\n", vals[ t ].value );
                }
                else if ( arm32Linux == g_AssemblyTarget )
                {
                    LoadArm32LineNumber( fp, "r0", vals[ t ].value );
                    fprintf( fp, "    bl       label_gosub\n" );
                }
                else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                {
                    fprintf( fp, "    adr      x0, line_number_%d\n", vals[ t ].value );
                    fprintf( fp, "    bl       label_gosub\n" );
                }
                else if ( i8080CPM == g_AssemblyTarget)
                    fprintf( fp, "    call     ln$%d\n", vals[ t ].value );
                else if ( mos6502Apple1 == g_AssemblyTarget)
                    fprintf( fp, "    jsr      line_number_%d\n", vals[ t ].value );
                else if ( i8086DOS == g_AssemblyTarget)
                    fprintf( fp, "    call     line_number_%d\n", vals[ t ].value );
                else if ( riscv64 == g_AssemblyTarget)
                {
                    fprintf( fp, "    lla      a0, line_number_%d\n", vals[ t ].value );
                    fprintf( fp, "    jal      label_gosub\n" );
                }
                else if ( m68kCPM == g_AssemblyTarget)
                {
                    fprintf( fp, "    lea      ln$%d, a0\n", vals[ t ].value );
                    fprintf( fp, "    jsr      (a0)\n" );
                }
                else if ( oiOS == g_AssemblyTarget)
                    fprintf( fp, "    callnf   line_number_%d\n", vals[ t ].value );

                break;
            }
            else if ( Token_GOTO == token )
            {
                if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                    fprintf( fp, "    jmp      line_number_%d\n", vals[ t ].value );
                else if (arm32Linux == g_AssemblyTarget )
                    fprintf( fp, "    b        line_number_%d\n", vals[ t ].value );
                else if (arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                    fprintf( fp, "    b        line_number_%d\n", vals[ t ].value );
                else if ( i8080CPM == g_AssemblyTarget )
                    fprintf( fp, "    jmp      ln$%d\n", vals[ t ].value );
                else if ( mos6502Apple1 == g_AssemblyTarget )
                    fprintf( fp, "    jmp      line_number_%d\n", vals[ t ].value );
                else if ( i8086DOS == g_AssemblyTarget)
                    fprintf( fp, "    jmp      line_number_%d\n", vals[ t ].value );
                else if ( riscv64 == g_AssemblyTarget)
                    fprintf( fp, "    j        line_number_%d\n", vals[ t ].value );
                else if ( m68kCPM == g_AssemblyTarget)
                    fprintf( fp, "    bra      ln$%d\n", vals[ t ].value );
                else if ( oiOS == g_AssemblyTarget)
                    fprintf( fp, "    jmp      line_number_%d\n", vals[ t ].value );

                break;
            }
            else if ( Token_RETURN == token )
            {
                if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                    fprintf( fp, "    jmp      label_gosub_return\n" );
                else if ( arm32Linux == g_AssemblyTarget )
                    fprintf( fp, "    b        label_gosub_return\n" );
                else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                    fprintf( fp, "    b        label_gosub_return\n" );
                else if ( i8080CPM == g_AssemblyTarget )
                    fprintf( fp, "    jmp      gosubReturn\n" );
                else if ( mos6502Apple1 == g_AssemblyTarget )
                    fprintf( fp, "    jmp      label_gosub_return\n" );
                else if ( i8086DOS == g_AssemblyTarget )
                    fprintf( fp, "    jmp      label_gosub_return\n" );
                else if ( riscv64 == g_AssemblyTarget)
                    fprintf( fp, "    j        label_gosub_return\n" );
                else if ( m68kCPM == g_AssemblyTarget)
                    fprintf( fp, "    bra      label_gosub_return\n" );
                else if ( oiOS == g_AssemblyTarget )
                    fprintf( fp, "    retnf\n" );

                break;
            }
            else if ( Token_PRINT == token )
            {
                bool withholdCRLF = ( 1 == vals[ t ].extra );
                t++;
    
                while ( t < vals.size() )
                {
                    if ( Token_SEMICOLON == vals[ t ].token )
                    {
                        t++;
                        continue;
                    }
                    else if ( Token_EXPRESSION != vals[ t ].token ) // likely ELSE
                        break;
    
                    assert( Token_EXPRESSION == vals[ t ].token );
    
                    if ( Token_STRING == vals[ t + 1 ].token )
                    {
                        if ( x64Win == g_AssemblyTarget )
                        {
                            fprintf( fp, "    lea      rcx, [strString]\n" );
                            fprintf( fp, "    lea      rdx, [str_%zd_%d]\n", l, t + 1 );
                            fprintf( fp, "    call     call_printf\n" );
                        }
                        else if ( arm32Linux == g_AssemblyTarget )
                        {
                            LoadArm32Label( fp, "r0", "strString" );
                            fprintf( fp, "    movw     r1, #:lower16:str_%zd_%d\n", l, t + 1 );
                            fprintf( fp, "    movt     r1, #:upper16:str_%zd_%d\n", l, t + 1 );
                            fprintf( fp, "    bl       call_printf\n" );
                        }
                        else if ( arm64Mac == g_AssemblyTarget )
                        {
                            LoadArm64Label( fp, "x0", "strString" );
                            fprintf( fp, "    adrp     x1, str_%zd_%d@PAGE\n", l, t + 1 );
                            fprintf( fp, "    add      x1, x1, str_%zd_%d@PAGEOFF\n", l, t + 1 );
                            fprintf( fp, "    bl       call_printf\n" );
                        }
                        else if ( arm64Win == g_AssemblyTarget )
                        {
                            LoadArm64Label( fp, "x0", "strString" );
                            fprintf( fp, "    adrp     x1, str_%zd_%d\n", l, t + 1 );
                            fprintf( fp, "    add      x1, x1, str_%zd_%d\n", l, t + 1 );
                            fprintf( fp, "    bl       call_printf\n" );
                        }
                        else if ( i8080CPM == g_AssemblyTarget )
                        {
                            fprintf( fp, "    lxi      h, s$%zd$%d\n", l, t + 1 );
                            fprintf( fp, "    call     DISPLAY\n" );
                        }
                        else if ( mos6502Apple1 == g_AssemblyTarget )
                        {
                            fprintf( fp, "    lda      #str_%zd_%d\n", l, t + 1 );
                            fprintf( fp, "    sta      printString\n" );
                            fprintf( fp, "    lda      /str_%zd_%d\n", l, t + 1 );
                            fprintf( fp, "    sta      printString+1\n" );
                            fprintf( fp, "    jsr      prstr\n" );
                        }
                        else if ( i8086DOS == g_AssemblyTarget )
                        {
                            fprintf( fp, "    mov      dx, offset str_%zd_%d\n", l, t + 1 );
                            fprintf( fp, "    call     printstring\n" );
                        }
                        else if ( x86Win == g_AssemblyTarget )
                        {
                            fprintf( fp, "    lea      eax, str_%zd_%d\n", l, t + 1 );
                            fprintf( fp, "    call     printString\n" );
                        }
                        else if ( riscv64 == g_AssemblyTarget )
                        {
                            fprintf( fp, "    lla      a0, str_%zd_%d\n", l, t + 1 );
                            fprintf( fp, "    jal      rvos_print_text\n" );
                        }
                        else if ( m68kCPM == g_AssemblyTarget )
                        {
                            fprintf( fp, "    lea      s$%zd$%d, a0\n", l, t + 1 );
                            fprintf( fp, "    move.l   a0, -(a7)\n" );
                            fprintf( fp, "    lea      strString, a0\n", l, t + 1 );
                            fprintf( fp, "    move.l   a0, -(a7)\n" );
                            fprintf( fp, "    jsr      _printf\n" );
                            fprintf( fp, "    add.l    #8, a7\n" );
                        }
                        else if ( oiOS == g_AssemblyTarget )
                        {
                            fprintf( fp, "    push     rarg1\n" );
                            fprintf( fp, "    ldi      rarg1, str_%zd_%d\n", l, t + 1 );
                            fprintf( fp, "    syscall  1\n" );
                            fprintf( fp, "    pop      rarg1\n" );
                        }

                        t += vals[ t ].value;
                    }
                    else if ( Token_TIME == vals[ t + 1 ].token )
                    {
                        // HH:MM:SS
                        // HH:MM:SS:mm (on DOS because why not?)

                        if ( x64Win == g_AssemblyTarget )
                        {
                            fprintf( fp, "    call     printTime\n" );
                        }
                        else if ( arm32Linux == g_AssemblyTarget )
                        {
                            // lots of arguments, so call _printf directly

                            fprintf( fp, "    save_volatile_registers\n" );
                            LoadArm32Label( fp, "r0", "rawTime" );
                            fprintf( fp, "    bl       time\n" );
                            LoadArm32Label( fp, "r0", "rawTime" );
                            fprintf( fp, "    bl       localtime\n" );
                            fprintf( fp, "    ldr      r3, [ r0 ]\n" );
                            fprintf( fp, "    ldr      r2, [ r0, #4 ]\n" );
                            fprintf( fp, "    ldr      r1, [ r0, #8 ]\n" );
                            LoadArm32Label( fp, "r0", "timeString" );
                            fprintf( fp, "    bl       printf\n" );
                            fprintf( fp, "    restore_volatile_registers\n" );
                        }
                        else if ( arm64Mac == g_AssemblyTarget )
                        {
                            // lots of arguments, so call _printf directly

                            fprintf( fp, "    save_volatile_registers\n" );
                            LoadArm64Label( fp, "x0", "rawTime" );
                            fprintf( fp, "    bl       _time\n" );
                            LoadArm64Label( fp, "x0", "rawTime" );
                            fprintf( fp, "    bl       _localtime\n" );
                            fprintf( fp, "    ldp      w9, w8, [ x0, #4 ]\n" );
                            fprintf( fp, "    ldr      w10, [x0]\n" );
                            fprintf( fp, "    stp      x9, x10, [ sp, #8 ]\n" );
                            fprintf( fp, "    str      x8, [sp]\n" );
                            LoadArm64Label( fp, "x0", "timeString" );
                            fprintf( fp, "    bl       _printf\n" );
                            fprintf( fp, "    restore_volatile_registers\n" );
                        }
                        else if ( arm64Win == g_AssemblyTarget )
                        {
                            fprintf( fp, "    bl       printTime\n" );
                        }
                        else if ( i8086DOS == g_AssemblyTarget )
                        {
                            fprintf( fp, "    call     printtime\n" );
                        }
                        else if ( x86Win == g_AssemblyTarget )
                        {
                            fprintf( fp, "    call     printCurrentTime\n" );
                        }
                        else if ( riscv64 == g_AssemblyTarget )
                            fprintf( fp, "    jal      print_time\n" );

                        t += vals[ t ].value;
                    }
                    else if ( Token_ELAP == vals[ t + 1 ].token )
                    {
                        // show elapsed time in milliseconds on platforms where it's possible

                        if ( x64Win == g_AssemblyTarget )
                        {
                            fprintf( fp, "    call     printElap\n" );
                        }
                        else if ( arm32Linux == g_AssemblyTarget )
                        {
                            fprintf( fp, "    save_volatile_registers\n" );
                            fprintf( fp, "    bl       clock\n" );
                            LoadArm32Label( fp, "r1", "startTicks" );
                            fprintf( fp, "    ldr      r1, [r1]\n" );
                            fprintf( fp, "    sub      r1, r0, r1\n" );
                            LoadArm32Label( fp, "r0", "elapString" );
                            fprintf( fp, "    bl       printf\n" );
                            fprintf( fp, "    restore_volatile_registers\n" );
                        }
                        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                        {
                            LoadArm64Label( fp, "x3", "startTicks" );
                            fprintf( fp, "    ldr      x0, [x3]\n" );
                            fprintf( fp, "    mrs      x1, cntvct_el0\n" ); //current time
                            fprintf( fp, "    sub      x1, x1, x0\n" ); // elapsed time
                            fprintf( fp, "    ldr      x4, =%#x\n", 1000000 ); // scale before divide
                            fprintf( fp, "    mul      x1, x1, x4\n" );

                            fprintf( fp, "    mrs      x2, cntfrq_el0\n" ); // frequency
                            fprintf( fp, "    udiv     x1, x1, x2\n" );
                            
                            LoadArm64Label( fp, "x0", "elapString" );
                            fprintf( fp, "    bl       call_printf\n" );
                        }
                        else if ( i8086DOS == g_AssemblyTarget )
                        {
                            fprintf( fp, "    call     printelap\n" );
                            fprintf( fp, "    mov      dx, offset elapString\n" );
                            fprintf( fp, "    call     printstring\n" );
                        }
                        else if ( x86Win == g_AssemblyTarget )
                            fprintf( fp, "    call     printElapTime\n" );
                        else if ( riscv64 == g_AssemblyTarget )
                            fprintf( fp, "    jal      print_elap\n" );

                        t += vals[ t ].value;
                    }
                    else if ( Token_CONSTANT == vals[ t + 1 ].token ||
                              Token_VARIABLE == vals[ t + 1 ].token )
                    {
                        assert( Token_EXPRESSION == vals[ t ].token );
                        GenerateOptimizedExpression( fp, varmap, t, vals );

                        if ( x64Win == g_AssemblyTarget )
                        {
                            fprintf( fp, "    lea      rcx, [intString]\n" );
                            fprintf( fp, "    mov      rdx, rax\n" );
                            fprintf( fp, "    call     call_printf\n" );
                        }
                        else if ( arm32Linux == g_AssemblyTarget )
                        {
                            fprintf( fp, "    mov      r1, r0\n" );
                            LoadArm32Label( fp, "r0", "intString" );
                            fprintf( fp, "    bl       call_printf\n" );
                        }
                        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                        {
                            fprintf( fp, "    mov      x1, x0\n" );
                            LoadArm64Label( fp, "x0", "intString" );
                            fprintf( fp, "    bl       call_printf\n" );
                        }
                        else if ( i8080CPM == g_AssemblyTarget )
                            fprintf( fp, "    call     puthl\n" );
                        else if ( mos6502Apple1 == g_AssemblyTarget )
                            fprintf( fp, "    jsr      print_int\n" );
                        else if ( i8086DOS == g_AssemblyTarget )
                            fprintf( fp, "    call     printint\n" );
                        else if ( x86Win == g_AssemblyTarget )
                            fprintf( fp, "    call     printInt\n" );
                        else if ( riscv64 == g_AssemblyTarget )
                            fprintf( fp, "    jal      print_int\n" );
                        else if ( m68kCPM == g_AssemblyTarget )
                            fprintf( fp, "    jsr      print_int\n" );
                        else if ( oiOS == g_AssemblyTarget )
                        {
                            fprintf( fp, "    push     rarg1\n" );
                            fprintf( fp, "    mov      rarg1, rres\n" );
                            fprintf( fp, "    syscall  2\n" );
                            fprintf( fp, "    pop      rarg1\n" );
                        }
                    }
                }

                if ( !withholdCRLF )
                {
                    if ( x64Win == g_AssemblyTarget )
                    {
                        fprintf( fp, "    lea      rcx, [newlineString]\n" );
                        fprintf( fp, "    call     call_printf\n" );
                    }
                    else if ( arm32Linux == g_AssemblyTarget )
                    {
                        LoadArm32Label( fp, "r0", "newlineString" );
                        fprintf( fp, "    bl       call_printf\n" );
                    }
                    else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                    {
                        LoadArm64Label( fp, "x0", "newlineString" );
                        fprintf( fp, "    bl       call_printf\n" );
                    }
                    else if ( i8080CPM == g_AssemblyTarget )
                    {
                        fprintf( fp, "    lxi      h, newlineString\n" );
                        fprintf( fp, "    call     DISPLAY\n" );
                    }
                    else if ( mos6502Apple1 == g_AssemblyTarget )
                        fprintf( fp, "    jsr      prcrlf\n" ); 
                    else if ( i8086DOS == g_AssemblyTarget )
                        fprintf( fp, "    call     printcrlf\n" );
                    else if ( x86Win == g_AssemblyTarget )
                        fprintf( fp, "    call     printcrlf\n" );
                    else if ( riscv64 == g_AssemblyTarget )
                        fprintf( fp, "    jal      print_crlf\n" );
                    else if ( m68kCPM == g_AssemblyTarget )
                        fprintf( fp, "    jsr      print_crlf\n" );
                    else if ( oiOS == g_AssemblyTarget )
                    {
                        fprintf( fp, "    push     rarg1\n" );
                        fprintf( fp, "    ldi      rarg1, newlineString\n" );
                        fprintf( fp, "    syscall  1\n" );
                        fprintf( fp, "    pop      rarg1\n" );
                    }
                }

                if ( t == vals.size() )
                    break;
            }
            else if ( Token_ATOMIC == token )
            {
                string & varname = vals[ t + 1 ].strValue;

                if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                    {
                        if ( Token_INC == vals[ t + 1 ].token )
                            fprintf( fp, "    inc      %s\n", GenVariableReg( varmap, varname ) );
                        else
                            fprintf( fp, "    dec      %s\n", GenVariableReg( varmap, varname ) );
                    }
                    else
                    {
                        if ( Token_INC == vals[ t + 1 ].token )
                            fprintf( fp, "    inc      DWORD PTR [%s]\n", GenVariableName( varname ) );
                        else
                            fprintf( fp, "    dec      DWORD PTR [%s]\n", GenVariableName( varname ) );
                    }
                }
                else if ( arm32Linux == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                    {
                        if ( Token_INC == vals[ t + 1 ].token )
                            fprintf( fp, "    add      %s, %s, #1\n", GenVariableReg( varmap, varname ),
                                                                      GenVariableReg( varmap, varname ) );
                        else
                            fprintf( fp, "    sub      %s, %s, #1\n", GenVariableReg( varmap, varname ),
                                                                      GenVariableReg( varmap, varname ) );
                    }
                    else
                    {
                        fprintf( fp, "    ldr      r0, =%s\n", GenVariableName( varname ) );
                        fprintf( fp, "    ldr      r1, [r0]\n" );

                        if ( Token_INC == vals[ t + 1 ].token )
                            fprintf( fp, "    add      r1, r1, #1\n" );
                        else
                            fprintf( fp, "    sub      r1, r1, #1\n" );

                        fprintf( fp, "    str      r1, [r0]\n" );
                    }
                }
                else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                    {
                        if ( Token_INC == vals[ t + 1 ].token )
                            fprintf( fp, "    add      %s, %s, 1\n", GenVariableReg( varmap, varname ),
                                                                     GenVariableReg( varmap, varname ) );
                        else
                            fprintf( fp, "    sub      %s, %s, 1\n", GenVariableReg( varmap, varname ),
                                                                     GenVariableReg( varmap, varname ) );
                    }
                    else
                    {
                        LoadArm64Address( fp, "x0", varname );
                        fprintf( fp, "    ldr      w1, [x0]\n" );
                        if ( Token_INC == vals[ t + 1 ].token )
                            fprintf( fp, "    add      x1, x1, 1\n" );
                        else
                            fprintf( fp, "    sub      x1, x1, 1\n" );
                        fprintf( fp, "    str      w1, [x0]\n" );

                    }
                }
                else if ( i8080CPM == g_AssemblyTarget )
                {
                    fprintf( fp, "    lhld     %s\n", GenVariableName( varname ) );

                    if ( Token_INC == vals[ t + 1 ].token )
                        fprintf( fp, "    inx      h\n" );
                    else
                        fprintf( fp, "    dcx      h\n" );

                    fprintf( fp, "    shld     %s\n", GenVariableName( varname ) );
                }
                else if ( mos6502Apple1 == g_AssemblyTarget )
                {
                    if ( Token_INC == vals[ t + 1 ].token )
                    {
                        fprintf( fp, "    inc      %s\n", GenVariableName( varname ) );
                        fprintf( fp, "    bne      _inc_no_high_%zd\n", l );
                        fprintf( fp, "    inc      %s+1\n", GenVariableName( varname ) );
                        fprintf( fp, "_inc_no_high_%zd\n", l );
                    }
                    else
                    {
                        fprintf( fp, "    lda      %s\n", GenVariableName( varname ) );
                        fprintf( fp, "    bne      _dec_no_high_%zd\n", l );
                        fprintf( fp, "    dec      %s+1\n", GenVariableName( varname ) );
                        fprintf( fp, "_dec_no_high_%zd\n", l );
                        fprintf( fp, "    dec      %s\n", GenVariableName( varname ) );
                    }
                }
                else if ( i8086DOS == g_AssemblyTarget )
                {
                    if ( Token_INC == vals[ t + 1 ].token )
                        fprintf( fp, "    inc      WORD PTR ds: [%s]\n", GenVariableName( varname ) );
                    else
                        fprintf( fp, "    dec      WORD PTR ds: [%s]\n", GenVariableName( varname ) );
                }
                else if ( riscv64 == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                    {
                        fprintf( fp, "    addi     %s, %s, %d\n", GenVariableReg( varmap, varname ),
                                                                  GenVariableReg( varmap, varname ),
                                                                  ( Token_INC == vals[ t + 1 ].token ) ? 1 : -1 );
                    }
                    else
                    {
                        fprintf( fp, "    lla      t0, %s\n", GenVariableName( varname ) );
                        fprintf( fp, "    lw       a0, (t0)\n" );
                        fprintf( fp, "    addi     a0, a0, %d\n", ( Token_INC == vals[ t + 1 ].token ) ? 1 : -1 );
                        fprintf( fp, "    sw       a0, (t0)\n" );
                    }
                }
                else if ( m68kCPM == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                        fprintf( fp, "    add.l     #%d, %s\n", ( Token_INC == vals[ t + 1 ].token ) ? 1 : -1, GenVariableReg( varmap, varname ) );
                    else
                        fprintf( fp, "    add.l    #%d, %s\n", ( Token_INC == vals[ t + 1 ].token ) ? 1 : -1, GenVariableName( varname ) );
                }
                else if ( oiOS == g_AssemblyTarget )
                {
                    if ( IsVariableInReg( varmap, varname ) )
                    {
                        if ( Token_INC == vals[ t + 1 ].token )
                            fprintf( fp, "    inc      %s\n", GenVariableReg( varmap, varname ) );
                        else
                            fprintf( fp, "    dec      %s\n", GenVariableReg( varmap, varname ) );
                    }
                    else
                    {
                        if ( Token_INC == vals[ t + 1 ].token )
                            fprintf( fp, "    inc      [%s]\n", GenVariableName( varname ) );
                        else
                            fprintf( fp, "    dec      [%s]\n", GenVariableName( varname ) );
                    }
                }

                break;
            }
            else if ( Token_IF == token )
            {
                activeIf = l;
    
                t++;
                assert( Token_EXPRESSION == vals[ t ].token );

                if ( !g_ExpressionOptimization )
                    goto label_no_if_optimization;

                // Optimize for really simple IF cases like "IF var/constant relational var/constant"

                if ( i8080CPM == g_AssemblyTarget &&
                     19 == vals.size() &&
                     16 == vals[ t ].value &&
                     Token_VARIABLE == vals[ t + 1 ].token &&
                     Token_EQ ==  vals[ t + 2 ].token &&
                     Token_OPENPAREN == vals[ t + 4 ].token &&
                     Token_CONSTANT == vals[ t + 6 ].token &&
                     Token_AND == vals[ t + 8 ].token &&
                     Token_VARIABLE == vals[ t + 9 ].token &&
                     Token_EQ == vals[ t + 10 ].token &&
                     Token_OPENPAREN == vals[ t + 12 ].token &&
                     Token_CONSTANT == vals[ t + 14 ].token &&
                     Token_THEN == vals[ t + 16 ].token &&
                     0 == vals[ t + 16 ].value &&
                     !stcmp( vals[ t + 3 ], vals[ t + 11 ] ) &&
                     Token_RETURN == vals[ t + 17 ].token )
                {
                    // line 2020 has 19 tokens  ====>> 2020 if wi% = b%( 1 ) and wi% = b%( 2 ) then return
                    //    0 IF, value 0, strValue ''
                    //    1 EXPRESSION, value 16, strValue ''
                    //    2 VARIABLE, value 0, strValue 'wi%'
                    //    3 EQ, value 0, strValue ''
                    //    4 VARIABLE, value 0, strValue 'b%'
                    //    5 OPENPAREN, value 0, strValue ''
                    //    6 EXPRESSION, value 2, strValue ''
                    //    7 CONSTANT, value 1, strValue ''
                    //    8 CLOSEPAREN, value 0, strValue ''
                    //    9 AND, value 0, strValue ''
                    //   10 VARIABLE, value 0, strValue 'wi%'
                    //   11 EQ, value 0, strValue ''
                    //   12 VARIABLE, value 0, strValue 'b%'
                    //   13 OPENPAREN, value 0, strValue ''
                    //   14 EXPRESSION, value 2, strValue ''
                    //   15 CONSTANT, value 2, strValue ''
                    //   16 CLOSEPAREN, value 0, strValue ''
                    //   17 THEN, value 0, strValue ''
                    //   18 RETURN, value 0, strValue ''

                    fprintf( fp, "    lhld     %s\n", GenVariableName( vals[ t + 1 ].strValue ) );
                    fprintf( fp, "    xchg\n" );
                    fprintf( fp, "    lxi      h, %s\n", GenVariableName( vals[ t + 3 ].strValue ) );
                    fprintf( fp, "    lxi      b, %d\n", 2 * vals[ t + 6 ].value );
                    fprintf( fp, "    dad      b\n" );
                    fprintf( fp, "    mov      a, m\n" );
                    fprintf( fp, "    cmp      e\n" );
                    fprintf( fp, "    jnz      ln$%zd\n", l + 1 );
                    fprintf( fp, "    inx      h\n" );
                    fprintf( fp, "    mov      a, m\n" );
                    fprintf( fp, "    cmp      d\n" );
                    fprintf( fp, "    jnz      ln$%zd\n", l + 1 );

                    fprintf( fp, "    lxi      h, %s\n", GenVariableName( vals[ t + 3 ].strValue ) );
                    fprintf( fp, "    lxi      b, %d\n", 2 * vals[ t + 14 ].value );
                    fprintf( fp, "    dad      b\n" );
                    fprintf( fp, "    mov      a, m\n" );
                    fprintf( fp, "    cmp      e\n" );
                    fprintf( fp, "    jnz      ln$%zd\n", l + 1 );
                    fprintf( fp, "    inx      h\n" );
                    fprintf( fp, "    mov      a, m\n" );
                    fprintf( fp, "    cmp      d\n" );
                    fprintf( fp, "    jz       gosubReturn\n" );

                    break;
                }
                else if ( mos6502Apple1 == g_AssemblyTarget &&
                          19 == vals.size() &&
                          16 == vals[ t ].value &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          Token_EQ ==  vals[ t + 2 ].token &&
                          Token_OPENPAREN == vals[ t + 4 ].token &&
                          Token_CONSTANT == vals[ t + 6 ].token &&
                          Token_AND == vals[ t + 8 ].token &&
                          Token_VARIABLE == vals[ t + 9 ].token &&
                          Token_EQ == vals[ t + 10 ].token &&
                          Token_OPENPAREN == vals[ t + 12 ].token &&
                          Token_CONSTANT == vals[ t + 14 ].token &&
                          Token_THEN == vals[ t + 16 ].token &&
                          0 == vals[ t + 16 ].value &&
                          !stcmp( vals[ t + 3 ], vals[ t + 11 ] ) &&
                          Token_RETURN == vals[ t + 17 ].token &&
                          vals[ t + 6 ].value < 64 &&
                          vals[ t + 14 ].value < 64 )
                {
                    // line 2020 has 19 tokens  ====>> 2020 if wi% = b%( 1 ) and wi% = b%( 2 ) then return
                    //    0 IF, value 0, strValue ''
                    //    1 EXPRESSION, value 16, strValue ''
                    //    2 VARIABLE, value 0, strValue 'wi%'
                    //    3 EQ, value 0, strValue ''
                    //    4 VARIABLE, value 0, strValue 'b%'
                    //    5 OPENPAREN, value 0, strValue ''
                    //    6 EXPRESSION, value 2, strValue ''
                    //    7 CONSTANT, value 1, strValue ''
                    //    8 CLOSEPAREN, value 0, strValue ''
                    //    9 AND, value 0, strValue ''
                    //   10 VARIABLE, value 0, strValue 'wi%'
                    //   11 EQ, value 0, strValue ''
                    //   12 VARIABLE, value 0, strValue 'b%'
                    //   13 OPENPAREN, value 0, strValue ''
                    //   14 EXPRESSION, value 2, strValue ''
                    //   15 CONSTANT, value 2, strValue ''
                    //   16 CLOSEPAREN, value 0, strValue ''
                    //   17 THEN, value 0, strValue ''
                    //   18 RETURN, value 0, strValue ''

                    fprintf( fp, "    lda      #%s\n", GenVariableName( vals[ t + 3 ].strValue ) );
                    fprintf( fp, "    sta      arrayOffset\n" );
                    fprintf( fp, "    lda      /%s\n", GenVariableName( vals[ t + 3 ].strValue ) );
                    fprintf( fp, "    sta      arrayOffset+1\n" );
                    fprintf( fp, "    ldy      #%d\n", 2 * vals[ t + 6 ].value );
                    fprintf( fp, "    lda      %s\n", GenVariableName( vals[ t + 1 ].strValue ) );
                    fprintf( fp, "    cmp      (arrayOffset),y\n" );
                    fprintf( fp, "    bne      line_number_%zd\n", l + 1 );
                    fprintf( fp, "    iny\n" );
                    fprintf( fp, "    lda      %s+1\n", GenVariableName( vals[ t + 1 ].strValue ) );
                    fprintf( fp, "    cmp      (arrayOffset),y\n" );
                    fprintf( fp, "    bne      line_number_%zd\n", l + 1 );
                    fprintf( fp, "    ldy      #%d\n", 2 * vals[ t + 14 ].value );
                    fprintf( fp, "    lda      %s\n", GenVariableName( vals[ t + 1 ].strValue ) );
                    fprintf( fp, "    cmp      (arrayOffset),y\n" );
                    fprintf( fp, "    bne      line_number_%zd\n", l + 1 );
                    fprintf( fp, "    iny\n" );
                    fprintf( fp, "    lda      %s+1\n", GenVariableName( vals[ t + 1 ].strValue ) );
                    fprintf( fp, "    cmp      (arrayOffset),y\n" );
                    fprintf( fp, "    bne      line_number_%zd\n", l + 1 );
                    fprintf( fp, "    jmp      label_gosub_return\n" );

                    break;
                }
                else if ( riscv64 == g_AssemblyTarget &&
                          19 == vals.size() &&
                          16 == vals[ t ].value &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          Token_EQ ==  vals[ t + 2 ].token &&
                          Token_OPENPAREN == vals[ t + 4 ].token &&
                          Token_CONSTANT == vals[ t + 6 ].token &&
                          Token_AND == vals[ t + 8 ].token &&
                          Token_VARIABLE == vals[ t + 9 ].token &&
                          Token_EQ == vals[ t + 10 ].token &&
                          Token_OPENPAREN == vals[ t + 12 ].token &&
                          Token_CONSTANT == vals[ t + 14 ].token &&
                          Token_THEN == vals[ t + 16 ].token &&
                          0 == vals[ t + 16 ].value &&
                          !stcmp( vals[ t + 3 ], vals[ t + 11 ] ) &&
                          Token_RETURN == vals[ t + 17 ].token &&
                          vals[ t + 6 ].value < 64 &&
                          vals[ t + 14 ].value < 64 &&
                          IsVariableInReg( varmap, vals[ t + 1 ].strValue ) )
                {
                    // line 2020 has 19 tokens  ====>> 2020 if wi% = b%( 1 ) and wi% = b%( 2 ) then return
                    //    0 IF, value 0, strValue ''
                    //    1 EXPRESSION, value 16, strValue ''
                    //    2 VARIABLE, value 0, strValue 'wi%'
                    //    3 EQ, value 0, strValue ''
                    //    4 VARIABLE, value 0, strValue 'b%'
                    //    5 OPENPAREN, value 0, strValue ''
                    //    6 EXPRESSION, value 2, strValue ''
                    //    7 CONSTANT, value 1, strValue ''
                    //    8 CLOSEPAREN, value 0, strValue ''
                    //    9 AND, value 0, strValue ''
                    //   10 VARIABLE, value 0, strValue 'wi%'
                    //   11 EQ, value 0, strValue ''
                    //   12 VARIABLE, value 0, strValue 'b%'
                    //   13 OPENPAREN, value 0, strValue ''
                    //   14 EXPRESSION, value 2, strValue ''
                    //   15 CONSTANT, value 2, strValue ''
                    //   16 CLOSEPAREN, value 0, strValue ''
                    //   17 THEN, value 0, strValue ''
                    //   18 RETURN, value 0, strValue ''

                    fprintf( fp, "    lla      t0, %s\n", GenVariableName( vals[ t + 3 ].strValue ) );
                    fprintf( fp, "    lw       t1, %d(t0)\n", 4 * vals[ t + 6 ].value );
                    fprintf( fp, "    lw       t0, %d(t0)\n", 4 * vals[ t + 14 ].value );
                    fprintf( fp, "    sub      t2, %s, t0\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                    fprintf( fp, "    sub      t3, %s, t1\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                    fprintf( fp, "    sltiu    a0, t2, 1\n" );
                    fprintf( fp, "    sltiu    a1, t3, 1\n" );
                    fprintf( fp, "    and      a0, a0, a1\n" );
                    fprintf( fp, "    bne      a0, zero, label_gosub_return\n" );

                    break;
                }
                else if ( m68kCPM == g_AssemblyTarget &&
                          19 == vals.size() &&
                          16 == vals[ t ].value &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          Token_EQ ==  vals[ t + 2 ].token &&
                          Token_OPENPAREN == vals[ t + 4 ].token &&
                          Token_CONSTANT == vals[ t + 6 ].token &&
                          Token_AND == vals[ t + 8 ].token &&
                          Token_VARIABLE == vals[ t + 9 ].token &&
                          Token_EQ == vals[ t + 10 ].token &&
                          Token_OPENPAREN == vals[ t + 12 ].token &&
                          Token_CONSTANT == vals[ t + 14 ].token &&
                          Token_THEN == vals[ t + 16 ].token &&
                          0 == vals[ t + 16 ].value &&
                          !stcmp( vals[ t + 3 ], vals[ t + 11 ] ) &&
                          Token_RETURN == vals[ t + 17 ].token &&
                          vals[ t + 6 ].value < 64 &&
                          vals[ t + 14 ].value < 64 &&
                          IsVariableInReg( varmap, vals[ t + 1 ].strValue ) )
                {
                    // line 2020 has 19 tokens  ====>> 2020 if wi% = b%( 1 ) and wi% = b%( 2 ) then return
                    //    0 IF, value 0, strValue ''
                    //    1 EXPRESSION, value 16, strValue ''
                    //    2 VARIABLE, value 0, strValue 'wi%'
                    //    3 EQ, value 0, strValue ''
                    //    4 VARIABLE, value 0, strValue 'b%'
                    //    5 OPENPAREN, value 0, strValue ''
                    //    6 EXPRESSION, value 2, strValue ''
                    //    7 CONSTANT, value 1, strValue ''
                    //    8 CLOSEPAREN, value 0, strValue ''
                    //    9 AND, value 0, strValue ''
                    //   10 VARIABLE, value 0, strValue 'wi%'
                    //   11 EQ, value 0, strValue ''
                    //   12 VARIABLE, value 0, strValue 'b%'
                    //   13 OPENPAREN, value 0, strValue ''
                    //   14 EXPRESSION, value 2, strValue ''
                    //   15 CONSTANT, value 2, strValue ''
                    //   16 CLOSEPAREN, value 0, strValue ''
                    //   17 THEN, value 0, strValue ''
                    //   18 RETURN, value 0, strValue ''

                    fprintf( fp, "    move.l   #%s, a0\n", GenVariableName( vals[ t + 3 ].strValue ) );
                    fprintf( fp, "    move.l   %d(a0), d1\n", 4 * vals[ t + 6 ].value );
                    fprintf( fp, "    move.l   %d(a0), d0\n", 4 * vals[ t + 14 ].value );
                    fprintf( fp, "    sub.l    %s, d1\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                    fprintf( fp, "    sub.l    %s, d0\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                    fprintf( fp, "    or.l     d0, d1\n" );
                    fprintf( fp, "    beq      label_gosub_return\n" );

                    break;
                }
                else if ( i8086DOS == g_AssemblyTarget &&
                     19 == vals.size() &&
                     16 == vals[ t ].value &&
                     Token_VARIABLE == vals[ t + 1 ].token &&
                     Token_EQ ==  vals[ t + 2 ].token &&
                     Token_OPENPAREN == vals[ t + 4 ].token &&
                     Token_CONSTANT == vals[ t + 6 ].token &&
                     Token_AND == vals[ t + 8 ].token &&
                     Token_VARIABLE == vals[ t + 9 ].token &&
                     Token_EQ == vals[ t + 10 ].token &&
                     Token_OPENPAREN == vals[ t + 12 ].token &&
                     Token_CONSTANT == vals[ t + 14 ].token &&
                     Token_THEN == vals[ t + 16 ].token &&
                     0 == vals[ t + 16 ].value &&
                     !stcmp( vals[ t + 3 ], vals[ t + 11 ] ) &&
                     Token_RETURN == vals[ t + 17 ].token )
                {
                    // line 2020 has 19 tokens  ====>> 2020 if wi% = b%( 1 ) and wi% = b%( 2 ) then return
                    //    0 IF, value 0, strValue ''
                    //    1 EXPRESSION, value 16, strValue ''
                    //    2 VARIABLE, value 0, strValue 'wi%'
                    //    3 EQ, value 0, strValue ''
                    //    4 VARIABLE, value 0, strValue 'b%'
                    //    5 OPENPAREN, value 0, strValue ''
                    //    6 EXPRESSION, value 2, strValue ''
                    //    7 CONSTANT, value 1, strValue ''
                    //    8 CLOSEPAREN, value 0, strValue ''
                    //    9 AND, value 0, strValue ''
                    //   10 VARIABLE, value 0, strValue 'wi%'
                    //   11 EQ, value 0, strValue ''
                    //   12 VARIABLE, value 0, strValue 'b%'
                    //   13 OPENPAREN, value 0, strValue ''
                    //   14 EXPRESSION, value 2, strValue ''
                    //   15 CONSTANT, value 2, strValue ''
                    //   16 CLOSEPAREN, value 0, strValue ''
                    //   17 THEN, value 0, strValue ''
                    //   18 RETURN, value 0, strValue ''

                    fprintf( fp, "    mov      ax, ds: [ %s ]\n", GenVariableName( vals[ t + 1 ].strValue ) );

                    fprintf( fp, "    cmp      ax, ds: [ %s + %d ]\n", GenVariableName( vals[ t + 3 ].strValue ), 2 * vals[ t + 6 ].value );
                    fprintf( fp, "    jne      line_number_%zd\n", l + 1 );

                    fprintf( fp, "    cmp      ax, ds: [ %s + %d ]\n", GenVariableName( vals[ t + 3 ].strValue ), 2 * vals[ t + 14 ].value );
                    fprintf( fp, "    jne      _uniq%d\n", s_uniqueLabel );
                    fprintf( fp, "    jmp      label_gosub_return\n" );
                    fprintf( fp, "  _uniq%d:\n", s_uniqueLabel );
                    s_uniqueLabel++;

                    break;
                }
                else if ( oiOS == g_AssemblyTarget &&
                     19 == vals.size() &&
                     16 == vals[ t ].value &&
                     Token_VARIABLE == vals[ t + 1 ].token &&
                     Token_EQ ==  vals[ t + 2 ].token &&
                     Token_OPENPAREN == vals[ t + 4 ].token &&
                     Token_CONSTANT == vals[ t + 6 ].token &&
                     Token_AND == vals[ t + 8 ].token &&
                     Token_VARIABLE == vals[ t + 9 ].token &&
                     Token_EQ == vals[ t + 10 ].token &&
                     Token_OPENPAREN == vals[ t + 12 ].token &&
                     Token_CONSTANT == vals[ t + 14 ].token &&
                     Token_THEN == vals[ t + 16 ].token &&
                     0 == vals[ t + 16 ].value &&
                     !stcmp( vals[ t + 3 ], vals[ t + 11 ] ) &&
                     Token_RETURN == vals[ t + 17 ].token )
                {
                    // line 2020 has 19 tokens  ====>> 2020 if wi% = b%( 1 ) and wi% = b%( 2 ) then return
                    //    0 IF, value 0, strValue ''
                    //    1 EXPRESSION, value 16, strValue ''
                    //    2 VARIABLE, value 0, strValue 'wi%'
                    //    3 EQ, value 0, strValue ''
                    //    4 VARIABLE, value 0, strValue 'b%'
                    //    5 OPENPAREN, value 0, strValue ''
                    //    6 EXPRESSION, value 2, strValue ''
                    //    7 CONSTANT, value 1, strValue ''
                    //    8 CLOSEPAREN, value 0, strValue ''
                    //    9 AND, value 0, strValue ''
                    //   10 VARIABLE, value 0, strValue 'wi%'
                    //   11 EQ, value 0, strValue ''
                    //   12 VARIABLE, value 0, strValue 'b%'
                    //   13 OPENPAREN, value 0, strValue ''
                    //   14 EXPRESSION, value 2, strValue ''
                    //   15 CONSTANT, value 2, strValue ''
                    //   16 CLOSEPAREN, value 0, strValue ''
                    //   17 THEN, value 0, strValue ''
                    //   18 RETURN, value 0, strValue ''

                    if ( !IsVariableInReg( varmap, vals[ t + 1 ].strValue ) )
                        fprintf( fp, "    ld       rtmp, [ %s ]\n", GenVariableName( vals[ t + 1 ].strValue ) );

                    fprintf( fp, "    ldi      rres, %d\n", vals[ t + 6 ].value );
                    fprintf( fp, "    ldae     %s[ rres ]\n", GenVariableName( vals[ t + 3 ].strValue ) );
                    fprintf( fp, "    j        %s, rres, ne, line_number_%zd\n",
                             IsVariableInReg( varmap, vals[ t + 1 ].strValue ) ? GenVariableReg( varmap, vals[ t + 1 ].strValue ) : "rtmp", l + 1 );

                    fprintf( fp, "    ldi      rres, %d\n", vals[ t + 14 ].value );
                    fprintf( fp, "    ldae     %s[ rres ]\n", GenVariableName( vals[ t + 3 ].strValue ) );
                    fprintf( fp, "    j        %s, rres, eq, retnf\n",
                             IsVariableInReg( varmap, vals[ t + 1 ].strValue ) ? GenVariableReg( varmap, vals[ t + 1 ].strValue ) : "rtmp" );
                    break;
                }
                else if ( i8080CPM != g_AssemblyTarget &&
                          mos6502Apple1 != g_AssemblyTarget &&
                          i8086DOS != g_AssemblyTarget &&
                          riscv64 != g_AssemblyTarget &&
                          oiOS != g_AssemblyTarget &&
                          ( !( ( x86Win == g_AssemblyTarget ) && !g_i386Target686 ) ) &&
                          10 == vals.size() &&
                          4 == vals[ t ].value &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          IsVariableInReg( varmap, vals[ t + 1 ].strValue ) &&
                          isOperatorRelational( vals[ t + 2 ].token ) &&
                          Token_VARIABLE == vals[ t + 3 ].token &&
                          IsVariableInReg( varmap, vals[ t + 3 ].strValue ) &&
                          Token_THEN == vals[ t + 4 ].token &&
                          0 == vals[ t + 4 ].value &&
                          Token_VARIABLE == vals[ t + 5 ].token &&
                          IsVariableInReg( varmap, vals[ t + 5 ].strValue ) &&
                          Token_EQ == vals[ t + 6 ].token &&
                          Token_EXPRESSION == vals[ t + 7 ].token &&
                          2 == vals[ t + 7 ].value &&
                          Token_VARIABLE == vals[ t + 8 ].token &&
                          IsVariableInReg( varmap, vals[ t + 8 ].strValue ) )
                {
                    // line 4342 has 10 tokens  ====>> 4342 if v% > al% then al% = v%
                    //   0 IF, value 0, strValue ''
                    //   1 EXPRESSION, value 4, strValue ''
                    //   2 VARIABLE, value 0, strValue 'v%'
                    //   3 GT, value 0, strValue ''
                    //   4 VARIABLE, value 0, strValue 'al%'
                    //   5 THEN, value 0, strValue ''
                    //   6 VARIABLE, value 0, strValue 'al%'
                    //   7 EQ, value 0, strValue ''
                    //   8 EXPRESSION, value 2, strValue ''
                    //   9 VARIABLE, value 0, strValue 'v%'

                    if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                    {
                        fprintf( fp, "    cmp      %s, %s\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ),
                                                              GenVariableReg( varmap, vals[ t + 3 ].strValue ) );

                        fprintf( fp, "    %-6s   %s, %s\n", CMovInstructionX64[ vals[ t + 2 ].token ],
                                 GenVariableReg( varmap, vals[ t + 5 ].strValue ),
                                 GenVariableReg( varmap, vals[ t + 8 ].strValue ) );
                    }
                    else if ( arm32Linux == g_AssemblyTarget )
                    {
                        fprintf( fp, "    cmp      %s, %s\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ),
                                                              GenVariableReg( varmap, vals[ t + 3 ].strValue ) );
                        fprintf( fp, "    mov%s     %s, %s\n", ConditionsArm[ vals[ t + 2 ].token ],
                                                               GenVariableReg( varmap, vals[ t + 5 ].strValue ),
                                                               GenVariableReg( varmap, vals[ t + 8 ].strValue ) );
                    }
                    else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                    {
                        fprintf( fp, "    cmp      %s, %s\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ),
                                                              GenVariableReg( varmap, vals[ t + 3 ].strValue ) );
                        fprintf( fp, "    csel     %s, %s, %s, %s\n", GenVariableReg( varmap, vals[ t + 5 ].strValue ),
                                                                      GenVariableReg( varmap, vals[ t + 8 ].strValue ),
                                                                      GenVariableReg( varmap, vals[ t + 5 ].strValue ),
                                                                      ConditionsArm[ vals[ t + 2 ].token ] );
                    }
                    else if ( m68kCPM == g_AssemblyTarget )
                    {
                        fprintf( fp, "    cmp.l    %s, %s\n", GenVariableReg( varmap, vals[ t + 3 ].strValue ),
                                                              GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                        fprintf( fp, "    b%s      ln$%zd\n", ConditionsNotm68kCPM[ vals[ t + 2 ].token ], l + 1 );
                        fprintf( fp, "    move.l   %s, %s\n",  GenVariableReg( varmap, vals[ t + 8 ].strValue ),
                                                               GenVariableReg( varmap, vals[ t + 5 ].strValue ) );
                    }
                    break;
                }
                else if ( mos6502Apple1 == g_AssemblyTarget &&
                          10 == vals.size() &&
                          4 == vals[ t ].value &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          isOperatorRelational( vals[ t + 2 ].token ) &&
                          Token_VARIABLE == vals[ t + 3 ].token &&
                          Token_THEN == vals[ t + 4 ].token &&
                          0 == vals[ t + 4 ].value &&
                          Token_VARIABLE == vals[ t + 5 ].token &&
                          Token_EQ == vals[ t + 6 ].token &&
                          Token_EXPRESSION == vals[ t + 7 ].token &&
                          2 == vals[ t + 7 ].value &&
                          Token_VARIABLE == vals[ t + 8 ].token )
                {
                    // line 4342 has 10 tokens  ====>> 4342 if v% > al% then al% = v%
                    //   0 IF, value 0, strValue ''
                    //   1 EXPRESSION, value 4, strValue ''
                    //   2 VARIABLE, value 0, strValue 'v%'
                    //   3 GT, value 0, strValue ''
                    //   4 VARIABLE, value 0, strValue 'al%'
                    //   5 THEN, value 0, strValue ''
                    //   6 VARIABLE, value 0, strValue 'al%'
                    //   7 EQ, value 0, strValue ''
                    //   8 EXPRESSION, value 2, strValue ''
                    //   9 VARIABLE, value 0, strValue 'v%'

                    Token op = vals[ t + 2 ].token;
                    string & lhs = vals[ t + 1 ].strValue;
                    string & rhs = vals[ t + 3 ].strValue;

                    char aclhs[ 100 ];
                    strcpy( aclhs, GenVariableName( lhs ) );

                    Generate6502Relation( fp, aclhs, GenVariableName( rhs ), op, "_if_true_", (int) l );
                    fprintf( fp, "    jmp      line_number_%zd\n", l + 1 );
                    fprintf( fp, "_if_true_%zd:\n", l );

                    fprintf( fp, "    lda      %s\n", GenVariableName( vals[ t + 8 ].strValue ) );
                    fprintf( fp, "    sta      %s\n", GenVariableName( vals[ t + 5 ].strValue ) );
                    fprintf( fp, "    lda      %s+1\n", GenVariableName( vals[ t + 8 ].strValue ) );
                    fprintf( fp, "    sta      %s+1\n", GenVariableName( vals[ t + 5 ].strValue ) );
                    break;
                }
                else if ( i8086DOS == g_AssemblyTarget &&
                          10 == vals.size() &&
                          4 == vals[ t ].value &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          isOperatorRelational( vals[ t + 2 ].token ) &&
                          Token_VARIABLE == vals[ t + 3 ].token &&
                          Token_THEN == vals[ t + 4 ].token &&
                          0 == vals[ t + 4 ].value &&
                          Token_VARIABLE == vals[ t + 5 ].token &&
                          Token_EQ == vals[ t + 6 ].token &&
                          Token_EXPRESSION == vals[ t + 7 ].token &&
                          2 == vals[ t + 7 ].value &&
                          Token_VARIABLE == vals[ t + 8 ].token )
                {
                    // line 4342 has 10 tokens  ====>> 4342 if v% > al% then al% = v%
                    //   0 IF, value 0, strValue ''
                    //   1 EXPRESSION, value 4, strValue ''
                    //   2 VARIABLE, value 0, strValue 'v%'
                    //   3 GT, value 0, strValue ''
                    //   4 VARIABLE, value 0, strValue 'al%'
                    //   5 THEN, value 0, strValue ''
                    //   6 VARIABLE, value 0, strValue 'al%'
                    //   7 EQ, value 0, strValue ''
                    //   8 EXPRESSION, value 2, strValue ''
                    //   9 VARIABLE, value 0, strValue 'v%'

                    Token op = vals[ t + 2 ].token;
                    string & lhs = vals[ t + 1 ].strValue;
                    string & rhs = vals[ t + 3 ].strValue;

                    fprintf( fp, "    mov      ax, ds: [ %s ]\n", GenVariableName( lhs ) );
                    fprintf( fp, "    cmp      ax, ds: [ %s ]\n", GenVariableName( rhs ) );
                    fprintf( fp, "    %-6s   line_number_%zd\n", RelationalNotInstructionX64[ op ], l + 1 ); // same as x64
                    if ( stcmp( lhs, vals[ t + 8 ].strValue ) )
                        fprintf( fp, "    mov      ax, ds: [ %s ]\n", GenVariableName( vals[ t + 8 ].strValue ) );
                    fprintf( fp, "    mov      WORD PTR ds: [ %s ], ax\n", GenVariableName( vals[ t + 5 ].strValue ) );
                    break;
                }
                else if ( oiOS == g_AssemblyTarget &&
                          10 == vals.size() &&
                          4 == vals[ t ].value &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          isOperatorRelational( vals[ t + 2 ].token ) &&
                          Token_VARIABLE == vals[ t + 3 ].token &&
                          Token_THEN == vals[ t + 4 ].token &&
                          0 == vals[ t + 4 ].value &&
                          Token_VARIABLE == vals[ t + 5 ].token &&
                          Token_EQ == vals[ t + 6 ].token &&
                          Token_EXPRESSION == vals[ t + 7 ].token &&
                          2 == vals[ t + 7 ].value &&
                          Token_VARIABLE == vals[ t + 8 ].token )
                {
                    // line 4342 has 10 tokens  ====>> 4342 if v% > al% then al% = v%
                    //   0 IF, value 0, strValue ''
                    //   1 EXPRESSION, value 4, strValue ''
                    //   2 VARIABLE, value 0, strValue 'v%'
                    //   3 GT, value 0, strValue ''
                    //   4 VARIABLE, value 0, strValue 'al%'
                    //   5 THEN, value 0, strValue ''
                    //   6 VARIABLE, value 0, strValue 'al%'
                    //   7 EQ, value 0, strValue ''
                    //   8 EXPRESSION, value 2, strValue ''
                    //   9 VARIABLE, value 0, strValue 'v%'

                    Token op = vals[ t + 2 ].token;
                    string & lhs = vals[ t + 1 ].strValue;
                    string & rhs = vals[ t + 3 ].strValue;

                    const char * pleft = "rtmp";
                    const char * pright = "rres";

                    if ( IsVariableInReg( varmap, lhs ) )
                        pleft = GenVariableReg( varmap, lhs );
                    else
                        fprintf( fp, "    ld       rtmp, [%s]\n", GenVariableName( lhs ) );

                    if ( IsVariableInReg( varmap, rhs ) )
                        pright = GenVariableReg( varmap, rhs );
                    else
                        fprintf( fp, "    ld       rres, [%s]\n", GenVariableName( rhs ) );

                    fprintf( fp, "    j        %s, %s, %s, line_number_%zd\n", pleft, pright, ConditionsNotArm[ op ], l + 1 );

                    const char * presult = "rtmp";
                    if ( IsVariableInReg( varmap, vals[ t + 8 ].strValue ) )
                        presult = GenVariableReg( varmap, vals[ t + 8 ].strValue );
                    else
                    {
                        if ( stcmp( lhs, vals[ t + 8 ].strValue ) )
                            fprintf( fp, "    ld       rtmp, [%s]\n", GenVariableName( vals[ t + 8 ].strValue ) );
                    }

                    if ( IsVariableInReg( varmap, vals[ t + 5 ].strValue ) )
                        fprintf( fp, "    mov      %s, %s\n", GenVariableReg( varmap, vals[ t + 5 ].strValue ), presult );
                    else
                        fprintf( fp, "    st       [%s], %s\n", GenVariableName( vals[ t + 5 ].strValue ), presult );
                    break;
                }
                else if ( i8080CPM != g_AssemblyTarget &&
                          mos6502Apple1 != g_AssemblyTarget &&
                          i8086DOS != g_AssemblyTarget &&
                          riscv64 != g_AssemblyTarget &&
                          m68kCPM != g_AssemblyTarget &&
                          oiOS != g_AssemblyTarget &&
                          19 == vals.size() &&
                          16 == vals[ t ].value &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          IsVariableInReg( varmap, vals[ t + 1 ].strValue ) &&
                          isOperatorRelational( vals[ t + 2 ].token ) &&
                          Token_OPENPAREN == vals[ t + 4 ].token &&
                          Token_CONSTANT == vals[ t + 6 ].token &&
                          Token_AND == vals[ t + 8 ].token &&
                          Token_VARIABLE == vals[ t + 9 ].token &&
                          IsVariableInReg( varmap, vals[ t + 9 ].strValue ) &&
                          isOperatorRelational( vals[ t + 10 ].token ) &&
                          Token_OPENPAREN == vals[ t + 12 ].token &&
                          Token_CONSTANT == vals[ t + 14 ].token &&
                          Token_THEN == vals[ t + 16 ].token &&
                          0 == vals[ t + 16 ].value &&
                          !stcmp( vals[ t + 3 ], vals[ t + 11 ] ) &&
                          Token_RETURN == vals[ t + 17 ].token )
                {
                    // line 2020 has 19 tokens  ====>> 2020 if wi% = b%( 1 ) and wi% = b%( 2 ) then return
                    //    0 IF, value 0, strValue ''
                    //    1 EXPRESSION, value 16, strValue ''
                    //    2 VARIABLE, value 0, strValue 'wi%'
                    //    3 EQ, value 0, strValue ''
                    //    4 VARIABLE, value 0, strValue 'b%'
                    //    5 OPENPAREN, value 0, strValue ''
                    //    6 EXPRESSION, value 2, strValue ''
                    //    7 CONSTANT, value 1, strValue ''
                    //    8 CLOSEPAREN, value 0, strValue ''
                    //    9 AND, value 0, strValue ''
                    //   10 VARIABLE, value 0, strValue 'wi%'
                    //   11 EQ, value 0, strValue ''
                    //   12 VARIABLE, value 0, strValue 'b%'
                    //   13 OPENPAREN, value 0, strValue ''
                    //   14 EXPRESSION, value 2, strValue ''
                    //   15 CONSTANT, value 2, strValue ''
                    //   16 CLOSEPAREN, value 0, strValue ''
                    //   17 THEN, value 0, strValue ''
                    //   18 RETURN, value 0, strValue ''

                    if ( x64Win == g_AssemblyTarget )
                    {
                        fprintf( fp, "    cmp      %s, DWORD PTR [ %s + %d ]\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ),
                                                                                 GenVariableName( vals[ t + 3 ].strValue ),
                                                                                 4 * vals[ t + 6 ].value );
                        fprintf( fp, "    %-6s   SHORT line_number_%zd\n", RelationalNotInstructionX64[ vals[ t + 2 ].token ], l + 1 );

                        fprintf( fp, "    cmp      %s, DWORD PTR [ %s + %d ]\n", GenVariableReg( varmap, vals[ t + 9 ].strValue ),
                                                                                 GenVariableName( vals[ t + 11 ].strValue ),
                                                                                 4 * vals[ t + 14 ].value );
                        fprintf( fp, "    %-6s   label_gosub_return\n", RelationalInstructionX64[ vals[ t + 10 ].token ] );
                    }
                    else if ( arm32Linux == g_AssemblyTarget )
                    {
                        int offsetA = 4 * vals[ t + 6 ].value;
                        int offsetB = 4 * vals[ t + 14 ].value;
                        string & vararray = vals[ t + 3 ].strValue;
                     
                        LoadArm32Address( fp, "r2", varmap, vals[ t + 3 ].strValue );
                        LoadArm32Constant( fp, "r1", offsetA );
                        fprintf( fp, "    add      r1, r1, r2\n" );
                        fprintf( fp, "    ldr      r0, [r1]\n" );

                        fprintf( fp, "    cmp      %s, r0\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                        fprintf( fp, "    b%s      line_number_%zd\n", ConditionsNotArm[ vals[ t + 2 ].token ], l + 1 );

                        LoadArm32Constant( fp, "r1", offsetB );
                        fprintf( fp, "    add      r1, r1, r2\n" );
                        fprintf( fp, "    ldr      r0, [r1]\n" );

                        fprintf( fp, "    cmp      %s, r0\n", GenVariableReg( varmap, vals[ t + 9 ].strValue ) );
                        fprintf( fp, "    b%s      label_gosub_return\n", ConditionsArm[ vals[ t + 10 ].token ] );
                    }
                    else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                    {
                        int offsetA = 4 * vals[ t + 6 ].value;
                        int offsetB = 4 * vals[ t + 14 ].value;
                        string & vararray = vals[ t + 3 ].strValue;
                     
                        if ( IsVariableInReg( varmap, vararray ) && fitsIn8Bits( offsetA ) && fitsIn8Bits( offsetB ) )
                        {
                            fprintf( fp, "    ldr      w0, [%s, %d]\n", GenVariableReg64( varmap, vararray ), offsetA );
                            fprintf( fp, "    cmp      %s, w0\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                            fprintf( fp, "    b.%s     line_number_%zd\n", ConditionsNotArm[ vals[ t + 2 ].token ], l + 1 );
                            fprintf( fp, "    ldr      w0, [%s, %d]\n", GenVariableReg64( varmap, vararray ), offsetB );
                        }
                        else
                        {
                            LoadArm64Address( fp, "x2", varmap, vals[ t + 3 ].strValue );

                            if ( fitsIn8Bits( offsetA ) )
                            {
                                fprintf( fp, "    ldr      w0, [x2, %d]\n", offsetA );
                            }
                            else
                            {
                                if ( fitsIn12Bits( offsetA ) )
                                    fprintf( fp, "    add      x1, x2, %d\n", offsetA );
                                else
                                {
                                    LoadArm64Constant( fp, "x1", offsetA );
                                    fprintf( fp, "    add      x1, x1, x2\n" );
                                }

                                fprintf( fp, "    ldr      w0, [x1]\n" );
                            }

                            fprintf( fp, "    cmp      %s, w0\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                            fprintf( fp, "    b.%s     line_number_%zd\n", ConditionsNotArm[ vals[ t + 2 ].token ], l + 1 );

                            if ( fitsIn8Bits( offsetB ) )
                            {
                                fprintf( fp, "    ldr      w0, [x2, %d]\n", offsetB );
                            }
                            else
                            {
                                if ( fitsIn12Bits( offsetB ) )
                                    fprintf( fp, "    add      x1, x2, %d\n", offsetB );
                                else
                                {
                                    LoadArm64Constant( fp, "x1", offsetB );
                                    fprintf( fp, "    add      x1, x1, x2\n" );
                                }

                                fprintf( fp, "    ldr      w0, [x1]\n" );
                            }
                        }

                        fprintf( fp, "    cmp      %s, w0\n", GenVariableReg( varmap, vals[ t + 9 ].strValue ) );
                        fprintf( fp, "    b.%s     label_gosub_return\n", ConditionsArm[ vals[ t + 10 ].token ] );
                    }
                    else if ( x86Win == g_AssemblyTarget )
                    {
                        fprintf( fp, "    cmp      %s, DWORD PTR [ %s + %d ]\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ),
                                                                                 GenVariableName( vals[ t + 3 ].strValue ),
                                                                                 4 * vals[ t + 6 ].value );
                        fprintf( fp, "    %-6s   SHORT line_number_%zd\n", RelationalNotInstructionX64[ vals[ t + 2 ].token ], l + 1 );

                        fprintf( fp, "    cmp      %s, DWORD PTR [ %s + %d ]\n", GenVariableReg( varmap, vals[ t + 9 ].strValue ),
                                                                                 GenVariableName( vals[ t + 11 ].strValue ),
                                                                                 4 * vals[ t + 14 ].value );
                        fprintf( fp, "    %-6s   label_gosub_return\n", RelationalInstructionX64[ vals[ t + 10 ].token ] );
                    }

                    break;
                }
                else if ( i8080CPM != g_AssemblyTarget &&
                          mos6502Apple1 != g_AssemblyTarget &&
                          i8086DOS != g_AssemblyTarget &&
                          arm32Linux != g_AssemblyTarget &&
                          riscv64 != g_AssemblyTarget &&
                          oiOS != g_AssemblyTarget &&
                          15 == vals.size() &&
                          4 == vals[ t ].value &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          IsVariableInReg( varmap, vals[ t + 1 ].strValue ) &&
                          Token_AND == vals[ t + 2 ].token  &&
                          Token_CONSTANT == vals[ t + 3 ].token &&
                          1 == vals[ t + 3 ].value &&
                          Token_THEN == vals[ t + 4 ].token &&
                          Token_VARIABLE == vals[ t + 5 ].token &&
                          IsVariableInReg( varmap, vals[ t + 5 ].strValue ) &&
                          Token_EQ == vals[ t + 6 ].token &&
                          2 == vals[ t + 7 ].value &&
                          Token_CONSTANT == vals[ t + 8 ].token &&
                          Token_ELSE == vals[ t + 9 ].token &&
                          Token_VARIABLE == vals[ t + 10 ].token &&
                          IsVariableInReg( varmap, vals[ t + 10 ].strValue ) &&
                          Token_EQ == vals[ t + 11 ].token &&
                          2 == vals[ t + 12 ].value &&
                          !stcmp( vals[ t + 5 ], vals[ t + 10 ] ) &&
                          Token_CONSTANT == vals[ t + 13 ].token )
                {
                    // line 4150 has 15 tokens  ====>> 4150 if st% and 1 then v% = 2 else v% = 9
                    // token   0 IF, value 0, strValue ''
                    // token   1 EXPRESSION, value 4, strValue ''
                    // token   2 VARIABLE, value 0, strValue 'st%'
                    // token   3 AND, value 0, strValue ''
                    // token   4 CONSTANT, value 1, strValue ''
                    // token   5 THEN, value 5, strValue ''
                    // token   6 VARIABLE, value 0, strValue 'v%'
                    // token   7 EQ, value 0, strValue ''
                    // token   8 EXPRESSION, value 2, strValue ''
                    // token   9 CONSTANT, value 2, strValue ''
                    // token  10 ELSE, value 0, strValue ''
                    // token  11 VARIABLE, value 0, strValue 'v%'
                    // token  12 EQ, value 0, strValue ''
                    // token  13 EXPRESSION, value 2, strValue ''
                    // token  14 CONSTANT, value 9, strValue ''

                    if ( x64Win == g_AssemblyTarget )
                    {
                        fprintf( fp, "    mov      %s, %d\n", GenVariableReg( varmap, vals[ t + 5 ].strValue ) , vals[ t + 13 ].value );
                        fprintf( fp, "    mov      eax, %d\n", vals[ t + 8 ].value );
                        fprintf( fp, "    test     %s, 1\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                        fprintf( fp, "    cmovnz   %s, eax\n", GenVariableReg( varmap, vals[ t + 5 ].strValue ) );
                    }
                    else if ( x86Win == g_AssemblyTarget )
                    {
                        fprintf( fp, "    mov      %s, %d\n", GenVariableReg( varmap, vals[ t + 5 ].strValue ) , vals[ t + 13 ].value );
                        fprintf( fp, "    test     %s, 1\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ) );

                        if ( g_i386Target686 )
                        {
                            fprintf( fp, "    mov      eax, %d\n", vals[ t + 8 ].value );
                            fprintf( fp, "    cmovnz   %s, eax\n", GenVariableReg( varmap, vals[ t + 5 ].strValue ) );
                        }
                        else
                        {
                            fprintf( fp, "    jz       line_number_%zd\n", l + 1 );
                            fprintf( fp, "    mov      %s, %d\n", GenVariableReg( varmap, vals[ t + 5 ].strValue ), vals[ t + 8 ].value );
                        }
                    }
                    else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                    {
                        LoadArm64Constant( fp, "x0", vals[ t + 8 ].value );
                        LoadArm64Constant( fp, "x1", vals[ t + 13 ].value );
                        fprintf( fp, "    tst      %s, 1\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                        fprintf( fp, "    csel     %s, w0, w1, ne\n", GenVariableReg( varmap, vals[ t + 5 ].strValue ) );
                    }
                    else if ( m68kCPM == g_AssemblyTarget )
                    {
                        fprintf( fp, "    move.l   #%d, d1\n", vals[ t + 8 ].value );
                        fprintf( fp, "    move.l   %s, d0\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                        fprintf( fp, "    andi.l   #1, d0\n" );
                        fprintf( fp, "    bne      un$%d\n", s_uniqueLabel );
                        fprintf( fp, "    move.l   #%d, d1\n", vals[ t + 13 ].value );
                        fprintf( fp, "un$%d:\n", s_uniqueLabel );
                        fprintf( fp, "    move.l   d1, %s\n", GenVariableReg( varmap, vals[ t + 5 ].strValue ) );
                        s_uniqueLabel++;
                    }

                    break;
                }
                else if ( i8080CPM == g_AssemblyTarget &&
                          15 == vals.size() &&
                          4 == vals[ t ].value &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          Token_AND == vals[ t + 2 ].token  &&
                          Token_CONSTANT == vals[ t + 3 ].token &&
                          Token_THEN == vals[ t + 4 ].token &&
                          Token_VARIABLE == vals[ t + 5 ].token &&
                          Token_EQ == vals[ t + 6 ].token &&
                          2 == vals[ t + 7 ].value &&
                          Token_CONSTANT == vals[ t + 8 ].token &&
                          Token_ELSE == vals[ t + 9 ].token &&
                          Token_VARIABLE == vals[ t + 10 ].token &&
                          Token_EQ == vals[ t + 11 ].token &&
                          2 == vals[ t + 12 ].value &&
                          !stcmp( vals[ t + 5 ], vals[ t + 10 ] ) &&
                          Token_CONSTANT == vals[ t + 13 ].token )
                {
                    // line 4150 has 15 tokens  ====>> 4150 if st% and 1 then v% = 2 else v% = 9
                    // token   0 IF, value 0, strValue ''
                    // token   1 EXPRESSION, value 4, strValue ''
                    // token   2 VARIABLE, value 0, strValue 'st%'
                    // token   3 AND, value 0, strValue ''
                    // token   4 CONSTANT, value 1, strValue ''
                    // token   5 THEN, value 5, strValue ''
                    // token   6 VARIABLE, value 0, strValue 'v%'
                    // token   7 EQ, value 0, strValue ''
                    // token   8 EXPRESSION, value 2, strValue ''
                    // token   9 CONSTANT, value 2, strValue ''
                    // token  10 ELSE, value 0, strValue ''
                    // token  11 VARIABLE, value 0, strValue 'v%'
                    // token  12 EQ, value 0, strValue ''
                    // token  13 EXPRESSION, value 2, strValue ''
                    // token  14 CONSTANT, value 9, strValue ''

                    fprintf( fp, "    lda      %s\n", GenVariableName( vals[ t + 1 ].strValue ) );
                    fprintf( fp, "    ani      %d\n", vals[ t + 3 ].value );
                    fprintf( fp, "    jz       uniq%d\n", s_uniqueLabel );
                    fprintf( fp, "    lxi      h, %d\n", vals[ t + 8 ].value );
                    fprintf( fp, "    jmp      uniq%d\n", s_uniqueLabel + 1 );

                    fprintf( fp, "  uniq%d:\n", s_uniqueLabel );
                    fprintf( fp, "    lxi      h, %d\n", vals[ t + 13 ].value );
                    s_uniqueLabel++;
                    fprintf( fp, "  uniq%d:\n", s_uniqueLabel );
                    fprintf( fp, "    shld     %s\n", GenVariableName( vals[ t + 10 ].strValue ) );

                    s_uniqueLabel++;

                    break;
                }
                else if ( mos6502Apple1 == g_AssemblyTarget &&
                          15 == vals.size() &&
                          4 == vals[ t ].value &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          Token_AND == vals[ t + 2 ].token  &&
                          Token_CONSTANT == vals[ t + 3 ].token &&
                          Token_THEN == vals[ t + 4 ].token &&
                          Token_VARIABLE == vals[ t + 5 ].token &&
                          Token_EQ == vals[ t + 6 ].token &&
                          2 == vals[ t + 7 ].value &&
                          Token_CONSTANT == vals[ t + 8 ].token &&
                          Token_ELSE == vals[ t + 9 ].token &&
                          Token_VARIABLE == vals[ t + 10 ].token &&
                          Token_EQ == vals[ t + 11 ].token &&
                          2 == vals[ t + 12 ].value &&
                          !stcmp( vals[ t + 5 ], vals[ t + 10 ] ) &&
                          Token_CONSTANT == vals[ t + 13 ].token )
                {
                    // line 4150 has 15 tokens  ====>> 4150 if st% and 1 then v% = 2 else v% = 9
                    // token   0 IF, value 0, strValue ''
                    // token   1 EXPRESSION, value 4, strValue ''
                    // token   2 VARIABLE, value 0, strValue 'st%'
                    // token   3 AND, value 0, strValue ''
                    // token   4 CONSTANT, value 1, strValue ''
                    // token   5 THEN, value 5, strValue ''
                    // token   6 VARIABLE, value 0, strValue 'v%'
                    // token   7 EQ, value 0, strValue ''
                    // token   8 EXPRESSION, value 2, strValue ''
                    // token   9 CONSTANT, value 2, strValue ''
                    // token  10 ELSE, value 0, strValue ''
                    // token  11 VARIABLE, value 0, strValue 'v%'
                    // token  12 EQ, value 0, strValue ''
                    // token  13 EXPRESSION, value 2, strValue ''
                    // token  14 CONSTANT, value 9, strValue ''

                    fprintf( fp, "    lda      %s\n", GenVariableName( vals[ t + 1 ].strValue ) );
                    fprintf( fp, "    and      #%d\n", vals[ t + 3 ].value );
                    fprintf( fp, "    beq      _uniq_%d\n", s_uniqueLabel );
                    fprintf( fp, "    lda      #%d\n", vals[ t + 8 ].value );
                    fprintf( fp, "    sta      %s\n", GenVariableName( vals[ t + 10 ].strValue ) );
                    fprintf( fp, "    lda      /%d\n", vals[ t + 8 ].value );
                    fprintf( fp, "    jmp      _uniq_%d\n", s_uniqueLabel + 1 );

                    fprintf( fp, "_uniq_%d:\n", s_uniqueLabel );
                    fprintf( fp, "    lda      #%d\n", vals[ t + 13 ].value );
                    fprintf( fp, "    sta      %s\n", GenVariableName( vals[ t + 10 ].strValue ) );
                    fprintf( fp, "    lda      /%d\n", vals[ t + 13 ].value );
                    s_uniqueLabel++;
                    fprintf( fp, "_uniq_%d:\n", s_uniqueLabel );
                    fprintf( fp, "    sta      %s+1\n", GenVariableName( vals[ t + 10 ].strValue ) );

                    s_uniqueLabel++;

                    break;
                }
                else if ( i8086DOS == g_AssemblyTarget &&
                          15 == vals.size() &&
                          4 == vals[ t ].value &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          Token_AND == vals[ t + 2 ].token  &&
                          Token_CONSTANT == vals[ t + 3 ].token &&
                          Token_THEN == vals[ t + 4 ].token &&
                          Token_VARIABLE == vals[ t + 5 ].token &&
                          Token_EQ == vals[ t + 6 ].token &&
                          2 == vals[ t + 7 ].value &&
                          Token_CONSTANT == vals[ t + 8 ].token &&
                          Token_ELSE == vals[ t + 9 ].token &&
                          Token_VARIABLE == vals[ t + 10 ].token &&
                          Token_EQ == vals[ t + 11 ].token &&
                          2 == vals[ t + 12 ].value &&
                          !stcmp( vals[ t + 5 ], vals[ t + 10 ] ) &&
                          Token_CONSTANT == vals[ t + 13 ].token )
                {
                    // line 4150 has 15 tokens  ====>> 4150 if st% and 1 then v% = 2 else v% = 9
                    // token   0 IF, value 0, strValue ''
                    // token   1 EXPRESSION, value 4, strValue ''
                    // token   2 VARIABLE, value 0, strValue 'st%'
                    // token   3 AND, value 0, strValue ''
                    // token   4 CONSTANT, value 1, strValue ''
                    // token   5 THEN, value 5, strValue ''
                    // token   6 VARIABLE, value 0, strValue 'v%'
                    // token   7 EQ, value 0, strValue ''
                    // token   8 EXPRESSION, value 2, strValue ''
                    // token   9 CONSTANT, value 2, strValue ''
                    // token  10 ELSE, value 0, strValue ''
                    // token  11 VARIABLE, value 0, strValue 'v%'
                    // token  12 EQ, value 0, strValue ''
                    // token  13 EXPRESSION, value 2, strValue ''
                    // token  14 CONSTANT, value 9, strValue ''

                    fprintf( fp, "    test     ds: [ %s ], %d\n", GenVariableName( vals[ t + 1 ].strValue ), vals[ t + 3 ].value );
                    fprintf( fp, "    jz       uniq_%d\n", s_uniqueLabel );
                    fprintf( fp, "    mov      bx, %d\n", vals[ t + 8 ].value );
                    fprintf( fp, "    jmp      uniq_%d\n", s_uniqueLabel + 1 );
                    fprintf( fp, "  uniq_%d:\n", s_uniqueLabel );
                    fprintf( fp, "    mov      bx, %d\n", vals[ t + 13 ].value );
                    s_uniqueLabel++;
                    fprintf( fp, "  uniq_%d:\n", s_uniqueLabel );
                    fprintf( fp, "     mov      ds: [ %s ], bx\n", GenVariableName( vals[ t + 10 ].strValue ) );
                    s_uniqueLabel++;

                    break;
                }
                else if ( oiOS == g_AssemblyTarget &&
                          15 == vals.size() &&
                          4 == vals[ t ].value &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          IsVariableInReg( varmap, vals[ t + 1 ].strValue ) &&
                          Token_AND == vals[ t + 2 ].token  &&
                          Token_CONSTANT == vals[ t + 3 ].token &&
                          Token_THEN == vals[ t + 4 ].token &&
                          Token_VARIABLE == vals[ t + 5 ].token &&
                          IsVariableInReg( varmap, vals[ t + 5 ].strValue ) &&
                          Token_EQ == vals[ t + 6 ].token &&
                          2 == vals[ t + 7 ].value &&
                          Token_CONSTANT == vals[ t + 8 ].token &&
                          Token_ELSE == vals[ t + 9 ].token &&
                          Token_VARIABLE == vals[ t + 10 ].token &&
                          Token_EQ == vals[ t + 11 ].token &&
                          2 == vals[ t + 12 ].value &&
                          !stcmp( vals[ t + 5 ], vals[ t + 10 ] ) &&
                          Token_CONSTANT == vals[ t + 13 ].token )
                {
                    // line 4150 has 15 tokens  ====>> 4150 if st% and 1 then v% = 2 else v% = 9
                    // token   0 IF, value 0, strValue ''
                    // token   1 EXPRESSION, value 4, strValue ''
                    // token   2 VARIABLE, value 0, strValue 'st%'
                    // token   3 AND, value 0, strValue ''
                    // token   4 CONSTANT, value 1, strValue ''
                    // token   5 THEN, value 5, strValue ''
                    // token   6 VARIABLE, value 0, strValue 'v%'
                    // token   7 EQ, value 0, strValue ''
                    // token   8 EXPRESSION, value 2, strValue ''
                    // token   9 CONSTANT, value 2, strValue ''
                    // token  10 ELSE, value 0, strValue ''
                    // token  11 VARIABLE, value 0, strValue 'v%'
                    // token  12 EQ, value 0, strValue ''
                    // token  13 EXPRESSION, value 2, strValue ''
                    // token  14 CONSTANT, value 9, strValue ''

                    int val = vals[ t + 3 ].value;
                    if ( 1 == val )
                        fprintf( fp, "    j        %s, rzero, even, _uniq_%d\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ), s_uniqueLabel );
                    else
                    {
                        fprintf( fp, "    ldi      rres, %d\n", vals[ t + 3 ].value );
                        fprintf( fp, "    and      rres, %s\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                        fprintf( fp, "    j        rres, rzero, eq, _uniq_%d\n", s_uniqueLabel );
                    }
                    fprintf( fp, "    ldi      %s, %d\n", GenVariableReg( varmap, vals[ t + 5 ].strValue ), vals[ t + 8 ].value );
                    fprintf( fp, "    j        rzero, rzero, eq, line_number_%zd\n", l + 1 );
                    fprintf( fp, "  _uniq_%d:\n", s_uniqueLabel );
                    fprintf( fp, "    ldi      %s, %d\n", GenVariableReg( varmap, vals[ t + 5 ].strValue ), vals[ t + 13 ].value );
                    s_uniqueLabel++;

                    break;
                }
                else if ( i8080CPM != g_AssemblyTarget &&
                          mos6502Apple1 != g_AssemblyTarget &&
                          i8086DOS != g_AssemblyTarget &&
                          x86Win != g_AssemblyTarget &&
                          arm32Linux != g_AssemblyTarget &&
                          riscv64 != g_AssemblyTarget &&
                          m68kCPM != g_AssemblyTarget &&
                          oiOS != g_AssemblyTarget &&
                          23 == vals.size() &&
                          4 == vals[ t ].value &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          IsVariableInReg( varmap, vals[ t + 1 ].strValue ) &&
                          Token_AND == vals[ t + 2 ].token  &&
                          Token_CONSTANT == vals[ t + 3 ].token &&
                          1 == vals[ t + 3 ].value &&
                          Token_THEN == vals[ t + 4 ].token &&
                          Token_OPENPAREN == vals[ t + 6 ].token &&
                          Token_CONSTANT == vals[ t + 12 ].token &&
                          Token_OPENPAREN == vals[ t + 15 ].token &&
                          Token_CONSTANT == vals[ t + 21 ].token &&
                          Token_VARIABLE == vals[ t + 8 ].token &&
                          Token_VARIABLE == vals[ t + 17 ].token &&
                          IsVariableInReg( varmap, vals[ t + 17 ].strValue ) &&
                          !stcmp( vals[ t + 5 ], vals[ t + 14 ] ) &&
                          !stcmp( vals[ t + 8 ], vals[ t + 17 ] ) )
                {
                    // line 4200 has 23 tokens  ====>> 4200 if st% and 1 then b%(p%) = 1 else b%(p%) = 2
                    //     0 IF, value 0, strValue ''
                    //     1 EXPRESSION, value 4, strValue ''
                    //     2 VARIABLE, value 0, strValue 'st%'
                    //     3 AND, value 0, strValue ''
                    //     4 CONSTANT, value 1, strValue ''
                    //     5 THEN, value 9, strValue ''
                    //     6 VARIABLE, value 0, strValue 'b%'
                    //     7 OPENPAREN, value 0, strValue ''
                    //     8 EXPRESSION, value 2, strValue ''
                    //     9 VARIABLE, value 0, strValue 'p%'
                    //    10 CLOSEPAREN, value 0, strValue ''
                    //    11 EQ, value 0, strValue ''
                    //    12 EXPRESSION, value 2, strValue ''
                    //    13 CONSTANT, value 1, strValue ''
                    //    14 ELSE, value 0, strValue ''
                    //    15 VARIABLE, value 0, strValue 'b%'
                    //    16 OPENPAREN, value 0, strValue ''
                    //    17 EXPRESSION, value 2, strValue ''
                    //    18 VARIABLE, value 0, strValue 'p%'
                    //    19 CLOSEPAREN, value 0, strValue ''
                    //    20 EQ, value 0, strValue ''
                    //    21 EXPRESSION, value 2, strValue ''
                    //    22 CONSTANT, value 2, strValue ''

                    if ( x64Win == g_AssemblyTarget )
                    {
                        fprintf( fp, "    mov      ecx, %d\n", vals[ t + 21 ].value );
                        fprintf( fp, "    mov      r8d, %d\n", vals[ t + 12 ].value );
                        fprintf( fp, "    test     %s, 1\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                        fprintf( fp, "    cmovnz   ecx, r8d\n" );
                        fprintf( fp, "    lea      rax, %s\n", GenVariableName( vals[ t + 5 ].strValue ) );
                        fprintf( fp, "    mov      ebx, %s\n", GenVariableReg( varmap, vals[ t + 8 ].strValue ) );
                        fprintf( fp, "    shl      ebx, 2\n" );
                        fprintf( fp, "    mov      DWORD PTR [ rbx + rax ], ecx\n" );
                    }
                    else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                    {
                        if ( IsVariableInReg( varmap, vals[ t + 5 ].strValue ) )
                            fprintf( fp, "    add      x3, %s, %s, lsl #2\n", GenVariableReg64( varmap, vals[ t + 5 ].strValue ), GenVariableReg64( varmap, vals[ t + 8 ].strValue ) );
                        else
                        {
                            LoadArm64Address( fp, "x3", varmap, vals[ t + 5 ].strValue );
                            fprintf( fp, "    add      x3, x3, %s, lsl #2\n", GenVariableReg64( varmap, vals[ t + 8 ].strValue ) );
                        }

                        LoadArm64Constant( fp, "x0", vals[ t + 12 ].value );
                        LoadArm64Constant( fp, "x1", vals[ t + 21 ].value );
                        fprintf( fp, "    tst      %s, 1\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                        fprintf( fp, "    csel     x4, x0, x1, ne\n" );
                        fprintf( fp, "    str      w4, [x3]\n" );
                    }

                    break;
                }
                else if ( m68kCPM == g_AssemblyTarget &&
                          23 == vals.size() &&
                          4 == vals[ t ].value &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          IsVariableInReg( varmap, vals[ t + 1 ].strValue ) &&
                          Token_AND == vals[ t + 2 ].token  &&
                          Token_CONSTANT == vals[ t + 3 ].token &&
                          1 == vals[ t + 3 ].value &&
                          Token_THEN == vals[ t + 4 ].token &&
                          Token_OPENPAREN == vals[ t + 6 ].token &&
                          Token_CONSTANT == vals[ t + 12 ].token &&
                          Token_OPENPAREN == vals[ t + 15 ].token &&
                          Token_CONSTANT == vals[ t + 21 ].token &&
                          Token_VARIABLE == vals[ t + 8 ].token &&
                          Token_VARIABLE == vals[ t + 17 ].token &&
                          IsVariableInReg( varmap, vals[ t + 17 ].strValue ) &&
                          !stcmp( vals[ t + 5 ], vals[ t + 14 ] ) &&
                          !stcmp( vals[ t + 8 ], vals[ t + 17 ] ) )
                {
                    // line 4200 has 23 tokens  ====>> 4200 if st% and 1 then b%(p%) = 1 else b%(p%) = 2
                    //     0 IF, value 0, strValue ''
                    //     1 EXPRESSION, value 4, strValue ''
                    //     2 VARIABLE, value 0, strValue 'st%'
                    //     3 AND, value 0, strValue ''
                    //     4 CONSTANT, value 1, strValue ''
                    //     5 THEN, value 9, strValue ''
                    //     6 VARIABLE, value 0, strValue 'b%'
                    //     7 OPENPAREN, value 0, strValue ''
                    //     8 EXPRESSION, value 2, strValue ''
                    //     9 VARIABLE, value 0, strValue 'p%'
                    //    10 CLOSEPAREN, value 0, strValue ''
                    //    11 EQ, value 0, strValue ''
                    //    12 EXPRESSION, value 2, strValue ''
                    //    13 CONSTANT, value 1, strValue ''
                    //    14 ELSE, value 0, strValue ''
                    //    15 VARIABLE, value 0, strValue 'b%'
                    //    16 OPENPAREN, value 0, strValue ''
                    //    17 EXPRESSION, value 2, strValue ''
                    //    18 VARIABLE, value 0, strValue 'p%'
                    //    19 CLOSEPAREN, value 0, strValue ''
                    //    20 EQ, value 0, strValue ''
                    //    21 EXPRESSION, value 2, strValue ''
                    //    22 CONSTANT, value 2, strValue ''

                    fprintf( fp, "    lea      %s, a0\n", GenVariableName( vals[ t + 5 ].strValue ) );
                    fprintf( fp, "    move.l   #%d, d1\n", vals[ t + 12 ].value  );
                    fprintf( fp, "    move.l   %s, d0\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                    fprintf( fp, "    andi.l   #1, d0\n" );
                    fprintf( fp, "    bne      un$%d\n", s_uniqueLabel );
                    fprintf( fp, "    move.l   #%d, d1\n", vals[ t + 21 ].value  );
                    fprintf( fp, "un$%d:\n", s_uniqueLabel );
                    fprintf( fp, "    move.l   %s, d0\n", GenVariableReg( varmap, vals[ t + 17 ].strValue ) );
                    fprintf( fp, "    lsl.l    #2, d0\n" );
                    fprintf( fp, "    move.l   d1, (a0, d0)\n" );
                    s_uniqueLabel++;

                    break;
                }
                else if ( oiOS == g_AssemblyTarget &&
                          23 == vals.size() &&
                          4 == vals[ t ].value &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          IsVariableInReg( varmap, vals[ t + 1 ].strValue ) &&
                          Token_AND == vals[ t + 2 ].token  &&
                          Token_CONSTANT == vals[ t + 3 ].token &&
                          1 == vals[ t + 3 ].value &&
                          Token_THEN == vals[ t + 4 ].token &&
                          Token_OPENPAREN == vals[ t + 6 ].token &&
                          Token_CONSTANT == vals[ t + 12 ].token &&
                          Token_OPENPAREN == vals[ t + 15 ].token &&
                          Token_CONSTANT == vals[ t + 21 ].token &&
                          Token_VARIABLE == vals[ t + 8 ].token &&
                          Token_VARIABLE == vals[ t + 17 ].token &&
                          !IsVariableInReg( varmap, vals[ t + 17 ].strValue ) &&
                          !stcmp( vals[ t + 5 ], vals[ t + 14 ] ) &&
                          !stcmp( vals[ t + 8 ], vals[ t + 17 ] ) )
                {
                    // line 4200 has 23 tokens  ====>> 4200 if st% and 1 then b%(p%) = 1 else b%(p%) = 2
                    //     0 IF, value 0, strValue ''
                    //     1 EXPRESSION, value 4, strValue ''
                    //     2 VARIABLE, value 0, strValue 'st%'
                    //     3 AND, value 0, strValue ''
                    //     4 CONSTANT, value 1, strValue ''
                    //     5 THEN, value 9, strValue ''
                    //     6 VARIABLE, value 0, strValue 'b%'
                    //     7 OPENPAREN, value 0, strValue ''
                    //     8 EXPRESSION, value 2, strValue ''
                    //     9 VARIABLE, value 0, strValue 'p%'
                    //    10 CLOSEPAREN, value 0, strValue ''
                    //    11 EQ, value 0, strValue ''
                    //    12 EXPRESSION, value 2, strValue ''
                    //    13 CONSTANT, value 1, strValue ''
                    //    14 ELSE, value 0, strValue ''
                    //    15 VARIABLE, value 0, strValue 'b%'
                    //    16 OPENPAREN, value 0, strValue ''
                    //    17 EXPRESSION, value 2, strValue ''
                    //    18 VARIABLE, value 0, strValue 'p%'
                    //    19 CLOSEPAREN, value 0, strValue ''
                    //    20 EQ, value 0, strValue ''
                    //    21 EXPRESSION, value 2, strValue ''
                    //    22 CONSTANT, value 2, strValue ''

                    fprintf( fp, "    ld       rtmp, [%s]\n", GenVariableName( vals[ t + 8 ].strValue ) );
                    fprintf( fp, "    j        %s, rzero, even, _uniq_%d\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ), s_uniqueLabel );
                    fprintf( fp, "    ldi      rres, %d\n", vals[ t + 12 ].value );
                    fprintf( fp, "    sto      %s[ rtmp ], rres\n", GenVariableName( vals[ t + 5 ].strValue ) );
                    fprintf( fp, "    j        rzero, rzero, eq, line_number_%zd\n", l + 1 );
                    fprintf( fp, "  _uniq_%d:\n", s_uniqueLabel );
                    fprintf( fp, "    ldi      rres, %d\n", vals[ t + 21 ].value );
                    fprintf( fp, "    sto      %s[ rtmp ], rres\n", GenVariableName( vals[ t + 14 ].strValue ) );
                    s_uniqueLabel++;

                    break;
                }
                else if ( i8080CPM != g_AssemblyTarget &&
                          mos6502Apple1 != g_AssemblyTarget &&
                          i8086DOS != g_AssemblyTarget &&
                          arm32Linux != g_AssemblyTarget &&
                          riscv64 != g_AssemblyTarget &&
                          m68kCPM != g_AssemblyTarget &&
                          ( !( ( x86Win == g_AssemblyTarget ) && !g_i386Target686 ) ) &&
                          oiOS != g_AssemblyTarget &&
                          11 == vals.size() &&
                          4 == vals[ t ].value &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          IsVariableInReg( varmap, vals[ t + 1 ].strValue ) &&
                          isOperatorRelational( vals[ t + 2 ].token ) &&
                          Token_CONSTANT == vals[ t + 3 ].token &&
                          Token_THEN == vals[ t + 4 ].token &&
                          0 == vals[ t + 4 ].value &&
                          Token_VARIABLE == vals[ t + 5 ].token &&
                          IsVariableInReg( varmap, vals[ t + 5 ].strValue ) &&
                          Token_CONSTANT == vals[ t + 8 ].token &&
                          Token_GOTO == vals[ t + 9 ].token )
                {
                    // line 4110 has 11 tokens  ====>> 4110 if wi% = 1 then re% = 6: goto 4280
                    //    0 IF, value 0, strValue ''
                    //    1 EXPRESSION, value 4, strValue ''
                    //    2 VARIABLE, value 0, strValue 'wi%'
                    //    3 EQ, value 0, strValue ''
                    //    4 CONSTANT, value 1, strValue ''
                    //    5 THEN, value 0, strValue ''
                    //    6 VARIABLE, value 0, strValue 're%'
                    //    7 EQ, value 0, strValue ''
                    //    8 EXPRESSION, value 2, strValue ''
                    //    9 CONSTANT, value 6, strValue ''
                    //   10 GOTO, value 4280, strValue ''

                    if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                    {
                        fprintf( fp, "    mov      eax, %d\n", vals[ t + 8 ].value );
                        fprintf( fp, "    cmp      %s, %d\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ),
                                                              vals[ t + 3 ].value );
                        fprintf( fp, "    %-6s   %s, eax\n", CMovInstructionX64[ vals[ t + 2 ].token ],
                                                             GenVariableReg( varmap, vals[ t + 5 ].strValue ) );
                        fprintf( fp, "    %-6s   line_number_%d\n", RelationalInstructionX64[ vals[ t + 2 ].token ],
                                                                    vals[ t + 9 ].value );
                    }
                    else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                    {
                        LoadArm64Constant( fp, "x1", vals[ t + 3 ].value );
                        LoadArm64Constant( fp, "x0", vals[ t + 8 ].value );
                        fprintf( fp, "    cmp      %s, w1\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ) );
                        fprintf( fp, "    csel     %s, w0, %s, %s\n", GenVariableReg( varmap, vals[ t + 5 ].strValue ),
                                                                      GenVariableReg( varmap, vals[ t + 5 ].strValue ),
                                                                      ConditionsArm[ vals[ t + 2 ].token ] );
                        fprintf( fp, "    b.%s     line_number_%d\n", ConditionsArm[ vals[ t + 2 ].token ],
                                                                      vals[ t + 9 ].value );
                    }
                    break;
                }
                else if ( i8086DOS == g_AssemblyTarget &&
                          11 == vals.size() &&
                          4 == vals[ t ].value &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          isOperatorRelational( vals[ t + 2 ].token ) &&
                          Token_CONSTANT == vals[ t + 3 ].token &&
                          Token_THEN == vals[ t + 4 ].token &&
                          0 == vals[ t + 4 ].value &&
                          Token_VARIABLE == vals[ t + 5 ].token &&
                          Token_CONSTANT == vals[ t + 8 ].token &&
                          Token_GOTO == vals[ t + 9 ].token )
                {
                    // line 4110 has 11 tokens  ====>> 4110 if wi% = 1 then re% = 6: goto 4280
                    //    0 IF, value 0, strValue ''
                    //    1 EXPRESSION, value 4, strValue ''
                    //    2 VARIABLE, value 0, strValue 'wi%'
                    //    3 EQ, value 0, strValue ''
                    //    4 CONSTANT, value 1, strValue ''
                    //    5 THEN, value 0, strValue ''
                    //    6 VARIABLE, value 0, strValue 're%'
                    //    7 EQ, value 0, strValue ''
                    //    8 EXPRESSION, value 2, strValue ''
                    //    9 CONSTANT, value 6, strValue ''
                    //   10 GOTO, value 4280, strValue ''

                    Token op = vals[ t + 2 ].token;
                    fprintf( fp, "    cmp      WORD PTR ds: [ %s ], %d\n", GenVariableName( vals[ t + 1 ].strValue ), vals[ t + 3 ].value );
                    fprintf( fp, "    %-6s   line_number_%zd\n", RelationalNotInstructionX64[ op ], l + 1 );
                    fprintf( fp, "    mov      WORD PTR ds: [ %s ], %d\n", GenVariableName( vals[ t + 5 ].strValue ), vals[ t + 8 ].value );
                    fprintf( fp, "    jmp      line_number_%d\n", vals[ t + 9 ].value );
                    break;
                }
                else if ( oiOS == g_AssemblyTarget &&
                          11 == vals.size() &&
                          4 == vals[ t ].value &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          isOperatorRelational( vals[ t + 2 ].token ) &&
                          Token_CONSTANT == vals[ t + 3 ].token &&
                          Token_THEN == vals[ t + 4 ].token &&
                          0 == vals[ t + 4 ].value &&
                          Token_VARIABLE == vals[ t + 5 ].token &&
                          Token_CONSTANT == vals[ t + 8 ].token &&
                          Token_GOTO == vals[ t + 9 ].token )
                {
                    // line 4110 has 11 tokens  ====>> 4110 if wi% = 1 then re% = 6: goto 4280
                    //    0 IF, value 0, strValue ''
                    //    1 EXPRESSION, value 4, strValue ''
                    //    2 VARIABLE, value 0, strValue 'wi%'
                    //    3 EQ, value 0, strValue ''
                    //    4 CONSTANT, value 1, strValue ''
                    //    5 THEN, value 0, strValue ''
                    //    6 VARIABLE, value 0, strValue 're%'
                    //    7 EQ, value 0, strValue ''
                    //    8 EXPRESSION, value 2, strValue ''
                    //    9 CONSTANT, value 6, strValue ''
                    //   10 GOTO, value 4280, strValue ''

                    Token op = vals[ t + 2 ].token;

                    fprintf( fp, "    ldi      rtmp, %d\n", vals[ t + 3 ].value );
                    if ( IsVariableInReg( varmap, vals[ t + 1 ].strValue ) )
                        fprintf( fp, "    j        rtmp, %s, %s, line_number_%zd\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ), ConditionsNotArm[ op ], l + 1 );
                    else
                    {
                        fprintf( fp, "    ld       rres, [%s]\n", GenVariableName( vals[ t + 1 ].strValue ) );
                        fprintf( fp, "    j        rtmp, rres, %s, line_number_%zd\n", ConditionsNotArm[ op ], l + 1 );
                    }

                    if ( IsVariableInReg( varmap, vals[ t + 5 ].strValue ) )
                        fprintf( fp, "    ldi      %s, %d\n", GenVariableReg( varmap, vals[ t + 5 ].strValue ), vals[ t + 8 ].value );
                    else
                    {
                        int val = vals[ t + 8 ].value;

                        if ( 0 == val )
                            fprintf( fp, "    st       [%s], rzero\n", GenVariableName( vals[ t + 5 ].strValue ) );
                        else if ( val >= -32 && val <= 31 )
                            fprintf( fp, "    sti      [%s], %d\n", GenVariableName( vals[ t + 5 ].strValue ), val );
                        else
                        {
                            fprintf( fp, "    ldi      rres, %d\n", val );
                            fprintf( fp, "    st       [%s], rres\n", GenVariableName( vals[ t + 5 ].strValue ) );
                        }
                    }

                    fprintf( fp, "    jmp      line_number_%d\n", vals[ t + 9 ].value );
                    break;
                }
                else if ( oiOS == g_AssemblyTarget &&
                          11 == vals.size() &&
                          Token_CONSTANT == vals[ t + 1 ].token &&
                          Token_EQ == vals[ t + 2 ].token &&
                          Token_VARIABLE == vals[ t + 6 ].token &&
                          IsVariableInReg( varmap, vals[ t + 6 ].strValue ) &&
                          Token_OPENPAREN == vals[ t + 4 ].token &&
                          Token_GOTO == vals[ t + 9 ].token )
                {
                    //  line 100 has 11 tokens  ====>> 100 IF 0 = FL%(I%) goto 180
                    //     0 IF, value 0, strValue ''
                    //     1 EXPRESSION, value 8, strValue ''
                    //     2 CONSTANT, value 0, strValue ''
                    //     3 EQ, value 0, strValue ''
                    //     4 VARIABLE, value 0, strValue 'fl%'
                    //     5 OPENPAREN, value 0, strValue ''
                    //     6 EXPRESSION, value 2, strValue ''
                    //     7 VARIABLE, value 0, strValue 'i%'
                    //     8 CLOSEPAREN, value 0, strValue ''
                    //     9 THEN, value 0, strValue ''
                    //    10 GOTO, value 180, strValue ''

                    fprintf( fp, "    ldo      rres, %s[%s]\n", GenVariableName( vals[ t + 3 ].strValue ), GenVariableReg( varmap, vals[ t + 6 ].strValue ) );

                    if ( 0 == vals[ t + 1 ].value )
                        fprintf( fp, "    j        rres, rzero, eq, line_number_%d\n", vals[ t + 9 ].value ); 
                    else
                    {
                        fprintf( fp, "    ldi      rtmp, %d\n", vals[ t + 1 ].value );
                        fprintf( fp, "    j        rres, rtmp, eq, line_number_%d\n", vals[ t + 9 ].value ); 
                    }

                    break;
                }
                else if ( oiOS == g_AssemblyTarget &&
                          5 == vals.size() &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          IsVariableInReg( varmap, vals[ t + 1 ].strValue ) &&
                          Token_GOTO == vals[ t + 3 ].token )
                {
                    // line 280 has 5 tokens  ====>> 280 if 0 <> n% then goto 245
                    //   0 IF, value 0, strValue ''
                    //   1 EXPRESSION, value 2, strValue ''
                    //   2 VARIABLE, value 0, strValue 'n%'
                    //   3 THEN, value 0, strValue ''
                    //   4 GOTO, value 12, strValue ''

                    fprintf( fp, "    j        %s, rzero, ne, line_number_%d\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ), vals[ t + 3 ].value );

                    break;
                }
                else if ( i8080CPM != g_AssemblyTarget &&
                          mos6502Apple1 != g_AssemblyTarget &&
                          i8086DOS != g_AssemblyTarget &&
                          arm32Linux != g_AssemblyTarget &&
                          riscv64 != g_AssemblyTarget &&
                          m68kCPM != g_AssemblyTarget &&
                          9 == vals.size() &&
                          6 == vals[ t ].value &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          Token_OPENPAREN == vals[ t + 2 ].token &&
                          Token_VARIABLE == vals[ t + 4 ].token &&
                          IsVariableInReg( varmap, vals[ t + 4 ].strValue ) &&
                          Token_THEN == vals[ t + 6 ].token &&
                          0 == vals[ t + 6 ].value &&
                          Token_GOTO == vals[ t + 7 ].token )
                {
                    // line 4180 has 9 tokens  ====>> 4180 if 0 <> b%(p%) then goto 4500
                    //   0 IF, value 0, strValue ''
                    //   1 EXPRESSION, value 6, strValue ''
                    //   2 VARIABLE, value 0, strValue 'b%'
                    //   3 OPENPAREN, value 0, strValue ''
                    //   4 EXPRESSION, value 2, strValue ''
                    //   5 VARIABLE, value 0, strValue 'p%'
                    //   6 CLOSEPAREN, value 0, strValue ''
                    //   7 THEN, value 0, strValue ''
                    //   8 GOTO, value 85, strValue ''

                    if ( x64Win == g_AssemblyTarget )
                    {
                        fprintf( fp, "    mov      ebx, %s\n", GenVariableReg( varmap, vals[ t + 4 ].strValue ) );
                        fprintf( fp, "    shl      rbx, 2\n" );
                        fprintf( fp, "    lea      rcx, %s\n", GenVariableName( vals[ t + 1 ].strValue ) );
                        fprintf( fp, "    mov      eax, DWORD PTR [rbx + rcx]\n" );
                        fprintf( fp, "    test     eax, eax\n" );
                        fprintf( fp, "    jnz      line_number_%d\n", vals[ t + 7 ].value );
                    }
                    else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                    {
                        if ( IsVariableInReg( varmap, vals[ t + 1 ].strValue ) )
                            fprintf( fp, "    add      x1, %s, %s, lsl #2\n", GenVariableReg64( varmap, vals[ t + 1 ].strValue ), GenVariableReg64( varmap, vals[ t + 4 ].strValue ) );
                        else
                        {
                            LoadArm64Address( fp, "x2", vals[ t + 1 ].strValue );
                            fprintf( fp, "    add      x1, x2, %s, lsl #2\n", GenVariableReg64( varmap, vals[ t + 4 ].strValue ) );
                        }

                        fprintf( fp, "    ldr      w0, [x1]\n" );
                        fprintf( fp, "    cbnz     w0, line_number_%d\n", vals[ t + 7 ].value );
                    }
                    else if ( x86Win == g_AssemblyTarget )
                    {
                        fprintf( fp, "    mov      ebx, %s\n", GenVariableReg( varmap, vals[ t + 4 ].strValue ) );
                        fprintf( fp, "    shl      ebx, 2\n" );
                        fprintf( fp, "    lea      edx, %s\n", GenVariableName( vals[ t + 1 ].strValue ) );
                        fprintf( fp, "    mov      eax, DWORD PTR [ebx + edx]\n" );
                        fprintf( fp, "    test     eax, eax\n" );
                        fprintf( fp, "    jnz      line_number_%d\n", vals[ t + 7 ].value );
                    }
                    else if ( oiOS == g_AssemblyTarget )
                    {
                        fprintf( fp, "    ldo      rres, %s[ %s ]\n", GenVariableName( vals[ t + 1 ].strValue ), GenVariableReg( varmap, vals[ t + 4 ].strValue ) );
                        fprintf( fp, "    j        rres, rzero, ne, line_number_%d\n", vals[ t + 7 ].value );
                    }
                    break;
                }
                else if ( i8080CPM != g_AssemblyTarget &&
                          mos6502Apple1 != g_AssemblyTarget &&
                          i8086DOS != g_AssemblyTarget &&
                          riscv64 != g_AssemblyTarget &&
                          oiOS != g_AssemblyTarget &&
                          7 == vals.size() &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          IsVariableInReg( varmap, vals[ t + 1 ].strValue ) &&
                          Token_AND == vals[ t + 2 ].token &&
                          Token_CONSTANT == vals[ t + 3 ].token &&
                          vals[ t + 3 ].value < 256 &&  // arm64 requires small values 
                          vals[ t + 3 ].value >= 0 &&
                          Token_THEN == vals[ t + 4 ].token &&
                          0 == vals[ t + 4 ].value &&
                          Token_GOTO == vals[ t + 5 ].token )
                {
                    // line 4330 has 7 tokens  ====>> 4330 if st% and 1 goto 4340
                    //    0 IF, value 0, strValue ''
                    //    1 EXPRESSION, value 4, strValue ''
                    //    2 VARIABLE, value 0, strValue 'st%'
                    //    3 AND, value 0, strValue ''
                    //    4 CONSTANT, value 1, strValue ''
                    //    5 THEN, value 0, strValue ''
                    //    6 GOTO, value 4340, strValue ''

                    if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                    {
                        fprintf( fp, "    test     %s, %d\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ), vals[ t + 3 ].value );
                        fprintf( fp, "    jnz      line_number_%d\n", vals[ t + 5 ].value );
                    }
                    else if ( arm32Linux == g_AssemblyTarget )
                    {
                        fprintf( fp, "    tst      %s, #%d\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ), vals[ t + 3 ].value );
                        fprintf( fp, "    bne      line_number_%d\n", vals[ t + 5 ].value );
                    }
                    else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                    {
                        fprintf( fp, "    tst      %s, %d\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ), vals[ t + 3 ].value );
                        fprintf( fp, "    b.ne     line_number_%d\n", vals[ t + 5 ].value );
                    }
                    else if ( m68kCPM == g_AssemblyTarget )
                    {
                        fprintf( fp, "    move.l   %s, d0\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ) ); // may be in an An register so no andi
                        fprintf( fp, "    andi.l   #%d, d0\n", vals[ t + 3 ].value );
                        fprintf( fp, "    bne      ln$%d\n", vals[ t + 5 ].value );
                    }

                    break;
                }
                else if ( i8080CPM == g_AssemblyTarget &&
                          7 == vals.size() &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          Token_AND == vals[ t + 2 ].token &&
                          Token_CONSTANT == vals[ t + 3 ].token &&
                          Token_THEN == vals[ t + 4 ].token &&
                          0 == vals[ t + 4 ].value &&
                          Token_GOTO == vals[ t + 5 ].token )
                {
                    // line 4330 has 7 tokens  ====>> 4330 if st% and 1 goto 4340
                    //    0 IF, value 0, strValue ''
                    //    1 EXPRESSION, value 4, strValue ''
                    //    2 VARIABLE, value 0, strValue 'st%'
                    //    3 AND, value 0, strValue ''
                    //    4 CONSTANT, value 1, strValue ''
                    //    5 THEN, value 0, strValue ''
                    //    6 GOTO, value 4340, strValue ''

                    fprintf( fp, "    lda      %s\n", GenVariableName( vals[ t + 1 ].strValue ) );
                    fprintf( fp, "    ani      %d\n", vals[ t + 3 ].value );
                    fprintf( fp, "    jnz      ln$%d\n", vals[ t + 5 ].value );

                    break;
                }
                else if ( mos6502Apple1 == g_AssemblyTarget &&
                          7 == vals.size() &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          Token_AND == vals[ t + 2 ].token &&
                          Token_CONSTANT == vals[ t + 3 ].token &&
                          Token_THEN == vals[ t + 4 ].token &&
                          0 == vals[ t + 4 ].value &&
                          Token_GOTO == vals[ t + 5 ].token )
                {
                    // line 4330 has 7 tokens  ====>> 4330 if st% and 1 goto 4340
                    //    0 IF, value 0, strValue ''
                    //    1 EXPRESSION, value 4, strValue ''
                    //    2 VARIABLE, value 0, strValue 'st%'
                    //    3 AND, value 0, strValue ''
                    //    4 CONSTANT, value 1, strValue ''
                    //    5 THEN, value 0, strValue ''
                    //    6 GOTO, value 4340, strValue ''

                    fprintf( fp, "    lda      %s\n", GenVariableName( vals[ t + 1 ].strValue ) );
                    fprintf( fp, "    and      #%d\n", vals[ t + 3 ].value );
                    fprintf( fp, "    beq      _uniq_%d\n", s_uniqueLabel );
                    fprintf( fp, "    jmp      line_number_%d\n", vals[ t + 5 ].value );
                    fprintf( fp, "_uniq_%d:\n", s_uniqueLabel );
                    s_uniqueLabel++;

                    break;
                }
                else if ( i8086DOS == g_AssemblyTarget &&
                          7 == vals.size() &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          Token_AND == vals[ t + 2 ].token &&
                          Token_CONSTANT == vals[ t + 3 ].token &&
                          Token_THEN == vals[ t + 4 ].token &&
                          0 == vals[ t + 4 ].value &&
                          Token_GOTO == vals[ t + 5 ].token )
                {
                    // line 4330 has 7 tokens  ====>> 4330 if st% and 1 goto 4340
                    //    0 IF, value 0, strValue ''
                    //    1 EXPRESSION, value 4, strValue ''
                    //    2 VARIABLE, value 0, strValue 'st%'
                    //    3 AND, value 0, strValue ''
                    //    4 CONSTANT, value 1, strValue ''
                    //    5 THEN, value 0, strValue ''
                    //    6 GOTO, value 4340, strValue ''

                    fprintf( fp, "    test     ds: [ %s ], %d\n", GenVariableName( vals[ t + 1 ].strValue ), vals[ t + 3 ].value );
                    fprintf( fp, "    jnz      line_number_%d\n", vals[ t + 5 ].value );

                    break;
                }
                else if ( oiOS == g_AssemblyTarget &&
                          7 == vals.size() &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          Token_AND == vals[ t + 2 ].token &&
                          Token_CONSTANT == vals[ t + 3 ].token &&
                          Token_THEN == vals[ t + 4 ].token &&
                          0 == vals[ t + 4 ].value &&
                          Token_GOTO == vals[ t + 5 ].token )
                {
                    // line 4330 has 7 tokens  ====>> 4330 if st% and 1 goto 4340
                    //    0 IF, value 0, strValue ''
                    //    1 EXPRESSION, value 4, strValue ''
                    //    2 VARIABLE, value 0, strValue 'st%'
                    //    3 AND, value 0, strValue ''
                    //    4 CONSTANT, value 1, strValue ''
                    //    5 THEN, value 0, strValue ''
                    //    6 GOTO, value 4340, strValue ''

                    if ( 1 == vals[ t + 3 ].value &&
                         IsVariableInReg( varmap, vals[ t + 1 ].strValue ) )
                        fprintf( fp, "    j        %s, rzero, odd, line_number_%d\n", GenVariableReg( varmap, vals[ t + 1 ].strValue ), vals[ t + 5 ].value );
                    else
                    {
                        if ( !IsVariableInReg( varmap, vals[ t + 1 ].strValue ) )
                            fprintf( fp, "    ld       rres, [%s]\n", GenVariableName( vals[ t + 1 ].strValue ) );
    
                        fprintf( fp, "    ldi      rtmp, %d\n", vals[ t + 3 ].value );
                        fprintf( fp, "    math     rres, %s, rtmp, and\n",
                                 IsVariableInReg( varmap, vals[ t + 1 ].strValue ) ? GenVariableReg( varmap, vals[ t + 1 ]. strValue ) : "rres" );
                        fprintf( fp, "    j        rres, rzero, eq, line_number_%zd\n", l + 1 );
                        fprintf( fp, "    jmp      line_number_%d\n", vals[ t + 5 ].value );
                    }

                    break;
                }
                else if ( ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget ) &&
                          6 == vals.size() &&
                          3 == vals[ t ].value &&
                          Token_NOT == vals[ t + 1 ].token &&
                          Token_VARIABLE == vals[ t + 2 ].token &&
                          IsVariableInReg( varmap, vals[ t + 2 ].strValue ) &&
                          Token_THEN == vals[ t + 3 ].token &&
                          0 == vals[ t + 3 ].value &&
                          Token_GOTO == vals[ t + 4 ].token )
                {
                    // line 2110 has 6 tokens  ====>> 2110 if 0 = wi% goto 2200
                    //  0 IF, value 0, strValue ''
                    //  1 EXPRESSION, value 3, strValue ''
                    //  2 NOT, value 0, strValue ''
                    //  3 VARIABLE, value 0, strValue 'wi%'
                    //  4 THEN, value 0, strValue ''
                    //  5 GOTO, value 33, strValue ''

                    fprintf( fp, "    cbz      %s, line_number_%d\n", GenVariableReg( varmap, vals [ t + 2 ].strValue ),
                                                                      vals[ t + 4 ].value );
                    break;
                }
                else if ( i8080CPM == g_AssemblyTarget &&
                          6 == vals.size() &&
                          3 == vals[ t ].value &&
                          Token_NOT == vals[ t + 1 ].token &&
                          Token_VARIABLE == vals[ t + 2 ].token &&
                          Token_THEN == vals[ t + 3 ].token &&
                          0 == vals[ t + 3 ].value &&
                          Token_GOTO == vals[ t + 4 ].token )
                {
                    // line 2110 has 6 tokens  ====>> 2110 if 0 = wi% goto 2200
                    //  0 IF, value 0, strValue ''
                    //  1 EXPRESSION, value 3, strValue ''
                    //  2 NOT, value 0, strValue ''
                    //  3 VARIABLE, value 0, strValue 'wi%'
                    //  4 THEN, value 0, strValue ''
                    //  5 GOTO, value 33, strValue ''

                    fprintf( fp, "    lhld     %s\n", GenVariableName( vals[ t + 2 ].strValue ) );
                    fprintf( fp, "    mov      a, h\n" );
                    fprintf( fp, "    ora      l\n" );
                    fprintf( fp, "    jz       ln$%d\n", vals[ t + 4 ].value );

                    break;
                }
                else if ( i8080CPM == g_AssemblyTarget &&
                          11 == vals.size() &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          Token_OPENPAREN == vals[ t + 2 ].token &&
                          Token_VARIABLE == vals[ t + 4 ].token &&
                          Token_EQ == vals[ t + 6 ].token &&
                          Token_CONSTANT == vals[ t + 7 ].token &&
                          0 == vals[ t + 7 ].value &&
                          Token_THEN == vals[ t + 8 ].token &&
                          Token_GOTO == vals[ t + 9 ].token )
                {
                    // line 100 has 11 tokens  ====>> 100 IF FL%(I%) = 0 THEN goto 180
                    //   token   0 IF, value 0, strValue ''
                    //   token   1 EXPRESSION, value 8, strValue ''
                    //   token   2 VARIABLE, value 0, strValue 'fl%'
                    //   token   3 OPENPAREN, value 0, strValue ''
                    //   token   4 EXPRESSION, value 2, strValue ''
                    //   token   5 VARIABLE, value 0, strValue 'i%'
                    //   token   6 CLOSEPAREN, value 0, strValue ''
                    //   token   7 EQ, value 0, strValue ''
                    //   token   8 CONSTANT, value 0, strValue ''
                    //   token   9 THEN, value 0, strValue ''
                    //   token  10 GOTO, value 180, strValue ''

                    fprintf( fp, "    lhld     %s\n", GenVariableName( vals[ t + 4 ].strValue ) );
                    fprintf( fp, "    dad      h\n" ); // 2 byte integers
                    fprintf( fp, "    lxi      d, %s\n", GenVariableName( vals[ t + 1 ].strValue ) );
                    fprintf( fp, "    dad      d\n" );
                    fprintf( fp, "    mov      a, m\n" );
                    fprintf( fp, "    inx      h\n" );
                    fprintf( fp, "    ora      m\n" );
                    fprintf( fp, "    jz       ln$%d\n", vals[ t + 9 ].value );

                    break;
                }
                else if ( i8080CPM == g_AssemblyTarget &&
                          7 == vals.size() &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          Token_GT == vals[ t + 2 ].token &&
                          Token_VARIABLE == vals[ t + 3 ].token &&
                          Token_THEN == vals[ t + 4 ].token &&
                          Token_GOTO == vals[ t + 5 ].token )
                {
                    // line 130 has 7 tokens  ====>> 130 IF K% > SI% THEN goto 170
                    //   token   0 IF, value 0, strValue ''
                    //   token   1 EXPRESSION, value 4, strValue ''
                    //   token   2 VARIABLE, value 0, strValue 'k%'
                    //   token   3 GT, value 0, strValue ''
                    //   token   4 VARIABLE, value 0, strValue 'si%'
                    //   token   5 THEN, value 0, strValue ''
                    //   token   6 GOTO, value 170, strValue ''

                    fprintf( fp, "    lhld     %s\n", GenVariableName( vals[ t + 3 ].strValue ) );
                    fprintf( fp, "    xchg\n" );
                    fprintf( fp, "    lhld     %s\n", GenVariableName( vals[ t + 1 ].strValue ) );

                    // are the high bits the same and so the same signs?
            
                    fprintf( fp, "    mov      a, d\n" );
                    fprintf( fp, "    xra      h\n" );
                    fprintf( fp, "    jp       ss%d\n", s_uniqueLabel );
            
                    fprintf( fp, "    xra      d\n" );
                    fprintf( fp, "    jm       nogoto%d\n", s_uniqueLabel );
                    fprintf( fp, "    jmp      ln$%d\n", vals[ t + 5 ].value );
            
                    fprintf( fp, "  ss%d:\n", s_uniqueLabel ); // same sign
            
                    fprintf( fp, "    mov      a, e\n" );
                    fprintf( fp, "    sub      l\n" );
                    fprintf( fp, "    mov      a, d\n" );
                    fprintf( fp, "    sbb      h\n" );
                    fprintf( fp, "    jc       ln$%d\n", vals[ t + 5 ].value );

                    fprintf( fp, "  nogoto%d:\n", s_uniqueLabel );
                    s_uniqueLabel++;

                    break;
                }
                else if ( mos6502Apple1 == g_AssemblyTarget &&
                          6 == vals.size() &&
                          3 == vals[ t ].value &&
                          Token_NOT == vals[ t + 1 ].token &&
                          Token_VARIABLE == vals[ t + 2 ].token &&
                          Token_THEN == vals[ t + 3 ].token &&
                          0 == vals[ t + 3 ].value &&
                          Token_GOTO == vals[ t + 4 ].token )
                {
                    // line 2110 has 6 tokens  ====>> 2110 if 0 = wi% goto 2200
                    //  0 IF, value 0, strValue ''
                    //  1 EXPRESSION, value 3, strValue ''
                    //  2 NOT, value 0, strValue ''
                    //  3 VARIABLE, value 0, strValue 'wi%'
                    //  4 THEN, value 0, strValue ''
                    //  5 GOTO, value 33, strValue ''

                    fprintf( fp, "    lda      %s\n", GenVariableName( vals[ t + 2 ].strValue ) );
                    fprintf( fp, "    bne      _uniq_%d\n", s_uniqueLabel );
                    fprintf( fp, "    lda      %s+1\n", GenVariableName( vals[ t + 2 ].strValue ) );
                    fprintf( fp, "    bne      _uniq_%d\n", s_uniqueLabel );
                    fprintf( fp, "    jmp      line_number_%d\n", vals[ t + 4 ].value );
                    fprintf( fp, "_uniq_%d:\n", s_uniqueLabel );
                    s_uniqueLabel++;

                    break;
                }
                else if ( i8086DOS == g_AssemblyTarget &&
                          6 == vals.size() &&
                          3 == vals[ t ].value &&
                          Token_NOT == vals[ t + 1 ].token &&
                          Token_VARIABLE == vals[ t + 2 ].token &&
                          Token_THEN == vals[ t + 3 ].token &&
                          0 == vals[ t + 3 ].value &&
                          Token_GOTO == vals[ t + 4 ].token )
                {
                    // line 2110 has 6 tokens  ====>> 2110 if 0 = wi% goto 2200
                    //  0 IF, value 0, strValue ''
                    //  1 EXPRESSION, value 3, strValue ''
                    //  2 NOT, value 0, strValue ''
                    //  3 VARIABLE, value 0, strValue 'wi%'
                    //  4 THEN, value 0, strValue ''
                    //  5 GOTO, value 33, strValue ''

                    fprintf( fp, "    cmp      WORD PTR ds: [ %s ], 0\n", GenVariableName( vals[ t + 2 ].strValue ) );
                    fprintf( fp, "    je       line_number_%d\n", vals[ t + 4 ].value );
                    break;
                }
                else if ( i8080CPM == g_AssemblyTarget &&
                          6 == vals.size() &&
                          3 == vals[ t ].value &&
                          Token_NOT == vals[ t + 1 ].token &&
                          Token_VARIABLE == vals[ t + 2 ].token &&
                          Token_THEN == vals[ t + 3 ].token &&
                          0 == vals[ t + 3 ].value &&
                          Token_RETURN == vals[ t + 4 ].token )
                {
                    // line 2110 has 6 tokens  ===>>> 4530 if st% = 0 then return
                    //  0 IF, value 0, strValue ''
                    //  1 EXPRESSION, value 3, strValue ''
                    //  2 NOT, value 0, strValue ''
                    //  3 VARIABLE, value 0, strValue 'wi%'
                    //  4 THEN, value 0, strValue ''
                    //  5 GOTO, value 33, strValue ''

                    fprintf( fp, "    lhld     %s\n", GenVariableName( vals[ t + 2 ].strValue ) );
                    fprintf( fp, "    mov      a, h\n" );
                    fprintf( fp, "    ora      l\n" );
                    fprintf( fp, "    jz       gosubReturn\n" );

                    break;
                }
                else if ( mos6502Apple1 == g_AssemblyTarget &&
                          6 == vals.size() &&
                          3 == vals[ t ].value &&
                          Token_NOT == vals[ t + 1 ].token &&
                          Token_VARIABLE == vals[ t + 2 ].token &&
                          Token_THEN == vals[ t + 3 ].token &&
                          0 == vals[ t + 3 ].value &&
                          Token_RETURN == vals[ t + 4 ].token )
                {
                    // line 2110 has 6 tokens  ===>>> 4530 if st% = 0 then return
                    //  0 IF, value 0, strValue ''
                    //  1 EXPRESSION, value 3, strValue ''
                    //  2 NOT, value 0, strValue ''
                    //  3 VARIABLE, value 0, strValue 'wi%'
                    //  4 THEN, value 0, strValue ''
                    //  5 GOTO, value 33, strValue ''

                    fprintf( fp, "    lda      %s\n", GenVariableName( vals[ t + 2 ].strValue ) );
                    fprintf( fp, "    bne      _uniq_%d\n", s_uniqueLabel );
                    fprintf( fp, "    lda      %s+1\n", GenVariableName( vals[ t + 2 ].strValue ) );
                    fprintf( fp, "    bne      _uniq_%d\n", s_uniqueLabel );
                    fprintf( fp, "    jmp      label_gosub_return\n" );
                    fprintf( fp, "_uniq_%d:\n", s_uniqueLabel );
                    s_uniqueLabel++;
                    break;
                }
                else if ( i8086DOS == g_AssemblyTarget &&
                          6 == vals.size() &&
                          3 == vals[ t ].value &&
                          Token_NOT == vals[ t + 1 ].token &&
                          Token_VARIABLE == vals[ t + 2 ].token &&
                          Token_THEN == vals[ t + 3 ].token &&
                          0 == vals[ t + 3 ].value &&
                          Token_RETURN == vals[ t + 4 ].token )
                {
                    // line 2110 has 6 tokens  ===>>> 4530 if st% = 0 then return
                    //  0 IF, value 0, strValue ''
                    //  1 EXPRESSION, value 3, strValue ''
                    //  2 NOT, value 0, strValue ''
                    //  3 VARIABLE, value 0, strValue 'wi%'
                    //  4 THEN, value 0, strValue ''
                    //  5 GOTO, value 33, strValue ''

                    fprintf( fp, "    cmp      WORD PTR ds: [ %s ], 0\n", GenVariableName( vals[ t + 2 ].strValue ) );
                    fprintf( fp, "    jne      _uniq_%d\n", s_uniqueLabel );
                    fprintf( fp, "    jmp      label_gosub_return\n" );
                    fprintf( fp, "_uniq_%d:\n", s_uniqueLabel );
                    s_uniqueLabel++;
                    break;
                }
                else if ( ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget ) &&
                          6 == vals.size() &&
                          3 == vals[ t ].value &&
                          Token_NOT == vals[ t + 1 ].token &&
                          Token_VARIABLE == vals[ t + 2 ].token &&
                          IsVariableInReg( varmap, vals[ t + 2 ].strValue ) &&
                          Token_THEN == vals[ t + 3 ].token &&
                          0 == vals[ t + 3 ].value &&
                          Token_RETURN == vals[ t + 4 ].token )
                {
                    // line 4530 has 6 tokens  ====>> 4530 if st% = 0 then return
                    //  0 IF, value 0, strValue ''
                    //  1 EXPRESSION, value 3, strValue ''
                    //  2 NOT, value 0, strValue ''
                    //  3 VARIABLE, value 0, strValue 'st%'
                    //  4 THEN, value 0, strValue ''
                    //  5 RETURN, value 0, strValue ''

                    fprintf( fp, "    cbz      %s, label_gosub_return\n", GenVariableReg( varmap, vals [ t + 2 ].strValue ) );
                    break;
                }
                else if ( m68kCPM == g_AssemblyTarget &&
                          6 == vals.size() &&
                          3 == vals[ t ].value &&
                          Token_NOT == vals[ t + 1 ].token &&
                          Token_VARIABLE == vals[ t + 2 ].token &&
                          IsVariableInReg( varmap, vals[ t + 2 ].strValue ) &&
                          Token_THEN == vals[ t + 3 ].token &&
                          0 == vals[ t + 3 ].value &&
                          Token_RETURN == vals[ t + 4 ].token )
                {
                    // line 4530 has 6 tokens  ====>> 4530 if st% = 0 then return
                    //  0 IF, value 0, strValue ''
                    //  1 EXPRESSION, value 3, strValue ''
                    //  2 NOT, value 0, strValue ''
                    //  3 VARIABLE, value 0, strValue 'st%'
                    //  4 THEN, value 0, strValue ''
                    //  5 RETURN, value 0, strValue ''

                    fprintf( fp, "    move.l   %s, d0\n", GenVariableReg( varmap, vals [ t + 2 ].strValue ) );
                    fprintf( fp, "    tst.l    d0\n" );
                    fprintf( fp, "    beq      label_gosub_return\n"  );
                    break;
                }
                else if ( i8080CPM != g_AssemblyTarget &&
                          mos6502Apple1 != g_AssemblyTarget &&
                          i8086DOS != g_AssemblyTarget &&
                          riscv64 != g_AssemblyTarget &&
                          oiOS != g_AssemblyTarget &&
                          4 == vals[ t ].value && 
                          isOperatorRelational( vals[ t + 2 ].token ) )
                {
                    // e.g.: if p% < 0 then goto 4180
                    //       if p% < r% then return else x% = x% + 1 

                    // line 4505 has 7 tokens  ====>> 4505 if p% < 9 then goto 4180
                    //    0 IF, value 0, strValue ''
                    //    1 EXPRESSION, value 4, strValue ''
                    //    2 VARIABLE, value 0, strValue 'p%'
                    //    3 LT, value 0, strValue ''
                    //    4 CONSTANT, value 9, strValue ''
                    //    5 THEN, value 0, strValue ''
                    //    6 GOTO, value 4180, strValue ''

                    Token ifOp = vals[ t + 2 ].token;

                    if ( Token_VARIABLE == vals[ 2 ].token && Token_CONSTANT == vals[ 4 ].token )
                    {
                        string & varname = vals[ 2 ].strValue;
                        if ( IsVariableInReg( varmap, varname ) )
                        {
                            if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                                fprintf( fp, "    cmp      %s, %d\n", GenVariableReg( varmap, varname ), vals[ 4 ].value );
                            else if ( arm32Linux == g_AssemblyTarget )
                            {
                                int constant = vals[ 4 ].value;
                                LoadArm32Constant( fp, "r1", constant );
                                fprintf( fp, "    cmp      %s, r1\n", GenVariableReg( varmap, varname ) );
                            }
                            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                            {
                                int constant = vals[ 4 ].value;
                                if ( fitsIn12Bits( constant ) )
                                    fprintf( fp, "    cmp      %s, %d\n", GenVariableReg( varmap, varname ), constant );
                                else
                                {
                                    LoadArm64Constant( fp, "x1", constant );
                                    fprintf( fp, "    cmp      %s, w1\n", GenVariableReg( varmap, varname ) );
                                }
                            }
                            else if ( m68kCPM == g_AssemblyTarget )
                                fprintf( fp, "    cmp.l    #%d, %s\n", vals[ 4 ].value, GenVariableReg( varmap, varname ), vals[ 4 ].value );
                        }
                        else
                        {
                            if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                                fprintf( fp, "    cmp      DWORD PTR [%s], %d\n", GenVariableName( varname ), vals[ 4 ].value );
                            else if ( arm32Linux == g_AssemblyTarget )
                            {
                                LoadArm32Address( fp, "r2", varname );
                                fprintf( fp, "    ldr      r0, [r2]\n" );
                                LoadArm32Constant( fp, "r1", vals[ 4 ].value );
                                fprintf( fp, "    cmp      r0, r1\n" );
                            }
                            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                            {
                                LoadArm64Address( fp, "x2", varname );
                                fprintf( fp, "    ldr      w0, [x2]\n" );
                                LoadArm64Constant( fp, "x1", vals[ 4 ].value );
                                fprintf( fp, "    cmp      w0, w1\n" );
                            }
                            else if ( m68kCPM == g_AssemblyTarget )
                            {
                                fprintf( fp, "    move.l   #%d, d0\n", vals[ 4 ].value );
                                fprintf( fp, "    move.l   %s, d1\n", GenVariableName( varname ) );
                                fprintf( fp, "    cmp.l    d0, d1\n" );
                            }
                        }
                    }
                    else if ( ( Token_VARIABLE == vals[ 2 ].token && Token_VARIABLE == vals[ 4 ].token ) &&
                              ( IsVariableInReg( varmap, vals[ 2 ].strValue ) || IsVariableInReg( varmap, vals[ 4 ].strValue ) ) )
                    {
                        string & varname2 = vals[ 2 ].strValue;
                        string & varname4 = vals[ 4 ].strValue;
                        if ( IsVariableInReg( varmap, varname2 ) )
                        {
                            if ( IsVariableInReg( varmap, varname4 ) )
                            {
                                if ( x64Win == g_AssemblyTarget || arm64Mac == g_AssemblyTarget || x86Win == g_AssemblyTarget ||
                                     arm32Linux == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                                    fprintf( fp, "    cmp      %s, %s\n", GenVariableReg( varmap, varname2 ), GenVariableReg( varmap, varname4 ) );
                                else if ( m68kCPM == g_AssemblyTarget )
                                    fprintf( fp, "    cmp.l    %s, %s\n", GenVariableReg( varmap, varname4 ), GenVariableReg( varmap, varname2 ) );
                            }
                            else
                            {
                                if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                                    fprintf( fp, "    cmp      %s, DWORD PTR [%s]\n", GenVariableReg( varmap, varname2 ), GenVariableName( varname4 ) );
                                else if ( arm32Linux == g_AssemblyTarget )
                                {
                                    LoadArm32Address( fp, "r2", varname4 );
                                    fprintf( fp, "    ldr      r1, [r2]\n" );
                                    fprintf( fp, "    cmp      %s, r1\n", GenVariableReg( varmap, varname2 ) );
                                }
                                else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                                {
                                    LoadArm64Address( fp, "x2", varname4 );
                                    fprintf( fp, "    ldr      w1, [x2]\n" );
                                    fprintf( fp, "    cmp      %s, w1\n", GenVariableReg( varmap, varname2 ) );
                                }
                                else if ( m68kCPM == g_AssemblyTarget )
                                {
                                    fprintf( fp, "    move.l   %s, d0\n", GenVariableName( varname4 ) );
                                    fprintf( fp, "    cmp.l    d0, %s\n", GenVariableReg( varmap, varname2 ) );
                                }
                            }
                        }
                        else
                        {
                            if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                                fprintf( fp, "    cmp      DWORD PTR[%s], %s\n", GenVariableName( varname2 ), GenVariableReg( varmap, varname4 ) );
                            else if ( arm32Linux == g_AssemblyTarget )
                            {
                                LoadArm32Address( fp, "r2", varname2 );
                                fprintf( fp, "    ldr      r0, [r2]\n" );
                                fprintf( fp, "    cmp      r0, %s\n", GenVariableReg( varmap, varname4 ) );
                            }
                            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                            {
                                LoadArm64Address( fp, "x2", varname2 );
                                fprintf( fp, "    ldr      w0, [x2]\n" );
                                fprintf( fp, "    cmp      w0, %s\n", GenVariableReg( varmap, varname4 ) );
                            }
                            else if ( m68kCPM == g_AssemblyTarget )
                            {
                                fprintf( fp, "    move.l   %s, d0\n", GenVariableName( varname2 ) );
                                fprintf( fp, "    cmp.l    %s, d0\n", GenVariableReg( varmap, varname4 ) );
                            }
                        }
                    }
                    else
                    {
                        if ( Token_CONSTANT == vals[ 2 ].token )
                        {
                            if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                                fprintf( fp, "    mov      eax, %d\n", vals[ 2 ].value );
                            else if ( arm32Linux == g_AssemblyTarget )
                                LoadArm32Constant( fp, "r0", vals[ 2 ].value );
                            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                                LoadArm64Constant( fp, "x0", vals[ 2 ].value );
                            else if ( m68kCPM == g_AssemblyTarget )
                                fprintf( fp, "    move.l   #%d, d0\n", vals[ 2 ].value );
                        }
                        else
                        {
                            string & varname = vals[ 2 ].strValue;
                            if ( IsVariableInReg( varmap, varname ) )
                            {
                                if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                                    fprintf( fp, "    mov      eax, %s\n", GenVariableReg( varmap, varname ) );
                                else if ( arm32Linux == g_AssemblyTarget )
                                    fprintf( fp, "    mov      r0, %s\n", GenVariableReg( varmap, varname ) );
                                else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                                    fprintf( fp, "    mov      x0, %s\n", GenVariableReg( varmap, varname ) );
                                else if ( m68kCPM == g_AssemblyTarget )
                                    fprintf( fp, "    move.l   %s, d0\n", GenVariableReg( varmap, varname ) );
                            }
                            else
                            {
                                if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                                    fprintf( fp, "    mov      eax, DWORD PTR [%s]\n", GenVariableName( varname ) );
                                else if ( arm32Linux == g_AssemblyTarget )
                                {
                                    LoadArm32Address( fp, "r2", varname );
                                    fprintf( fp, "    ldr      r0, [r2]\n" );
                                }
                                else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                                {
                                    LoadArm64Address( fp, "x2", varname );
                                    fprintf( fp, "    ldr      w0, [x2]\n" );
                                }
                                else if ( m68kCPM == g_AssemblyTarget )
                                    fprintf( fp, "    move.l  %s, d0\n", GenVariableName( varname ) );
                            }
                        }
    
                        if ( Token_CONSTANT == vals[ 4 ].token )
                        {
                            if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                                fprintf( fp, "    cmp      eax, %d\n", vals[ 4 ].value );
                            else if ( arm32Linux == g_AssemblyTarget )
                            {
                                LoadArm32Constant( fp, "r1", vals[ 4 ].value );
                                fprintf( fp, "    cmp      r0, r1\n" );
                            }
                            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                            {
                                LoadArm64Constant( fp, "x1", vals[ 4 ].value );
                                fprintf( fp, "    cmp      w0, w1\n" );
                            }
                            else if ( m68kCPM == g_AssemblyTarget )
                                fprintf( fp, "    cmp.l    d0, d1\n" );
                        }
                        else
                        {
                            string & varname = vals[ 4 ].strValue;
                            if ( IsVariableInReg( varmap, varname ) )
                            {
                                if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                                    fprintf( fp, "    cmp      eax, %s\n", GenVariableReg( varmap, varname ) );
                                else if ( arm32Linux == g_AssemblyTarget )
                                    fprintf( fp, "    cmp      r0, %s\n", GenVariableReg( varmap, varname ) );
                                else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                                    fprintf( fp, "    cmp      w0, %s\n", GenVariableReg( varmap, varname ) );
                                else if ( m68kCPM == g_AssemblyTarget )
                                    fprintf( fp, "    cmp.l    %s, d0\n", GenVariableReg( varmap, varname ) );
                            }
                            else
                            {
                                if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                                    fprintf( fp, "    cmp      eax, DWORD PTR [%s]\n", GenVariableName( varname ) );
                                else if ( arm32Linux == g_AssemblyTarget )
                                {
                                    LoadArm32Address( fp, "r2", varname );
                                    fprintf( fp, "    ldr      r1, [r2]\n" );
                                    fprintf( fp, "    cmp      r0, r1\n" );
                                }
                                else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                                {
                                    LoadArm64Address( fp, "x2", varname );
                                    fprintf( fp, "    ldr      w1, [x2]\n" );
                                    fprintf( fp, "    cmp      w0, w1\n" );
                                }
                                else if ( m68kCPM == g_AssemblyTarget )
                                {
                                    fprintf( fp, "    move.l   %s, d1\n", GenVariableName( varname ) );
                                    fprintf( fp, "    cmp.l    d1, d0\n" );
                                }
                            }
                        }
                    }

                    t += vals[ t ].value;
                    assert( Token_THEN == vals[ t ].token );
                    t++;

                    if ( Token_GOTO == vals[ t ].token )
                    {
                        if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                            fprintf( fp, "    %-6s   line_number_%d\n", RelationalInstructionX64[ ifOp ], vals[ t ].value );
                        else if ( arm32Linux == g_AssemblyTarget )
                            fprintf( fp, "    b%s      line_number_%d\n", ConditionsArm[ ifOp ], vals[ t ].value );
                        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                            fprintf( fp, "    b.%s     line_number_%d\n", ConditionsArm[ ifOp ], vals[ t ].value );
                        else if ( m68kCPM == g_AssemblyTarget )
                            fprintf( fp, "    b%s      ln$%d\n", Conditionsm68kCPM[ ifOp ], vals[ t ].value );
                        break;
                    }
                    else if ( Token_RETURN == vals[ t ].token )
                    {
                        if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                            fprintf( fp, "    %-6s   label_gosub_return\n", RelationalInstructionX64[ ifOp ] );
                        else if ( arm32Linux == g_AssemblyTarget )
                            fprintf( fp, "    b%s       label_gosub_return\n", ConditionsArm[ ifOp ] );
                        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                            fprintf( fp, "    b.%s      label_gosub_return\n", ConditionsArm[ ifOp ] );
                        else if ( m68kCPM == g_AssemblyTarget )
                            fprintf( fp, "    b%s       label_gosub_return\n", Conditionsm68kCPM[ ifOp ] );
                        break;
                    }
                    else
                    {
                        if ( vals[ t - 1 ].value ) // is there an else clause?
                        {
                            if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                                fprintf( fp, "    %-6s   SHORT label_else_%zd\n", RelationalNotInstructionX64[ ifOp ], l );
                            else if ( arm32Linux == g_AssemblyTarget )
                                fprintf( fp, "    b%s       label_else_%zd\n", ConditionsNotArm[ ifOp ], l );
                            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                                fprintf( fp, "    b.%s      label_else_%zd\n", ConditionsNotArm[ ifOp ], l );
                            else if ( m68kCPM == g_AssemblyTarget )
                                fprintf( fp, "    b%s       els$%zd\n", ConditionsNotm68kCPM[ ifOp ], l );
                        }
                        else
                        {
                            if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                                fprintf( fp, "    %-6s   SHORT line_number_%zd\n", RelationalNotInstructionX64[ ifOp ], l + 1 );
                            else if ( arm32Linux == g_AssemblyTarget )
                                fprintf( fp, "    b%s     line_number_%zd\n", ConditionsNotArm[ ifOp ], l + 1 );
                            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                                fprintf( fp, "    b.%s    line_number_%zd\n", ConditionsNotArm[ ifOp ], l + 1 );
                            else if ( m68kCPM == g_AssemblyTarget )
                                fprintf( fp, "    b%s       ln$%zd\n", ConditionsNotm68kCPM[ ifOp ], l + 1 );
                        }
                    }
                }
                else if ( mos6502Apple1 == g_AssemblyTarget &&
                          4 == vals[ t ].value &&
                          0 == vals[ t + 4 ].value &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          ( Token_CONSTANT == vals[ t + 3 ].token || Token_VARIABLE == vals[ t + 3 ].token ) &&
                          isOperatorRelational( vals[ t + 2 ].token ) &&
                          Token_GOTO == vals[ t + 5 ].token )

                {
                    // line 4505 has 7 tokens  ====>> 4505 if p% < 9 then goto 4180
                    //    0 IF, value 0, strValue ''
                    //    1 EXPRESSION, value 4, strValue ''
                    //    2 VARIABLE, value 0, strValue 'p%'
                    //    3 LT, value 0, strValue ''
                    //    4 CONSTANT, value 9, strValue ''
                    //    5 THEN, value 0, strValue ''
                    //    6 GOTO, value 4180, strValue ''

                    Token op = vals[ t + 2 ].token;
                    string & lhs = vals[ t + 1 ].strValue;

                    char aclhs[ 100 ];
                    strcpy( aclhs, GenVariableName( lhs ) );

                    if ( Token_VARIABLE == vals[ t + 3 ].token )
                    {
                        string & rhs = vals[ t + 3 ].strValue;
                        Generate6502Relation( fp, aclhs, GenVariableName( rhs ), op, "_if_true_", (int) l );
                    }
                    else
                    {
                        fprintf( fp, "    lda      #%d\n", vals[ t + 3 ].value );
                        fprintf( fp, "    sta      curOperand\n" );
                        fprintf( fp, "    lda      /%d\n", vals[ t + 3 ].value );
                        fprintf( fp, "    sta      curOperand+1\n" );
                        Generate6502Relation( fp, aclhs, "curOperand", op, "_if_true_", (int) l );
                    }

                    fprintf( fp, "    jmp      line_number_%zd\n", l + 1 );
                    fprintf( fp, "_if_true_%zd:\n", l );
                    fprintf( fp, "    jmp      line_number_%d\n", vals[ t + 5 ].value );

                    break;
                }
                else if ( i8086DOS == g_AssemblyTarget &&
                          4 == vals[ t ].value &&
                          0 == vals[ t + 4 ].value &&
                          Token_VARIABLE == vals[ t + 1 ].token &&
                          ( Token_CONSTANT == vals[ t + 3 ].token || Token_VARIABLE == vals[ t + 3 ].token ) &&
                          isOperatorRelational( vals[ t + 2 ].token ) &&
                          Token_GOTO == vals[ t + 5 ].token )

                {
                    // line 4505 has 7 tokens  ====>> 4505 if p% < 9 then goto 4180
                    //    0 IF, value 0, strValue ''
                    //    1 EXPRESSION, value 4, strValue ''
                    //    2 VARIABLE, value 0, strValue 'p%'
                    //    3 LT, value 0, strValue ''
                    //    4 CONSTANT, value 9, strValue ''
                    //    5 THEN, value 0, strValue ''
                    //    6 GOTO, value 4180, strValue ''

                    Token op = vals[ t + 2 ].token;
                    string & lhs = vals[ t + 1 ].strValue;


                    if ( Token_VARIABLE == vals[ t + 3 ].token )
                    {
                        fprintf( fp, "    mov      ax, ds: [ %s ]\n", GenVariableName( lhs ) );
                        fprintf( fp, "    cmp      ax, ds: [ %s ]\n", GenVariableName( vals[ t + 3 ].strValue ) );
                    }
                    else
                        fprintf( fp, "    cmp      WORD PTR ds: [ %s ], %d\n", GenVariableName( lhs ), vals[ t + 3 ].value );

                    fprintf( fp, "    %-6s   _uniq%d\n", RelationalNotInstructionX64[ op ], s_uniqueLabel );
                    fprintf( fp, "    jmp       line_number_%d\n", vals[ t + 5 ].value );
                    fprintf( fp, "  _uniq%d:\n", s_uniqueLabel );
                    s_uniqueLabel++;
                    break;
                }
                else if ( oiOS == g_AssemblyTarget &&
                          4 == vals[ t ].value &&
                          0 == vals[ t + 4 ].value &&
                          isTokenSimpleValue( vals[ t + 1 ].token ) &&
                          isTokenSimpleValue( vals[ t + 3 ].token ) &&
                          isOperatorRelational( vals[ t + 2 ].token ) &&
                          Token_GOTO == vals[ t + 5 ].token )

                {
                    // line 4505 has 7 tokens  ====>> 4505 if p% < 9 then goto 4180
                    //    0 IF, value 0, strValue ''
                    //    1 EXPRESSION, value 4, strValue ''
                    //    2 VARIABLE, value 0, strValue 'p%'
                    //    3 LT, value 0, strValue ''
                    //    4 CONSTANT, value 9, strValue ''
                    //    5 THEN, value 0, strValue ''
                    //    6 GOTO, value 4180, strValue ''

                    Token op = vals[ t + 2 ].token;
                    string & lhs = vals[ t + 1 ].strValue;
                    const char * pleft = "rres";

                    if ( Token_VARIABLE == vals[ t + 1 ].token )
                    {
                        if ( IsVariableInReg( varmap, lhs ) )
                            pleft = GenVariableReg( varmap, lhs );
                        else
                            fprintf( fp, "    ld       rres, [ %s ]\n", GenVariableName( lhs ) );
                    }
                    else
                        fprintf( fp, "    ldi      rres, %d\n", vals[ t + 1 ].value );

                    const char * pright = "rtmp";

                    if ( Token_VARIABLE == vals[ t + 3 ].token )
                    {
                        if ( IsVariableInReg( varmap, vals[ t + 3 ].strValue ) )
                            pright = GenVariableReg( varmap, vals[ t + 3 ].strValue );
                        else
                            fprintf( fp, "    ld       rtmp, [%s]\n", GenVariableName( vals[ t + 3 ].strValue ) );
                    }
                    else
                        fprintf( fp, "    ldi      rtmp, %d\n", vals[ t + 3 ].value );

#if 1 // jump target likely += 32k away
                    fprintf( fp, "    j        %s, %s, %s, line_number_%d\n", pleft, pright, ConditionsArm[ op ], vals[ t + 5 ].value );
#else
                    fprintf( fp, "    j        %s, %s, %s, line_number_%zd\n", pleft, pright, ConditionsNotArm[ op ], l + 1 );
                    fprintf( fp, "    jmp      line_number_%d\n", vals[ t + 5 ].value );
#endif
                    break;
                }
                else if ( i8080CPM != g_AssemblyTarget &&
                          mos6502Apple1 != g_AssemblyTarget &&
                          i8086DOS != g_AssemblyTarget &&
                          riscv64 != g_AssemblyTarget &&
                          m68kCPM != g_AssemblyTarget &&
                          3 == vals[ t ].value && 
                          Token_NOT == vals[ t + 1 ].token && 
                          Token_VARIABLE == vals[ t + 2 ].token )
                {
                    // line 4530 has 6 tokens  ====>> 4530 if st% = 0 then return
                    // token   0 IF, value 0, strValue ''
                    // token   1 EXPRESSION, value 3, strValue ''
                    // token   2 NOT, value 0, strValue ''
                    // token   3 VARIABLE, value 0, strValue 'st%'
                    // token   4 THEN, value 0, strValue ''
                    // token   5 RETURN, value 0, strValue ''

                    string & varname = vals[ t + 2 ].strValue;
                    const char * pOIOSReg = "rres";
                    if ( IsVariableInReg( varmap, varname ) )
                    {
                        if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                            fprintf( fp, "    test     %s, %s\n", GenVariableReg( varmap, varname ), GenVariableReg( varmap, varname ) );
                        else if ( arm64Mac == g_AssemblyTarget || arm32Linux == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                            fprintf( fp, "    cmp      %s, #0\n", GenVariableReg( varmap, varname ) );
                        else if ( oiOS == g_AssemblyTarget )
                            pOIOSReg = GenVariableReg( varmap, varname );
                    }
                    else
                    {
                        if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                            fprintf( fp, "    cmp      DWORD PTR [%s], 0\n", GenVariableName( varname ) );
                        else if ( arm32Linux == g_AssemblyTarget )
                        {
                            LoadArm32Address( fp, "r1", varname );
                            fprintf( fp, "    ldr      r0, [r1]\n" );
                            fprintf( fp, "    cmp      r0, #0\n" );
                        }
                        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                        {
                            LoadArm64Address( fp, "x1", varname );
                            fprintf( fp, "    ldr      w0, [x1]\n" );
                            fprintf( fp, "    cmp      w0, 0\n" );
                        }
                        else if ( oiOS == g_AssemblyTarget )
                            fprintf( fp, "    ld       rres, [%s]\n", GenVariableName( varname ) );
                    }

                    t += vals[ t ].value;
                    assert( Token_THEN == vals[ t ].token );
                    t++;

                    if ( Token_GOTO == vals[ t ].token )
                    {
                        if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                            fprintf( fp, "    je       line_number_%d\n", vals[ t ].value );
                        else if ( arm32Linux == g_AssemblyTarget )
                            fprintf( fp, "    beq      line_number_%d\n", vals[ t ].value );
                        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                            fprintf( fp, "    b.eq     line_number_%d\n", vals[ t ].value );
                        else if ( oiOS == g_AssemblyTarget )
                            fprintf( fp, "    j        %s, rzero, eq, line_number_%d\n", pOIOSReg, vals[ t ].value );
                        break;
                    }
                    else if ( Token_RETURN == vals[ t ].token )
                    {
                        if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                            fprintf( fp, "    je       label_gosub_return\n" );
                        else if ( arm32Linux == g_AssemblyTarget )
                            fprintf( fp, "    beq      label_gosub_return\n" );
                        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                            fprintf( fp, "    b.eq     label_gosub_return\n" );
                        else if ( oiOS == g_AssemblyTarget )
                            fprintf( fp, "    j        %s, rzero, eq, retnf\n", pOIOSReg );
                        break;
                    }
                    else
                    {
                        if ( vals[ t - 1 ].value ) // is there an else clause?
                        {
                            if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                                fprintf( fp, "    jne      SHORT label_else_%zd\n", l );
                            else if ( arm32Linux == g_AssemblyTarget )
                                fprintf( fp, "    bne      label_else_%zd\n", l );
                            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                                fprintf( fp, "    b.ne     label_else_%zd\n", l );
                            else if ( oiOS == g_AssemblyTarget )
                                fprintf( fp, "    j        %s, rzero, ne, label_else_%zd\n", pOIOSReg, l );
                        }
                        else
                        {
                            if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                                fprintf( fp, "    jne      SHORT line_number_%zd\n", l + 1 );
                            else if ( arm32Linux == g_AssemblyTarget )
                                fprintf( fp, "    bne      line_number_%zd\n", l + 1 );
                            else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                                fprintf( fp, "    b.ne     line_number_%zd\n", l + 1 );
                            else if ( oiOS == g_AssemblyTarget )
                                fprintf( fp, "    j        %s, rzero, ne, line_number_%zd\n", pOIOSReg, l + 1 );
                        }
                    }
                }
                else
                {
label_no_if_optimization:

                    // This general case will work for all the cases above, if with worse generated code

                    GenerateOptimizedExpression( fp, varmap, t, vals );
                    assert( Token_THEN == vals[ t ].token );
                    t++;

                    if ( x64Win == g_AssemblyTarget )                                                                                                            
                        fprintf( fp, "    cmp      rax, 0\n" );
                    else if ( arm32Linux == g_AssemblyTarget )
                        fprintf( fp, "    cmp      r0, #0\n" );
                    else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                        fprintf( fp, "    cmp      x0, 0\n" );
                    else if ( i8080CPM == g_AssemblyTarget )
                    {
                        fprintf( fp, "    mov      a, h\n" );
                        fprintf( fp, "    ora      l\n" );
                    }
                    else if ( mos6502Apple1 == g_AssemblyTarget )
                    {
                        fprintf( fp, "    lda      curOperand\n" );
                        fprintf( fp, "    ora      curOperand+1\n" );
                    }
                    else if ( i8086DOS == g_AssemblyTarget )                                                                                                            
                        fprintf( fp, "    cmp      ax, 0\n" );
                    else if ( x86Win == g_AssemblyTarget )                                                                                                            
                        fprintf( fp, "    cmp      eax, 0\n" );
                    else if ( m68kCPM == g_AssemblyTarget )
                        fprintf( fp, "    tst.l    d0\n" );
                    // no code here for riscv64 or oiOS

                    if ( Token_GOTO == vals[ t ].token )
                    {
                        if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                            fprintf( fp, "    jne      line_number_%d\n", vals[ t ].value );
                        else if ( arm32Linux == g_AssemblyTarget )
                            fprintf( fp, "    bne      line_number_%d\n", vals[ t ].value );
                        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                            fprintf( fp, "    b.ne     line_number_%d\n", vals[ t ].value );
                        else if ( i8080CPM == g_AssemblyTarget )
                            fprintf( fp, "    jnz      ln$%d\n", vals[ t ].value );
                        else if ( mos6502Apple1 == g_AssemblyTarget )
                        {
                            fprintf( fp, "    beq      line_number_%zd\n", l + 1 );
                            fprintf( fp, "    jmp      line_number_%d\n", vals[ t ].value );
                        }
                        else if ( i8086DOS == g_AssemblyTarget )
                        {
                            fprintf( fp, "    je       line_number_%zd\n", l + 1 ); // jump may be too far for jne
                            fprintf( fp, "    jmp      line_number_%d\n", vals[ t ].value );
                        }
                        else if ( riscv64 == g_AssemblyTarget )
                            fprintf( fp, "    bne      a0, zero, line_number_%d\n", vals[ t ].value );
                        else if ( m68kCPM == g_AssemblyTarget )
                            fprintf( fp, "    bne      ln$%d\n", vals[ t ].value );
                        else if ( oiOS == g_AssemblyTarget )
                            fprintf( fp, "    j        rres, rzero, ne, line_number_%d\n", vals[ t ].value );

                        break;
                    }
                    else if ( Token_RETURN == vals[ t ].token )
                    {
                        if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                            fprintf( fp, "    jne      label_gosub_return\n" );
                        else if ( arm32Linux == g_AssemblyTarget )
                            fprintf( fp, "    bne      label_gosub_return\n" );
                        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                            fprintf( fp, "    b.ne     label_gosub_return\n" );
                        else if ( i8080CPM == g_AssemblyTarget )
                            fprintf( fp, "    jnz      gosubReturn\n" );
                        else if ( mos6502Apple1 == g_AssemblyTarget )
                        {
                            fprintf( fp, "    beq      _continue_if_%zd\n", l );
                            fprintf( fp, "    jmp      label_gosub_return\n" );
                            fprintf( fp, "_continue_if_%zd\n", l );
                        }
                        else if ( i8086DOS == g_AssemblyTarget )
                        {
                            fprintf( fp, "    je       _continue_if_%zd\n", l );
                            fprintf( fp, "    jmp      label_gosub_return\n" );
                            fprintf( fp, "_continue_if_%zd:\n", l );
                        }
                        else if ( riscv64 == g_AssemblyTarget )
                            fprintf( fp, "    bne      a0, zero, label_gosub_return\n" );
                        else if ( m68kCPM == g_AssemblyTarget )
                            fprintf( fp, "    bne      label_gosub_return\n" );
                        else if ( oiOS == g_AssemblyTarget )
                            fprintf( fp, "    j        rres, rzero, ne, label_gosub_return\n" );

                        break;
                    }
                    else
                    {
                        if ( x64Win == g_AssemblyTarget || x86Win == g_AssemblyTarget )
                        {
                            if ( vals[ t - 1 ].value ) // is there an else clause?
                                fprintf( fp, "    je       label_else_%zd\n", l );
                            else
                                fprintf( fp, "    je       line_number_%zd\n", l + 1 );
                        }
                        else if ( arm32Linux == g_AssemblyTarget )
                        {
                            if ( vals[ t - 1 ].value )
                                fprintf( fp, "    beq      label_else_%zd\n", l );
                            else
                                fprintf( fp, "    beq      line_number_%zd\n", l + 1 );
                        }
                        else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                        {
                            if ( vals[ t - 1 ].value )
                                fprintf( fp, "    b.eq     label_else_%zd\n", l );
                            else
                                fprintf( fp, "    b.eq     line_number_%zd\n", l + 1 );
                        }
                        else if ( i8080CPM == g_AssemblyTarget )
                        {
                            if ( vals[ t - 1 ].value ) // is there an else clause?
                                fprintf( fp, "    jz       els$%zd\n", l );
                            else
                                fprintf( fp, "    jz       ln$%zd\n", l + 1 );
                        }
                        else if ( mos6502Apple1 == g_AssemblyTarget )
                        {
                            if ( vals[ t - 1 ].value ) // is there an else clause?
                                fprintf( fp, "    beq      label_else_%zd\n", l );
                            else
                                fprintf( fp, "    beq      line_number_%zd\n", l + 1 );
                        }
                        else if ( i8086DOS == g_AssemblyTarget )
                        {
                            if ( vals[ t - 1 ].value ) // is there an else clause?
                                fprintf( fp, "    je       label_else_%zd\n", l );
                            else
                                fprintf( fp, "    je       line_number_%zd\n", l + 1 );
                        }
                        else if ( riscv64 == g_AssemblyTarget )
                        {
                            if ( vals[ t - 1 ].value )
                                fprintf( fp, "    beq      a0, zero, label_else_%zd\n", l );
                            else
                                fprintf( fp, "    beq      a0, zero, line_number_%zd\n", l + 1 );
                        }
                        else if ( m68kCPM == g_AssemblyTarget )
                        {
                            if ( vals[ t - 1 ].value )
                                fprintf( fp, "    beq      els$%zd\n", l );
                            else
                                fprintf( fp, "    beq      ln$%zd\n", l + 1 );
                        }
                        else if ( oiOS == g_AssemblyTarget )
                        {
                            if ( vals[ t - 1 ].value )
                                fprintf( fp, "    j        rres, rzero, eq, label_else_%zd\n", l );
                            else
                                fprintf( fp, "    j        rres, rzero, eq, line_number_%zd\n", l + 1 );
                        }
                    }
                }
            }
            else if ( Token_ELSE == token )
            {
                assert( -1 != activeIf );
                if ( x64Win == g_AssemblyTarget )
                {
                    fprintf( fp, "    jmp      line_number_%zd\n", l + 1 );
                    fprintf( fp, "    align    16\n" );
                }
                else if ( x86Win == g_AssemblyTarget )
                    fprintf( fp, "    jmp      line_number_%zd\n", l + 1 );
                else if ( arm32Linux == g_AssemblyTarget )
                {
                    fprintf( fp, "    b        line_number_%zd\n", l + 1 );
                    fprintf( fp, "  .p2align 2\n" );
                }
                else if ( arm64Mac == g_AssemblyTarget || arm64Win == g_AssemblyTarget )
                {
                    fprintf( fp, "    b        line_number_%zd\n", l + 1 );
                    if ( arm64Mac == g_AssemblyTarget )
                        fprintf( fp, "  .p2align 2\n" );
                }
                else if ( i8080CPM == g_AssemblyTarget )
                    fprintf( fp, "    jmp      ln$%zd\n", l + 1 );
                else if ( mos6502Apple1 == g_AssemblyTarget )
                    fprintf( fp, "    jmp      line_number_%zd\n", l + 1 );
                else if ( i8086DOS == g_AssemblyTarget )
                    fprintf( fp, "    jmp      line_number_%zd\n", l + 1 );
                else if ( riscv64 == g_AssemblyTarget )
                    fprintf( fp, "    j        line_number_%zd\n", l + 1 );
                else if ( m68kCPM == g_AssemblyTarget )
                    fprintf( fp, "    bra      ln$%zd\n", l + 1 );
                else if ( oiOS == g_AssemblyTarget )
                    fprintf( fp, "    jmp      line_number_%zd\n", l + 1 );

                if ( i8080CPM == g_AssemblyTarget )
                    fprintf( fp, "  els$%zd:\n", activeIf );
                else if ( mos6502Apple1 == g_AssemblyTarget )
                    fprintf( fp, "label_else_%zd:\n", activeIf );
                else if ( arm64Win == g_AssemblyTarget )
                    fprintf( fp, "label_else_%zd\n", activeIf );
                else if ( m68kCPM == g_AssemblyTarget )
                    fprintf( fp, "els$%zd:\n", activeIf );
                else
                    fprintf( fp, "  label_else_%zd:\n", activeIf );

                activeIf = -1;
                t++;
            }
            else
            {
                break;
            }

            token = vals[ t ].token;
        } while( true );

        if ( -1 != activeIf )
            activeIf = -1;
    }

    if ( x64Win == g_AssemblyTarget )
    {
        // validate there is an active GOSUB before returning (or not)

        fprintf( fp, "  label_gosub_return:\n" );
        // fprintf( fp, "    dec      [gosubCount]\n" );   // should we protect against return without gosub?
        // fprintf( fp, "    cmp      [gosubCount], 0\n" );
        // fprintf( fp, "    jl       error_exit\n" );
        fprintf( fp, "    pop      rax\n" ); // rax is thrown away; it was pushed to keep 16-byte alignment
        fprintf( fp, "    ret\n" );

        fprintf( fp, "  label_gosub:\n" );
        fprintf( fp, "    push     rax\n" );  // keep the stack 16-byte aligned; save not needed
        fprintf( fp, "    jmp      rax\n" );

        fprintf( fp, "  error_exit:\n" );
        fprintf( fp, "    lea      rcx, [errorString]\n" );
        fprintf( fp, "    call     call_printf\n" );
        fprintf( fp, "    jmp      leave_execution\n" );

        fprintf( fp, "  end_execution:\n" );
        if ( !g_Quiet )
        {
            fprintf( fp, "    lea      rcx, [stopString]\n" );
            fprintf( fp, "    call     call_printf\n" );
        }

        fprintf( fp, "  leave_execution:\n" );
        fprintf( fp, "    xor      rcx, rcx\n" );
        fprintf( fp, "    call     call_exit\n" );
        fprintf( fp, "    ret    ; should never get here\n" );
        fprintf( fp, "main ENDP\n" );

        // These stubs are required to setup stack frame spill locations for printf when in a gosub/return statement.
        // They are also required so that volatile registers are persisted (r9, r10, r11).
        // Also push r8 to keep the stack 16-byte aligned, which is required for newer versions of the C++
        // runtime that use instructions like movdqa with stack pointers, which require 16-byte alignment

        if ( elapReferenced )
        {
            fprintf( fp, "align 16\n" );
            fprintf( fp, "printElap PROC\n" );
            fprintf( fp, "    push     r8\n" );
            fprintf( fp, "    push     r9\n" );
            fprintf( fp, "    push     r10\n" ); 
            fprintf( fp, "    push     r11\n" ); 
            fprintf( fp, "    push     rbp\n" );
            fprintf( fp, "    mov      rbp, rsp\n" );
            fprintf( fp, "    sub      rsp, 32\n" );
    
            fprintf( fp, "    lea      rcx, [currentTicks]\n" );
            fprintf( fp, "    call     call_QueryPerformanceCounter\n" );
            fprintf( fp, "    mov      rax, [currentTicks]\n" );
            fprintf( fp, "    sub      rax, [startTicks]\n" );
            fprintf( fp, "    mov      rcx, [perfFrequency]\n" );
            fprintf( fp, "    xor      rdx, rdx\n" );
            fprintf( fp, "    mov      rbx, 1000000\n" );
            fprintf( fp, "    mul      rbx\n" );
            fprintf( fp, "    div      rcx\n" );
    
            fprintf( fp, "    lea      rcx, [elapString]\n" );
            fprintf( fp, "    mov      rdx, rax\n" );
            fprintf( fp, "    call     printf\n" );
    
            fprintf( fp, "    leave\n" );
            fprintf( fp, "    pop      r11\n" );
            fprintf( fp, "    pop      r10\n" );
            fprintf( fp, "    pop      r9\n" );
            fprintf( fp, "    pop      r8\n" );
            fprintf( fp, "    ret\n" );
            fprintf( fp, "printElap ENDP\n" );
        }

        //////////////////////////////////////////////////////////////////

        if ( timeReferenced )
        {
            fprintf( fp, "align 16\n" );
            fprintf( fp, "printTime PROC\n" );
            fprintf( fp, "    push     r8\n" );
            fprintf( fp, "    push     r9\n" );
            fprintf( fp, "    push     r10\n" ); 
            fprintf( fp, "    push     r11\n" ); 
            fprintf( fp, "    push     rbp\n" );
            fprintf( fp, "    mov      rbp, rsp\n" );
            fprintf( fp, "    sub      rsp, 64\n" );
    
            fprintf( fp, "    lea      rcx, [currentTime]\n" );
            fprintf( fp, "    call     GetLocalTime\n" );
            fprintf( fp, "    lea      rax, [currentTime]\n" );
            fprintf( fp, "    lea      rcx, [timeString]\n" );
            fprintf( fp, "    movzx    rdx, WORD PTR [currentTime + 8]\n" );
            fprintf( fp, "    movzx    r8, WORD PTR [currentTime + 10]\n" );
            fprintf( fp, "    movzx    r9, WORD PTR [currentTime + 12]\n" );
            fprintf( fp, "    movzx    r10, WORD PTR [currentTime + 14]\n" );
            fprintf( fp, "    mov      QWORD PTR [rsp + 32], r10\n" );
            fprintf( fp, "    call     printf\n" );
    
            fprintf( fp, "    leave\n" );
            fprintf( fp, "    pop      r11\n" );
            fprintf( fp, "    pop      r10\n" );
            fprintf( fp, "    pop      r9\n" );
            fprintf( fp, "    pop      r8\n" );
            fprintf( fp, "    ret\n" );
            fprintf( fp, "printTime ENDP\n" );
       }

        //////////////////////////////////////////////////////////////////

        fprintf( fp, "align 16\n" );
        fprintf( fp, "call_printf PROC\n" );
        fprintf( fp, "    push     r8\n" );
        fprintf( fp, "    push     r9\n" );
        fprintf( fp, "    push     r10\n" ); 
        fprintf( fp, "    push     r11\n" ); 
        fprintf( fp, "    push     rbp\n" );
        fprintf( fp, "    mov      rbp, rsp\n" );
        fprintf( fp, "    sub      rsp, 32\n" );
        fprintf( fp, "    call     printf\n" );
        fprintf( fp, "    leave\n" );
        fprintf( fp, "    pop      r11\n" );
        fprintf( fp, "    pop      r10\n" );
        fprintf( fp, "    pop      r9\n" );
        fprintf( fp, "    pop      r8\n" );
        fprintf( fp, "    ret\n" );
        fprintf( fp, "call_printf ENDP\n" );

        //////////////////////////////////////////////////////////////////

        fprintf( fp, "align 16\n" );
        fprintf( fp, "call_exit PROC\n" );
        fprintf( fp, "    push     rbp\n" );
        fprintf( fp, "    mov      rbp, rsp\n" );
        fprintf( fp, "    sub      rsp, 32\n" );
        fprintf( fp, "    call     exit\n" );
        fprintf( fp, "    leave   ; should never get here\n" );
        fprintf( fp, "    ret\n" );
        fprintf( fp, "call_exit ENDP\n" );

        //////////////////////////////////////////////////////////////////

        if ( elapReferenced )
        {
            fprintf( fp, "align 16\n" );
            fprintf( fp, "call_QueryPerformanceCounter PROC\n" );
            fprintf( fp, "    push     r8\n" );
            fprintf( fp, "    push     r9\n" );   
            fprintf( fp, "    push     r10\n" ); 
            fprintf( fp, "    push     r11\n" ); 
            fprintf( fp, "    push     rbp\n" );
            fprintf( fp, "    mov      rbp, rsp\n" );
            fprintf( fp, "    sub      rsp, 32\n" );
            fprintf( fp, "    call     QueryPerformanceCounter\n" );
            fprintf( fp, "    leave\n" );
            fprintf( fp, "    pop      r11\n" );
            fprintf( fp, "    pop      r10\n" );
            fprintf( fp, "    pop      r9\n" );
            fprintf( fp, "    pop      r8\n" );
            fprintf( fp, "    ret\n" );
            fprintf( fp, "call_QueryPerformanceCounter ENDP\n" );
        }

        // end of the code segment and program

        fprintf( fp, "code_segment ENDS\n" );
        fprintf( fp, "END\n" );
    }
    else if ( arm32Linux == g_AssemblyTarget )
    {
        fprintf( fp, ".p2align 2\n" );
        fprintf( fp, "label_gosub:\n" );
        fprintf( fp, "    push     {ip, lr}\n" ); // push return address and dummy register
        fprintf( fp, "    bx       r0\n" );

        fprintf( fp, ".p2align 2\n" );
        fprintf( fp, "label_gosub_return:\n" );
        fprintf( fp, "    pop       {ip, pc}\n" );

        fprintf( fp, ".p2align 2\n" );
        fprintf( fp, "error_exit:\n" );
        fprintf( fp, "    ldr      r0, =errorString\n" );
        fprintf( fp, "    bl       call_printf\n" ); 
        fprintf( fp, "    b        leave_execution\n" );

        fprintf( fp, ".p2align 2\n" );
        fprintf( fp, "end_execution:\n" );
        if ( !g_Quiet )
        {
            fprintf( fp, "    ldr      r0, =stopString\n" );
            fprintf( fp, "    bl       call_printf\n" );
        }
        fprintf( fp, "    b        leave_execution\n" );
        
        fprintf( fp, ".p2align 2\n" );
        fprintf( fp, "leave_execution:\n" );
        fprintf( fp, "    mov      r0, #0\n" );
        fprintf( fp, "    b        exit\n" );

        fprintf( fp, ".p2align 2\n" );
        fprintf( fp, "call_printf:\n" );
        fprintf( fp, "    push     {fp, lr}\n" );
        fprintf( fp, "    save_volatile_registers\n" );
        fprintf( fp, "    bl       printf\n" );
        fprintf( fp, "    restore_volatile_registers\n" );
        fprintf( fp, "    pop      {fp, pc}\n" );
    }
    else if ( arm64Mac == g_AssemblyTarget )
    {
        fprintf( fp, ".p2align 2\n" );
        fprintf( fp, "label_gosub:\n" );
        fprintf( fp, "    str      x30, [sp, #-16]!\n" );
        fprintf( fp, "    br       x0\n" );

        fprintf( fp, ".p2align 2\n" );
        fprintf( fp, "label_gosub_return:\n" );
        fprintf( fp, "    ldr      x30, [sp], #16\n" );
        fprintf( fp, "    ret\n" );

        fprintf( fp, ".p2align 2\n" );
        fprintf( fp, "error_exit:\n" );
        LoadArm64Label( fp, "x0", "errorString" );
        fprintf( fp, "    bl       call_printf\n" ); 
        fprintf( fp, "    b        leave_execution\n" );

        fprintf( fp, ".p2align 2\n" );
        fprintf( fp, "end_execution:\n" );
        if ( !g_Quiet )
        {
            LoadArm64Label( fp, "x0", "stopString" );
            fprintf( fp, "    bl       call_printf\n" );
        }
        fprintf( fp, "    b        leave_execution\n" );
        
        fprintf( fp, ".p2align 2\n" );
        fprintf( fp, "leave_execution:\n" );
        fprintf( fp, "    ; OS system call to exit the app\n" );
        fprintf( fp, "    mov      x0, 0\n" );
        fprintf( fp, "    mov      x16, 1\n" );
        fprintf( fp, "    svc      0x80\n" );

        fprintf( fp, ".p2align 2\n" );
        fprintf( fp, "call_printf:\n" );
        fprintf( fp, "    save_volatile_registers\n" );
        fprintf( fp, "    sub      sp, sp, #32\n" );
        fprintf( fp, "    stp      x29, x30, [sp, #16]\n" );
        fprintf( fp, "    add      x29, sp, #16\n" );
        fprintf( fp, "    str      x1, [sp]\n" );
        fprintf( fp, "    bl       _printf\n" );
        fprintf( fp, "    ldp      x29, x30, [sp, #16]\n" );
        fprintf( fp, "    add      sp, sp, #32\n" );
        fprintf( fp, "    restore_volatile_registers\n" );
        fprintf( fp, "    ret\n" );

        for ( int i = 0; i < g_lohCount; i += 2 )
            fprintf( fp, ".loh AdrpAdd   Lloh%d, Lloh%d\n", i, i + 1 );
    }
    else if ( arm64Win == g_AssemblyTarget )
    {
        fprintf( fp, "    ENDP\n" );
        fprintf( fp, "  align 16\n" );
        fprintf( fp, "label_gosub\n" );
        fprintf( fp, "    str      x30, [sp, #-16]!\n" );
        fprintf( fp, "    br       x0\n" );

        fprintf( fp, "  align 16\n" );
        fprintf( fp, "label_gosub_return\n" );
        fprintf( fp, "    ldr      x30, [sp], #16\n" );
        fprintf( fp, "    ret\n" );

        fprintf( fp, "  align 16\n" );
        fprintf( fp, "error_exit\n" );
        LoadArm64Label( fp, "x0", "errorString" );
        fprintf( fp, "    bl       call_printf\n" ); 
        fprintf( fp, "    b        leave_execution\n" );

        fprintf( fp, "  align 16\n" );
        fprintf( fp, "end_execution\n" );
        if ( !g_Quiet )
        {
            LoadArm64Label( fp, "x0", "stopString" );
            fprintf( fp, "    bl       call_printf\n" );
        }
        fprintf( fp, "    b        leave_execution\n" );
        
        fprintf( fp, "  align 16\n" );
        fprintf( fp, "leave_execution\n" );
        fprintf( fp, "    bl       exit\n" );

        //////////////////////////////////////////////////////

        if ( timeReferenced )
        {
            fprintf( fp, "  align 16\n" );
            fprintf( fp, "printTime\n" );
            fprintf( fp, "    save_volatile_registers\n" );
            fprintf( fp, "    sub      sp, sp, #32\n" );
            fprintf( fp, "    stp      x29, x30, [sp, #16]\n" );
            fprintf( fp, "    add      x29, sp, #16\n" );
    
            LoadArm64Label( fp, "x0", "currentTime" );
            fprintf( fp, "    bl       GetLocalTime\n" );
            LoadArm64Label( fp, "x0", "currentTime" );
            fprintf( fp, "    ldrh     w1, [x0, #8]\n" );
            fprintf( fp, "    ldrh     w2, [x0, #10]\n" );
            fprintf( fp, "    ldrh     w3, [x0, #12]\n" );
            fprintf( fp, "    ldrh     w4, [x0, #14]\n" );
            LoadArm64Label( fp, "x0", "timeString" );
            fprintf( fp, "    bl       printf\n" );
    
            fprintf( fp, "    ldp      x29, x30, [sp, #16]\n" );
            fprintf( fp, "    add      sp, sp, #32\n" );
            fprintf( fp, "    restore_volatile_registers\n" );
            fprintf( fp, "    ret\n" );
        }

        //////////////////////////////////////////////////////

        fprintf( fp, "  align 16\n" );
        fprintf( fp, "call_printf\n" );
        fprintf( fp, "    save_volatile_registers\n" );
        fprintf( fp, "    sub      sp, sp, #32\n" );
        fprintf( fp, "    stp      x29, x30, [sp, #16]\n" );
        fprintf( fp, "    add      x29, sp, #16\n" );
        //fprintf( fp, "    str      x1, [sp]\n" );     Only needed for Mac calling convention, not Windows
        fprintf( fp, "    bl       printf\n" );
        fprintf( fp, "    ldp      x29, x30, [sp, #16]\n" );
        fprintf( fp, "    add      sp, sp, #32\n" );
        fprintf( fp, "    restore_volatile_registers\n" );
        fprintf( fp, "    ret\n" );

        fprintf( fp, "    END\n" );
    }
    else if ( i8080CPM == g_AssemblyTarget )
    {
        fprintf( fp, "    jmp      0\n" );

        fprintf( fp, "gosubReturn:\n" );
        fprintf( fp, "    ret\n" );

        fprintf( fp, "errorExit:\n" );
        fprintf( fp, "    lxi      h, errorString\n" );
        fprintf( fp, "    call     DISPLAY\n" );
        fprintf( fp, "    jmp      leaveExecution\n" );

        fprintf( fp, "endExecution:\n" );
        if ( !g_Quiet )
        {
            fprintf( fp, "    lxi      h, stopString\n" );
            fprintf( fp, "    call     DISPLAY\n" );
        }

        fprintf( fp, "leaveExecution:\n" );
        fprintf( fp, "    pop      h\n" );
        fprintf( fp, "    pop      d\n" );
        fprintf( fp, "    pop      b\n" );
        fprintf( fp, "    jmp      0\n" );

        /////////////////////////////////////////
        // displays the chracter in A

        fprintf( fp, "DisplayOneCharacter:\n" );
        fprintf( fp, "    push    b\n" );
        fprintf( fp, "    push    d\n" );
        fprintf( fp, "    push    h\n" );
        fprintf( fp, "    mvi     c, WCONF\n" );
        fprintf( fp, "    mov     e, a\n" );
        fprintf( fp, "    call    BDOS\n" );
        fprintf( fp, "    pop     h\n" );
        fprintf( fp, "    pop     d\n" );
        fprintf( fp, "    pop     b\n" );
        fprintf( fp, "    ret\n" );

        /////////////////////////////////////////
        // displays a null-terminated string in hl (not $-terminated like the BDOS)

        fprintf( fp, "DISPLAY:\n" );
        fprintf( fp, "    push    h\n" );
        fprintf( fp, "    push    d\n" );
        fprintf( fp, "    push    b\n" );
        fprintf( fp, "    mov     b, h\n" );
        fprintf( fp, "    mov     c, l\n" );
        fprintf( fp, "  DNEXT:\n" );
        fprintf( fp, "    ldax    b\n" );
        fprintf( fp, "    cpi     0\n" );
        fprintf( fp, "    jz      DDONE\n" );
        fprintf( fp, "    call    DisplayOneCharacter\n" );
        fprintf( fp, "    inx     b\n" );
        fprintf( fp, "    jmp     DNEXT\n" );
        fprintf( fp, "  DDONE:\n" );
        fprintf( fp, "    pop     b\n" );
        fprintf( fp, "    pop     d\n" );
        fprintf( fp, "    pop     h\n" );
        fprintf( fp, "    ret\n" );

        /////////////////////////////////////////
        // zeroes memory. byte count in de, starting at address bc

        fprintf( fp, "zeromem:\n" );
        fprintf( fp, "    mvi      a, 0\n" );
        fprintf( fp, "  zmAgain:\n" );
        fprintf( fp, "    cmp      d\n" );
        fprintf( fp, "    jnz      zmWrite\n" );
        fprintf( fp, "    cmp      e\n" );
        fprintf( fp, "    rz\n" );
        fprintf( fp, "  zmWrite:\n" );
        fprintf( fp, "    stax     b\n" );
        fprintf( fp, "    inx      b\n" );
        fprintf( fp, "    dcx      d\n" );
        fprintf( fp, "    jmp      zmAgain\n" );

        /////////////////////////////////////////

        /////////////////////////////////////////
        // negate the de register pair. handy for idiv and imul
        // negate using complement then add 1

        fprintf( fp, "neg$de:\n" );
        fprintf( fp, "    mov      a, d\n" );
        fprintf( fp, "    cma\n" );
        fprintf( fp, "    mov      d, a\n" );
        fprintf( fp, "    mov      a, e\n" );
        fprintf( fp, "    cma\n" );
        fprintf( fp, "    mov      e, a\n" );
        fprintf( fp, "    inx      d\n" );
        fprintf( fp, "    ret\n" );

        /////////////////////////////////////////

        /////////////////////////////////////////
        // negate the hl register pair. handy for idiv and imul
        // negate using complement then add 1

        fprintf( fp, "neg$hl:\n" );
        fprintf( fp, "    mov      a, h\n" );
        fprintf( fp, "    cma\n" );
        fprintf( fp, "    mov      h, a\n" );
        fprintf( fp, "    mov      a, l\n" );
        fprintf( fp, "    cma\n" );
        fprintf( fp, "    mov      l, a\n" );
        fprintf( fp, "    inx      h\n" );
        fprintf( fp, "    ret\n" );

        /////////////////////////////////////////

        /////////////////////////////////////////
        // multiply de by hl, result in hl
        // incredibly slow iterative addition.

        fprintf( fp, "imul:\n" );
        fprintf( fp, "    mov      a, l\n" );
        fprintf( fp, "    ora      h\n" );
        fprintf( fp, "    jnz      mul$notzero\n" );
        fprintf( fp, "    ret\n" );
        fprintf( fp, "  mul$notzero:\n" );
        fprintf( fp, "    mvi      b, 80h\n" );
        fprintf( fp, "    mov      a, h\n" );
        fprintf( fp, "    ana      b\n" );
        fprintf( fp, "    jz       mul$notneg\n" );
        fprintf( fp, "    call     neg$hl\n" );
        fprintf( fp, "    call     neg$de\n" );
        fprintf( fp, "  mul$notneg:\n" );
        fprintf( fp, "    mov      b, h\n" );
        fprintf( fp, "    mov      c, l\n" );
        fprintf( fp, "    lxi      h, 0\n" );
        fprintf( fp, "  mul$loop:\n" );
        fprintf( fp, "    dad      d\n" );
        fprintf( fp, "    jnc      mul$done\n" );
        fprintf( fp, "  mul$done:\n" );
        fprintf( fp, "    dcx      b\n" );
        fprintf( fp, "    mov      a, b\n" );
        fprintf( fp, "    ora      c\n" );
        fprintf( fp, "    jnz      mul$loop\n" );
        fprintf( fp, "    ret\n" );

        /////////////////////////////////////////

        /////////////////////////////////////////
        // divide de by hl, result in hl. remainder in divRem
        // incredibly slow iterative subtraction

        fprintf( fp, "idiv:\n" );
        fprintf( fp, "    xchg\n" ); // now it's hl / de
        fprintf( fp, "    mvi      c, 0\n" );
        fprintf( fp, "    mvi      b, 80h\n" );
        fprintf( fp, "    mov      a, d\n" );
        fprintf( fp, "    ana      b\n" );
        fprintf( fp, "    jz       div$denotneg\n" );
        fprintf( fp, "    inr      c\n" );
        fprintf( fp, "    call     neg$de\n" );
        fprintf( fp, "  div$denotneg:\n" );
        fprintf( fp, "    mov      a, h\n" );
        fprintf( fp, "    ana      b\n" );
        fprintf( fp, "    jz       div$hlnotneg\n" );
        fprintf( fp, "    inr      c\n" );
        fprintf( fp, "    call     neg$hl\n" );
        fprintf( fp, "  div$hlnotneg:\n" );
        fprintf( fp, "    push     b\n" );    // save c -- count of negatives
        fprintf( fp, "    lxi      b, 0\n" );
        fprintf( fp, "  div$loop:\n" );
        fprintf( fp, "    mov      a, l\n" );
        fprintf( fp, "    sub      e\n" );
        fprintf( fp, "    mov      l, a\n" );
        fprintf( fp, "    mov      a, h\n" );
        fprintf( fp, "    sbb      d\n" );
        fprintf( fp, "    mov      h, a\n" );
        fprintf( fp, "    jc       div$done\n" );
        fprintf( fp, "    inx      b\n" );
        fprintf( fp, "    jmp      div$loop\n" );
        fprintf( fp, "  div$done:\n" );
        fprintf( fp, "    dad      d\n" );
        fprintf( fp, "    shld     divRem\n" );
        fprintf( fp, "    mov      l, c\n" );
        fprintf( fp, "    mov      h, b\n" );
        fprintf( fp, "    pop      b\n" );
        fprintf( fp, "    mov      a, c\n" );
        fprintf( fp, "    ani      1\n" );
        fprintf( fp, "    cnz      neg$hl\n" ); // if 1 of the inputs was negative, negate
        fprintf( fp, "    ret\n" );

        /////////////////////////////////////////

        if ( FindVariable( varmap, "av%" ) )
        {
            fprintf( fp, "atou:                               ; in: hl points to string. out: hl has integer value. positive base-10 is assumed\n" );
            fprintf( fp, "        push   b\n" );
            fprintf( fp, "        push   d\n" );
            fprintf( fp, "        lxi    b, 0                 ; running total is in bc\n" );
            fprintf( fp, "  atouSpaceLoop:                    ; skip past spaces\n" );
            fprintf( fp, "        mov    a, m\n" );
            fprintf( fp, "        cpi    ' '\n" );
            fprintf( fp, "        jnz    atouNext\n" );
            fprintf( fp, "        inx    h\n" );
            fprintf( fp, "        jmp    atouSpaceLoop\n" );
            fprintf( fp, "  atouNext:\n" );
            fprintf( fp, "        mov    a, m                 ; check if we're at the end of string or the data isn't a number\n" );
            fprintf( fp, "        cpi    '0'\n" );
            fprintf( fp, "        jm     atouDone             ; < '0' isn't a digit\n" );
            fprintf( fp, "        cpi    '9' + 1\n" );
            fprintf( fp, "        jp     atouDone             ; > '9' isn't a digit\n" );
            fprintf( fp, "        lxi    d, 10                ; multiply what we have so far by 10\n" );
            fprintf( fp, "        push   h\n" );
            fprintf( fp, "        mov    h, b\n" );
            fprintf( fp, "        mov    l, c\n" );
            fprintf( fp, "        call   imul\n" );
            fprintf( fp, "        mov    b, h\n" );
            fprintf( fp, "        mov    c, l\n" );
            fprintf( fp, "        pop    h\n" );
            fprintf( fp, "        mov    a, m                 ; restore the digit in a because imul trashed it\n" );
            fprintf( fp, "        sui    '0'                  ; change ascii to a number\n" );
            fprintf( fp, "        add    c                    ; add this new number to the running total in bc\n" );
            fprintf( fp, "        mov    c, a\n" );
            fprintf( fp, "        mov    a, b\n" );
            fprintf( fp, "        aci    0                    ; if there was a carry from the add, reflect that\n" );
            fprintf( fp, "        mov    b, a\n" );
            fprintf( fp, "        inx    h                    ; move to the next character\n" );
            fprintf( fp, "        jmp    atouNext             ; and process it\n" );
            fprintf( fp, "  atouDone:\n" );
            fprintf( fp, "        mov    h, b                 ; the result goes in hl\n" );
            fprintf( fp, "        mov    l, c\n" );
            fprintf( fp, "        pop    d\n" );
            fprintf( fp, "        pop    b\n" );
            fprintf( fp, "        ret\n" );
        }

        /////////////////////////////////////////
        // function I found in the internet to print the integer in hl

        fprintf( fp, "puthl:  mov     a,h     ; Get the sign bit of the integer,\n" );
        fprintf( fp, "        ral             ; which is the top bit of the high byte\n" );
        fprintf( fp, "        sbb     a       ; A=00 if positive, FF if negative\n" );
        fprintf( fp, "        sta     negf    ; Store it as the negative flag\n" );
        fprintf( fp, "        cnz     neg$hl  ; And if HL was negative, make it positive\n" );
        fprintf( fp, "        lxi     d,num   ; Load pointer to end of number string\n" );
        fprintf( fp, "        push    d       ; Onto the stack\n" );
        fprintf( fp, "        lxi     b,-10   ; Divide by ten (by trial subtraction)\n" );
        fprintf( fp, "digit:  lxi     d,-1    ; DE = quotient. There is no 16-bit subtraction,\n" );
        fprintf( fp, "dgtdiv: dad     b       ; so we just add a negative value,\n" );
        fprintf( fp, "        inx     d\n" );
        fprintf( fp, "        jc      dgtdiv  ; while that overflows.\n" );
        fprintf( fp, "        mvi     a,'0'+10        ; The loop runs once too much so we're 10 out\n" );
        fprintf( fp, "        add     l       ; The remainder (minus 10) is in L\n" );
        fprintf( fp, "        xthl            ; Swap HL with top of stack (i.e., the string pointer)\n" );
        fprintf( fp, "        dcx     h       ; Go back one byte\n" );
        fprintf( fp, "        mov     m,a     ; And store the digit\n" );
        fprintf( fp, "        xthl            ; Put the pointer back on the stack\n" );
        fprintf( fp, "        xchg            ; Do all of this again with the quotient\n" );
        fprintf( fp, "        mov     a,h     ; If it is zero, we're done\n" );
        fprintf( fp, "        ora     l\n" );
        fprintf( fp, "        jnz     digit   ; But if not, there are more digits\n" );
        fprintf( fp, "        mvi     c, PRSTR  ; Prepare to call CP/M and print the string\n" );
        fprintf( fp, "        pop     d       ; Put the string pointer from the stack in DE\n" );
        fprintf( fp, "        lda     negf    ; See if the number was supposed to be negative\n" );
        fprintf( fp, "        inr     a\n" );
        fprintf( fp, "        jnz     bdos    ; If not, print the string we have and return\n" );
        fprintf( fp, "        dcx     d       ; But if so, we need to add a minus in front\n" );
        fprintf( fp, "        mvi     a,'-'\n" );
        fprintf( fp, "        stax    d\n" );
        fprintf( fp, "        jmp     bdos    ; And only then print the string\n" );
        fprintf( fp, "negf:   db      0       ; Space for negative flag\n" );
        fprintf( fp, "        db      '-00000'\n" );
        fprintf( fp, "num:    db      '$'     ; Space for number\n" );

        /////////////////////////////////////////

        fprintf( fp, "    end\n" );
    }
    else if ( mos6502Apple1 == g_AssemblyTarget )
    {
        fprintf( fp, "    jmp      exitapp\n" );

        fprintf( fp, "label_gosub_return:\n" );
        fprintf( fp, "    rts\n" );

        fprintf( fp, "error_exit:\n" );
        fprintf( fp, "    lda      #errorString\n" ); // # means lower byte
        fprintf( fp, "    sta      printString\n" );
        fprintf( fp, "    lda      /errorString\n" ); // / means upper byte
        fprintf( fp, "    sta      printString+1\n" );
        fprintf( fp, "    jsr      prstr\n" );

        fprintf( fp, "    jmp      leave_execution\n" );

        fprintf( fp, "end_execution:\n" );
        if ( !g_Quiet )
        {
            fprintf( fp, "    lda      #stopString\n" );
            fprintf( fp, "    sta      printString\n" );
            fprintf( fp, "    lda      /stopString\n" );
            fprintf( fp, "    sta      printString+1\n" );
            fprintf( fp, "    jsr      prstr\n" );
        }

        fprintf( fp, "leave_execution:\n" );
        fprintf( fp, "    jmp      exitapp\n" );

        /////////////////////////////////////////
        // zeroes memory. byte count in curOperand, starting at address in otherOperand

        fprintf( fp, "zeromem:\n" );
        fprintf( fp, "    lda      #0\n" );
        fprintf( fp, "    ldy      #0\n" );
        fprintf( fp, "_z_next:\n" );
        fprintf( fp, "    cmp      curOperand\n" );
        fprintf( fp, "    bne      _z_store\n" );
        fprintf( fp, "    cmp      curOperand+1\n" );
        fprintf( fp, "    beq      _z_done\n" );
        fprintf( fp, "_z_store:\n" );
        fprintf( fp, "    sta      (otherOperand), y\n" );

        fprintf( fp, "    cmp      curOperand\n" );
        fprintf( fp, "    bne      _z_justlow\n" );
        fprintf( fp, "    dec      curOperand+1\n" ); // high byte must be non-0

        fprintf( fp, "_z_justlow:\n" );
        fprintf( fp, "    dec      curOperand\n" );
        fprintf( fp, "    inc      otherOperand\n" );
        fprintf( fp, "    bne      _z_next\n" );
        fprintf( fp, "    inc      otherOperand+1\n" );
        fprintf( fp, "    jmp      _z_next\n" );

        fprintf( fp, "_z_done:\n" );
        fprintf( fp, "    rts\n" );

        /////////////////////////////////////////

        /////////////////////////////////////////
        // prints a carriage return and line feed

        fprintf( fp, "prcrlf:\n" );
        fprintf( fp, "    lda      #$0d\n" );
        fprintf( fp, "    jsr      echo\n" );
        fprintf( fp, "    lda      #$0a\n" );
        fprintf( fp, "    jsr      echo\n" );
        fprintf( fp, "    rts\n" );

        /////////////////////////////////////////

        /////////////////////////////////////////
        // prints a null-terminated ascii string pointed to by printString

        fprintf( fp, "prstr:\n" );
        fprintf( fp, "    ldy      #0\n" );
        fprintf( fp, "_prstr_next:\n" );
        fprintf( fp, "    lda      (printString), y\n" );
        fprintf( fp, "    beq      _prstr_done\n" );
        fprintf( fp, "    jsr      echo\n" );
        fprintf( fp, "    iny\n" );
        fprintf( fp, "    jmp      _prstr_next\n" );

        fprintf( fp, "_prstr_done:\n" );
        fprintf( fp, "    rts\n" );

        /////////////////////////////////////////

        fprintf( fp, "negate_otherOperand:\n" );
        fprintf( fp, "    lda      #$ff\n" );
        fprintf( fp, "    sec\n" );
        fprintf( fp, "    sbc      otherOperand+1\n" );
        fprintf( fp, "    sta      otherOperand+1\n" );
        fprintf( fp, "    lda      #$ff\n" );
        fprintf( fp, "    sec\n" );
        fprintf( fp, "    sbc      otherOperand\n" );
        fprintf( fp, "    sta      otherOperand\n" );
        fprintf( fp, "    inc      otherOperand\n" );
        fprintf( fp, "    bne      _negate_other_done\n" );
        fprintf( fp, "    inc      otherOperand+1\n" );
        fprintf( fp, "_negate_other_done:\n" );
        fprintf( fp, "    rts\n" );

        /////////////////////////////////////////

        fprintf( fp, "negate_curOperand:\n" );
        fprintf( fp, "    lda      #$ff\n" );
        fprintf( fp, "    sec\n" );
        fprintf( fp, "    sbc      curOperand+1\n" );
        fprintf( fp, "    sta      curOperand+1\n" );
        fprintf( fp, "    lda      #$ff\n" );
        fprintf( fp, "    sec\n" );
        fprintf( fp, "    sbc      curOperand\n" );
        fprintf( fp, "    sta      curOperand\n" );
        fprintf( fp, "    inc      curOperand\n" );
        fprintf( fp, "    bne      _negate_cur_done\n" );
        fprintf( fp, "    inc      curOperand+1\n" );
        fprintf( fp, "_negate_cur_done:\n" );
        fprintf( fp, "    rts\n" );

        /////////////////////////////////////////

        /////////////////////////////////////////
        // division: otherOperand / curOperand, result in curOperand, remainder in divRem
        // originally from https://llx.com/Neil/a2/mult.html

        fprintf( fp, "idiv:\n" );
        fprintf( fp, "    ldx      #0\n" );
        fprintf( fp, "    lda      #$80\n" );
        fprintf( fp, "    and      otherOperand+1\n" );
        fprintf( fp, "    beq      _div_prev_positive\n" );
        fprintf( fp, "    inx\n" );
        fprintf( fp, "    jsr      negate_otherOperand\n" );
        fprintf( fp, "_div_prev_positive:\n" );
        fprintf( fp, "    lda      #$80\n" );
        fprintf( fp, "    and      curOperand+1\n" );
        fprintf( fp, "    beq      _div_cur_positive\n" );
        fprintf( fp, "    inx\n" );
        fprintf( fp, "    jsr      negate_curOperand\n" );
        fprintf( fp, "_div_cur_positive:\n" );
        fprintf( fp, "    txa\n" );
        fprintf( fp, "    pha\n" );     // save count of negative arguments
        fprintf( fp, "    lda      #0                ;Initialize divRem to 0\n" );
        fprintf( fp, "    sta      divRem\n" );
        fprintf( fp, "    sta      divRem+1\n" );
        fprintf( fp, "    ldx      #16               ;There are 16 bits in otherOperand\n" );
        fprintf( fp, "_div_l1:\n" );
        fprintf( fp, "    asl      otherOperand      ;Shift hi bit of otherOperand into divRem\n" );
        fprintf( fp, "    rol      otherOperand+1    ;(vacating the lo bit, which will be used for the quotient)\n" );
        fprintf( fp, "    rol      divRem\n" );
        fprintf( fp, "    rol      divRem+1\n" );
        fprintf( fp, "    lda      divRem\n" );
        fprintf( fp, "    sec                        ;Trial subtraction\n" );
        fprintf( fp, "    sbc      curOperand\n" );
        fprintf( fp, "    tay\n" );
        fprintf( fp, "    lda      divRem+1\n" );
        fprintf( fp, "    sbc      curOperand+1\n" );
        fprintf( fp, "    bcc      _div_l2           ;Did subtraction succeed?\n" );
        fprintf( fp, "    sta      divRem+1          ;If yes, save it\n" );
        fprintf( fp, "    sty      divRem\n" );
        fprintf( fp, "    inc      otherOperand      ;and record a 1 in the quotient\n" );
        fprintf( fp, "_div_l2:\n" );
        fprintf( fp, "    dex\n" );
        fprintf( fp, "    bne      _div_l1\n" );
        fprintf( fp, "    lda      otherOperand\n" );
        fprintf( fp, "    sta      curOperand\n" );
        fprintf( fp, "    lda      otherOperand+1\n" );
        fprintf( fp, "    sta      curOperand+1\n" );
        fprintf( fp, "    pla\n" );     // restore count of negative arguments
        fprintf( fp, "    and      #1\n" );
        fprintf( fp, "    beq      _div_evenneg\n" );
        fprintf( fp, "    jsr      negate_curOperand\n" );
        fprintf( fp, "_div_evenneg:\n" );
        fprintf( fp, "    rts\n" );

        /////////////////////////////////////////

        /////////////////////////////////////////
        // multiplication: otherOperand * curOperand, result in curOperand
        // originally from https://llx.com/Neil/a2/mult.html

        fprintf( fp, "imul:\n" );
        fprintf( fp, "    lda      #0                ;Initialize mulResult to 0\n" );
        fprintf( fp, "    sta      mulResult+2\n" );
        fprintf( fp, "    ldx      #16               ;There are 16 bits in curOperand\n" );
        fprintf( fp, "_mul_l1:\n" );
        fprintf( fp, "    lsr      curOperand+1      ;Get low bit of curOperand\n" );
        fprintf( fp, "    ror      curOperand\n" );
        fprintf( fp, "    bcc      _mul_l2           ;0 or 1?\n" );
        fprintf( fp, "    tay                        ;If 1, add otherOperand (hi byte of mulResult is in A)\n" );
        fprintf( fp, "    clc\n" );
        fprintf( fp, "    lda      otherOperand\n" );
        fprintf( fp, "    adc      mulResult+2\n" );
        fprintf( fp, "    sta      mulResult+2\n" );
        fprintf( fp, "    tya\n" );
        fprintf( fp, "    adc      otherOperand+1\n" );
        fprintf( fp, "_mul_l2:\n" );
        fprintf( fp, "    ror      a                 ;Stairstep shift\n" );
        fprintf( fp, "    ror      mulResult+2\n" );
        fprintf( fp, "    ror      mulResult+1\n" );
        fprintf( fp, "    ror      mulResult\n" );
        fprintf( fp, "    dex\n" );
        fprintf( fp, "    bne      _mul_l1\n" );
        fprintf( fp, "    sta      mulResult+3\n" );
        fprintf( fp, "    lda      mulResult\n" );
        fprintf( fp, "    sta      curOperand\n" );
        fprintf( fp, "    lda      mulResult+1\n" );
        fprintf( fp, "    sta      curOperand+1\n" );
        fprintf( fp, "    rts\n" );

        /////////////////////////////////////////

        /////////////////////////////////////////
        // print_int in curOperand
        // extremely simple/slow solution because this almost never happens

        fprintf( fp, "print_int:\n" );

        // check if the integer is negative. If so, negate it then print a '-' 

        fprintf( fp, "    lda      #$80\n" );
        fprintf( fp, "    and      curOperand+1\n" );
        fprintf( fp, "    beq      _print_prev_positive\n" );
        fprintf( fp, "    jsr      negate_curOperand\n" );
        fprintf( fp, "    lda      #45\n" );
        fprintf( fp, "    jsr      echo\n" );
        fprintf( fp, "_print_prev_positive:\n" );

        // save curOperand

        fprintf( fp, "    lda      curOperand\n" );
        fprintf( fp, "    sta      tempWord\n" );
        fprintf( fp, "    lda      curOperand+1\n" );
        fprintf( fp, "    sta      tempWord+1\n" );

        // make arrayOffset point to the null terminating the output string

        fprintf( fp, "    lda      #intString\n" );
        fprintf( fp, "    clc\n" );
        fprintf( fp, "    adc      #5\n" );  // point at the null termination, past "32768"
        fprintf( fp, "    sta      arrayOffset\n" );
        fprintf( fp, "    lda      /intString\n" );
        fprintf( fp, "    sta      arrayOffset+1\n" );
        fprintf( fp, "    bcc      _print_no_carry\n" );
        fprintf( fp, "    inc      arrayOffset+1\n" );
        fprintf( fp, "_print_no_carry:\n" );

        // divide the number by 10

        fprintf( fp, "_print_int_again:\n" );
        fprintf( fp, "    lda      tempWord\n" );
        fprintf( fp, "    sta      otherOperand\n" );
        fprintf( fp, "    lda      tempWord+1\n" );
        fprintf( fp, "    sta      otherOperand+1\n" );
        fprintf( fp, "    lda      #10\n" );
        fprintf( fp, "    sta      curOperand\n" );
        fprintf( fp, "    lda      #0\n" );
        fprintf( fp, "    sta      curOperand+1\n" );
        fprintf( fp, "    jsr      idiv\n" );

        // Move the string pointer back one byte

        fprintf( fp, "    lda      arrayOffset\n" );
        fprintf( fp, "    bne      _print_no_hidec\n" );
        fprintf( fp, "    dec      arrayOffset+1\n" );
        fprintf( fp, "_print_no_hidec:\n" );
        fprintf( fp, "    dec      arrayOffset\n" );

        // Use the remainder to create the next character.

        fprintf( fp, "    lda      divRem\n" );
        fprintf( fp, "    clc\n" );
        fprintf( fp, "    adc      #48\n" ); // 48 == '0'
        fprintf( fp, "    ldy      #0\n" );
        fprintf( fp, "    sta      (arrayOffset), y\n" );

        // store the division result in the tempWord in case we loop again

        fprintf( fp, "    lda      curOperand\n" );
        fprintf( fp, "    sta      tempWord\n" );
        fprintf( fp, "    lda      curOperand+1\n" );
        fprintf( fp, "    sta      tempWord+1\n" );

        // if the result isn't 0, go back for more

        fprintf( fp, "    lda      #0\n" );
        fprintf( fp, "    cmp      curOperand\n" );
        fprintf( fp, "    bne      _print_int_again\n" );
        fprintf( fp, "    cmp      curOperand+1\n" );
        fprintf( fp, "    bne      _print_int_again\n" );

        // move the string pointer to printString and print it

        fprintf( fp, "    lda      arrayOffset\n" );
        fprintf( fp, "    sta      printString\n" );
        fprintf( fp, "    lda      arrayOffset+1\n" );
        fprintf( fp, "    sta      printString+1\n" );
        fprintf( fp, "    jsr      prstr\n" );
        fprintf( fp, "    rts\n" );

        /////////////////////////////////////////

        // Put arrays at the end of 6502 images so they can be discarded and not transferred

        for ( size_t l = 0; l < g_linesOfCode.size(); l++ )
        {
            LineOfCode & loc = g_linesOfCode[ l ];
            vector<TokenValue> & vals = loc.tokenValues;
    
            if ( Token_DIM == vals[ 0 ].token )
            {
                int cdwords = vals[ 0 ].dims[ 0 ];
                if ( 2 == vals[ 0 ].dimensions )
                    cdwords *= vals[ 0 ].dims[ 1 ];
    
                Variable * pvar = FindVariable( varmap, vals[ 0 ].strValue );
    
                // If an array is declared but never referenced later (and so not in varmap), ignore it
    
                if ( 0 != pvar )
                {
                    pvar->dimensions = vals[ 0 ].dimensions;
                    pvar->dims[ 0 ] = vals[ 0 ].dims[ 0 ];
                    pvar->dims[ 1 ] = vals[ 0 ].dims[ 1 ];
    
                    // if ( mos6502Apple1 == g_AssemblyTarget )
                    {
                        // The assembler has no way to implicitly fill bytes with 0.
                        // So fill with random numbers and zeromem() later.
    
                        fprintf( fp, "%s:\n", GenVariableName( vals[ 0 ].strValue ) ); 
                        fprintf( fp, "    .rf %d\n", cdwords * 2 ); 
                    }
                }
            }
        }
    }
    else if ( i8086DOS == g_AssemblyTarget )
    {
        fprintf( fp, "    jmp      leave_execution\n" );

        fprintf( fp, "label_gosub_return:\n" );
        fprintf( fp, "    ret\n" );

        fprintf( fp, "error_exit:\n" );
        fprintf( fp, "    mov      dx, offset errorString\n" );
        fprintf( fp, "    call     printstring\n" );
        fprintf( fp, "    jmp      leave_execution\n" );

        fprintf( fp, "end_execution:\n" );
        if ( !g_Quiet )
        {
            fprintf( fp, "    mov      dx, offset stopString\n" );
            fprintf( fp, "    call     printstring\n" );
        }

        fprintf( fp, "leave_execution:\n" );
        fprintf( fp, "     mov      al, 0\n" );
        fprintf( fp, "     mov      ah, dos_exit\n" );
        fprintf( fp, "     int      21h\n" );

        //////////////////

        if ( FindVariable( varmap, "av%" ) )
        {
            fprintf( fp, "atou proc near ; string input in cx. unsigned 16-bit integer result in ax\n" );
            fprintf( fp, "        push    di\n" );
            fprintf( fp, "        push    bx\n" );
            fprintf( fp, "        mov     bx, 0               ; running total is in bx\n" );
            fprintf( fp, "        mov     di, cx\n" );
            fprintf( fp, "        mov     cx, 10\n" );
            fprintf( fp, "skipspaces:\n" );
            fprintf( fp, "        cmp     byte ptr [di ], ' '\n" );
            fprintf( fp, "        jne     atouNext\n" );
            fprintf( fp, "        inc     di\n" );
            fprintf( fp, "        jmp     skipspaces\n" );
            fprintf( fp, "atouNext:\n" );
            fprintf( fp, "        cmp     byte ptr [ di ], '0'     ; if not a digit, we're done. Works with null and 0x0d terminated strings\n" );
            fprintf( fp, "        jb      atouDone\n" );
            fprintf( fp, "        cmp     byte ptr [ di ], '9' + 1\n" );
            fprintf( fp, "        jge     atouDone\n" );
            fprintf( fp, "        mov     ax, bx\n" );
            fprintf( fp, "        mul     cx\n" );
            fprintf( fp, "        mov     bx, ax\n" );
            fprintf( fp, "        xor     ah, ah\n" );
            fprintf( fp, "        mov     al, byte ptr [ di ]\n" );
            fprintf( fp, "        sub     ax, '0'\n" );
            fprintf( fp, "        add     bx, ax\n" );
            fprintf( fp, "        inc     di\n" );
            fprintf( fp, "        jmp     atouNext\n" );
            fprintf( fp, "atouDone:\n" );
            fprintf( fp, "        mov     ax, bx\n" );
            fprintf( fp, "        pop     bx\n" );
            fprintf( fp, "        pop     di\n" );
            fprintf( fp, "        ret\n" );
            fprintf( fp, "atou endp\n" );
        }

        //////////////////

        if ( elapReferenced )
        {
            fprintf( fp, "printelap PROC NEAR\n" );
            fprintf( fp, "    xor      ax, ax\n" );
            fprintf( fp, "    int      1ah\n" );        // "ticks" in cx:dx, with 18.2 ticks per second. 
            fprintf( fp, "    mov      WORD PTR ds: [ scratchpad ], dx\n" ); // low word
            fprintf( fp, "    mov      WORD PTR ds: [ scratchpad + 2 ], cx\n" ); // high word
    
            // subtract the current tick count from the app start tickcount
    
            fprintf( fp, "    mov      dl, 0\n" );
            fprintf( fp, "    mov      ax, WORD PTR ds: [ scratchpad ]\n" );
            fprintf( fp, "    mov      bx, WORD PTR ds: [ starttime ]\n" );
            fprintf( fp, "    sub      ax, bx\n" );
            fprintf( fp, "    mov      word ptr ds: [ result ], ax\n" );
            fprintf( fp, "    mov      ax, WORD PTR ds: [ scratchpad + 2 ]\n" );
            fprintf( fp, "    mov      bx, WORD PTR ds: [ starttime + 2 ]\n" );
            fprintf( fp, "    sbb      ax, bx\n" );
            fprintf( fp, "    mov      word ptr ds: [ result + 2 ], ax\n" );
    
            // 1193180 / 65536 = 18.20648193...
            // multiply by 10000 (to retain precision for the upcoming divide)
    
            fprintf( fp, "    mov      dx, word ptr ds: [ result + 2 ]\n" );
            fprintf( fp, "    mov      ax, word ptr ds: [ result ]\n" );
            fprintf( fp, "    mov      bx, 10000\n" );
            fprintf( fp, "    mul      bx\n" );
    
            // divide by 18206. After the divide, the result must fit in 16 bits, which means
            // a maximum of 3276 seconds (printint is signed!), or about 54 minutes.
            // elapsed times beyond that aren't supported.
    
            fprintf( fp, "    mov      bx, 18206\n" );
            fprintf( fp, "    div      bx\n" );
    
            // it's now in tenths of a second. divide by 10 to get it back to seconds
    
            fprintf( fp, "    xor      dx, dx\n" );
            fprintf( fp, "    mov      bx, 10\n" );
            fprintf( fp, "    div      bx\n" );
            fprintf( fp, "    push     dx\n" ); // this is the remainder (tenths of a second)
    
            // print the seconds, a period, and the remainder which is tenths of a second.
            // given 18.2 ticks per second, it's more like 1/5 of a second resolution
    
            fprintf( fp, "    call     printint\n" );
            fprintf( fp, "    call     prperiod\n" );
            fprintf( fp, "    pop      ax\n" );
            fprintf( fp, "    call     printint\n" );
    
            fprintf( fp, "    ret\n" );
            fprintf( fp, "printelap ENDP\n" );
        }

        //////////////////

        if ( timeReferenced )
        {
            fprintf( fp, "printtime PROC NEAR\n" );
            fprintf( fp, "    mov      ah, 2ch\n" );
            fprintf( fp, "    int      21h\n" );
    
            fprintf( fp, "    push     dx\n" );
            fprintf( fp, "    push     cx\n" );
            fprintf( fp, "    xor      ax, ax\n" );
            fprintf( fp, "    mov      al, ch\n" );
            fprintf( fp, "    call     print2digits\n" );
            fprintf( fp, "    call     prcolon\n" );
    
            fprintf( fp, "    pop      cx\n" );
            fprintf( fp, "    xor      ax, ax\n" );
            fprintf( fp, "    mov      al, cl\n" );
            fprintf( fp, "    call     print2digits\n" );
            fprintf( fp, "    call     prcolon\n" );
    
            fprintf( fp, "    pop      dx\n" );
            fprintf( fp, "    push     dx\n" );
            fprintf( fp, "    xor      ax, ax\n" );
            fprintf( fp, "    mov      al, dh\n" );
            fprintf( fp, "    call     print2digits\n" );
            fprintf( fp, "    call     prperiod\n" );  // period because what's next is in 1/100 of a second
    
            fprintf( fp, "    pop      dx\n" );
            fprintf( fp, "    xor      ax, ax\n" );
            fprintf( fp, "    mov      al, dl\n" );
            fprintf( fp, "    call     print2digits\n" );
            fprintf( fp, "    ret\n" );
            fprintf( fp, "printtime ENDP\n" );
        }

        //////////////////////////

        fprintf( fp, "printstring PROC NEAR\n" );
        fprintf( fp, "        push     ax\n" );
        fprintf( fp, "        push     bx\n" );
        fprintf( fp, "        push     cx\n" );
        fprintf( fp, "        push     dx\n" );
        fprintf( fp, "        push     di\n" );
        fprintf( fp, "        push     si\n" );
        fprintf( fp, "        mov      di, dx\n" );
        fprintf( fp, "  _psnext:\n" );
        fprintf( fp, "        mov      al, BYTE PTR ds: [ di ]\n" );
        fprintf( fp, "        cmp      al, 0\n" );
        fprintf( fp, "        je       _psdone\n" );
        fprintf( fp, "        mov      dx, ax\n" );
        fprintf( fp, "        mov      ah, dos_write_char\n" );
        fprintf( fp, "        int      21h\n" );
        fprintf( fp, "        inc      di\n" );
        fprintf( fp, "        jmp      _psnext\n" );
        fprintf( fp, "  _psdone:\n" );
        fprintf( fp, "        pop      si\n" );
        fprintf( fp, "        pop      di\n" );
        fprintf( fp, "        pop      dx\n" );
        fprintf( fp, "        pop      cx\n" );
        fprintf( fp, "        pop      bx\n" );
        fprintf( fp, "        pop      ax\n" );
        fprintf( fp, "        ret\n" );
        fprintf( fp, "printstring ENDP\n" );

        /////////////////////////

        fprintf( fp, "; print 2-digit number in ax with a potential leading zero\n" );
        fprintf( fp, "\n" );
        fprintf( fp, "print2digits PROC NEAR\n" );
        fprintf( fp, "    push     ax\n" );
        fprintf( fp, "    cmp      ax, 9\n" );
        fprintf( fp, "    jg       _pr_noleadingzero\n" );
        fprintf( fp, "    call     przero\n" );
        fprintf( fp, "  _pr_noleadingzero:\n" );
        fprintf( fp, "    pop      ax\n" );
        fprintf( fp, "    cmp      ax, 99\n" );
        fprintf( fp, "    jle      _pr_ok\n" );
        fprintf( fp, "    xor      ax, ax\n" );
        fprintf( fp, "  _pr_ok:\n" );
        fprintf( fp, "    call     printint\n" );
        fprintf( fp, "    ret\n" );
        fprintf( fp, "print2digits ENDP\n" );
        fprintf( fp, "\n" );

        ///////////////////////////////

        fprintf( fp, "; print the integer in ax\n" );
        fprintf( fp, "\n" );
        fprintf( fp, "printint PROC NEAR\n" );
        fprintf( fp, "     test     ah, 80h\n" );
        fprintf( fp, "     jz       _prpositive\n" );
        fprintf( fp, "     neg      ax                 ; just one instruction for complement + 1\n" );
        fprintf( fp, "     push     ax\n" );
        fprintf( fp, "     mov      dx, '-'\n" );
        fprintf( fp, "     mov      ah, dos_write_char\n" );
        fprintf( fp, "     int      21h\n" );
        fprintf( fp, "     pop      ax\n" );
        fprintf( fp, "  _prpositive:\n" );
        fprintf( fp, "     xor      cx, cx\n" );
        fprintf( fp, "     xor      dx, dx\n" );
        fprintf( fp, "     cmp      ax, 0\n" );
        fprintf( fp, "     je       _pr_just_zero\n" );
        fprintf( fp, "  _prlabel1:\n" );
        fprintf( fp, "     cmp      ax, 0\n" );
        fprintf( fp, "     je       _prprint1     \n" );
        fprintf( fp, "     mov      bx, 10       \n" );
        fprintf( fp, "     div      bx                 \n" );
        fprintf( fp, "     push     dx             \n" );
        fprintf( fp, "     inc      cx             \n" );
        fprintf( fp, "     xor      dx, dx\n" );
        fprintf( fp, "     jmp      _prlabel1\n" );
        fprintf( fp, "  _prprint1:\n" );
        fprintf( fp, "     cmp      cx, 0\n" );
        fprintf( fp, "     je       _prexit\n" );
        fprintf( fp, "     pop      dx\n" );
        fprintf( fp, "     add      dx, 48\n" );
        fprintf( fp, "     mov      ah, dos_write_char\n" );
        fprintf( fp, "     int      21h\n" );
        fprintf( fp, "     dec      cx\n" );
        fprintf( fp, "     jmp      _prprint1\n" );
        fprintf( fp, "  _pr_just_zero:\n" );
        fprintf( fp, "     call     przero\n" );
        fprintf( fp, "  _prexit:\n" );
        fprintf( fp, "     ret\n" );
        fprintf( fp, "printint ENDP\n" );
        fprintf( fp, "\n" );
        fprintf( fp, "prcolon PROC NEAR\n" );
        fprintf( fp, "     mov      dx, ':'\n" );
        fprintf( fp, "     mov      ah, dos_write_char\n" );
        fprintf( fp, "     int      21h\n" );
        fprintf( fp, "     ret\n" );
        fprintf( fp, "prcolon ENDP\n" );
        fprintf( fp, "prperiod PROC NEAR\n" );
        fprintf( fp, "     mov      dx, '.'\n" );
        fprintf( fp, "     mov      ah, dos_write_char\n" );
        fprintf( fp, "     int      21h\n" );
        fprintf( fp, "     ret\n" );
        fprintf( fp, "prperiod ENDP\n" );
        fprintf( fp, "przero PROC NEAR\n" );
        fprintf( fp, "     mov      dx, '0'\n" );
        fprintf( fp, "     mov      ah, dos_write_char\n" );
        fprintf( fp, "     int      21h\n" );
        fprintf( fp, "     ret\n" );
        fprintf( fp, "przero ENDP\n" );
        fprintf( fp, "printcrlf PROC NEAR\n" );
        fprintf( fp, "     mov      dx, offset crlfmsg\n" );
        fprintf( fp, "     call     printstring\n" );
        fprintf( fp, "     ret\n" );
        fprintf( fp, "printcrlf ENDP\n" );
        fprintf( fp, "\n" );
        fprintf( fp, "END\n" );
    }
    else if ( x86Win == g_AssemblyTarget )
    {
        // validate there is an active GOSUB before returning (or not)

        fprintf( fp, "  label_gosub_return:\n" );
        // fprintf( fp, "    dec      DWORD PTR [gosubCount]\n" );   // should we protect against return without gosub?
        // fprintf( fp, "    cmp      DWORD PTR [gosubCount], 0\n" );
        // fprintf( fp, "    jl       error_exit\n" );
        fprintf( fp, "    ret\n" );

        fprintf( fp, "  error_exit:\n" );
        fprintf( fp, "    push     offset errorString\n" );
        fprintf( fp, "    call     printf\n" );
        fprintf( fp, "    add      esp, 4\n" );
        fprintf( fp, "    jmp      leave_execution\n" );

        fprintf( fp, "  end_execution:\n" );
        if ( !g_Quiet )
        {
            fprintf( fp, "    push     offset stopString\n" );
            fprintf( fp, "    call     printf\n" );
            fprintf( fp, "    add      esp, 4\n" );
        }

        fprintf( fp, "  leave_execution:\n" );
        fprintf( fp, "    push     0\n" );
        fprintf( fp, "    call     exit\n" );
        fprintf( fp, "    ret\n" ); // should never get here...
        fprintf( fp, "main ENDP\n" );

        //////////////////////////////////////

        if ( timeReferenced )
        {
            fprintf( fp, "align 16\n" );
            fprintf( fp, "printCurrentTime PROC\n" );
            fprintf( fp, "    push     ebp\n" );
            fprintf( fp, "    mov      ebp, esp\n" );
            fprintf( fp, "    push     edi\n" );
            fprintf( fp, "    push     esi\n" );
            fprintf( fp, "    push     ecx\n" );
            fprintf( fp, "    push     offset currentTime\n" );
            fprintf( fp, "    call     GetLocalTime@4\n" );
            fprintf( fp, "    movsx    eax, WORD PTR [currentTime + 14]\n" );
            fprintf( fp, "    push     eax\n" );
            fprintf( fp, "    movsx    eax, WORD PTR [currentTime + 12]\n" );
            fprintf( fp, "    push     eax\n" );
            fprintf( fp, "    movsx    eax, WORD PTR [currentTime + 10]\n" );
            fprintf( fp, "    push     eax\n" );
            fprintf( fp, "    movsx    eax, WORD PTR [currentTime + 8]\n" );
            fprintf( fp, "    push     eax\n" );
            fprintf( fp, "    push     offset timeString\n" );
            fprintf( fp, "    call     printf\n" );
            fprintf( fp, "    add      esp, 32\n" );
            fprintf( fp, "    pop      ecx\n" );
            fprintf( fp, "    pop      esi\n" );
            fprintf( fp, "    pop      edi\n" );
            fprintf( fp, "    mov      esp, ebp\n" );
            fprintf( fp, "    pop      ebp\n" );
            fprintf( fp, "    ret\n" );
            fprintf( fp, "printCurrentTime ENDP\n" );
        }

        //////////////////////////////////////

        fprintf( fp, "align 16\n" );
        fprintf( fp, "printString PROC\n" );
        fprintf( fp, "    push     ebp\n" );
        fprintf( fp, "    mov      ebp, esp\n" );
        fprintf( fp, "    push     edi\n" );
        fprintf( fp, "    push     esi\n" );
        fprintf( fp, "    push     ecx\n" );
        fprintf( fp, "    push     eax\n" );
        fprintf( fp, "    push     offset strString\n" );
        fprintf( fp, "    call     printf\n" );
        fprintf( fp, "    add      esp, 8\n" );
        fprintf( fp, "    pop      ecx\n" );
        fprintf( fp, "    pop      esi\n" );
        fprintf( fp, "    pop      edi\n" );
        fprintf( fp, "    mov      esp, ebp\n" );
        fprintf( fp, "    pop      ebp\n" );
        fprintf( fp, "    ret\n" );
        fprintf( fp, "printString ENDP\n" );

        //////////////////////////////////////

        fprintf( fp, "align 16\n" );
        fprintf( fp, "printcrlf PROC\n" );
        fprintf( fp, "    push     ebp\n" );
        fprintf( fp, "    mov      ebp, esp\n" );
        fprintf( fp, "    push     edi\n" );
        fprintf( fp, "    push     esi\n" );
        fprintf( fp, "    push     ecx\n" );
        fprintf( fp, "    push     offset newlineString\n" );
        fprintf( fp, "    call     printf\n" );
        fprintf( fp, "    add      esp, 4\n" );
        fprintf( fp, "    pop      ecx\n" );
        fprintf( fp, "    pop      esi\n" );
        fprintf( fp, "    pop      edi\n" );
        fprintf( fp, "    mov      esp, ebp\n" );
        fprintf( fp, "    pop      ebp\n" );
        fprintf( fp, "    ret\n" );
        fprintf( fp, "printcrlf ENDP\n" );

        //////////////////////////////////////

        fprintf( fp, "align 16\n" );
        fprintf( fp, "printInt PROC\n" );
        fprintf( fp, "    push     ebp\n" );
        fprintf( fp, "    mov      ebp, esp\n" );
        fprintf( fp, "    push     edi\n" );
        fprintf( fp, "    push     esi\n" );
        fprintf( fp, "    push     ecx\n" );
        fprintf( fp, "    push     eax\n" );
        fprintf( fp, "    push     offset intString\n" );
        fprintf( fp, "    call     printf\n" );
        fprintf( fp, "    add      esp, 8\n" );
        fprintf( fp, "    pop      ecx\n" );
        fprintf( fp, "    pop      esi\n" );
        fprintf( fp, "    pop      edi\n" );
        fprintf( fp, "    mov      esp, ebp\n" );
        fprintf( fp, "    pop      ebp\n" );
        fprintf( fp, "    ret\n" );
        fprintf( fp, "printInt ENDP\n" );

        //////////////////////////////////////

        if ( elapReferenced )
        {
            fprintf( fp, "align 16\n" );
            fprintf( fp, "printElapTime PROC\n" );
            fprintf( fp, "    push     ebp\n" );
            fprintf( fp, "    mov      ebp, esp\n" );
            fprintf( fp, "    push     edi\n" );
            fprintf( fp, "    push     esi\n" );
            fprintf( fp, "    push     ecx\n" );
            fprintf( fp, "    push     offset currentTicks\n" );
            fprintf( fp, "    call     QueryPerformanceCounter@4\n" );
            fprintf( fp, "    mov      eax, DWORD PTR [currentTicks]\n" );
            fprintf( fp, "    mov      edx, DWORD PTR [currentTicks + 4]\n" );
            fprintf( fp, "    mov      ebx, DWORD PTR [startTicks]\n" );
            fprintf( fp, "    mov      ecx, DWORD PTR [startTicks + 4]\n" );
            fprintf( fp, "    sub      eax, ebx\n" );
            fprintf( fp, "    sbb      edx, ecx\n" );
            fprintf( fp, "    idiv     DWORD PTR [perfFrequency]\n" );
            fprintf( fp, "    xor      edx, edx\n" );
            fprintf( fp, "    mov      ecx, 1000\n" );
            fprintf( fp, "    idiv     ecx\n" );
            fprintf( fp, "    push     eax\n" );
            fprintf( fp, "    push     offset elapString\n" );
            fprintf( fp, "    call     printf\n" );
            fprintf( fp, "    add      esp, 8\n" );
            fprintf( fp, "    pop      ecx\n" );
            fprintf( fp, "    pop      esi\n" );
            fprintf( fp, "    pop      edi\n" );
            fprintf( fp, "    mov      esp, ebp\n" );
            fprintf( fp, "    pop      ebp\n" );
            fprintf( fp, "    ret\n" );
            fprintf( fp, "printElapTime ENDP\n" );
        }

        // end of the code segment and program

        fprintf( fp, "code_segment ENDS\n" );
        fprintf( fp, "END\n" );
    }
    else if ( riscv64 == g_AssemblyTarget )
    {
        fprintf( fp, "label_gosub:\n" );
        RiscVPush( fp, "ra" );
        fprintf( fp, "    jalr     ra, a0\n" );

        /**************************************************************************/

        fprintf( fp, "label_gosub_return:\n" );
        RiscVPop( fp, "ra" );
        fprintf( fp, "    jr       ra\n" );

        /**************************************************************************/

        fprintf( fp, "error_exit:\n" );
        fprintf( fp, "    lla      a0, errorString\n" );
        fprintf( fp, "    jal      rvos_print_text\n" );
        fprintf( fp, "    j        leave_execution\n" );

        /**************************************************************************/

        fprintf( fp, "print_crlf:\n" );
        fprintf( fp, "    addi     sp, sp, -32\n" );
        fprintf( fp, "    sd       ra, 16(sp)\n" );

        fprintf( fp, "    lla      a0, newlineString\n" );
        fprintf( fp, "    jal      rvos_print_text\n" );

        fprintf( fp, "    ld       ra, 16(sp)\n" );
        fprintf( fp, "    addi     sp, sp, 32\n" );
        fprintf( fp, "    jr       ra\n" );

        /**************************************************************************/

        fprintf( fp, "print_int:\n" );
        fprintf( fp, "    addi     sp, sp, -32\n" );
        fprintf( fp, "    sd       ra, 16(sp)\n" );

        fprintf( fp, "    lla      a1, print_buffer\n" );
        fprintf( fp, "    li       a2, 10\n" );
        fprintf( fp, "    jal      _my_lltoa\n" );
        fprintf( fp, "    lla      a0, print_buffer\n" );
        fprintf( fp, "    jal      rvos_print_text\n" );

        fprintf( fp, "    ld       ra, 16(sp)\n" );
        fprintf( fp, "    addi     sp, sp, 32\n" );
        fprintf( fp, "    jr       ra\n" );

        /**************************************************************************/

        if ( timeReferenced )
        {
            fprintf( fp, "print_time:\n" );
            fprintf( fp, "    addi     sp, sp, -32\n" );
            fprintf( fp, "    sd       ra, 16(sp)\n" );
    
            fprintf( fp, "    lla      a0, print_buffer\n" );
            fprintf( fp, "    jal      rvos_get_datetime\n" );
            fprintf( fp, "    lla      a0, print_buffer\n" );
            fprintf( fp, "    jal      rvos_print_text\n" );
    
            fprintf( fp, "    ld       ra, 16(sp)\n" );
            fprintf( fp, "    addi     sp, sp, 32\n" );
            fprintf( fp, "    jr       ra\n" );
        }

        /**************************************************************************/
     
        if ( FindVariable( varmap, "av%" ) )
        {
            fprintf( fp, "a_to_uint64:\n" );
            fprintf( fp, "        addi    sp, sp, -128\n" );
            fprintf( fp, "        sd      ra, 16(sp)\n" );
            fprintf( fp, "        sd      s0, 24(sp)\n" );
            fprintf( fp, "        sd      s1, 32(sp)\n" );
            fprintf( fp, "        sd      s2, 40(sp)\n" );
            fprintf( fp, "        sd      s3, 48(sp)\n" );
            fprintf( fp, "        sd      s4, 56(sp)\n" );
            fprintf( fp, "        sd      s5, 64(sp)\n" );
            fprintf( fp, "        sd      s6, 72(sp)\n" );
            fprintf( fp, "        sd      s7, 80(sp)\n" );
            fprintf( fp, "        sd      s8, 88(sp)\n" );
            fprintf( fp, "        sd      s9, 96(sp)\n" );
            fprintf( fp, "        sd      s10, 104(sp)\n" );
            fprintf( fp, "        sd      s11, 112(sp)\n" );
            fprintf( fp, "        li      s0, 0                # running total in s0\n" );
            fprintf( fp, "        li      s1, 0                # offset of next char in s1\n" );
            fprintf( fp, "        mv      s2, a0\n" );
            fprintf( fp, "        li      s3, ' '\n" );
            fprintf( fp, "        li      s4, '0'\n" );
            fprintf( fp, "        li      s5, '9' + 1\n" );
            fprintf( fp, "        li      s6, 10\n" );
            fprintf( fp, "  .a_to_uint64_skip_spaces:\n" );
            fprintf( fp, "        lbu     t0, (s2)\n" );
            fprintf( fp, "        bne     t0, s3, .a_to_uint64_next\n" );
            fprintf( fp, "        addi    s2, s2, 1\n" );
            fprintf( fp, "        j       .a_to_uint64_skip_spaces\n" );
            fprintf( fp, "  .a_to_uint64_next:\n" );
            fprintf( fp, "        lbu     t0, (s2)\n" );
            fprintf( fp, "        blt     t0, s4, .a_to_uint64_done\n" );
            fprintf( fp, "        bge     t0, s5, .a_to_uint64_done\n" );
            fprintf( fp, "        mul     s0, s0, s6           # multiply running total by 10\n" );
            fprintf( fp, "        sub     t0, t0, s4\n" );
            fprintf( fp, "        add     s0, s0, t0           # add the next digit\n" );
            fprintf( fp, "        addi    s2, s2, 1            # advance the string pointer\n" );
            fprintf( fp, "        j       .a_to_uint64_next\n" );
            fprintf( fp, "  .a_to_uint64_done:\n" );
            fprintf( fp, "        mv      a0, s0\n" );
            fprintf( fp, "        ld      ra, 16(sp)\n" );
            fprintf( fp, "        ld      s0, 24(sp)\n" );
            fprintf( fp, "        ld      s1, 32(sp)\n" );
            fprintf( fp, "        ld      s2, 40(sp)\n" );
            fprintf( fp, "        ld      s3, 48(sp)\n" );
            fprintf( fp, "        ld      s4, 56(sp)\n" );
            fprintf( fp, "        ld      s5, 64(sp)\n" );
            fprintf( fp, "        ld      s6, 72(sp)\n" );
            fprintf( fp, "        ld      s7, 80(sp)\n" );
            fprintf( fp, "        ld      s8, 88(sp)\n" );
            fprintf( fp, "        ld      s9, 96(sp)\n" );
            fprintf( fp, "        ld      s10, 104(sp)\n" );
            fprintf( fp, "        ld      s11, 112(sp)\n" );
            fprintf( fp, "        addi    sp, sp, 128\n" );
            fprintf( fp, "        jr      ra\n" );
        }

        /**************************************************************************/

        if ( elapReferenced )
        {
            fprintf( fp, "print_elap:\n" );
            fprintf( fp, "    addi     sp, sp, -32\n" );
            fprintf( fp, "    sd       ra, 16(sp)\n" );
    
            fprintf( fp, ".ifdef MAIXDUINO\n" );
            fprintf( fp, "    rdcycle  a0  # rdtime doesn't work on the K210 CPU\n" );
            fprintf( fp, ".else\n" );
            fprintf( fp, "    rdtime   a0  # time in nanoseconds\n" );
            fprintf( fp, ".endif\n" );
            fprintf( fp, "    lla      t0, startTicks\n" );
            fprintf( fp, "    ld       t0, (t0)\n" );
            fprintf( fp, "    sub      a0, a0, t0\n" );
            fprintf( fp, ".ifdef MAIXDUINO\n" );
            fprintf( fp, "    li       t0, 400  # the k210 runs at 400Mhz and rdtime doesn't work\n" );
            fprintf( fp, ".else\n" );
            fprintf( fp, "    li       t0, 1000 # when running on an emulator with ns as the source\n" );
            fprintf( fp, ".endif\n" );
            fprintf( fp, "    div      a0, a0, t0\n" );
            fprintf( fp, "    lla      a1, print_buffer\n" );
            fprintf( fp, "    li       a2, 10\n" );
            fprintf( fp, "    jal      _my_lltoa\n" );
            fprintf( fp, "    lla      a0, print_buffer\n" );
            fprintf( fp, "    jal      rvos_print_text\n" );
            fprintf( fp, "    lla      a0, elapString\n" );
            fprintf( fp, "    jal      rvos_print_text\n" );
    
            fprintf( fp, "    ld       ra, 16(sp)\n" );
            fprintf( fp, "    addi     sp, sp, 32\n" );
            fprintf( fp, "    jr       ra\n" );
        }

        /**************************************************************************/

        fprintf( fp, "_my_lltoa:\n" );
        fprintf( fp, "    li       t1, 9\n" );
        fprintf( fp, "    bne      a0, zero, .my_lltoa_not_zero\n" );
        fprintf( fp, "    li       t0, '0'\n" );
        fprintf( fp, "    sb       t0, 0(a1)\n" );
        fprintf( fp, "    sb       zero, 1(a1)\n" );
        fprintf( fp, "    j        .my_lltoa_exit\n" );
        fprintf( fp, "  .my_lltoa_not_zero:\n" );
        fprintf( fp, "    li       t2, 0           # offset into the string\n" );
        fprintf( fp, "    mv       t6, zero        # default to unsigned\n" );
        fprintf( fp, "    li       t0, 10          # negative numbers only exist for base 10\n" );
        fprintf( fp, "    bne      a2, t0, .my_lltoa_digit_loop\n" );
        fprintf( fp, "    li       t0, 0x8000000000000000\n" );
        fprintf( fp, "    and      t0, a0, t0\n" );
        fprintf( fp, "    beq      t0, zero, .my_lltoa_digit_loop\n" );
        fprintf( fp, "    li       t6, 1           # it's negative\n" );
        fprintf( fp, "    neg      a0, a0          # this is just sub a0, zero, a0\n" );
        fprintf( fp, "  .my_lltoa_digit_loop:\n" );
        fprintf( fp, "    beq      a0, zero, .my_lltoa_digits_done\n" );
        fprintf( fp, "    rem      t0, a0, a2\n" );
        fprintf( fp, "    bgt      t0, t1, .my_lltoa_more_than_nine\n" );
        fprintf( fp, "    addi     t0, t0, '0'\n" );
        fprintf( fp, "    j       .my_lltoa_after_base\n" );
        fprintf( fp, "  .my_lltoa_more_than_nine:\n" );
        fprintf( fp, "    addi     t0, t0, 'a' - 10\n" );
        fprintf( fp, "  .my_lltoa_after_base:\n" );
        fprintf( fp, "    add      t3, a1, t2\n" );
        fprintf( fp, "    sb       t0, 0(t3)\n" );
        fprintf( fp, "    addi     t2, t2, 1\n" );
        fprintf( fp, "    div      a0, a0, a2\n" );
        fprintf( fp, "    j        .my_lltoa_digit_loop\n" );
        fprintf( fp, "  .my_lltoa_digits_done:\n" );
        fprintf( fp, "    beq      t6, zero, .my_lltoa_no_minus\n" );
        fprintf( fp, "    li       t0, '-'\n" );
        fprintf( fp, "    add      t3, a1, t2\n" );
        fprintf( fp, "    sb       t0, 0(t3)\n" );
        fprintf( fp, "    addi     t2, t2, 1\n" );
        fprintf( fp, "  .my_lltoa_no_minus:\n" );
        fprintf( fp, "    add      t3, a1, t2      # null-terminate the string\n" );
        fprintf( fp, "    sb       zero, 0(t3)\n" );
        fprintf( fp, "    mv       t4, a1          # reverse the string. t4 = left\n" );
        fprintf( fp, "    add      t5, a1, t2      # t5 = right\n" );
        fprintf( fp, "    addi     t5, t5, -1\n" );
        fprintf( fp, "  .my_lltoa_reverse_next:\n" );
        fprintf( fp, "    bge      t4, t5, .my_lltoa_exit\n" );
        fprintf( fp, "    lbu      t0, (t4)\n" );
        fprintf( fp, "    lbu      t1, (t5)\n" );
        fprintf( fp, "    sb       t0, (t5)\n" );
        fprintf( fp, "    sb       t1, (t4)\n" );
        fprintf( fp, "    addi     t4, t4, 1\n" );
        fprintf( fp, "    addi     t5, t5, -1\n" );
        fprintf( fp, "    j       .my_lltoa_reverse_next\n" );
        fprintf( fp, "  .my_lltoa_exit:\n" );
        fprintf( fp, "    jr       ra\n" );

        /**************************************************************************/

        fprintf( fp, "end_execution:\n" );
        if ( !g_Quiet )
        {
            fprintf( fp, "    lla      a0, stopString\n" );
            fprintf( fp, "    jal      rvos_print_text\n" );
        }
        fprintf( fp, "    j        leave_execution\n" );
        
        /**************************************************************************/

        fprintf( fp, "leave_execution:\n" );
        fprintf( fp, "    mv       a0, zero\n" );
        fprintf( fp, "    ld       ra, 16(sp)\n" );
        fprintf( fp, "    ld       s0, 24(sp)\n" );
        fprintf( fp, "    ld       s1, 32(sp)\n" );
        fprintf( fp, "    ld       s2, 40(sp)\n" );
        fprintf( fp, "    ld       s3, 48(sp)\n" );
        fprintf( fp, "    ld       s4, 56(sp)\n" );
        fprintf( fp, "    ld       s5, 64(sp)\n" );
        fprintf( fp, "    ld       s6, 72(sp)\n" );
        fprintf( fp, "    ld       s7, 80(sp)\n" );
        fprintf( fp, "    ld       s8, 88(sp)\n" );
        fprintf( fp, "    ld       s9, 96(sp)\n" );
        fprintf( fp, "    ld       s10, 104(sp)\n" );
        fprintf( fp, "    ld       s11, 112(sp)\n" );
        fprintf( fp, "    addi     sp, sp, 128\n" );
        fprintf( fp, "    jr       ra\n" );
        fprintf( fp, "    .cfi_endproc\n" );
    }
    else if ( m68kCPM == g_AssemblyTarget )
    {
        fprintf( fp, "    bra      end_execution\n" );

        fprintf( fp, "label_gosub_return:\n" );
        fprintf( fp, "    rts\n" );

        fprintf( fp, "print_int:\n" );
        fprintf( fp, "    move.l   d0, -(a7)\n" );
        fprintf( fp, "    lea      intString, a0\n" );
        fprintf( fp, "    move.l   a0, -(a7)\n" );
        fprintf( fp, "    jsr      _printf\n" );
        fprintf( fp, "    add.l    #8, a7\n" );
        fprintf( fp, "    rts\n" );

        fprintf( fp, "print_crlf:\n" );
        fprintf( fp, "    lea      newlineString, a0\n" );
        fprintf( fp, "    move.l   a0, -(a7)\n" );
        fprintf( fp, "    jsr      _printf\n" );
        fprintf( fp, "    add.l     #4, a7\n" );
        fprintf( fp, "    rts\n" );

        fprintf( fp, "error_exit:\n" );
        fprintf( fp, "    lea      errorString, a0\n" );
        fprintf( fp, "    move.l   a0, -(a7)\n" );
        fprintf( fp, "    jsr      _printf\n" );
        fprintf( fp, "    add.l    #4, a7\n" );
        fprintf( fp, "    bra      leave_execution\n" );

        fprintf( fp, "end_execution:\n" );
        if ( !g_Quiet )
        {
            fprintf( fp, "    lea      stopString, a0\n" );
            fprintf( fp, "    move.l   a0, -(a7)\n" );
            fprintf( fp, "    jsr      _printf\n" );
            fprintf( fp, "    add.l    #4, a7\n" );
        }

        fprintf( fp, "leave_execution:\n" );
        fprintf( fp, "    rts\n" );

        //////////////////

        fprintf( fp, "\n" );
    }
    else if ( oiOS == g_AssemblyTarget )
    {
        fprintf( fp, "    jmp      leave_execution\n" );

        fprintf( fp, "label_gosub_return:\n" );
        fprintf( fp, "    retnf\n" );

        fprintf( fp, "error_exit:\n" );
        fprintf( fp, "    ldi      rarg1, errorString\n" );
        fprintf( fp, "    syscall  1\n" );
        fprintf( fp, "    jmp      leave_execution\n" );

        fprintf( fp, "end_execution:\n" );
        if ( !g_Quiet )
        {
            fprintf( fp, "    ldi      rarg1, stopString\n" );
            fprintf( fp, "    syscall  1\n" );
        }

        fprintf( fp, "leave_execution:\n" );
        fprintf( fp, "    ldi      rarg1, 0\n" );
        fprintf( fp, "    syscall  0\n" );

        //////////////////

        fprintf( fp, "\n" );
        fprintf( fp, ".codeend\n" );
    }

    if ( !g_Quiet )
        printf( "created assembler file: %s, use registers: %s, expression optimization: %s\n",
                outputfile, YesNo( useRegistersInASM ), YesNo( g_ExpressionOptimization ) );
} //GenerateASM

#endif //BA_ENABLE_COMPILER

void ParseInputFile( const char * inputfile )
{
    CFile fileInput( fopen( inputfile, "rb" ) );
    if ( NULL == fileInput.get() )
    {
        printf( "can't open input file %s\n", inputfile );
        Usage();
    }

    if ( !g_Quiet )
        printf( "parsing input file %s\n", inputfile );

    long filelen = portable_filelen( fileInput.get() );
    vector<char> input( filelen + 1 );
    size_t lread = fread( input.data(), 1, filelen, fileInput.get() );
    if ( 0 == lread )
    {
        printf( "unable to read input file\n" );
        Usage();
    }

    fileInput.Close();
    input.data()[ lread ] = 0;

    char * pbuf = input.data();
    char * pbeyond = pbuf + lread;
    char line[ 300 ];
    const int MaxLineLen = _countof( line ) - 1;
    int fileLine = 0;
    int prevLineNum = 0;

    while ( pbuf < pbeyond )
    {
        int len = 0;
        while ( ( pbuf < pbeyond ) && ( ( *pbuf != 10 ) && ( *pbuf != 13 ) ) && ( len < MaxLineLen ) )
            line[ len++ ] = *pbuf++;

        while ( ( pbuf < pbeyond ) && ( *pbuf == 10 || *pbuf == 13 ) )
            pbuf++;

        fileLine++;

        if ( 0 != len )
        {
            line[ len ] = 0;
            if ( EnableTracing && g_Tracing )
                printf( "read line %d: %s\n", fileLine, line );

            int lineNum = readNum( line );
            if ( -1 == lineNum )
                Fail( "expected a line number", fileLine, 0, line );

            if ( lineNum <= prevLineNum )
                Fail( "line numbers are out of order", fileLine, 0, line );

            prevLineNum = lineNum;

            const char * pline = pastNum( line );
            pline = pastWhite( pline );

            int tokenLen = 0;
            Token token = readToken( pline, tokenLen );

            if ( Token_INVALID == token )
                Fail( "invalid token", fileLine, 0, line );

            LineOfCode loc( lineNum, line );
            g_linesOfCode.push_back( loc );

            TokenValue tokenValue( token );
            vector<TokenValue> & lineTokens = g_linesOfCode[ g_linesOfCode.size() - 1 ].tokenValues;

            if ( isTokenStatement( token ) )
            {
                pline = ParseStatements( token, lineTokens, pline, line, fileLine );
            }
            else if ( Token_FOR == token )
            {
                pline = pastWhite( pline + tokenLen );
                token = readToken( pline, tokenLen );
                if ( Token_VARIABLE == token )
                {
                    tokenValue.strValue.insert( 0, pline, tokenLen );
                    makelower( tokenValue.strValue );
                    lineTokens.push_back( tokenValue );
                }
                else
                    Fail( "expected a variable after FOR statement", fileLine, 1 + pline - line, line );

                pline = pastWhite( pline + tokenLen );
                token = readToken( pline, tokenLen );
                if ( Token_EQ != token )
                    Fail( "expected an equal sign in FOR statement", fileLine, 1 + pline - line, line );

                pline = pastWhite( pline + tokenLen );
                pline = ParseExpression( lineTokens, pline, line, fileLine );

                pline = pastWhite( pline );
                token = readToken( pline, tokenLen );
                if ( Token_TO != token )
                    Fail( "expected a TO in FOR statement", fileLine, 1 + pline - line, line );

                pline = pastWhite( pline + tokenLen );
                pline = ParseExpression( lineTokens, pline, line, fileLine );
            }
            else if ( Token_IF == token )
            {
                lineTokens.push_back( tokenValue );
                pline = pastWhite( pline + tokenLen );
                pline = ParseExpression( lineTokens, pline, line, fileLine );
                if ( Token_EXPRESSION == lineTokens[ lineTokens.size() - 1 ].token )
                    Fail( "expected an expression after an IF statement", fileLine, 1 + pline - line, line );

                pline = pastWhite( pline );
                token = readToken( pline, tokenLen );
                if ( Token_THEN == token )
                {
                    // THEN is optional in the source code but manditory in p-code

                    pline = pastWhite( pline + tokenLen );
                    token = readToken( pline, tokenLen );
                }

                tokenValue.Clear();
                tokenValue.token = Token_THEN;
                size_t thenOffset = lineTokens.size();
                lineTokens.push_back( tokenValue );

                pline = ParseStatements( token, lineTokens, pline, line, fileLine );

                if ( Token_THEN == lineTokens[ lineTokens.size() - 1 ].token )
                    Fail( "expected a statement after a THEN", fileLine, 1 + pline - line, line );

                pline = pastWhite( pline );
                token = readToken( pline, tokenLen );
                if ( Token_ELSE == token )
                {
                    tokenValue.Clear();
                    tokenValue.token = token;
                    lineTokens.push_back( tokenValue );
                    lineTokens[ thenOffset ].value = (int) ( lineTokens.size() - thenOffset - 1 );
                    
                    pline = pastWhite( pline + tokenLen );
                    token = readToken( pline, tokenLen );
                    pline = ParseStatements( token, lineTokens, pline, line, fileLine );
                    if ( Token_ELSE == lineTokens[ lineTokens.size() - 1 ].token )
                        Fail( "expected a statement after an ELSE", fileLine, 1 + pline - line, line );
                }
            }
            else if ( Token_REM == token )
            {
                // can't just throw out REM statements yet because a goto/gosub may reference them

                lineTokens.push_back( tokenValue );
            }
            else if ( Token_TRON == token )
            {
                lineTokens.push_back( tokenValue );
            }
            else if ( Token_TROFF == token )
            {
                lineTokens.push_back( tokenValue );
            }
            else if ( Token_NEXT == token )
            {
                pline = pastWhite( pline + tokenLen );
                token = readToken( pline, tokenLen );
                if ( Token_VARIABLE == token )
                {
                    tokenValue.strValue.insert( 0, pline, tokenLen );
                    makelower( tokenValue.strValue );
                    lineTokens.push_back( tokenValue );
                }
                else
                    Fail( "expected a variable with NEXT statement", fileLine, 1 + pline - line, line );
            }
            else if ( Token_DIM == token )
            {
                pline = pastWhite( pline + tokenLen );
                token = readToken( pline, tokenLen );
                if ( Token_VARIABLE == token )
                {
                    tokenValue.strValue.insert( 0, pline, tokenLen );
                    makelower( tokenValue.strValue );
                    pline = pastWhite( pline + tokenLen );
                    token = readToken( pline, tokenLen );

                    if ( Token_OPENPAREN != token )
                        Fail( "expected open paren for DIM statment", fileLine, 1 + pline - line, line );

                    pline = pastWhite( pline + tokenLen );
                    token = readToken( pline, tokenLen );

                    if ( Token_CONSTANT != token )
                        Fail( "expected a numeric constant first dimension", fileLine, 1 + pline - line, line );

                    tokenValue.dims[ 0 ] = atoi( pline );
                    if ( tokenValue.dims[ 0 ] <= 0 )
                        Fail( "array dimension isn't positive", fileLine, 1 + pline - line, line );

                    pline = pastWhite( pline + tokenLen );
                    token = readToken( pline, tokenLen );
                    tokenValue.dimensions = 1;

                    if ( Token_COMMA == token )
                    {
                        pline = pastWhite( pline + tokenLen );
                        token = readToken( pline, tokenLen );

                        if ( Token_CONSTANT != token )
                            Fail( "expected a numeric constant second dimension", fileLine, 1 + pline - line, line );

                        tokenValue.dims[ 1 ] = atoi( pline );
                        if ( tokenValue.dims[ 1 ] <= 0 )
                            Fail( "array dimension isn't positive", fileLine, 1 + pline - line, line );

                        pline = pastWhite( pline + tokenLen );
                        token = readToken( pline, tokenLen );
                        tokenValue.dimensions = 2;
                    }

                    if ( Token_CLOSEPAREN == token )
                        lineTokens.push_back( tokenValue );
                    else
                        Fail( "expected close paren or next dimension", fileLine, 1 + pline - line, line );
                }
                else
                    Fail( "expected a variable after DIM", fileLine, 1 + pline - line, line );
            }
        }
    }
} //ParseInputFile

void InterpretCode( map<string, Variable> & varmap, bool showExecutionTime )
{
    // The cost of this level of indirection is 15% in NDEBUG interpreter performance.
    // So it's not worth it to control the behavior at runtime except for testing / debug

    #ifdef DEBUG
        typedef int ( * EvaluateProc )( int & iToken, vector<TokenValue> const & vals );
        EvaluateProc evalProc = EvaluateLogicalExpression;
        if ( g_ExpressionOptimization )
            evalProc = EvaluateExpressionOptimized;
    #else
        #define evalProc EvaluateExpressionOptimized
    #endif

    static Stack<ForGosubItem> forGosubStack;
    bool basicTracing = false;
    g_pc = 0;  // program counter

    #ifdef BA_ENABLE_INTERPRETER_EXECUTION_TIME
        int pcPrevious = 0;
        g_linesOfCode[ 0 ].timesExecuted--; // avoid off by 1 on first iteration of loop
        uint64_t timePrevious = __rdtsc();
    #endif

#ifdef WATCOM
    uint32_t timeBegin = DosTimeInMS();
#else
    steady_clock::time_point timeBegin = steady_clock::now();
#endif

    do
    {
        label_next_pc:

        // The overhead of tracking execution time makes programs run ~ 2x slower.
        // As a result, about half of the times shown in the report are just overhead of tracking the data.
        // Look for relative differences when determining where to optimize.

        #ifdef BA_ENABLE_INTERPRETER_EXECUTION_TIME
            if ( showExecutionTime )
            {
                // __rdtsc makes the app run 2x slower.
                // steady_clock makes the app run 5x slower. I'm probably doing something wrong?
                // The M1 mac implementation has nearly 0 overhead.

                uint64_t timeNow = __rdtsc();
                g_linesOfCode[ pcPrevious ].duration += ( timeNow - timePrevious );
                g_linesOfCode[ pcPrevious ].timesExecuted++;
                timePrevious = timeNow;
                pcPrevious = g_pc;
            }
        #endif //BA_ENABLE_INTERPRETER_EXECUTION_TIME

        vector<TokenValue> const & vals = g_linesOfCode[ g_pc ].tokenValues;
        Token token = g_linesOfCode[ g_pc ].firstToken;
        int t = 0;

        if ( EnableTracing && basicTracing )
            printf( "executing line %d\n", g_lineno );

        do
        {
            if ( EnableTracing && g_Tracing )
                printf( "executing pc %d line number %d ==> %s\n", g_pc, g_lineno, g_linesOfCode[ g_pc ].sourceCode.c_str() );

            // MSC doesn't support goto jump tables like g++. MSC will optimize switch statements if the default has
            // an __assume(false), but the generated code for the lookup table is complex and slower than if/else if...
            // If more tokens are added to the list below then at some point the lookup table will be faster.
            // Note that g++ and clang++ generate slower code overall than MSC, and use of a goto jump table
            // with those compilers is still slower than MSC without a jump table.
            // A table of function pointers is much slower.
            // The order of the tokens is based on usage in the ttt app.

            if ( Token_IF == token )
            {
                t++;
                int val = evalProc( t, vals );
                assert( Token_THEN == vals[ t ].token );

                if ( val )
                    t++;
                else
                {
                    // offset of ELSE token from THEN or 0 if there is no ELSE

                    if ( 0 == vals[ t ].value )
                    {
                        g_pc++;
                        goto label_next_pc;
                    }
                    else
                    {
                        int elseOffset = vals[ t ].value;
                        assert( Token_ELSE == vals[ t + elseOffset ].token );
                        t += ( elseOffset + 1 );
                    }
                }
            }
            else if ( Token_VARIABLE == token )
            {
                Variable *pvar = vals[ t ].pVariable;
                assert( pvar && "variable hasn't been declared or cached" );

                t++;

                if ( Token_OPENPAREN == vals[ t ].token )
                {
                    if ( 0 == pvar->dimensions )
                        RuntimeFail( "variable used as array isn't an array", g_lineno );

                    t++;
                    int arrayIndex = evalProc( t, vals );

                    if ( RangeCheckArrays && FailsRangeCheck( arrayIndex, pvar->dims[ 0 ] ) )
                        RuntimeFail( "array offset out of bounds", g_lineno );

                    if ( Token_COMMA == vals[ t ].token )
                    {
                        t++;

                        if ( 2 != pvar->dimensions )
                            RuntimeFail( "single-dimensional array used with 2 dimensions", g_lineno );

                        int indexB = evalProc( t, vals );

                        if ( RangeCheckArrays && FailsRangeCheck( indexB, pvar->dims[ 1 ] ) )
                            RuntimeFail( "second dimension array offset out of bounds", g_lineno );

                        arrayIndex *= pvar->dims[ 1 ];
                        arrayIndex +=  indexB;
                    }

                    assert( Token_CLOSEPAREN == vals[ t ].token );
                    assert( Token_EQ == vals[ t + 1 ].token );

                    t += 2; // past ) and =
                    int val = evalProc( t, vals );

                    pvar->array[ arrayIndex ] = val;
                }
                else
                {
                    assert( Token_EQ == vals[ t ].token );

                    t++;
                    int val = evalProc( t, vals );

                    if ( RangeCheckArrays && ( 0 != pvar->dimensions ) )
                        RuntimeFail( "array used as if it's a scalar", g_lineno );

                    pvar->value = val;
                }

                // have we consumed all tokens in the instruction?

                if ( t == vals.size() )
                {
                    g_pc++;
                    goto label_next_pc;
                }
            }
            else if ( Token_GOTO == token )
            {
                g_pc = vals[ t ].value;
                goto label_next_pc;
            }
            else if ( Token_ATOMIC == token )
            {
                Variable * pvar = vals[ t + 1 ].pVariable;
                assert( pvar && "atomic variable hasn't been declared or cached" );

                if ( Token_INC == vals[ t + 1 ].token )
                    pvar->value++;
                else
                {
                    assert( Token_DEC == vals[ t + 1 ].token );
                    pvar->value--;
                }

                g_pc++;
                goto label_next_pc;
            }
            else if ( Token_GOSUB == token )
            {
                ForGosubItem fgi( false, g_pc + 1 );
                forGosubStack.push( fgi );

                g_pc = vals[ t ].value;
                goto label_next_pc;
            }
            else if ( Token_RETURN == token )
            {
                do 
                {
                    if ( 0 == forGosubStack.size() )
                        RuntimeFail( "return without gosub", g_lineno );

                    // remove any active FOR items to get to the next GOSUB item and return

                    ForGosubItem & item = forGosubStack.top();
                    forGosubStack.pop();
                    if ( !item.isFor )
                    {
                        g_pc = item.pcReturn;
                        break;
                    }
                } while( true );

                goto label_next_pc;
            }
            else if ( Token_FOR == token )
            {
                bool continuation = false;

                if ( 1 == vals[ 0 ].extra )
                {
                    int teval = t + 1;
                    int startValue = evalProc( teval, vals );

                    int tokens = vals[ t + 1 ].value;
                    int tokenStart = t + 1 + tokens;
                    int endValue = evalProc( tokenStart, vals );
                    Variable * pvar = vals[ 0 ].pVariable;
                    pvar->value = endValue;

                    vector<TokenValue> const & vals1 = g_linesOfCode[ g_pc + 1 ].tokenValues;
                    int fillValue = vals1[ 7 ].value;

                    if ( EnableTracing && g_Tracing )
                        printf( "optimizing for loop with memset start %d end %d fill %d\n", startValue, endValue, fillValue );

                    for ( int x = startValue; x <= endValue; x++ )
                        vals1[ 0 ].pVariable->array[ x ] = fillValue;

                    g_pc += 2; // point at next
                }
                else
                {
                    if  ( forGosubStack.size() >  0 )
                    {
                        ForGosubItem & item = forGosubStack.top();
                        if ( item.isFor && item.pcReturn == g_pc )
                            continuation = true;
                    }
    
                    Variable * pvar = vals[ 0 ].pVariable;
    
                    if ( continuation )
                        pvar->value += 1;
                    else
                    {
                        int teval = t + 1;
                        pvar->value = evalProc( teval, vals );
                    }
    
                    int tokens = vals[ t + 1 ].value;
                    int tokenStart = t + 1 + tokens;
                    int endValue = evalProc( tokenStart, vals );
    
                    if ( EnableTracing && g_Tracing )
                        printf( "for loop for variable %s current %d, end value %d\n", vals[ 0 ].strValue.c_str(), pvar->value, endValue );
    
                    if ( !continuation )
                    {
                        ForGosubItem item( true, g_pc );
                        forGosubStack.push( item );
                    }
    
                    if ( pvar->value > endValue )
                    {
                        // find NEXT and set g_pc to one beyond it.
    
                        forGosubStack.pop();
    
                        do
                        {
                            g_pc++;
    
                            if ( g_pc >= g_linesOfCode.size() )
                                RuntimeFail( "no matching NEXT found for FOR", g_lineno );
    
                            if ( g_linesOfCode[ g_pc ].tokenValues.size() > 0 &&
                                 Token_NEXT == g_linesOfCode[ g_pc ].tokenValues[ 0 ].token &&
                                 ! stcmp( g_linesOfCode[ g_pc ].tokenValues[ 0 ], vals[ 0 ] ) )
                                break;
                        } while ( true );
                    }
                }

                g_pc++;
                goto label_next_pc;
            }
            else if ( Token_NEXT == token )
            {
                if ( 0 == forGosubStack.size() )
                    RuntimeFail( "NEXT without FOR", g_lineno );

                ForGosubItem & item = forGosubStack.top();
                if ( !item.isFor )
                    RuntimeFail( "NEXT without FOR", g_lineno );

                string & loopVal = g_linesOfCode[ item.pcReturn ].tokenValues[ 0 ].strValue;

                if ( stcmp( loopVal, vals[ t ].strValue ) )
                    RuntimeFail( "NEXT statement variable doesn't match current FOR loop variable", g_lineno );

                g_pc = item.pcReturn;
                goto label_next_pc;
            }
            else if ( Token_PRINT == token )
            {
                int printToken = t;
                g_pc++;
                t++;

                while ( t < vals.size() )
                {
                    if ( Token_SEMICOLON == vals[ t ].token )
                    {
                        t++;
                        continue;
                    }
                    else if ( Token_EXPRESSION != vals[ t ].token ) // ELSE is typical
                        break;

                    assert( Token_EXPRESSION == vals[ t ].token );

                    if ( Token_STRING == vals[ t + 1 ].token )
                    {
                        printf( "%s", vals[ t + 1 ].strValue.c_str() );
                        t += 2;
                    }
                    else if ( Token_TIME == vals[ t + 1 ].token )
                    {
#ifdef WATCOM
                        struct dostime_t now;
                        _dos_gettime( & now );
                        printf( "%02u:%02u:%02u.%03u", now.hour, now.minute, now.second, now.hsecond * 10 );
#else
                        auto now = system_clock::now();
                        uint64_t ms = duration_cast<milliseconds>( now.time_since_epoch() ).count() % 1000;
                        auto timer = system_clock::to_time_t( now );
                        std::tm bt = * /*std::*/ localtime( &timer );
                        printf( "%02u:%02u:%02u.%03u", (uint32_t) bt.tm_hour, (uint32_t) bt.tm_min, (uint32_t) bt.tm_sec, (uint32_t) ms );
#endif
                        t += 2;
                    }
                    else if ( Token_ELAP == vals[ t + 1 ].token )
                    {
#ifdef WATCOM
                        uint32_t timeNow = DosTimeInMS();
                        long long duration = timeNow - timeBegin;
#else
                        steady_clock::time_point timeNow = steady_clock::now();
                        long long duration = duration_cast<std::chrono::milliseconds>( timeNow - timeBegin ).count();
#endif
                        static char acElap[ 100 ];
                        acElap[ 0 ] = 0;
                        PrintNumberWithCommas( acElap, duration );
                        printf( "%s ms", acElap );
                        t += 2;
                    }
                    else
                    {
                        int val = evalProc( t, vals );
                        printf( "%d", val );
                    }
                }

                if ( 0 == vals[ printToken ].extra )
                    printf( "\n" );
                goto label_next_pc;
            }
            else if ( Token_ELSE == token )
            {
                g_pc++;
                goto label_next_pc;
            }
            else if ( Token_END == token )
            {
                #ifdef BA_ENABLE_INTERPRETER_EXECUTION_TIME
                    if ( showExecutionTime )
                    {
                        uint64_t timeNow = __rdtsc();
                        g_linesOfCode[ g_pc ].duration += ( timeNow - timePrevious );
                        g_linesOfCode[ g_pc ].timesExecuted++;
                    }
                #endif
                goto label_exit_execution;
            }
            else if ( Token_DIM == token )
            {
                // if the variable has already been defined, delete it first.

                Variable * pvar = FindVariable( varmap, vals[ 0 ].strValue );
                if ( pvar )
                {
                    pvar = 0;
                    varmap.erase( vals[ 0 ].strValue.c_str() );
                }

                Variable var( vals[ 0 ].strValue.c_str() );

                var.dimensions = vals[ 0 ].dimensions;
                var.dims[ 0 ] = vals[ 0 ].dims[ 0 ];
                var.dims[ 1 ] = vals[ 0 ].dims[ 1 ];
                int items = var.dims[ 0 ];
                if ( 2 == var.dimensions )
                    items *= var.dims[ 1 ];
                var.array.resize( items );
#ifdef WATCOM
                varmap.insert( std::pair<string, Variable> ( var.name, var ) );
#else
                varmap.emplace( var.name, var );
#endif

                // update all references to this array

                pvar = FindVariable( varmap, vals[ 0 ].strValue );

                for ( size_t l = 0; l < g_linesOfCode.size(); l++ )
                {
                    LineOfCode & lineOC = g_linesOfCode[ l ];
    
                    for ( size_t z = 0; z < lineOC.tokenValues.size(); z++ )
                    {
                        TokenValue & tv = lineOC.tokenValues[ z ];
                        if ( Token_VARIABLE == tv.token && !stcmp( tv, vals[ 0 ] ) )
                            tv.pVariable = pvar;
                    }
                }

                g_pc++;
                goto label_next_pc;
            }
            else if ( Token_TRON == token )
            {
                basicTracing = true;
                g_pc++;
                goto label_next_pc;
            }
            else if ( Token_TROFF == token )
            {
                basicTracing = false;
                g_pc++;
                goto label_next_pc;
            }
            else
            {
                printf( "unexpected token %s\n", TokenStr( token ) );
                RuntimeFail( "internal error: unexpected token in top-level interpreter loop", g_lineno );
            }

            token = vals[ t ].token;
        } while( true );
    } while( true );

    label_exit_execution:

    #ifdef BA_ENABLE_INTERPRETER_EXECUTION_TIME
        if ( showExecutionTime )
        {
            static char acTimes[ 100 ];
            static char acDuration[ 100 ];
            printf( "execution times in rdtsc hardware ticks:\n" );
            printf( "   line #          times           duration         average\n" );
            for ( size_t l = 0; l < g_linesOfCode.size(); l++ )
            {
                LineOfCode & loc = g_linesOfCode[ l ];

                // if the END statment added to the end was never executed then ignore it

                if ( ( l == ( g_linesOfCode.size() - 1 ) ) && ( 0 == loc.timesExecuted ) )
                    continue;
    
                acTimes[ 0 ] = 0;
                acDuration[ 0 ] = 0;
                PrintNumberWithCommas( acTimes, loc.timesExecuted );

                long long duration = loc.duration;
                PrintNumberWithCommas( acDuration, duration );

                double average = loc.timesExecuted ? ( (double) duration / (double) loc.timesExecuted ) : 0.0;
                printf( "  %7d  %13s    %15s    %12.3lf", loc.lineNumber, acTimes, acDuration, average );
                printf( "\n" );
            }
        }
    #endif //BA_ENABLE_INTERPRETER_EXECUTION_TIME

    if ( !g_Quiet )
        printf( "exiting the basic interpreter\n" );
} //InterpretCode

extern int main( int argc, char *argv[] )
{
    g_argc = argc;
    g_argv = argv;

#ifdef WATCOM
    uint32_t timeAppStart = DosTimeInMS();
#else
    steady_clock::time_point timeAppStart = steady_clock::now();
#endif

    // validate the parallel arrays and enum are actually parallel

    assert( ( Token_INVALID + 1 ) == _countof( Tokens ) );
    assert( ( Token_INVALID + 1 ) == _countof( Operators ) );
    assert( 11 == Token_MULT );

    // not critical, but interpreted performance is faster if it's a multiple of 2.

#ifndef WATCOM
    if ( 8 == sizeof( size_t ) && 64 != sizeof( TokenValue ) )
        printf( "sizeof tokenvalue: %zd\n", sizeof( TokenValue ) );
    assert( 4 == sizeof( size_t ) || 64 == sizeof( TokenValue ) );
#endif

    bool showListing = false;
    bool executeCode = true;
    bool showExecutionTime = false;
    bool showParseTime = false;
    bool generateASM = false;
    bool useRegistersInASM = true;
    static char inputfile[ 300 ] = {0};
    static char asmfile[ 300 ] = {0};
    int argvalue = 0; // the one optional integer argument after the basic input filename

    for ( int i = 1; i < argc; i++ )
    {
        char * parg = argv[ i ];
        char c0 = parg[ 0 ];
        char c1 = (char) tolower( parg[ 1 ] );

        if ( '-' == c0 || '/' == c0 )
        {
            if ( 'a' == c1 )
            {
                if ( ':' != parg[ 2 ] || 4 != strlen( parg ) )
                    Usage();

                generateASM = true;
                char a = (char) tolower( parg[ 3 ] );
                if ( 'x' == a )
                    g_AssemblyTarget = x64Win;
                else if ( 'm' == a )
                    g_AssemblyTarget = arm64Mac;
                else if ( '3' == a )
                    g_AssemblyTarget = arm32Linux;
                else if ( '6' == a )
                    g_AssemblyTarget = mos6502Apple1;
                else if ( '8' == a )
                    g_AssemblyTarget = i8080CPM;
                else if ( 'c' == a )
                    g_AssemblyTarget = m68kCPM;
                else if ( 'd' == a )
                    g_AssemblyTarget = i8086DOS;
                else if ( 'a' == a )
                    g_AssemblyTarget = arm64Win;
                else if ( 'o' == a )
                    g_AssemblyTarget = oiOS;
                else if ( 'r' == a )
                    g_AssemblyTarget = riscv64;
                else if ( 'i' == a )
                {
                    g_AssemblyTarget = x86Win;
                    if ( 'I' == parg[ 3 ] )
                        g_i386Target686 = false;
                }
                else
                    Usage();
            }
            else if ( 'd' == c1 )
                g_GenerateAppleDollar = true;
            else if ( 'e' == c1 )
                showExecutionTime = true;
            else if ( 'l' == c1 )
                showListing = true;
            else if ( 'o' == c1 )
                g_ExpressionOptimization = false;
            else if ( 'p' == c1 )
                showParseTime = true;
            else if ( 'q' == c1 )
                g_Quiet = true;
            else if ( 'r' == c1 )
                useRegistersInASM = false;
            else if ( 't' == c1 )
                g_Tracing = true;
            else if ( 'x' == c1 )
                executeCode = false;
            else
                Usage();
        }
        else
        {
            if ( strlen( argv[1] ) >= _countof( inputfile ) )
                Usage();

            if ( 0 != inputfile[ 0 ] )
                argvalue = atoi( argv[ i ] );
            else
                strcpy_s( inputfile, _countof( inputfile ), argv[ i ] );
        }
    }

    if ( !inputfile[0] )
    {
        printf( "input file not specified\n" );
        Usage();
    }

    // append ".bas" to a filename if it's not there already and the file doesn't exist.

    CFile ftest( fopen( inputfile, "r" ) );
    if ( ftest.get() )
        ftest.Close();
    else if ( !strstr( inputfile, ".bas" ) )
        strcat( inputfile, ".bas" );

    ParseInputFile( inputfile );

    AddENDStatement();

    RemoveREMStatements();

    if ( showListing )
    {
        printf( "lines of code: %zd\n", g_linesOfCode.size() );
    
        for ( size_t l = 0; l < g_linesOfCode.size(); l++ )
            ShowLocListing( g_linesOfCode[ l ] );
    }

    PatchGotoAndGosubNumbers();

    OptimizeWithRewrites( showListing );

    SetFirstTokens();

    // Create all non-array variables and update references so lookups are always fast later

    map<string, Variable> varmap;
    CreateVariables( varmap );

    // set AV% as an integer value of the first argument following the BASIC input filename

    Variable * pAV = FindVariable( varmap, "av%" );
    if ( 0 != pAV )
        pAV->value = argvalue;

    if ( showParseTime )
    {
#ifdef WATCOM
        uint32_t timeParseComplete = DosTimeInMS();
        long long durationParse = timeParseComplete - timeAppStart;
        double parseInMS = (double) durationParse;
#else
        steady_clock::time_point timeParseComplete = steady_clock::now();      
        long long durationParse = duration_cast<std::chrono::nanoseconds>( timeParseComplete - timeAppStart ).count();
        double parseInMS = (double) durationParse / 1000000.0;
#endif
        printf( "Time to parse %s: %lf ms\n", inputfile, parseInMS );
    }

#ifdef BA_ENABLE_COMPILER

    if ( generateASM )
    {
        strcpy_s( asmfile, _countof( asmfile ), inputfile );
        char * dot = strrchr( asmfile, '.' );
        if ( !dot )
            dot = asmfile + strlen( asmfile );

        if ( arm64Mac == g_AssemblyTarget || arm32Linux == g_AssemblyTarget || mos6502Apple1 == g_AssemblyTarget ||
             riscv64 == g_AssemblyTarget || oiOS == g_AssemblyTarget || m68kCPM == g_AssemblyTarget )
            strcpy_s( dot, _countof( asmfile) - ( dot - asmfile ), ".s" );
        else
            strcpy_s( dot, _countof( asmfile) - ( dot - asmfile ), ".asm" );

        GenerateASM( asmfile, varmap, useRegistersInASM );
    }

#endif

    if ( executeCode )
        InterpretCode( varmap, showExecutionTime );
} //main

