/*
    Motorola 68000 emulator
    Written by David Lee in April 2025

    Useful: https://en.wikipedia.org/wiki/Motorola_68000
            http://goldencrystal.free.fr/M68kOpcodes-v2.3.pdf
            https://web.njit.edu/~rosensta/classes/architecture/252software/code.pdf
            https://en.wikipedia.org/wiki/SREC_(file_format)
            http://www.retroarchive.org/docs/cpm68_prog_guide_pt1.pdf

    notes:  -- trap 0 (32) maps to linux-equivalent system calls
            -- trap 2 (34) maps to cp/m 68k bdos functions
            -- trap 3 (35) maps to cp/m 68k bios functions
            -- trap 15 (47) maps to a couple m68k system calls
*/

#include <stdint.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

#include <djltrace.hxx>

#include "m68000.hxx"

using namespace std;
using namespace std::chrono;

#if defined( __GNUC__ ) && !defined( __APPLE__ ) && !defined( __clang__ )     // bogus warning in g++ (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0
#pragma GCC diagnostic ignored "-Wformat="
#endif

static uint32_t g_State = 0; // 32 instead of 8 bits is faster with the msft compiler

const uint32_t stateTraceInstructions = 1;
const uint32_t stateEndEmulation = 2;
const uint32_t stateInstructionTrace = 4;

bool m68000::trace_instructions( bool t )
{
    bool previous = ( 0 != ( g_State & stateTraceInstructions ) );
    if ( t )
        g_State |= stateTraceInstructions;
    else
        g_State &= ~stateTraceInstructions;
    return previous;
} //trace_instructions

void m68000::end_emulation() { g_State |= stateEndEmulation; }

static inline uint16_t get_bit16( uint16_t x, uint32_t bit_number )
{
    assert( bit_number < 16 );
    return ( ( x >> bit_number ) & 1 );
} //get_bit16

static inline uint32_t get_bit32( uint32_t x, uint32_t bit_number )
{
    assert( bit_number < 32 );
    return ( ( x >> bit_number ) & 1 );
} //get_bit32

static inline uint16_t get_bits16( uint16_t x, uint16_t lowbit, uint16_t len )
{
    uint16_t val = ( x >> lowbit );
    assert( 16 != len ); // the next line of code wouldn't work but there are no callers that do this
    return ( val & ( ( 1 << len ) - 1 ) );
} //get_bits16

static inline bool sign8( uint8_t b )
{
    return( 0 != ( 0x80 & b ) );
} //sign8

static inline bool sign16( uint16_t w )
{
    return( 0 != ( 0x8000 & w ) );
} //sign16

static inline bool sign32( uint32_t l )
{
    return( 0 != ( 0x80000000 & l ) );
} //sign32

const char * m68000::render_flags()
{
    static char flags[ 8 ] = { 0 };

    flags[ 0 ] = flag_t() ? 'T' : 't';
    flags[ 1 ] = flag_s() ? 'S' : 's';
    flags[ 2 ] = flag_c() ? 'C' : 'c';
    flags[ 3 ] = flag_v() ? 'V' : 'v';
    flags[ 4 ] = flag_z() ? 'Z' : 'z';
    flags[ 5 ] = flag_n() ? 'N' : 'n';
    flags[ 6 ] = flag_x() ? 'X' : 'x';

    return flags;
} //render_flags

const char * m68000::effective_string()
{
    static char ea[ 40 ];
    ea[0] = 0;

    switch ( ea_mode )
    {
        case 0: // Data register Dn
        {
            snprintf( ea, _countof( ea ), "d%u", ea_reg );
            break;
        }
        case 1: // Address register An
        {
            snprintf( ea, _countof( ea ), "a%u", ea_reg );
            break;
        }
        case 2: // Address (An)
        {
            snprintf( ea, _countof( ea ), "(a%u)", ea_reg );
            break;
        }
        case 3: // Address with postincrement (An)+
        {
            snprintf( ea, _countof( ea ), "(a%u)+", ea_reg );
            break;
        }
        case 4: // Address with predecrement -(An)
        {
            snprintf( ea, _countof( ea ), "-(a%u)", ea_reg );
            break;
        }
        case 5: // Address with displacement (d16, An)
        {
            pc += 2;
            int16_t displacement = (int16_t) getui16( pc );
            snprintf( ea, _countof( ea ), "%d(a%u)", displacement, ea_reg );
            break;
        }
        case 6: // Address with index (d8, An, Xn)
        {
            // 16 bits: 15        14-12       11                10-9                8  7-0
            //          1=a, 0=d  0-7 Xn reg  1=l, 0=w from Xn  scale 0=1 on 68000  0  signed 8-bit displacement
            pc += 2;
            uint16_t extension = getui16( pc );
            bool isa = get_bit16( extension, 15 );
            uint16_t Xn = get_bits16( extension, 12, 3 );
            bool isl = get_bit16( extension, 11 );
            uint16_t scale = get_bits16( extension, 9, 2 );
            if ( 0 != scale || get_bit16( extension, 8 ) )
                unhandled(); // > 68000 instruction
            int32_t displacement = sign_extend( 0xff & extension, 7 );
            snprintf( ea, _countof( ea ), "(%d,a%u,%c%u.%c)", displacement, ea_reg, isa ? 'a' : 'd', Xn, isl ? 'l' : 'w' );
            break;
        }
        case 7: // several
        {
            switch ( ea_reg )
            {
                case 0: // absolute short (xxx).w
                {
                    pc += 2;
                    uint32_t address = sign_extend( getui16( pc ), 15 );
                    snprintf( ea, _countof( ea ), "(#$%x)", address );
                    break;
                }
                case 1: // absolute long (xxx).l
                {
                    uint32_t address = getui32( pc + 2 );
                    pc += 4;
                    snprintf( ea, _countof( ea ), "(#$%x)", address );
                    break;
                }
                case 2: // ( d16, pc )
                {
                    pc += 2;
                    int32_t displacement = (int32_t) (int16_t) getui16( pc );
                    snprintf( ea, _countof( ea ), "(#%d,pc)", displacement );
                    break;
                }
                case 3: // program counter with index. ( d8, PC, Dn )
                {
                    pc += 2;
                    uint16_t extension = getui16( pc );
                    bool is_a = get_bit16( extension, 15 );
                    uint16_t Xn = get_bits16( extension, 12, 3 );
                    bool is_l = get_bit16( extension, 11 );
                    int32_t displacement = (int32_t) sign_extend( 0xff & extension, 7 );
                    snprintf( ea, _countof( ea ), "(#%d,pc,%c%u.%c)", displacement, is_a ? 'a' : 'd', Xn, is_l ? 'l' : 'w' );
                    break;
                }
                case 4: // immediate #imm
                {
                    uint32_t address;
                    if ( op_size < 2 )
                    {
                        pc += 2;
                        address = sign_extend( getui16( pc ), 15 );
                    }
                    else
                    {
                        address = getui32( pc + 2 );
                        pc += 4;
                    }
                    snprintf( ea, _countof( ea ), "#$%x", address );
                    break;
                }
                default:
                    unhandled();
            }
            break;
        }
        default:
            unhandled();
    }

    return ea;
} //effective_string

const char * m68000::effective_string2( uint16_t mval, uint16_t regval )
{
    uint16_t tmp_m = ea_mode;
    uint16_t tmp_ea_reg = ea_reg;
    ea_mode = mval;
    ea_reg = regval;
    const char * pea = effective_string();
    ea_mode = tmp_m;
    ea_reg = tmp_ea_reg;
    return pea;
} //effective_string2

int32_t m68000::get_ea_displacement()
{
    // 16 bits: 15        14-12       11                10-9                8  7-0
    //          1=a, 0=d  0-7 Xn reg  1=l, 0=w from Xn  scale 0=1 on 68000  ?  signed 8-bit displacement

    pc += 2;
    uint16_t extension = getui16( pc );
    bool isa = get_bit16( extension, 15 );
    uint16_t Xn = get_bits16( extension, 12, 3 );
    bool isl = get_bit16( extension, 11 );
    uint16_t scale = get_bits16( extension, 9, 2 );
    if ( 0 != scale || get_bit16( extension, 8 ) )
        unhandled(); // if not 0, it's a >68000 instruction
    int32_t displacement = (int32_t) sign_extend( 0xff & extension, 7 );
    int32_t reg_displacement = isa ? aregs[ Xn ] : dregs[ Xn ].l;
    if ( !isl )
        reg_displacement = sign_extend( reg_displacement, 15 ); // both A and D registers behave like this per experimentation
    return displacement + reg_displacement;
} //get_ea_displacement

uint32_t m68000::effective_address()
{
    //tracer.Trace( "\nea pc %x, ea_mode %x, ea_reg %x\n", pc, m, ea_reg );
    switch ( ea_mode & 7 ) // mask to help msc to generate better code
    {
        case 0: // Data register Dn
        {
            if ( 0 == op_size )
                return dregs[ ea_reg ].b;
            if ( 1 == op_size )
                return dregs[ ea_reg ].w;
            return dregs[ ea_reg ].l;
        }
        case 1: // Address register An
        {
            return aregs[ ea_reg ];
        }
        case 2: // Address (An)
        {
            return aregs[ ea_reg ];
        }
        case 3: // Address with postincrement (An)+
        {
            uint32_t result = aregs[ ea_reg ];
            uint32_t delta = ( ( 7 == ea_reg ) && ( 0 == op_size ) ) ? 2 : ( 1 << op_size ); // keep stack pointer 2-byte aligned
            aregs[ ea_reg ] += delta;
            return result;
        }
        case 4: // Address with predecrement -(An)
        {
            uint32_t delta = ( ( 7 == ea_reg ) && ( 0 == op_size ) ) ? 2 : ( 1 << op_size ); // keep stack pointer 2-byte aligned
            aregs[ ea_reg ] -= delta;
            return aregs[ ea_reg ];
        }
        case 5: // Address with displacement (d16, An)
        {
            pc += 2;
            return (uint32_t) ( aregs[ ea_reg ] + (int16_t) getui16( pc ) );
        }
        case 6: // Address with index ( d8, An, Xn )
        {
            return aregs[ ea_reg ] + get_ea_displacement();
        }
        case 7: // several
        {
            switch ( ea_reg )
            {
                case 0: // absolute short (xxx).w
                {
                    pc += 2;
                    return (uint32_t) sign_extend( getui16( pc ), 15 );
                }
                case 1: // absolute long (xxx).l
                {
                    uint32_t address = getui32( pc + 2 );
                    pc += 4;
                    return address;
                }
                case 2: // Program counter with displacement ( d16, pc )
                {
                    pc += 2;
                    return pc + (int16_t) getui16( pc );
                }
                case 3: // program counter with index. ( d8, PC, Xn )
                {
                    int32_t displacement = get_ea_displacement(); // this modifies pc so must be a separate line
                    return pc + displacement;
                }
                case 4: // immediate #imm
                {
                    uint32_t address;
                    if ( op_size < 2 )
                    {
                        pc += 2;
                        address = sign_extend( getui16( pc ), 15 );
                    }
                    else
                    {
                        address = getui32( pc + 2 );
                        pc += 4;
                    }
                    return address;
                }
                default:
                    unhandled();
            }
        }
        default:
            unhandled();
    }

    assume_false;
} //effective_address

uint32_t m68000::effective_address2( uint16_t mval, uint16_t regval )
{
    // Only called for some move.X instructions. It's slow and a hack but very infrequently called.

    uint16_t tmp_ea_mode = ea_mode;
    uint16_t tmp_ea_reg = ea_reg;
    ea_mode = mval;
    ea_reg = regval;
    uint32_t result = effective_address();
    ea_mode = tmp_ea_mode;
    ea_reg = tmp_ea_reg;
    return result;
} //effective_address2

inline uint32_t m68000::effective_value32( uint32_t x )
{
    if ( ea_mode <= 1 || ( 7 == ea_mode && 4 == ea_reg ) )
        return x;
    return getui32( x );
} //effective_value32

inline uint16_t m68000::effective_value16( uint32_t x )
{
    if ( ea_mode <= 1 || ( 7 == ea_mode && 4 == ea_reg ) )
        return (uint16_t) x;
    return getui16( x );
} //effective_value16

inline uint8_t m68000::effective_value8( uint32_t x )
{
    if ( ea_mode <= 1 || ( 7 == ea_mode && 4 == ea_reg ) )
        return (uint8_t) x;
    return getui8( x );
} //effective_value8

static const char * condition_string( uint16_t c )
{
    switch( c )
    {
        case  0: return "ra"; // T -- normal branch
        case  1: return "sr"; // F -- subroutine
        case  2: return "hi"; // higher
        case  3: return "ls"; // lower or same
        case  4: return "cc"; // carry clear
        case  5: return "cs"; // carry set
        case  6: return "ne"; // not equal
        case  7: return "eq"; // equal
        case  8: return "vc"; // overflow clear
        case  9: return "vs"; // overflow set
        case 10: return "pl"; // plus
        case 11: return "mi"; // minus
        case 12: return "ge"; // greater than or equal
        case 13: return "lt"; // less than
        case 14: return "gt"; // greater than
        case 15: return "le"; // less than or equal
        default:
        {
            assert( false );
            return "???";
        }
    }
} //condition_string

static const char * movem_a7_d0( uint16_t x ) // 0..15 == a7..d0. predecrement mode
{
    static char ac[ 16 * 3 ];
    size_t len = 0;

    for ( int i = 15; i >= 0; i-- )
    {
        if ( get_bit16( x, (uint16_t) i ) )
        {
            if ( 0 != len )
                ac[ len++ ] = '/';

            if ( i >= 8 )
            {
                ac[ len++ ] = 'd';
                ac[ len++ ] = '0' + 15 - (char) i;
            }
            else
            {
                ac[ len++ ] = 'a';
                ac[ len++ ] = '0' + 7 - (char) i;
            }
        }
    }

    ac[ len ] = 0;
    return ac;
} //movem_a7_d0

static const char * movem_d0_a7( uint16_t x ) // 0..15 == d0..a7. post-increment and control modes
{
    static char ac[ 16 * 3 ];
    size_t len = 0;

    for ( int i = 15; i >= 0; i-- )
    {
        if ( get_bit16( x, (uint16_t) i ) )
        {
            if ( 0 != len )
                ac[ len++ ] = '/';

            if ( i >= 8 )
            {
                ac[ len++ ] = 'a';
                ac[ len++ ] = '0' + (char) i - 8;
            }
            else
            {
                ac[ len++ ] = 'd';
                ac[ len++ ] = '0' + (char) i;
            }
        }
    }

    ac[ len ] = 0;
    return ac;
} //movem_d0_a7

void m68000::trace_state()
{
    uint32_t save_pc = pc; // pc needs to move ahead for some cases of accumulating addressing modes.
    uint16_t save_op_size = op_size; // op_size gets overridden for various instrutions.
    static const char * previous_symbol = 0;
    uint32_t offset;
    const char * symbol_name = emulator_symbol_lookup( pc, offset );
    if ( symbol_name == previous_symbol )
        symbol_name = "";
    else
        previous_symbol = symbol_name;

    char symbol_offset[40];
    symbol_offset[ 0 ] = 0;

    if ( 0 != symbol_name[ 0 ] )
    {
        if ( 0 != offset )
            snprintf( symbol_offset, _countof( symbol_offset ), " + %x", offset );
        strcat( symbol_offset, "\n            " );
    }

    tracer.Trace( "pc %8x %s%s op %5x %8x %s", pc, symbol_name, symbol_offset, op, getui32( pc + 2 ), render_flags() );

    static char acregs[ 16 * 16 + 10 ]; // way too much.
    acregs[ 0 ] = 0;
    int len = 0;
    for ( int r = 0; r < 8; r++ )
        if ( 0 != dregs[ r ].l )
            len += snprintf( & acregs[ len ], 16, "d%u:%x ", r, dregs[ r ].l );
    for ( int r = 0; r < 8; r++ )
        if ( 0 != aregs[ r ] )
            len += snprintf( & acregs[ len ], 16, "a%u:%x ", r, aregs[ r ] );
    tracer.Trace( " %s==> ", acregs );

    switch( hi4 )
    {
        case 0: // many math and cmp instructions
        {
            uint16_t bits11_8 = opbits( 8, 4 );
            uint16_t bits11_6 = opbits( 6, 6 );

            if ( 0x003c == op ) // ori to CCR
                tracer.Trace( "ori #%x, ccr\n", getui16( pc + 2 ) & 0xff );
            else if ( 0x007c == op ) // ori to SR
                tracer.Trace( "ori #%x, ssr\n", getui16( pc + 2 ) );
            else if ( 0x023c == op ) // andi to CCR
                tracer.Trace( "andi #%x, ccr\n", getui16( pc + 2 ) & 0xff );
            else if ( 0x027c == op ) // andi to SR
                tracer.Trace( "andi #%x, ssr\n", getui16( pc + 2 ) );
            else if ( 0x0a3c == op ) // eori to CCR
                tracer.Trace( "eori #%x, ccr\n", getui16( pc + 2 ) & 0xff );
            else if ( 0x0a7c == op ) // eori to SR
                tracer.Trace( "eori #%x, ssr\n", getui16( pc + 2 ) );
            else if ( 0 == bits11_8 ) // ori
            {
                uint32_t val = ( 2 == op_size ) ? getui32( pc + 2 ) : ( 1 == op_size ) ? getui16( pc + 2 ) : ( getui16( pc + 2 ) & 0xff );
                pc += ( 2 == op_size ) ? 4 : 2;
                if ( 1 == ea_mode ) // this is invalid on the 68000
                    unhandled();
                tracer.Trace( "ori.%c #%u, %s\n", get_size(), val, effective_string() );
            }
            else if ( 0xa == bits11_8 ) // eori
            {
                uint32_t val = ( 2 == op_size ) ? getui32( pc + 2 ) : ( 1 == op_size ) ? getui16( pc + 2 ) : ( getui16( pc + 2 ) & 0xff );
                pc += ( 2 == op_size ) ? 4 : 2;
                tracer.Trace( "eori.%c #%u, %s\n", get_size(), val, effective_string() );
            }
            else if ( 2 == bits11_8 ) // andi
            {
                uint32_t val = ( 2 == op_size ) ? getui32( pc + 2 ) : ( 1 == op_size ) ? getui16( pc + 2 ) : ( getui16( pc + 2 ) & 0xff );
                pc += ( 2 == op_size ) ? 4 : 2;
                tracer.Trace( "andi.%c #%d, %s\n", get_size(), val, effective_string() );
            }
            else if ( 0xc == bits11_8 ) // cmpi
            {
                uint32_t val = ( 2 == op_size ) ? getui32( pc + 2 ) : ( 1 == op_size ) ? getui16( pc + 2 ) : ( getui16( pc + 2 ) & 0xff );
                pc += ( 2 == op_size ) ? 4 : 2;
                tracer.Trace( "cmpi.%c #%d, %s\n", get_size(), val, effective_string() );
            }
            else if ( 4 == bits11_8 ) // subi
            {
                uint32_t val = ( 2 == op_size ) ? getui32( pc + 2 ) : ( 1 == op_size ) ? getui16( pc + 2 ) : ( getui16( pc + 2 ) & 0xff );
                pc += ( 2 == op_size ) ? 4 : 2;
                tracer.Trace( "subi.%c #%d, %s\n", get_size(), val, effective_string() );
            }
            else if ( 6 == bits11_8 ) // addi
            {
                uint32_t val = ( 2 == op_size ) ? getui32( pc + 2 ) : ( 1 == op_size ) ? getui16( pc + 2 ) : ( getui16( pc + 2 ) & 0xff );
                pc += ( 2 == op_size ) ? 4 : 2;
                tracer.Trace( "addi.%c #%d, %s\n", get_size(), val, effective_string() );
            }
            else if ( opbit( 8 ) && 1 == ea_mode ) // movep
            {
                pc += 2;
                if ( 0 == opbit( 7 ) )
                    tracer.Trace( "movep.%c (%u,a%u), d%u\n", opbit( 6 ) ? 'l' : 'w', getui16( pc ), ea_reg, op_reg );
                else
                    tracer.Trace( "movep.%c d%u, (%u,a%u)\n", opbit( 6 ) ? 'l' : 'w', op_reg, getui16( pc ), ea_reg );
            }
            else if ( 0x20 == bits11_6 ) // btst using address
            {
                pc += 2;
                uint16_t bit_number = ( 0xff & getui16( pc ) );
                tracer.Trace( "btst #%u, %s\n", bit_number, effective_string() );
            }
            else if ( 4 == op_mode ) // btst using register
            {
                op_size = 2; // always 32-bit
                tracer.Trace( "btst d%u, %s\n", effective_string() );
            }
            else if ( 0x22 == bits11_6 ) // bclr using address
            {
                pc += 2;
                uint8_t bitnum = getui16( pc ) & 0xff;
                bitnum %= ( 0 == ea_mode ) ? 32 : 8;
                op_size = 0; // always a byte operation
                tracer.Trace( "bclr.%c #%u, %s\n", ( 0 == ea_mode ) ? 'l' : 'b', bitnum, effective_string() );
            }
            else if ( 6 == op_mode ) // bclr using register
                tracer.Trace( "bclr.%c d%u, %s\n", ( 0 == ea_mode ) ? 'l' : 'b', op_reg, effective_string() );
            else if ( 0x21 == bits11_6 ) // bchg using address
            {
                pc += 2;
                uint8_t bitnum = getui16( pc ) & 0xff;
                bitnum %= ( 0 == ea_mode ) ? 32 : 8;
                op_size = 0; // always a byte operation
                tracer.Trace( "bchg #%u, %s\n", bitnum, effective_string() );
            }
            else if ( 5 == op_mode ) // bchg using rgister
                tracer.Trace( "bchg d%u, %s\n", op_reg, effective_string() );
            else if ( 0x23 == bits11_6 ) // bset using address
            {
                pc += 2;
                uint8_t bitnum = getui16( pc ) & 0xff;
                bitnum %= ( 0 == ea_mode ) ? 32 : 8;
                op_size = 0; // always a byte operation
                tracer.Trace( "bset #%u, %s\n", bitnum, effective_string() );
            }
            else if ( 7 == op_mode ) // bset using register
                tracer.Trace( "bset d%u, %s\n", op_reg, effective_string() );
            else
                unhandled();
            break;
        }
        case 1: // move.b
        case 2: // move.l + movea.l
        case 3: // move.w + movea.w
        {
            uint16_t moves = opbits( 12, 2 );
            op_size = ( 1 == moves ) ? 0 : ( 3 == moves ) ? 1 : 2; // map moves to s
            if ( 1 == opbits( 6, 3 ) ) // movea.l or movea.w
                tracer.Trace( "movea.%c %s, a%u\n", get_size(), effective_string(), op_reg );
            else // move.b / move.w / move.l
            {
                const char * psrc = effective_string();
                tracer.Trace( "move.%c %s", get_size(), psrc );
                const char * pdst = effective_string2( op_mode, op_reg );
                tracer.Trace( ", %s\n", pdst );
            }
            break;
        }
        case 4:
        {
            if ( 0x4e73 == op ) // rte
                tracer.Trace( "rte\n" );
            else if ( 0x4e75 == op ) // rts
                tracer.Trace( "rts\n" );
            else if ( 0x4e73 == op ) // trapv
                tracer.Trace( "trapv\n" );
            else if ( 0x4e77 == op ) // rtr
                tracer.Trace( "rtr\n" );
            else if ( 0x4e70 == op ) // reset
                tracer.Trace( "reset\n" );
            else if ( 0x4e72 == op ) // stop
                tracer.Trace( "stop\n" );
            else if ( 0x4e71 == op ) // nop
                tracer.Trace( "nop\n" );
            else if ( 0x4afc == op ) // illegal
                tracer.Trace( "illegal\n" );
            else
            {
                uint16_t bits11_6 = opbits( 6, 6 );
                uint16_t bits11_7 = opbits( 7, 5 );
                uint16_t bits11_8 = opbits( 8, 4 );
                uint16_t bits11_3 = opbits( 3, 9 );
                uint16_t bits11_4 = opbits( 4, 8 );

                if ( 3 == bits11_6 ) // move from sr
                {
                    op_size = 1;
                    tracer.Trace( "move sr, %s\n", effective_string() );
                }
                else if ( 0x13 == bits11_6 ) // move to ccr
                {
                    op_size = 1;
                    tracer.Trace( "move %s, ccr\n", effective_string() );
                }
                else if ( 0x1b == bits11_6 ) // move to sr
                {
                    op_size = 1;
                    tracer.Trace( "move %s, sr\n", effective_string() );
                }
                else if ( 4 == bits11_8 ) // neg
                    tracer.Trace( "neg.%c %s\n", get_size(), effective_string() );
                else if ( 0xe4 == bits11_4 ) // trap
                    tracer.Trace( "trap %#u + 32 = %#u\n", op & 0xf, ( op & 0xf ) + 32 );
                else if ( 0x11 == bits11_7 && 0 == ea_mode ) // ext
                    tracer.Trace( "ext.%c d%u\n", opbit( 6 ) ? 'l' : 'w', ea_reg );
                else if ( 0xa == bits11_8 ) // tst
                    tracer.Trace( "tst.%c, %s\n", get_size(), effective_string() );
                else if ( 0x1ca == bits11_3 ) // link
                    tracer.Trace( "link a%u, #%d\n", ea_reg, (int32_t) (int16_t) getui16( pc + 2 ) );
                else if ( 0x1cb == bits11_3 ) // unlk
                    tracer.Trace( "unlk a%u\n", ea_reg );
                else if ( 0x21 == bits11_6 && 0 == ea_mode ) // swap
                    tracer.Trace( "swap d%u\n", ea_reg );
                else if ( 0x21 == bits11_6 ) // pea
                    tracer.Trace( "pea %s\n", effective_string() );
                else if ( 0x3a == bits11_6 ) // jsr
                {
                    tracer.Trace( "jsr %s  ;  ", effective_string() );
                    uint8_t * pb = (uint8_t *) getmem( aregs[ 7 ] );
                    tracer.Trace( "%02x %02x %02x %02x %02x %02x %02x %02x\n", pb[ 0 ], pb[ 1 ], pb[ 2 ], pb[ 3 ], pb[ 4 ], pb[ 5 ], pb[ 6 ], pb[ 7 ] );
                }
                else if ( 0x3b == bits11_6 ) // jmp
                    tracer.Trace( "jmp %s\n", effective_string() );
                else if ( 0x22 == ( 0x2e & bits11_6 ) ) // movem
                {
                    bool memory_to_register = opbit( 10 );
                    bool size_long = opbit( 6 );
                    char size_c = size_long ? 'l' : 'w';
                    if ( 4 == ea_mode && !memory_to_register ) // pre decrement, register to memory
                        tracer.Trace( "movem.%c %s, -(a%u)\n", size_c, movem_a7_d0( getui16( pc + 2 ) ), ea_reg );
                    else if ( 3 == ea_mode && memory_to_register ) // post increment, memory to register
                        tracer.Trace( "movem.%c (a%u)+, %s\n", size_c, ea_reg, movem_d0_a7( getui16( pc + 2 ) ) );
                    else if ( memory_to_register ) // list, ea   == memory to register
                    {
                        pc += 2;
                        const char * pregs = movem_d0_a7( getui16( pc ) ); // control mode is always d0..a7 == 0..15
                        tracer.Trace( "movem.%c %s, %s\n", size_c, effective_string(), pregs );
                    }
                    else // ea, list   == register to memory
                    {
                        pc += 2;
                        const char * pregs = movem_d0_a7( getui16( pc ) ); // control mode is always d0..a7
                        tracer.Trace( "movem.%c %s, %s\n", size_c, pregs, effective_string() );
                    }
                }
                else if ( 7 == ( 7 & bits11_6 ) ) // lea
                    tracer.Trace( "lea %s, a%u\n", effective_string(), op_reg );
                else if ( opbit( 8 ) && !opbit( 6 ) ) // chk
                {
                    op_size = opbit( 7 ) ? 1 : 2;
                    tracer.Trace( "chk.%c %s, d%u\n", opbit( 7 ) ? 'w' : 'l', effective_string(), op_reg );
                }
                else if ( 2 == bits11_8 ) // clr
                    tracer.Trace( "clr.%c %s\n", get_size(), effective_string() ); // documentation is ambiguous, but apparently only .w is supported
                else if ( 0 == bits11_8 ) // negx
                    tracer.Trace( "negx.%c %s\n", get_size(), effective_string() );
                else if ( 6 == bits11_8 ) // not
                    tracer.Trace( "not.%c %s\n", get_size(), effective_string() );
                else if ( 0x31 == bits11_6 ) // divul.l (68020+)
                {
                    pc += 2;
                    uint16_t dr_dq = getui16( pc );
                    uint16_t dr = dr_dq & 7;
                    uint16_t dq = ( dr_dq >> 12 ) & 7;
                    tracer.Trace( "divul.l %s, d%u:d%u\n", effective_string(), dr, dq );
                }
                else if ( 0xe6 == bits11_4 ) // move usp
                {
                    if ( opbit( 3 ) )
                        tracer.Trace( "move usp, a%u\n", ea_reg );
                    else
                        tracer.Trace( "move a%u, usp\n", ea_reg );
                }
                else if ( 0x20 == bits11_6 ) // nbcd
                {
                    op_size = 0;
                    tracer.Trace( "nbcd %s\n", effective_string() );
                }
                else if ( 0x2b == bits11_6 ) // tas
                    tracer.Trace( "tas %s\n", effective_string() );
                else
                    unhandled();
            }
            break;
        }
        case 5: // addq / subq / Scc / DBcc
        {
            if ( 1 == ea_mode && 3 == op_size ) // DBcc
            {
                uint16_t condition = opbits( 8, 4 );
                int16_t displacement = (int16_t) getui16( pc + 2 );
                tracer.Trace( "db%s d%u, %d\n", condition_string( condition ), ea_reg, displacement + 2 );
            }
            else if ( 3 == op_size ) // Scc
            {
                uint16_t condition = opbits( 8, 4 );
                op_size = 0; // size is always byte
                tracer.Trace( "s%s %s\n", condition_string( condition ), effective_string() );
            }
            else if ( opbit( 8 ) ) // subq
            {
                uint32_t data = opbits( 9, 3 );
                if ( 0 == data )
                    data = 8;
                tracer.Trace( "subq.%c #%u, %s\n", get_size(), data, effective_string() );
            }
            else // addq
            {
                uint32_t data = opbits( 9, 3 );
                if ( 0 == data )
                    data = 8;
                tracer.Trace( "addq.%c #%u, %s\n", get_size(), data, effective_string() );
            }
            break;
        }
        case 6: // bra / bsr / bcc
        {
            uint16_t condition = opbits( 8, 4 );
            int16_t displacement = op & 0xff;
            if ( 0 != displacement )
                displacement = sign_extend16( displacement, 7 );
            else
                displacement = (int16_t) getui16( pc + 2 );

            tracer.Trace( "b%s %d\n", condition_string( condition ), displacement + 2 );
            break;
        }
        case 7: // moveq
        {
            if ( opbit( 8 ) )
                unhandled();  // it's not a 68000 instruction if this bit is set

            uint32_t data = sign_extend( opbits( 0, 8 ), 7 );
            tracer.Trace( "moveq #%d, d%u\n", data, op_reg );
            break;
        }
        case 8: // divu / divs / sbcd / or
        {
            uint16_t bit8 = opbit( 8 );
            uint16_t bits8_4 = opbits( 4, 5 );

            if ( 0x10 == bits8_4 ) // sbcd
            {
                if ( opbit( 3 ) ) // address register mode
                    tracer.Trace( "sbcd -(a%u), -(a%u)\n", ea_reg, op_reg );
                else // data register mode
                    tracer.Trace( "sbcd d%u, d%u\n", ea_reg, op_reg );
            }
            else if ( !bit8 && 3 == op_size ) // divu
            {
                op_size = 1; // word operation
                tracer.Trace( "divu.w %s, d%u\n", effective_string(), op_reg );
            }
            else if ( bit8 && 3 == op_size ) // divs
            {
                op_size = 1; // word operation
                tracer.Trace( "divs.w %s, d%u\n", effective_string(), op_reg );
            }
            else // or
            {
                if ( bit8 )
                    tracer.Trace( "or.%c d%u, %s\n", get_size(), op_reg, effective_string() );
                else
                    tracer.Trace( "or.%c %s, d%u\n", get_size(), effective_string(), op_reg );
            }
            break;
        }
        case 0x9: // sub / subx / suba
        {
            if ( 3 == opbits( 6, 2 ) ) // suba
            {
                op_size = opbit( 8 ) ? 2 : 1;
                tracer.Trace( "suba.%c %s, a%u\n", opbit( 8 ) ? 'l' : 'w', effective_string(), op_reg );
            }
            else if ( 0 == opbits( 4, 2 ) && opbit( 8 ) ) // subx
            {
                if ( opbit( 3 ) ) // true if both operands are memory with a register predecrement mode, false if both are d registers
                    tracer.Trace( "subx.%c -(a%u), -(a%u)\n", get_size(), ea_reg, op_reg );
                else
                    tracer.Trace( "subx.%c d%u, d%u\n", get_size(), ea_reg, op_reg );
            }
            else // sub
            {
                if ( 0 == ( op_mode & 4 ) ) // // Dn - <ea> => Dn
                    tracer.Trace( "sub.%c %s, d%u\n", get_size(), effective_string(), op_reg );
                else // <ea> - Dn => <ea>
                    tracer.Trace( "sub.%c d%u, %s\n", get_size(), op_reg, effective_string() );
            }
            break;
        }
        case 0xb: // eor, cmpm, cmp, cmpa
        {
            uint16_t bit8 = opbit( 8 );
            if ( 3 == op_mode || 7 == op_mode ) // cmpa
            {
                op_size = bit8 ? 2 : 1;
                tracer.Trace( "cmpa.%c %s, a%u\n", bit8 ? 'l' : 'w', effective_string(), op_reg );
            }
            else if ( op_mode <= 2 ) // cmp
                tracer.Trace( "cmp.%c %s, d%u\n", ( 0 == op_mode ) ? 'b' : ( 1 == op_mode ) ? 'w' : 'l', effective_string(), op_reg );
            else if ( bit8 && 1 == ea_mode ) // cmpm
                tracer.Trace( "cmpm.%c (a%u)+, (a%u)+\n", get_size(), ea_reg, op_reg );
            else if ( bit8 ) // eor
                tracer.Trace( "eor.%c d%u, %s\n", get_size(), op_reg, effective_string() );
            else
                unhandled();
            break;
        }
        case 0xc: // mulu / muls / abcd / exg / and
        {
            uint16_t bit8 = opbit( 8 );
            uint16_t bits7_6 = opbits( 6, 2);
            uint16_t bits5_4 = opbits( 4, 2 );

            if ( bit8 && 0 == bits7_6 && 0 == bits5_4 ) // abcd
            {
                if ( opbit( 3 ) ) // address register mode
                    tracer.Trace( "abcd -(a%u), -(a%u)\n", ea_reg, op_reg );
                else // data register mode
                    tracer.Trace( "abcd d%u, d%u\n", ea_reg, op_reg );
            }
            else if ( !bit8 && 3 == bits7_6 ) // mulu
            {
                op_size = 1;
                tracer.Trace( "mulu.w %s, d%u\n", effective_string(), op_reg );
            }
            else if ( bit8 && 3 == bits7_6 ) // muls
            {
                op_size = 1;
                tracer.Trace( "muls.w %s, d%u\n", effective_string(), op_reg );
            }
            else if ( bit8 && 0 == bits5_4 ) // exg
            {
                uint16_t the_op_mode = opbits( 3, 5 );
                if ( 8 == the_op_mode )
                    tracer.Trace( "exg d%u, d%u\n", op_reg, ea_reg );
                else if ( 9 == the_op_mode )
                    tracer.Trace( "exg d%u, a%u\n", op_reg, ea_reg );
                else if ( 0x11 == the_op_mode )
                    tracer.Trace( "exg a%u, a%u\n", op_reg, ea_reg );
                else
                    unhandled();
            }
            else // and
            {
                if ( bit8 )
                    tracer.Trace( "and.%c d%u, %s\n", get_size(), op_reg, effective_string() );
                else
                    tracer.Trace( "and.%c %s, d%u\n", get_size(), effective_string(), op_reg );
            }
            break;
        }
        case 0xd: // add / addx / adda
        {
            if ( 3 == opbits( 6, 2 ) ) // adda
            {
                op_size = opbit( 8 ) ? 2 : 1;
                tracer.Trace( "adda.%c %s, a%u\n", opbit( 8 ) ? 'l' : 'w', effective_string(), op_reg );
            }
            else if ( 0 == opbits( 4, 2 ) && opbit( 8 ) ) // addx
            {
                if ( opbit( 3 ) ) // true if both operands are memory with a register predecrement mode, false if both are d registers
                    tracer.Trace( "addx.%c -(a%u), -(a%u)\n", get_size(), ea_reg, op_reg );
                else
                    tracer.Trace( "addx.%c d%u, d%u\n", get_size(), ea_reg, op_reg );
            }
            else // add
            {
                if ( 0 != ( op_mode & 4 ) ) // Dn + <ea> => ea
                    tracer.Trace( "add.%c d%u, %s\n", get_size(), op_reg, effective_string() );
                else // <ea> + Dn => Dn
                    tracer.Trace( "add.%c %s, d%u\n", get_size(), effective_string(), op_reg );
            }
            break;
        }
        case 0xe: // asl / asr / lsl / lsr / ROXL / ROXR / ROL / ROR
        {
            uint16_t bits4_3 = opbits( 3, 2 );
            if ( 0 == op_reg && 3 == op_size ) // ASd memory
                tracer.Trace( "as%c.w %s\n", opbit( 8 ) ? 'l' : 'r', effective_string() );
            else if ( 1 == op_reg && 3 == op_size ) // LSd memory
                tracer.Trace( "ls%c.w %s\n", opbit( 8 ) ? 'l' : 'r', effective_string() );
            else if ( 2 == op_reg && 3 == op_size ) // ROXd memory
                tracer.Trace( "rox%c.w %s\n", opbit( 8 ) ? 'l' : 'r', effective_string() );
            else if ( 3 == op_reg && 3 == op_size ) // ROd memory
                tracer.Trace( "ro%c.w %s\n", opbit( 8 ) ? 'l' : 'r', effective_string() );
            else if ( 0 == bits4_3 ) // ASd
            {
                bool is_left = opbit( 8 );
                uint16_t shift = 0;
                bool is_imm = !opbit( 5 );
                if ( is_imm )
                {
                    shift = op_reg;
                    if ( 0 == shift )
                        shift = 8;
                }
                else
                    shift = dregs[ op_reg ].l % 64;

                tracer.Trace( "as%c.%c #%u, d%u\n", is_left ? 'l' : 'r', get_size(), shift, ea_reg );
            }
            else if ( 1 == bits4_3 ) // LSd
            {
                bool is_left = opbit( 8 );
                bool is_imm = !opbit( 5 );
                if ( is_imm )
                {
                    uint16_t shift = ( 0 == op_reg ) ? 8 : op_reg;
                    tracer.Trace( "ls%c.%c #%u, d%u\n", is_left ? 'l' : 'r', get_size(), shift, ea_reg );
                }
                else
                    tracer.Trace( "ls%c.%c d%u, d%u\n", is_left ? 'l' : 'r', get_size(), op_reg, ea_reg );
            }
            else if ( 2 == bits4_3 ) // ROXd
            {
                bool is_left = opbit( 8 );
                bool is_imm = !opbit( 5 );
                if ( is_imm )
                {
                    uint16_t shift = ( 0 == op_reg ) ? 8 : op_reg;
                    tracer.Trace( "rox%c.%c #%u, d%u\n", is_left ? 'l' : 'r', get_size(), shift, ea_reg );
                }
                else
                    tracer.Trace( "rox%c.%c d%u, d%u\n", is_left ? 'l' : 'r', get_size(), op_reg, ea_reg );
            }
            else if ( 3 == bits4_3 ) // ROd
            {
                bool is_left = opbit( 8 );
                bool is_imm = !opbit( 5 );
                if ( is_imm )
                {
                    uint16_t shift = ( 0 == op_reg ) ? 8 : op_reg;
                    tracer.Trace( "ror%c.%c #%u, d%u\n", is_left ? 'l' : 'r', get_size(), shift, ea_reg );
                }
                else
                    tracer.Trace( "ror%c.%c d%u, d%u\n", is_left ? 'l' : 'r', get_size(), op_reg, ea_reg );
            }
            break;
        }
        default:
            unhandled();
    }

    //tracer.Trace( "b224: %#x\n", getui32( 0xb224 ) );
    //tracer.Trace( "80a28964: " ); tracer.TraceBinaryData( getmem( 0x80a28964 ), 4, 4 );

    pc = save_pc;
    op_size = save_op_size;
} //trace_state

template < typename T, typename W > inline void m68000::set_flags( T a, T b, T result, W result_wide, bool setx, bool xbehavior )
{
    assert( sizeof( W ) == ( 2 * sizeof( T ) ) );
    T signbit = (T) ( 1 << ( sizeof( T ) * 8 - 1 ) );
    bool a_neg = ( 0 != ( signbit & a ) );
    bool b_neg = ( 0 != ( signbit & b ) );
    bool r_neg = ( 0 != ( signbit & result ) );

    setflag_n( r_neg );
    if ( xbehavior )
    {
        if ( 0 != result )
            setflag_z( false );
    }
    else
        setflag_z( 0 == result );
    setflag_c( 0 != ( result_wide & ( ( (W) signbit ) << 1 ) ) );
    setflag_v( ( b_neg == r_neg ) && ( a_neg != b_neg ) );
    if ( setx )
        setflag_x( flag_c() );
} //set_flags

uint32_t m68000::sub32( uint32_t a, uint32_t b, bool setflags, bool setx, bool subx )
{
    uint64_t result_wide = (uint64_t) a - (uint64_t) b - ( subx ? (uint64_t) flag_x() : 0 );
    uint32_t result = (uint32_t) result_wide;

    if ( setflags )
        set_flags( a, b, result, result_wide, setx, subx );

    return result;
} //sub32

uint16_t m68000::sub16( uint16_t a, uint16_t b, bool setflags, bool setx, bool subx )
{
    uint32_t result_wide = (uint32_t) a - (uint32_t) b - ( subx ? (uint32_t) flag_x() : 0 );
    uint16_t result = (uint16_t) result_wide;

    if ( setflags )
        set_flags( a, b, result, result_wide, setx, subx );

    return result;
} //sub16

uint8_t m68000::sub8( uint8_t a, uint8_t b, bool setflags, bool setx, bool subx )
{
    uint16_t result_wide = (uint16_t) a - (uint16_t) b - ( subx ? (uint16_t) flag_x() : 0 );
    uint8_t result = (uint8_t) result_wide;

    if ( setflags )
        set_flags( a, b, result, result_wide, setx, subx );

    return result;
} //sub8

uint32_t m68000::add32( uint32_t a, uint32_t b, bool setflags, bool setx, bool addx )
{
    uint64_t result_wide = (uint64_t) a + (uint64_t) b + ( addx ? (uint64_t) flag_x() : 0 );
    uint32_t result = (uint32_t) result_wide;

    if ( setflags )
        set_flags( a, b, result, result_wide, setx, addx );

    return result;
} //add32

uint16_t m68000::add16( uint16_t a, uint16_t b, bool setflags, bool setx, bool addx )
{
    uint32_t result_wide = (uint32_t) a + (uint32_t) b + ( addx ? (uint32_t) flag_x() : 0 );
    uint16_t result = (uint16_t) result_wide;

    if ( setflags )
        set_flags( a, b, result, result_wide, setx, addx );

    return result;
} //add16

uint8_t m68000::add8( uint8_t a, uint8_t b, bool setflags, bool setx, bool addx )
{
    uint16_t result_wide = (uint16_t) a + (uint16_t) b + ( addx ? (uint16_t) flag_x() : 0 );
    uint8_t result = (uint8_t) result_wide;

    if ( setflags )
        set_flags( a, b, result, result_wide, setx, addx );

    return result;
} //add8

uint8_t m68000::bcd_add( uint8_t a, uint8_t b )
{
    uint8_t carry_lo = 0;
    uint8_t sum_lo = ( a & 0xf ) + ( b & 0xf ) + (uint8_t) flag_x();
    if ( sum_lo >= 10 )
    {
        carry_lo = 1;
        sum_lo -= 10;
    }

    uint8_t sum_hi = ( ( a >> 4 ) & 0xf ) + ( ( b >> 4 ) & 0xf ) + carry_lo;
    if ( sum_hi >= 10 )
    {
        setflag_c( true );
        sum_hi -= 10;
    }
    else
        setflag_c( false );

    uint8_t result = ( sum_hi << 4 ) | sum_lo;
    setflag_x( flag_c() );
    if ( 0 != result )
        setflag_z( false );
    return result;
} //bcd_add

uint8_t m68000::bcd_sub( uint8_t a, uint8_t b )
{
    uint8_t borrow_lo = 0;
    uint8_t diff_lo = ( a & 0xf ) - ( b & 0xf ) - (uint8_t) flag_x();
    if ( diff_lo >= 10 )
    {
        borrow_lo = 1;
        diff_lo += 10;
    }

    uint8_t diff_hi = ( ( a >> 4 ) & 0xf ) - ( ( b >> 4 ) & 0xf ) - borrow_lo;
    if ( diff_hi >= 10 )
    {
        setflag_c( true );
        diff_hi += 10;
    }
    else
        setflag_c( false );

    uint8_t result = ( diff_hi << 4 ) | diff_lo;
    setflag_x( flag_c() );
    if ( 0 != result )
        setflag_z( false );
    return result;
} //bcd_sub

bool m68000::check_condition( uint16_t c )
{
    switch( c )
    {
        case  0: return true;                                        // T -- normal branch
        case  1: return false;                                       // F -- subroutine
        case  2: return ! ( flag_c() || flag_z() );                  // higher
        case  3: return flag_c() || flag_z();                        // lower or same
        case  4: return ! flag_c();                                  // carry clear
        case  5: return flag_c();                                    // carry set
        case  6: return ! flag_z();                                  // not equal
        case  7: return flag_z();                                    // equal
        case  8: return ! flag_v();                                  // overflow clear
        case  9: return flag_v();                                    // overflow set
        case 10: return ! flag_n();                                  // plus (>= 0)
        case 11: return flag_n();                                    // minus (< 0)
        case 12: return ( flag_n() == flag_v() );                    // greater than or equal
        case 13: return ( flag_n() != flag_v() );                    // less than
        case 14: return ( ! flag_z() && ( flag_n() == flag_v() ) );  // greater than
        default: return ( flag_z() || ( flag_n() != flag_v() ) );    // less than or equal
    }

    assume_false;
} //check_condition

static const char * get_vector( uint16_t vector )
{
    switch( vector )
    {
        case  0: return "(not a vector) reset sp";
        case  1: return "(not a vector) reset pc";
        case  2: return "bus error";
        case  3: return "address error";
        case  4: return "illegal instruction";
        case  5: return "division by zero";
        case  6: return "chk instruction out of bounds";
        case  7: return "trapv";
        case  8: return "privilege violation";
        case  9: return "trace";
        case 10: return "unimplemented instruction line A";
        case 11: return "unimplemented instruction line F";
        default: return "unknown";
    }
} //get_vector

bool m68000::handle_trap( uint16_t vector, uint32_t pc_return )
{
    if ( 0 != base ) // likely running an .elf executable built with a modern compiler; not cpm 68k.
    {
        tracer.Trace( "base isn't 0 so no trap vectors are available\n" );
        return false;
    }

    if ( 5 == vector && 0 == getui32( vector * 4 ) ) // if divide by 0 and no vector is available, let the poor app run
    {
        tracer.Trace( "divide by 0 but no handler is available, so ignoring\n" );
        return false;
    }

    uint16_t original_sr = sr;
    set_supervisor_state();
    push( pc_return );
    push16( original_sr );
    pc = getui32( vector * 4 );
    if ( 0 == pc )
    {
        tracer.Trace( "no trap handler for %u == %s\n", vector, get_vector( vector ) );
        printf( "no trap handler for %u == %s\n", vector, get_vector( vector ) );
        emulator_hard_termination( *this, "trap vector entry is null.", vector );
    }

    tracer.Trace( "invoked trap %u == %s\n", vector, get_vector( vector ) );
    return true;
} //handle_trap

#ifdef _WIN32
__declspec(noinline)
#endif
void m68000::unhandled()
{
    printf( "unhandled op %x\n", op );
    tracer.Trace( "unhandled op %x\n", op );
    handle_trap( 4, pc ); // 4 is illegal instruction.
    emulator_hard_termination( *this, "opcode not handled:", op ); // there won't be a a handler for gcc apps so terminate
} //unhandled

template < typename T > inline void do_swap( T & a, T & b ) { T tmp = a; a = b; b = tmp; }

uint64_t m68000::run()
{
    tracer.Trace( "code at pc %x:\n", pc );
    tracer.TraceBinaryData( getmem( pc ), 128, 4 );

    bool skip_trace = false;
    uint64_t cycles = 0;

    for ( ;; )
    {
        #ifndef NDEBUG
            if ( enforce_pc_sp_constraints )
            {
                if ( aregs[ 7 ] > ( stack_top + 0x1000 ) ) // cut some slack to read argc/argv etc
                    emulator_hard_termination( *this, "stack pointer is above the top of its starting point:", aregs[ 7 ] );

                if ( pc < base )
                    emulator_hard_termination( *this, "pc is lower than memory:", pc );

                if ( pc >= ( base + mem_size - stack_size ) )
                    emulator_hard_termination( *this, "pc is higher than it should be:", pc );
            }

            if ( 0 != ( aregs[ 7 ] & 1 ) ) // to avoid alignment faults
                emulator_hard_termination( *this, "the stack pointer isn't 2-byte aligned:", aregs[ 7 ] );

            if ( 0 != ( pc & 1 ) ) // to avoid alignment faults
                emulator_hard_termination( *this, "the pc isn't 2-byte aligned:", pc );
        #endif

        // fetch and decode the opcode. few instructions use all of these, but it's simpler to decode all at once.
        // 15 14 13 12   11 10 09   08 07     06   05 04  03    02 01 00
        // ----hi4----   -op_reg-      -op_size-   -ea_mode-    -ea_reg-
        //                          --op_mode---

        op = getui16( pc );              // 21% of runtime doing this decoding
        uint16_t t = op;
        ea_reg = t & 7;
        t >>= 3;
        ea_mode = t & 7;
        t >>= 3;
        op_mode = t & 7;
        op_size = op_mode & 3;           // this is generally accurate, but instructions like move have size elsewhere
        t >>= 3;
        op_reg = t & 7;
        hi4 = t >> 3;

        if ( 0 != g_State )              // 3.5% of runtime on this check
        {
            if ( g_State & stateEndEmulation )
            {
                g_State &= ~stateEndEmulation;
                break;
            }

            if ( ( g_State & stateInstructionTrace ) && ! flag_s() ) // no tracing in supervisor mode
            {
                if ( skip_trace ) // once tracing is enabled, execute one instruction and then trap.
                    skip_trace = false;
                else
                {
                    g_State &= ~stateInstructionTrace;
                    handle_trap( 9, pc );
                    continue;
                }
            }

            if ( g_State & stateTraceInstructions )
                trace_state();
        }

        switch ( hi4 )                   // 16% of runtime setting up for the switch
        {
            case 0: // many math and cmp instructions. I coded the below as a switch() on the second highest nibble and that was slower
            {
                uint16_t bits11_8 = opbits( 8, 4 );
                uint16_t bits11_6 = opbits( 6, 6 );

                if ( 0x003c == op ) // ori to CCR
                {
                    pc += 2;
                    sr |= ( getui16( pc ) & 0xff );
                }
                else if ( 0x007c == op ) // ori to SR
                {
                    if ( !flag_s() )
                        if ( handle_trap( 8, pc ) ) // not in supervisor state
                            continue;

                    pc += 2;
                    sr |= getui16( pc );
                    perhaps_restore_usermode_state();
                }
                else if ( 0x023c == op ) // andi to CCR
                {
                    pc += 2;
                    sr &= ( 0xff00 | ( getui16( pc ) & 0xff ) );
                }
                else if ( 0x027c == op ) // andi to SR
                {
                    if ( !flag_s() )
                        if ( handle_trap( 8, pc ) ) // not in supervisor state
                            continue;

                    pc += 2;
                    sr &= getui16( pc );
                    perhaps_restore_usermode_state();
                }
                else if ( 0x0a3c == op ) // eori to CCR
                {
                    pc += 2;
                    sr ^= ( getui16( pc ) & 0xff );
                }
                else if ( 0x0a7c == op ) // eori to SR
                {
                    if ( !flag_s() )
                        if ( handle_trap( 8, pc ) ) // not in supervisor state
                            continue;

                    pc += 2;
                    sr ^= getui16( pc );
                    perhaps_restore_usermode_state();
                }
                else if ( 0 == bits11_8 ) // ori
                {
                    if ( 1 == ea_mode ) // likely a 68020+ instruction
                        unhandled();

                    pc += 2;
                    if ( 0 == ea_mode )
                    {
                        if ( 0 == op_size )
                        {
                            dregs[ ea_reg ].b |= (uint8_t) getui16( pc );
                            set_nzcv8( dregs[ ea_reg ].b );
                        }
                        else if ( 1 == op_size )
                        {
                            dregs[ ea_reg ].w |= getui16( pc );
                            set_nzcv16( dregs[ ea_reg ].w );
                        }
                        else
                        {
                            uint32_t imm = getui32( pc );
                            pc += 2;
                            dregs[ ea_reg ].l |= imm;
                            set_nzcv32( dregs[ ea_reg ].l );
                        }
                    }
                    else
                    {
                        if ( 0 == op_size )
                        {
                            uint8_t imm = (uint8_t) getui16( pc );
                            uint32_t dst = effective_address();
                            imm |= getui8( dst );
                            setui8( dst, imm );
                            set_nzcv8( imm );
                        }
                        else if ( 1 == op_size )
                        {
                            uint16_t imm = getui16( pc );
                            uint32_t dst = effective_address();
                            imm |= getui16( dst );
                            setui16( dst, imm );
                            set_nzcv16( imm );
                        }
                        else
                        {
                            uint32_t imm = getui32( pc );
                            pc += 2;
                            uint32_t dst = effective_address();
                            imm |= getui32( dst );
                            setui32( dst, imm );
                            set_nzcv32( imm );
                        }
                    }
                }
                else if ( 2 == bits11_8 ) // andi
                {
                    pc += 2;
                    if ( 0 == ea_mode )
                    {
                        if ( 0 == op_size )
                        {
                            dregs[ ea_reg ].b &= (uint8_t) getui16( pc );
                            set_nzcv8( dregs[ ea_reg ].b );
                        }
                        else if ( 1 == op_size )
                        {
                            dregs[ ea_reg ].w &= getui16( pc );
                            set_nzcv16( dregs[ ea_reg ].w );
                        }
                        else
                        {
                            uint32_t imm = getui32( pc );
                            pc += 2;
                            dregs[ ea_reg ].l &= imm;
                            set_nzcv32( dregs[ ea_reg ].l );
                        }
                    }
                    else
                    {
                        if ( 0 == op_size )
                        {
                            uint8_t imm = (uint8_t) getui16( pc );
                            uint32_t dst = effective_address();
                            imm &= getui8( dst );
                            setui8( dst, imm );
                            set_nzcv8( imm );
                        }
                        else if ( 1 == op_size )
                        {
                            uint16_t imm = getui16( pc );
                            uint32_t dst = effective_address();
                            imm &= getui16( dst );
                            setui16( dst, imm );
                            set_nzcv16( imm );
                        }
                        else
                        {
                            uint32_t imm = getui32( pc );
                            pc += 2;
                            uint32_t dst = effective_address();
                            imm &= getui32( dst );
                            setui32( dst, imm );
                            set_nzcv32( imm );
                        }
                    }
                }
                else if ( 0xa == bits11_8 ) // eori
                {
                    pc += 2;
                    if ( 0 == ea_mode )
                    {
                        if ( 0 == op_size )
                        {
                            dregs[ ea_reg ].b ^= (uint8_t) getui16( pc );
                            set_nzcv8( dregs[ ea_reg ].b );
                        }
                        else if ( 1 == op_size )
                        {
                            dregs[ ea_reg ].w ^= getui16( pc );
                            set_nzcv16( dregs[ ea_reg ].w );
                        }
                        else
                        {
                            uint32_t imm = getui32( pc );
                            pc += 2;
                            dregs[ ea_reg ].l ^= imm;
                            set_nzcv32( dregs[ ea_reg ].l );
                        }
                    }
                    else
                    {
                        if ( 0 == op_size )
                        {
                            uint8_t imm = (uint8_t) getui16( pc );
                            uint32_t dst = effective_address();
                            imm ^= getui8( dst );
                            setui8( dst, imm );
                            set_nzcv8( imm );
                        }
                        else if ( 1 == op_size )
                        {
                            uint16_t imm = getui16( pc );
                            uint32_t dst = effective_address();
                            imm ^= getui16( dst );
                            setui16( dst, imm );
                            set_nzcv16( imm );
                        }
                        else
                        {
                            uint32_t imm = getui32( pc );
                            pc += 2;
                            uint32_t dst = effective_address();
                            imm ^= getui32( dst );
                            setui32( dst, imm );
                            set_nzcv32( imm );
                        }
                    }
                }
                else if ( 0xc == bits11_8 ) // cmpi
                {
                    if ( 0 == op_size )
                    {
                        pc += 2;
                        uint8_t imm = getui16( pc ) & 0xff;
                        uint8_t val = effective_value8( effective_address() ); // mode 7 reg 4 imm is illegal
                        sub8( val, imm, true, false, false );
                    }
                    else if ( 1 == op_size )
                    {
                        pc += 2;
                        uint16_t imm = getui16( pc );
                        uint16_t val = effective_value16( effective_address() ); // mode 7 reg 4 imm is illegal
                        sub16( val, imm, true, false, false );
                    }
                    else
                    {
                        uint32_t imm = getui32( pc + 2 );
                        pc += 4;
                        uint32_t val = effective_value32( effective_address() );
                        sub32( val, imm, true, false, false );
                    }
                }
                else if ( 4 == bits11_8 ) // subi
                {
                    if ( 0 == op_size )
                    {
                        pc += 2;
                        uint8_t imm = getui16( pc ) & 0xff;
                        uint32_t address = effective_address();
                        uint8_t val = effective_value8( address );
                        uint8_t result = sub8( val, imm, true, true, false );
                        if ( 0 == ea_mode )
                            dregs[ ea_reg ].b = result;
                        else
                            setui8( address, result );
                    }
                    else if ( 1 == op_size )
                    {
                        pc += 2;
                        uint16_t imm = getui16( pc );
                        uint32_t address = effective_address();
                        uint16_t val = effective_value16( address );
                        uint16_t result = sub16( val, imm, true, true, false );
                        if ( 0 == ea_mode )
                            dregs[ ea_reg ].w = result;
                        else
                            setui16( address, result );
                    }
                    else
                    {
                        uint32_t imm = getui32( pc + 2 );
                        pc += 4;
                        uint32_t address = effective_address();
                        uint32_t val = effective_value32( address );
                        uint32_t result = sub32( val, imm, true, true, false );
                        if ( 0 == ea_mode )
                            dregs[ ea_reg ].l = result;
                        else
                            setui32( address, result );
                    }
                }
                else if ( 6 == bits11_8 ) // addi
                {
                    if ( 0 == op_size )
                    {
                        pc += 2;
                        uint8_t imm = getui16( pc ) & 0xff;
                        uint32_t address = effective_address();
                        uint8_t val = effective_value8( address );
                        uint8_t result = add8( val, imm, true, true, false );
                        if ( 0 == ea_mode )
                            dregs[ ea_reg ].b = result;
                        else
                            setui8( address, result );
                    }
                    else if ( 1 == op_size )
                    {
                        pc += 2;
                        uint16_t imm = getui16( pc );
                        uint32_t address = effective_address();
                        uint16_t val = effective_value16( address );
                        uint16_t result = add16( val, imm, true, true, false );
                        if ( 0 == ea_mode )
                            dregs[ ea_reg ].w = result;
                        else
                            setui16( address, result );
                    }
                    else
                    {
                        uint32_t imm = getui32( pc + 2 );
                        pc += 4;
                        uint32_t address = effective_address();
                        uint32_t val = effective_value32( address );
                        uint32_t result = add32( val, imm, true, true, false );
                        if ( 0 == ea_mode )
                            dregs[ ea_reg ].l = result;
                        else
                            setui32( address, result );
                    }
                }
                else if ( opbit( 8 ) && 1 == ea_mode ) // movep
                {
                    pc += 2;
                    int32_t displacement = (int32_t) (int16_t) getui16( pc );
                    uint32_t address = aregs[ ea_reg ] + displacement;
                    if ( 0 == opbit( 7 ) ) // memory to register
                    {
                        if ( opbit( 6 ) ) // long
                        {
                            uint32_t val = getui8( address ) << 24;
                            address += 2;
                            val |= getui8( address ) << 16;
                            address += 2;
                            val |= getui8( address ) << 8;
                            address += 2;
                            val |= getui8( address );
                            dregs[ op_reg ].l = val;
                        }
                        else
                        {
                            uint16_t val = getui8( address ) << 8;
                            address += 2;
                            val |= getui8( address );
                            dregs[ op_reg ].w = val;
                        }
                    }
                    else // register to memory
                    {
                        uint32_t val = dregs[ op_reg ].l;
                        if ( opbit( 6 ) ) // long
                        {
                            setui8( address, val >> 24 );
                            address += 2;
                            setui8( address, 0xff & ( val >> 16 ) );
                            address += 2;
                        }
                        setui8( address, 0xff & ( val >> 8 ) );
                        address += 2;
                        setui8( address, 0xff & val );
                    }
                }
                else if ( 0x20 == bits11_6 || 4 == op_mode ) // btst
                {
                    uint32_t bitnum;

                    if ( 0x20 == bits11_6 )
                    {
                        pc += 2;
                        bitnum = getui16( pc ) & 0xff;
                    }
                    else
                        bitnum = dregs[ op_reg ].l;

                    if ( 0 == ea_mode )
                    {
                        bitnum %= 32;
                        setflag_z( 0 == ( ( 1 << bitnum ) & dregs[ ea_reg ].l ) );
                    }
                    else
                    {
                        bitnum %= 8;
                        uint8_t val = effective_value8( effective_address() );
                        setflag_z( 0 == ( ( 1 << bitnum ) & val ) );
                    }
                }
                else if ( 0x22 == bits11_6 || 6 == op_mode ) // bclr
                {
                    uint32_t bitnum;

                    if ( 0x22 == bits11_6 )
                    {
                        pc += 2;
                        bitnum = getui16( pc ) & 0xff;;
                    }
                   else
                        bitnum = dregs[ op_reg ].l;

                    if ( 0 == ea_mode )
                    {
                        bitnum %= 32;
                        setflag_z( 0 == get_bit32( dregs[ ea_reg ].l, bitnum ) );
                        dregs[ ea_reg ].l &= ( ~ ( 1 << bitnum ) );
                    }
                    else
                    {
                        bitnum %= 8;
                        op_size = 0; // always 1 byte when ea_mode <> 0
                        uint32_t address = effective_address();
                        uint8_t val = effective_value8( address );
                        setflag_z( 0 == get_bit16( val, bitnum ) );
                        val &= ( ~ ( 1 << bitnum ) );
                        setui8( address, val );
                    }
                }
                else if ( 0x21 == bits11_6 || 5 == op_mode ) // bchg
                {
                    uint32_t bitnum;

                    if ( 0x21 == bits11_6 )
                    {
                        pc += 2;
                        bitnum = getui16( pc ) & 0xff;;
                    }
                   else
                        bitnum = dregs[ op_reg ].l;

                    if ( 0 == ea_mode )
                    {
                        bitnum %= 32;
                        setflag_z( 0 == get_bit32( dregs[ ea_reg ].l, bitnum ) );
                        if ( flag_z() )
                            dregs[ ea_reg ].l |= ( 1 << bitnum );
                        else
                            dregs[ ea_reg ].l &= ( ~ ( 1 << bitnum ) );
                    }
                    else
                    {
                        bitnum %= 8;
                        op_size = 0; // always a byte operation
                        uint32_t address = effective_address();
                        uint8_t val = effective_value8( address );
                        setflag_z( 0 == get_bit16( val, bitnum ) );
                        if ( flag_z() )
                            val |= ( 1 << bitnum );
                        else
                            val &= ( ~ ( 1 << bitnum ) );
                        setui8( address, val );
                    }
                }
                else if ( 0x23 == bits11_6 || 7 == op_mode ) // bset
                {
                    uint32_t bitnum;

                    if ( 0x23 == bits11_6 )
                    {
                        pc += 2;
                        bitnum = getui16( pc ) & 0xff;;
                    }
                   else
                        bitnum = dregs[ op_reg ].l;

                    if ( 0 == ea_mode )
                    {
                        bitnum %= 32;
                        setflag_z( 0 == get_bit32( dregs[ ea_reg ].l, bitnum ) );
                        dregs[ ea_reg ].l |= ( 1 << bitnum );
                    }
                    else
                    {
                        bitnum %= 8;
                        op_size = 0; // always a byte operation
                        uint32_t address = effective_address();
                        uint8_t val = effective_value8( address );
                        setflag_z( 0 == get_bit16( val, bitnum ) );
                        val |= ( 1 << bitnum );
                        setui8( address, val );
                    }
                }
                else
                    unhandled();

                break;
            }
            case 1: // move.b
            case 2: // move.l + movea.l
            case 3: // move.w + movea.w
            {
                op_size = ( 1 == hi4 ) ? 0 : ( 3 == hi4 ) ? 1 : 2; // map move size to op_size

                if ( 1 == opbits( 6, 3 ) ) // movea.l / movea.w
                {
                    if ( 2 == op_size )
                        aregs[ op_reg ] = effective_value32( effective_address() );
                    else
                        aregs[ op_reg ] = sign_extend( effective_value16( effective_address() ), 15 );
                }
                else // move.l / move.w / move.b
                {
                    uint32_t address = effective_address();
                    if ( 0 == op_mode )
                    {
                        if ( 0 == op_size )
                        {
                            uint8_t src = effective_value8( address );
                            dregs[ op_reg ].b = src;
                            set_nzcv8( src );
                        }
                        else if ( 1 == op_size )
                        {
                            uint16_t src = effective_value16( address );
                            dregs[ op_reg ].w = src;
                            set_nzcv16( src );
                        }
                        else
                        {
                            uint32_t src = effective_value32( address );
                            dregs[ op_reg ].l = src;
                            set_nzcv32( src );
                        }
                    }
                    else if ( 1 == op_mode )
                        unhandled(); // aregs[ op_reg ] = effective_value32( address );
                    else
                    {
                        if ( 0 == op_size )
                        {
                            uint8_t src = effective_value8( address );
                            uint32_t dst = effective_address2( op_mode, op_reg );
                            setui8( dst, src );
                            //tracer.Trace( "wrote byte %#x to address %#x\n", src, dst );
                            set_nzcv8( src );

#if 0
                            // enormous hack. The gcc clib I'm using writes to 0xffffc when it wants to output a character to stdout
                            if ( 0xffffc == dst )
                            {
                                uint32_t save0 = dregs[ 0 ].l;
                                uint32_t save7 = dregs[ 7 ].l;

                                dregs[ 1 ].l = src; // the character to print
                                dregs[ 0 ].l = 0x2007; // emulator syscall to emit a character
                                emulator_invoke_svc( *this );

                                dregs[ 1 ].l = save0;
                                dregs[ 0 ].l = save7;
                            }
#endif
                        }
                        else if ( 1 == op_size )
                        {
                            uint16_t src = effective_value16( address );
                            uint32_t dst = effective_address2( op_mode, op_reg );
                            setui16( dst, src );
                            set_nzcv16( src );
                        }
                        else
                        {
                            uint32_t src = effective_value32( address );
                            uint32_t dst = effective_address2( op_mode, op_reg );
                            setui32( dst, src );
                            set_nzcv32( src );
                        }
                    }
                }
                break;
            }
            case 4:
            {
                if ( 0x4e75 == op ) // rts
                {
                    pc = pop();
                    continue;
                }
                else if ( 0x4e73 == op ) // rte
                {
                    if ( !flag_s() )
                        if ( handle_trap( 8, pc ) ) // not in supervisor state
                            continue;

                    sr = pop16();
                    if ( flag_t() )
                    {
                        g_State |= stateInstructionTrace;
                        skip_trace = true;
                    }
                    pc = pop(); // pop from the supervisor stack
                    perhaps_restore_usermode_state();
                    continue;
                }
                else if ( 0x4e73 == op ) // trapv
                {
                    if ( handle_trap( 0x1c, pc + 2 ) )
                        continue;
                }
                else if ( 0x4e77 == op ) // rtr
                {
                    sr = ( ( sr & 0xff00 ) | ( pop16() & 0xff ) );
                    pc = pop();
                }
                else if ( 0x4e70 == op ) // reset
                {
                    // do nothing. should assert -----RESET line for 124 clock periods but no other impact
                }
                else if ( 0x4e72 == op ) // stop
                {
#if 0
                    // cp68.68k, the first pass of the C compiler, has a stop instruction but it's not running as supervisor.
                    // mimic cp/m 68k behavior by pretending there is a handler that allows it.

                    if ( !flag_s() )
                        if ( handle_trap( 8, pc ) ) // not in supervisor state
                            continue;
#endif

                    bool was_super = flag_s();
                    pc += 2;
                    sr = getui16( pc );
                    if ( was_super )
                        perhaps_restore_usermode_state();

                    // the gnu cc I'm using invokes this instruction in exit(). make a linux standard exit call
                    // it's also used by the cp/m 68k emulator when an app returns from its entrypoint

                    dregs[ 1 ].l = dregs[ 0 ].l; // exit code. probably not really there but it's worth a try
                    dregs[ 0 ].l = 93; // exit syscall
                    emulator_invoke_svc( *this );
                }
                else if ( 0x4e71 == op ) {} // nop
                else if ( 0x4afc == op ) // illegal
                {
                    if ( handle_trap( 4, pc ) ) // likely DDT's breakpoint. Want to return to this location after the interrupt restores the original instruction.
                        continue;
                }
                else
                {
                    uint16_t bits11_8 = opbits( 8, 4 );
                    uint16_t bits11_7 = opbits( 7, 5 );
                    uint32_t bits11_6 = opbits( 6, 6 );
                    uint16_t bits11_4 = opbits( 4, 8 );
                    uint16_t bits11_3 = opbits( 3, 9 );

                    if ( 3 == bits11_6 ) // move from sr. Unpriviledged. Popek and Goldberg be damned.
                    {
                        if ( 0 == ea_mode )
                            dregs[ ea_reg ].w = sr;
                        else
                        {
                            op_size = 1;
                            setui16( effective_address(), sr );
                        }
                    }
                    else if ( 0x13 == bits11_6 ) // move to ccr
                    {
                        op_size = 1;
                        sr = ( sr & 0xff00 ) | ( 0xff & effective_value16( effective_address() ) );
                    }
                    else if ( 0x1b == bits11_6 ) // move to sr
                    {
                        if ( !flag_s() )
                            if ( handle_trap( 8, pc ) ) // not in supervisor state
                                continue;

                        op_size = 1;
                        sr = effective_value16( effective_address() );
                        if ( flag_t() )
                        {
                            g_State |= stateInstructionTrace;
                            skip_trace = true;
                        }
                        perhaps_restore_usermode_state();
                    }
                    else if ( 4 == bits11_8 ) // neg
                    {
                        if ( 0 == ea_mode )
                        {
                            if ( 0 == op_size )
                                dregs[ ea_reg ].b = sub8( 0, dregs[ ea_reg ].b, true, true, false );
                            else if ( 1 == op_size )
                                dregs[ ea_reg ].w = sub16( 0, dregs[ ea_reg ].w, true, true, false );
                            else
                                dregs[ ea_reg ].l = sub32( 0, dregs[ ea_reg ].l, true, true, false );
                        }
                        else
                        {
                            uint32_t address = effective_address();
                            if ( 0 == op_size )
                                setui8( address, sub8( 0, effective_value8( address ), true, true, false ) );
                            else if ( 1 == op_size )
                                setui16( address, sub16( 0, effective_value16( address ), true, true, false ) );
                            else
                                setui32( address, sub32( 0, effective_value32( address ), true, true, false ) );
                        }
                    }
                    else if ( 0xe4 == bits11_4 ) // trap
                    {
                        uint16_t vector = op & 0xf;
                        if ( 15 == vector )
                        {
                            emulator_invoke_68k_trap15( *this ); // 68k emulator
                            pc += 2;
                        }
                        else if ( 0 == vector )
                            emulator_invoke_svc( *this ); // linux-style syscall
                        else if ( 2 == vector )
                        {
                            uint32_t oldpc = pc;
                            emulator_invoke_68k_trap2( *this ); // digital research cp/m 68k bdos vector
                            if ( pc != oldpc ) // likely chained to a new program via bdos function 47
                                continue;
                        }
                        else if ( 3 == vector )
                            emulator_invoke_68k_trap3( *this ); // digital research cp/m 68k bios vector
                        else
                        {
                            if ( handle_trap( vector + 0x20, pc + 2 ) )
                                continue;
                        }
                    }
                    else if ( 0x11 == bits11_7 && 0 == ea_mode ) // ext
                    {
                        bool is_long = opbit( 6 );
                        if ( is_long )
                        {
                            dregs[ ea_reg ].l = sign_extend( dregs[ ea_reg ].l, 15 );
                            set_nzcv32( dregs[ ea_reg ].l );
                        }
                        else
                        {
                            dregs[ ea_reg ].w = sign_extend16( dregs[ ea_reg ].w, 7 );
                            set_nzcv16( dregs[ ea_reg ].w );
                        }
                    }
                    else if ( 0xa == bits11_8 ) // tst
                    {
                        uint32_t val = effective_address();

                        if ( 0 == op_size )
                            set_nzcv8( effective_value8( val ) );
                        else if ( 1 == op_size )
                            set_nzcv16( effective_value16( val ) );
                        else
                            set_nzcv32( effective_value32( val ) );
                    }
                    else if ( 0x1ca == bits11_3 ) // link
                    {
                        push( aregs[ ea_reg ] );
                        aregs[ ea_reg ] = aregs[ 7 ];
                        pc += 2;
                        int32_t offset = (int32_t) (int16_t) getui16( pc );
                        aregs[ 7 ] += offset;
                    }
                    else if ( 0x1cb == bits11_3 ) // unlk
                    {
                        aregs[ 7 ] = aregs[ ea_reg ];
                        aregs[ ea_reg ] = pop();
                    }
                    else if ( 0x21 == bits11_6 && 0 == ea_mode ) // swap
                    {
                        uint16_t highpart = dregs[ ea_reg ].l >> 16;
                        dregs[ ea_reg ].l = ( dregs[ ea_reg ].l << 16 ) | highpart;
                    }
                    else if ( 0x21 == bits11_6 ) // pea
                        push( effective_address() );
                    else if ( 0x3a == bits11_6 ) // jsr
                    {
                        uint32_t address = effective_address();
                        push( pc + 2 );
                        pc = address;
                        continue;
                    }
                    else if ( 0x3b == bits11_6 ) // jmp
                    {
                        pc = effective_address();
                        continue;
                    }
                    else if ( 0x22 == ( 0x2e & bits11_6 ) ) // movem
                    {
                        bool memory_to_register = opbit( 10 );
                        bool size_long = opbit( 6 );

                        pc += 2;
                        uint16_t mask = getui16( pc );
                        uint16_t bit = 1;

                        if ( size_long )
                        {
                            if ( 4 == ea_mode && !memory_to_register ) // pre decrement, register to memory
                            {
                                for ( uint16_t i = 0; i < 16; i++ )
                                {
                                    if ( mask & bit )
                                    {
                                        aregs[ ea_reg ] -= 4;
                                        if ( i >= 8 )
                                            setui32( aregs[ ea_reg ], dregs[ 15 - i ].l );
                                        else
                                            setui32( aregs[ ea_reg ], aregs[ 7 - i ] );
                                    }
                                    bit <<= 1;
                                }
                            }
                            else if ( 3 == ea_mode && memory_to_register ) // post increment, memory to register
                            {
                                for ( uint16_t i = 0; i < 16; i++ )
                                {
                                    if ( mask & bit )
                                    {
                                        if ( i >= 8 )
                                            aregs[ i - 8 ] = getui32( aregs[ ea_reg ] );
                                        else
                                            dregs[ i ].l = getui32( aregs[ ea_reg ] );
                                        aregs[ ea_reg ] += 4;
                                    }
                                    bit <<= 1;
                                }
                            }
                            else if ( memory_to_register ) // some ea including (d16,An), memory to register
                            {
                                uint32_t address = effective_address();
                                for ( uint16_t i = 0; i < 16; i++ )
                                {
                                    if ( mask & bit )
                                    {
                                        if ( i >= 8 )
                                            aregs[ i - 8 ] = getui32( address );
                                        else
                                            dregs[ i ].l = getui32( address );
                                        address += 4;
                                    }
                                    bit <<= 1;
                                }
                            }
                            else // movem list, ea, register to memory
                            {
                                uint32_t address = effective_address();
                                for ( uint16_t i = 0; i < 16; i++ )
                                {
                                    if ( mask & bit )
                                    {
                                        if ( i >= 8 )
                                            setui32( address, aregs[ i - 8 ] );
                                        else
                                            setui32( address, dregs[ i ].l );
                                        address += 4;
                                    }
                                    bit <<= 1;
                                }
                            }
                        }
                        else // word
                        {
                            if ( 4 == ea_mode && !memory_to_register ) // pre decrement, register to memory
                            {
                                for ( uint16_t i = 0; i < 16; i++ )
                                {
                                    if ( mask & bit )
                                    {
                                        aregs[ ea_reg ] -= 2;
                                        if ( i >= 8 )
                                            setui16( aregs[ ea_reg ], dregs[ 15 - i ].w );
                                        else
                                            setui16( aregs[ ea_reg ], (uint16_t) aregs[ 7 - i ] );
                                    }
                                    bit <<= 1;
                                }
                            }
                            else if ( 3 == ea_mode && memory_to_register ) // post increment, memory to register
                            {
                                for ( uint16_t i = 0; i < 16; i++ )
                                {
                                    if ( mask & bit )
                                    {
                                        if ( i >= 8 )
                                            aregs[ i - 8 ] = sign_extend( getui16( aregs[ ea_reg ] ), 15 );
                                        else
                                            dregs[ i ].l = sign_extend( getui16( aregs[ ea_reg ] ), 15 );
                                        aregs[ ea_reg ] += 2;
                                    }
                                    bit <<= 1;
                                }
                            }
                            else if ( memory_to_register ) // some ea including (d16,An), memory to register
                            {
                                uint32_t address = effective_address();
                                for ( uint16_t i = 0; i < 16; i++ )
                                {
                                    if ( mask & bit )
                                    {
                                        if ( i >= 8 )
                                            aregs[ i - 8 ] = sign_extend( getui16( address ), 15 );
                                        else
                                            dregs[ i ].l = sign_extend( getui16( address ), 15 );
                                        address += 2;
                                    }
                                    bit <<= 1;
                                }
                            }
                            else // movem list, ea, register to memory
                            {
                                uint32_t address = effective_address();
                                for ( uint16_t i = 0; i < 16; i++ )
                                {
                                    if ( mask & bit )
                                    {
                                        if ( i >= 8 )
                                            setui16( address, (uint16_t) aregs[ i - 8 ] );
                                        else
                                            setui16( address, dregs[ i ].w );
                                        address += 2;
                                    }
                                    bit <<= 1;
                                }
                            }
                        }
                    }
                    else if ( 7 == ( 7 & bits11_6 ) ) // lea
                        aregs[ op_reg ] = effective_address();
                    else if ( opbit( 8 ) && !opbit( 6 ) ) // chk
                    {
                        if ( opbit( 7 ) ) // word
                        {
                            op_size = 1;
                            int16_t val = (int16_t) effective_value16( effective_address() );
                            int16_t dval = (int16_t) dregs[ op_reg ].w;
                            if ( dval < 0 )
                            {
                                setflag_n( true );
                                if ( handle_trap( 6, pc + 2 ) )
                                    continue;
                            }
                            else if ( dval > val )
                            {
                                setflag_n( false );
                                if ( handle_trap( 6, pc + 2 ) )
                                    continue;
                            }
                        }
                        else // long. The documentation is ambiguous, but apparently only .w is supported
                        {
                            op_size = 2;
                            int32_t val = (int32_t) effective_value32( effective_address() );
                            int32_t dval = (int32_t) dregs[ op_reg ].l;
                            if ( dval < 0 )
                            {
                                setflag_n( true );
                                if ( handle_trap( 6, pc ) )
                                    continue;
                            }
                            else if ( dval > val )
                            {
                                setflag_n( false );
                                if ( handle_trap( 6, pc ) )
                                    continue;
                            }
                        }
                    }
                    else if ( 2 == bits11_8 ) // clr
                    {
                        if ( 0 == ea_mode )
                        {
                            if ( 0 == op_size )
                                dregs[ ea_reg ].b = 0;
                            else if ( 1 == op_size )
                                dregs[ ea_reg ].w = 0;
                            else
                                dregs[ ea_reg ].l = 0;
                        }
                        else
                        {
                            uint32_t address = effective_address();
                            if ( 0 == op_size )
                                setui8( address, 0 );
                            else if ( 1 == op_size )
                                setui16( address, 0 );
                            else
                                setui32( address, 0 );
                        }
                        set_nzcv32( 0 );
                    }
                    else if ( 0 == bits11_8 ) // negx
                    {
                        bool original_z = flag_z();

                        if ( 0 == ea_mode )
                        {
                            if ( 0 == op_size )
                            {
                                dregs[ ea_reg ].b = sub8( 0, dregs[ ea_reg ].b, true, true, true );
                                setflag_z( ( 0 != dregs[ ea_reg ].b ) ? false : original_z );
                            }
                            else if ( 1 == op_size )
                            {
                                dregs[ ea_reg ].w = sub16( 0, dregs[ ea_reg ].w, true, true, true );
                                setflag_z( ( 0 != dregs[ ea_reg ].w ) ? false : original_z );
                            }
                            else
                            {
                                dregs[ ea_reg ].l = sub32( 0, dregs[ ea_reg ].l, true, true, true );
                                setflag_z( ( 0 != dregs[ ea_reg ].l ) ? false : original_z );
                            }
                        }
                        else
                        {
                            uint32_t address = effective_address();
                            if ( 0 == op_size )
                                setui8( address, sub8( 0, effective_value8( address ), true, true, true ) );
                            else if ( 1 == op_size )
                                setui16( address, sub16( 0, effective_value16( address ), true, true, true ) );
                            else
                                setui32( address, sub32( 0, effective_value32( address ), true, true, true ) );
                        }
                    }
                    else if ( 6 == bits11_8 ) // not
                    {
                        if ( 0 == ea_mode )
                        {
                            if ( 0 == op_size )
                            {
                                dregs[ ea_reg ].b = ~dregs[ ea_reg ].b;
                                set_nzcv8( dregs[ ea_reg ].b );
                            }
                            else if ( 1 == op_size )
                            {
                                dregs[ ea_reg ].w = ~dregs[ ea_reg ].w;
                                set_nzcv16( dregs[ ea_reg ].w );
                            }
                            else
                            {
                                dregs[ ea_reg ].l = ~dregs[ ea_reg ].l;
                                set_nzcv32( dregs[ ea_reg ].l );
                            }
                        }
                        else
                        {
                            uint32_t address = effective_address();
                            if ( 0 == op_size )
                            {
                                uint8_t val = ~getui8( address );
                                set_nzcv8( val );
                                setui8( address, val );
                            }
                            else if ( 1 == op_size )
                            {
                                uint16_t val = ~getui16( address );
                                set_nzcv16( val );
                                setui16( address, val );
                            }
                            else
                            {
                                uint32_t val = ~getui32( address );
                                set_nzcv32( val );
                                setui32( address, val );
                            }
                        }
                    }
                    else if ( 0x31 == bits11_6 ) // divul.l (68020+)
                    {
                        pc += 2;
                        uint16_t dr_dq = getui16( pc );
                        uint16_t dr = dr_dq & 7;
                        uint16_t dq = ( dr_dq >> 12 ) & 7;   // really couldn't find the doc for this; lots of conflicting info online
                        uint32_t denom = effective_value32( effective_address() );
                        if ( 0 == denom )
                        {
                            if ( handle_trap( 5, pc ) )
                                continue;

                            dregs[ dq ].l = 0; // just be nice to bad apps
                            dregs[ dr ].l = 0;
                        }
                        else
                        {
                            uint32_t quotient = dregs[ dq ].l / denom;
                            uint32_t remainder = dregs[ dq ].l % denom;
                            dregs[ dq ].l = quotient;
                            dregs[ dr ].l = remainder;
                        }

                        setflag_c( false );
                        setflag_z( 0 == dregs[ dq ].l );
                        setflag_n( sign32( dregs[ dq ].l ) );
                    }
                    else if ( 0xe6 == bits11_4 ) // move usp
                    {
                        if ( opbit( 3 ) )
                            aregs[ ea_reg ] = usp;
                        else
                            usp = aregs[ ea_reg ];
                    }
                    else if ( 0x20 == bits11_6 ) // nbcd
                    {
                        if ( 0 == ea_mode )
                            dregs[ ea_reg ].b = bcd_sub( 0, dregs[ ea_reg ].b );
                        else
                        {
                            op_size = 0;
                            uint32_t address = effective_address();
                            setui8( address, bcd_sub( 0, effective_value8( address ) ) );
                        }
                    }
                    else if ( 0x2b == bits11_6 ) // tas
                    {
                        if ( 0 == ea_mode )
                        {
                            set_nzcv8( dregs[ ea_reg ].b );
                            dregs[ ea_reg ].b |= 0x80;
                        }
                        else
                        {
                            uint32_t address = effective_address();
                            uint8_t val = effective_value8( address );
                            set_nzcv8( val );
                            setui8( address, val & 0x80 );
                        }
                    }
                    else
                        unhandled();
                }
                break;
            }
            case 5: // addq / subq / Scc / DBcc
            {
                if ( 1 == ea_mode && 3 == op_size ) // DBcc
                {
                    pc += 2;
                    uint16_t condition = opbits( 8, 4 );
                    if ( !check_condition( condition ) )
                    {
                        dregs[ ea_reg ].w--;
                        if ( 0xffff != dregs[ ea_reg ].w )
                        {
                            int16_t displacement = (int16_t) getui16( pc );
                            pc += displacement;
                            continue;
                        }
                    }
                }
                else if ( 3 == op_size ) // Scc
                {
                    op_size = 0; // size is always byte
                    uint8_t value = check_condition( opbits( 8, 4 ) ) ? 0xff : 0;
                    if ( 0 == ea_mode )
                        dregs[ ea_reg ].b = value;
                    else
                        setui8( effective_address(), value );
                }
                else if ( opbit( 8 ) ) // subq
                {
                    uint32_t data = opbits( 9, 3 );
                    if ( 0 == data )
                        data = 8;

                    if ( 0 == ea_mode )
                    {
                        if ( 0 == op_size )
                            dregs[ ea_reg ].b = sub8( dregs[ ea_reg ].b, (uint8_t) data, true, true, false );
                        else if ( 1 == op_size )
                            dregs[ ea_reg ].w = sub16( dregs[ ea_reg ].w, (uint16_t) data, true, true, false );
                        else
                            dregs[ ea_reg ].l = sub32( dregs[ ea_reg ].l, data, true, true, false );
                    }
                    else if ( 1 == ea_mode )
                        aregs[ ea_reg ] -= data;
                    else
                    {
                        uint32_t address = effective_address();
                        if ( 0 == op_size )
                            setui8( address, sub8( getui8( address ), (uint8_t) data, true, true, false ) );
                        else if ( 1 == op_size )
                            setui16( address, sub16( getui16( address ), (uint16_t) data, true, true, false ) );
                        else
                            setui32( address, sub32( getui32( address ), data, true, true, false ) );
                    }
                }
                else // addq
                {
                    uint32_t data = opbits( 9, 3 );
                    if ( 0 == data )
                        data = 8;

                    if ( 0 == ea_mode )
                    {
                        if ( 0 == op_size )
                            dregs[ ea_reg ].b = add8( dregs[ ea_reg ].b, (uint8_t) data, true, true, false );
                        else if ( 1 == op_size )
                            dregs[ ea_reg ].w = add16( dregs[ ea_reg ].w, (uint16_t) data, true, true, false );
                        else
                            dregs[ ea_reg ].l = add32( dregs[ ea_reg ].l, data, true, true, false );
                    }
                    else if ( 1 == ea_mode )
                        aregs[ ea_reg ] += data;
                    else
                    {
                        uint32_t address = effective_address();
                        if ( 0 == op_size )
                            setui8( address, add8( getui8( address ), (uint8_t) data, true, true, false ) );
                        else if ( 1 == op_size )
                            setui16( address, add16( getui16( address ), (uint16_t) data, true, true, false ) );
                        else
                            setui32( address, add32( getui32( address ), data, true, true, false ) );
                    }
                }
                break;
            }
            case 6: // bra / bsr / bcc
            {
                uint16_t condition = opbits( 8, 4 );
                int16_t displacement = op & 0xff;
                bool two_byte_displacement = false;
                if ( 0 != displacement )
                    displacement = sign_extend16( displacement, 7 );
                else
                {
                    two_byte_displacement = true;
                    displacement = (int16_t) getui16( pc + 2 );
                }

                if ( 1 == condition ) // bsr
                {
                    pc += 2;
                    push( pc + ( two_byte_displacement ? 2 : 0 ) );
                    pc += displacement;
                    continue;
                }
                else if ( check_condition( condition ) ) // bcc including bra
                {
                    pc += ( 2 + displacement );
                    continue;
                }

                if ( two_byte_displacement )
                    pc += 2;
                break;
            }
            case 7: // moveq
            {
                if ( opbit( 8 ) )
                    unhandled();

                uint32_t data = sign_extend( opbits( 0, 8 ), 7 );
                dregs[ op_reg ].l = data;
                set_nzcv32( data );
                break;
            }
            case 8: // divu / divs / sbcd / or
            {
                uint16_t bit8 = opbit( 8 );
                uint16_t bits8_4 = opbits( 4, 5 );

                if ( 0x10 == bits8_4 ) // sbcd
                {
                    if ( opbit( 3 ) ) // address register predecrement mode
                    {
                        aregs[ ea_reg ]--;
                        aregs[ op_reg ]--;
                        setui8( aregs[ op_reg ], bcd_sub( getui8( aregs[ op_reg ] ), getui8( aregs[ ea_reg ] ) ) );
                    }
                    else // data register mode
                        dregs[ op_reg ].b = bcd_sub( dregs[ op_reg ].b, dregs[ ea_reg ].b );
                }
                else if ( !bit8 && 3 == op_size ) // divu
                {
                    op_size = 1; // word operation
                    uint32_t dividend = dregs[ op_reg ].l;
                    uint16_t divisor = effective_value16( effective_address() );
                    if ( 0 == divisor )
                    {
                        if ( handle_trap( 5, pc ) )
                            continue;

                        dregs[ op_reg ].l = 0; // just be nice
                    }
                    else
                    {
                        uint32_t quotient = dividend / (uint32_t) divisor;
                        if ( quotient > USHRT_MAX )
                            setflag_v( true );
                        else
                        {
                            setflag_v( false );
                            uint16_t remainder = (uint16_t) ( dividend % divisor );
                            dregs[ op_reg ].l = ( quotient & 0xffff ) | ( ( (uint32_t) remainder ) << 16 );
                            setflag_z( 0 == quotient );
                            setflag_n( sign16( (uint16_t) quotient ) );
                        }
                    }
                    setflag_c( false );
                }
                else if ( bit8 && 3 == op_size ) // divs
                {
                    op_size = 1; // word operation
                    int32_t dividend = (int32_t) dregs[ op_reg ].l;
                    int16_t divisor = (int16_t) effective_value16( effective_address() );
                    if ( 0 == divisor )
                    {
                        if ( handle_trap( 5, pc ) )
                            continue;

                        dregs[ op_reg ].l = 0;
                    }
                    else
                    {
                        int32_t quotient = dividend / (int32_t) divisor;
                        if ( quotient > SHRT_MAX || quotient < SHRT_MIN )
                            setflag_v( true );
                        else
                        {
                            setflag_v( false );
                            int16_t remainder = ( dividend % (int32_t) divisor );
                            dregs[ op_reg ].l = ( quotient & 0xffff ) | ( ( (uint32_t) remainder ) << 16 );
                            setflag_z( 0 == quotient );
                            setflag_n( sign16( (uint16_t) quotient ) );
                        }
                    }
                    setflag_c( false );
                }
                else // or
                {
                    uint32_t address = effective_address();
                    if ( bit8 ) // Dn | <ea> => <ea>
                    {
                        if ( 0 == op_size )
                        {
                            uint8_t val = effective_value8( address );
                            val |= dregs[ op_reg ].b;
                            set_nzcv8( val );
                            setui8( address, val );
                        }
                        else if ( 1 == op_size )
                        {
                            uint16_t val = effective_value16( address );
                            val |= dregs[ op_reg ].w;
                            set_nzcv16( val );
                            setui16( address, val );
                        }
                        else
                        {
                            uint32_t val = effective_value32( address );
                            val |= dregs[ op_reg ].l;
                            set_nzcv32( val );
                            setui32( address, val );
                        }
                    }
                    else // <ea> | Dn => Dn
                    {
                        if ( 0 == op_size )
                        {
                            dregs[ op_reg ].b |= effective_value8( address );
                            set_nzcv8( dregs[ op_reg ].b );
                        }
                        else if ( 1 == op_size )
                        {
                            dregs[ op_reg ].w |= effective_value16( address );
                            set_nzcv16( dregs[ op_reg ].w );
                        }
                        else
                        {
                            dregs[ op_reg ].l |= effective_value32( address );
                            set_nzcv32( dregs[ op_reg ].l );
                        }
                    }
                }
                break;
            }
            case 0x9: // sub / subx / suba
            {
                if ( 3 == opbits( 6, 2 ) ) // suba
                {
                    if ( opbit( 8 ) ) // long
                    {
                        op_size = 2;
                        aregs[ op_reg ] -= effective_value32( effective_address() );
                    }
                    else // word
                    {
                        op_size = 1;
                        aregs[ op_reg ] -= sign_extend( effective_value16( effective_address() ), 15 );
                    }
                }
                else if ( 0 == opbits( 4, 2 ) && opbit( 8 ) ) // subx
                {
                    if ( opbit( 3 ) ) // true if both operands are memory with a register predecrement mode, false if both are d registers
                    {
                        if ( 0 == op_size )
                        {
                            aregs[ op_reg ] -= 1;
                            aregs[ ea_reg ] -= 1;
                            setui8( aregs[ op_reg ], sub8( getui8( aregs[ op_reg ] ), getui8( aregs[ ea_reg ] ), true, true, true ) );
                        }
                        else if ( 1 == op_size )
                        {
                            aregs[ op_reg ] -= 2;
                            aregs[ ea_reg ] -= 2;
                            setui16( aregs[ op_reg ], sub16( getui16( aregs[ op_reg ] ), getui16( aregs[ ea_reg ] ), true, true, true ) );
                        }
                        else
                        {
                            aregs[ op_reg ] -= 4;
                            aregs[ ea_reg ] -= 4;
                            setui32( aregs[ op_reg ], sub32( getui32( aregs[ op_reg ] ), getui32( aregs[ ea_reg ] ), true, true, true ) );
                        }
                    }
                    else
                    {
                        if ( 0 == op_size )
                            dregs[ op_reg ].b = sub8( dregs[ op_reg ].b, dregs[ ea_reg ].b, true, true, true );
                        else if ( 1 == op_size )
                            dregs[ op_reg ].w = sub16( dregs[ op_reg ].w, dregs[ ea_reg ].w, true, true, true );
                        else
                            dregs[ op_reg ].l = sub32( dregs[ op_reg ].l, dregs[ ea_reg ].l, true, true, true );
                    }
                }
                else // sub
                {
                    uint32_t address = effective_address();
                    if ( 0 == ( op_mode & 4 ) ) // Dn - <ea> => Dn
                    {
                        if ( 0 == op_size ) // byte
                            dregs[ op_reg ].b = sub8( dregs[ op_reg ].b, effective_value8( address ), true, true, false );
                        else if ( 1 == op_size ) // word
                            dregs[ op_reg ].w = sub16( dregs[ op_reg ].w, effective_value16( address ), true, true, false );
                        else // long
                            dregs[ op_reg ].l = sub32( dregs[ op_reg ].l, effective_value32( address ), true, true, false );
                    }
                    else // <ea> - Dn => <ea>
                    {
                        if ( 0 == op_size ) // byte
                        {
                            uint8_t val = effective_value8( address );
                            uint8_t result = sub8( val, dregs[ op_reg ].b, true, true, false );

                            if ( 0 == ea_mode )
                                dregs[ ea_reg ].b = result;
                            else if ( 1 == ea_mode )
                                aregs[ ea_reg ] = result;
                            else
                                setui8( address, result );
                        }
                        else if ( 1 == op_size ) // word
                        {
                            uint16_t val = effective_value16( address );
                            uint16_t result = sub16( val, dregs[ op_reg ].w, true, true, false );

                            if ( 0 == ea_mode )
                                dregs[ ea_reg ].w = result;
                            else if ( 1 == ea_mode )
                                aregs[ ea_reg ] = result;
                            else
                                setui16( address, result );
                        }
                        else // long
                        {
                            uint32_t val = effective_value32( address );
                            uint32_t result = sub32( val, dregs[ op_reg ].l, true, true, false );

                            if ( 0 == ea_mode )
                                dregs[ ea_reg ].l = result;
                            else if ( 1 == ea_mode )
                                aregs[ ea_reg ] = result;
                            else
                                setui32( address, result );
                        }
                    }
                }
                break;
            }
            case 0xb: // eor, cmpm, cmp, cmpa
            {
                uint16_t bit8 = opbit( 8 );
                if ( 3 == op_mode || 7 == op_mode ) // cmpa
                {
                    uint32_t source;

                    if ( bit8 )
                    {
                        op_size = 2;
                        source = effective_value32( effective_address() );
                    }
                    else
                    {
                        op_size = 1;
                        source = sign_extend( effective_value16( effective_address() ), 15 );
                    }
                    sub32( aregs[ op_reg ], source, true, false, false );
                }
                else if ( op_mode <= 2 ) // cmp
                {
                    uint32_t address = effective_address();
                    if ( 0 == op_mode )
                        sub8( dregs[ op_reg ].b, effective_value8( address ), true, false, false );
                    else if ( 1 == op_mode )
                        sub16( dregs[ op_reg ].w, effective_value16( address ), true, false, false );
                    else
                        sub32( dregs[ op_reg ].l, effective_value32( address ), true, false, false );
                }
                else if ( bit8 && 1 == ea_mode ) // cmpm
                {
                    if ( 0 == op_size )
                    {
                        sub8( getui8( aregs[ op_reg ] ), getui8( aregs[ ea_reg ] ), true, false, false );
                        aregs[ op_reg ]++;
                        aregs[ ea_reg ]++;
                    }
                    else if ( 1 == op_size )
                    {
                        sub16( getui16( aregs[ op_reg ] ), getui16( aregs[ ea_reg ] ), true, false, false );
                        aregs[ op_reg ] += 2;
                        aregs[ ea_reg ] += 2;
                    }
                    else
                    {
                        sub32( getui32( aregs[ op_reg ] ), getui32( aregs[ ea_reg ] ), true, false, false );
                        aregs[ op_reg ] += 4;
                        aregs[ ea_reg ] += 4;
                    }
                }
                else if ( bit8 ) // eor
                {
                    if ( 0 == ea_mode )
                    {
                        if ( 0 == op_size )
                        {
                            dregs[ ea_reg ].b ^= dregs[ op_reg ].b;
                            set_nzcv8( dregs[ ea_reg ].b );
                        }
                        else if ( 1 == op_size )
                        {
                            dregs[ ea_reg ].w ^= dregs[ op_reg ].w;
                            set_nzcv16( dregs[ ea_reg ].w );
                        }
                        else
                        {
                            dregs[ ea_reg ].l ^= dregs[ op_reg ].l;
                            set_nzcv32( dregs[ ea_reg ].l );
                        }
                    }
                    else
                    {
                        uint32_t address = effective_address();
                        if ( 0 == op_size )
                        {
                            uint8_t val = dregs[ op_reg ].b ^ effective_value8( address );
                            set_nzcv8( val );
                            setui8( address, val );
                        }
                        else if ( 1 == op_size )
                        {
                            uint16_t val = dregs[ op_reg ].w ^ effective_value16( address );
                            set_nzcv16( val );
                            setui16( address, val );
                        }
                        else
                        {
                            uint32_t val = dregs[ op_reg ].l ^ effective_value32( address );
                            set_nzcv32( val );
                            setui32( address, val );
                        }
                    }
                }
                else
                    unhandled();
                break;
            }
            case 0xc: // mulu / muls / abcd / exg / and
            {
                uint16_t bit8 = opbit( 8 );
                uint16_t bits7_6 = opbits( 6, 2);
                uint16_t bits5_4 = opbits( 4, 2 );

                if ( bit8 && 0 == bits7_6 && 0 == bits5_4 ) // abcd
                {
                    if ( opbit( 3 ) ) // address register predecrement mode
                    {
                        aregs[ ea_reg ]--;
                        aregs[ op_reg ]--;
                        setui8( aregs[ op_reg ], bcd_add( getui8( aregs[ ea_reg ] ), getui8( aregs[ op_reg ] ) ) );
                    }
                    else // data register mode
                        dregs[ op_reg ].b = bcd_add( dregs[ op_reg ].b, dregs[ ea_reg ].b );
                }
                else if ( !bit8 && 3 == bits7_6 ) // mulu
                {
                    op_size = 1; // word operation
                    uint32_t left = effective_value16( effective_address() );
                    uint32_t right = dregs[ op_reg ].w;
                    uint32_t result = left * right;
                    set_nzcv32( result );
                    dregs[ op_reg ].l = result;
                }
                else if ( bit8 && 3 == bits7_6 ) // muls
                {
                    op_size = 1; // word operation
                    int32_t left = (int16_t) effective_value16( effective_address() );
                    int32_t right = (int16_t) dregs[ op_reg ].w;
                    int32_t result = left * right;
                    set_nzcv32( result );
                    dregs[ op_reg ].l = result;
                }
                else if ( bit8 && 0 == bits5_4 ) // exg
                {
                    uint16_t the_op_mode = opbits( 3, 5 );
                    if ( 8 == the_op_mode )
                        do_swap( dregs[ op_reg ].l, dregs[ ea_reg ].l );
                    else if ( 9 == the_op_mode )
                        do_swap( aregs[ op_reg ], aregs[ ea_reg ] );
                    else if ( 0x11 == the_op_mode )
                        do_swap( dregs[ op_reg ].l, aregs[ ea_reg ] );
                    else
                        unhandled();
                }
                else // and
                {
                    uint32_t address = effective_address();
                    if ( bit8 ) // Dn & <ea> => <ea>
                    {
                        if ( 0 == op_size )
                        {
                            uint8_t val = effective_value8( address );
                            val &= dregs[ op_reg ].b;
                            set_nzcv8( val );
                            setui8( address, val );
                        }
                        else if ( 1 == op_size )
                        {
                            uint16_t val = effective_value16( address );
                            val &= dregs[ op_reg ].w;
                            set_nzcv16( val );
                            setui16( address, val );
                        }
                        else
                        {
                            uint32_t val = effective_value32( address );
                            val &= dregs[ op_reg ].l;
                            set_nzcv32( val );
                            setui32( address, val );
                        }
                    }
                    else // <ea> & Dn => Dn
                    {
                        if ( 0 == op_size )
                        {
                            dregs[ op_reg ].b &= effective_value8( address );
                            set_nzcv8( dregs[ op_reg ].b );
                        }
                        else if ( 1 == op_size )
                        {
                            dregs[ op_reg ].w &= effective_value16( address );
                            set_nzcv16( dregs[ op_reg ].w );
                        }
                        else
                        {
                            dregs[ op_reg ].l &= effective_value32( address );
                            set_nzcv32( dregs[ op_reg ].l );
                        }
                    }
                }
                break;
            }
            case 0xd: // add / addx / adda
            {
                if ( 3 == opbits( 6, 2 ) ) // adda
                {
                    if ( opbit( 8 ) ) // long
                    {
                        op_size = 2;
                        aregs[ op_reg ] += effective_value32( effective_address() );
                    }
                    else // word
                    {
                        op_size = 1;
                        aregs[ op_reg ] += sign_extend( effective_value16( effective_address() ), 15 );
                    }
                }
                else if ( 0 == opbits( 4, 2 ) && opbit( 8 ) ) // addx
                {
                    if ( opbit( 3 ) ) // true if both operands are memory with a register predecrement mode, false if both are d registers
                    {
                        if ( 0 == op_size )
                        {
                            aregs[ op_reg ] -= 1;
                            aregs[ ea_reg ] -= 1;
                            setui8( aregs[ op_reg ], add8( getui8( aregs[ op_reg ] ), getui8( aregs[ ea_reg ] ), true, true, true ) );
                        }
                        else if ( 1 == op_size )
                        {
                            aregs[ op_reg ] -= 2;
                            aregs[ ea_reg ] -= 2;
                            setui16( aregs[ op_reg ], add16( getui16( aregs[ op_reg ] ), getui16( aregs[ ea_reg ] ), true, true, true ) );
                        }
                        else
                        {
                            aregs[ op_reg ] -= 4;
                            aregs[ ea_reg ] -= 4;
                            setui32( aregs[ op_reg ], add32( getui32( aregs[ op_reg ] ), getui32( aregs[ ea_reg ] ), true, true, true ) );
                        }
                    }
                    else // both are d registers
                    {
                        if ( 0 == op_size )
                            dregs[ op_reg ].b = add8( dregs[ op_reg ].b, dregs[ ea_reg ].b, true, true, true );
                        else if ( 1 == op_size )
                            dregs[ op_reg ].w = add16( dregs[ op_reg ].w, dregs[ ea_reg ].w, true, true, true );
                        else
                            dregs[ op_reg ].l = add32( dregs[ op_reg ].l, dregs[ ea_reg ].l, true, true, true );
                    }
                }
                else // add
                {
                    uint32_t address = effective_address();
                    if ( 0 != ( op_mode & 4 ) ) // Dn + <ea> => ea
                    {
                        if ( 0 == op_size ) // byte
                            setui8( address, add8( effective_value8( address ), dregs[ op_reg ].b, true, true, false ) );
                        else if ( 1 == op_size ) // word
                            setui16( address, add16( effective_value16( address ), dregs[ op_reg ].w, true, true, false ) );
                        else
                            setui32( address, add32( effective_value32( address ), dregs[ op_reg ].l, true, true, false ) );
                    }
                    else // <ea> + Dn => Dn
                    {
                        if ( 0 == op_size ) // byte
                            dregs[ op_reg ].b = add8( dregs[ op_reg ].b, effective_value8( address ), true, true, false );
                        else if ( 1 == op_size ) // word
                            dregs[ op_reg ].w = add16( dregs[ op_reg ].w, effective_value16( address ), true, true, false );
                        else
                            dregs[ op_reg ].l = add32( dregs[ op_reg ].l, effective_value32( address ), true, true, false );
                    }
                }
                break;
            }
            case 0xe: // ASd / LSd / ROXd / ROd
            {
                uint16_t bits4_3 = opbits( 3, 2 );
                if ( ( 0 == op_reg || 1 == op_reg ) && 3 == op_size ) // ASd / LSd memory
                {
                    bool is_asd = ( 0 == op_reg );
                    bool is_left = opbit( 8 );
                    uint32_t address = effective_address();
                    uint16_t value = effective_value16( address );
                    bool original_signed = sign16( value );

                    if ( is_left )
                    {
                        setflags_cx( original_signed );
                        value <<= 1;
                    }
                    else
                    {
                        setflags_cx( value & 1 );
                        value >>= 1;
                        if ( is_asd && original_signed )
                            value |= 0x8000;
                    }
                    bool result_signed = sign16( value );
                    if ( is_asd )
                        setflag_v( original_signed != result_signed );
                    else
                        setflag_v( false );
                    setflag_n( result_signed );
                    setflag_z( 0 == value );
                    setui16( address, value );
                }
                else if ( ( 3 == op_reg || 2 == op_reg ) && 3 == op_size ) // ROd / ROXd memory
                {
                    bool is_rox = ( 2 == op_reg );
                    bool is_left = opbit( 8 );
                    uint32_t address = effective_address();
                    uint16_t value = effective_value16( address );
                    bool original_x = flag_x();
                    bool original_signed = sign16( value );

                    if ( is_left )
                    {
                        if ( is_rox )
                            setflags_cx( sign16( value ) );
                        else
                            setflag_c( original_signed );

                        value <<= 1;

                        if ( ( is_rox && original_x ) || ( !is_rox && original_signed ) )
                            value |= 1;
                    }
                    else
                    {
                        setflag_c( value & 1 );
                        if ( is_rox )
                            setflag_x( value & 1 );

                        value >>= 1;

                        if ( ( is_rox && original_x ) || ( !is_rox && flag_c() ) )
                            value |= 0x8000;
                    }
                    setflag_v( false );
                    setflag_n( sign16( value ) );
                    setflag_z( 0 == value );
                    setui16( address, value );
                }
                else if ( 0 == bits4_3 || 1 == bits4_3 ) // ASd / LSd register
                {
                    bool is_arithmetic = ( 0 == bits4_3 );
                    bool is_left = opbit( 8 );
                    uint16_t shift = 0;
                    bool is_imm = !opbit( 5 );
                    if ( is_imm )
                    {
                        shift = op_reg;
                        if ( 0 == shift )
                            shift = 8;
                    }
                    else
                        shift = dregs[ op_reg ].l % 64;

                    bool sign_changed = false;

                    if ( is_left )
                    {
                        if ( 0 == op_size )
                        {
                            for ( uint16_t i = 0; i < shift; i++ )
                            {
                                bool start_sign = sign8( dregs[ ea_reg ].b );
                                setflags_cx( start_sign );
                                dregs[ ea_reg ].b <<= 1;
                                if ( start_sign != sign8( dregs[ ea_reg ].b ) )
                                    sign_changed = true;
                            }
                            setflag_n( sign8( dregs[ ea_reg ].b ) );
                            setflag_z( 0 == dregs[ ea_reg ].b );
                        }
                        else if ( 1 == op_size )
                        {
                            for ( uint16_t i = 0; i < shift; i++ )
                            {
                                bool start_sign = sign16( dregs[ ea_reg ].w );
                                setflags_cx( start_sign );
                                dregs[ ea_reg ].w <<= 1;
                                if ( start_sign != sign16( dregs[ ea_reg ].w ) )
                                    sign_changed = true;
                            }
                            setflag_n( sign16( dregs[ ea_reg ].w ) );
                            setflag_z( 0 == dregs[ ea_reg ].w );
                        }
                        else
                        {
                            for ( uint16_t i = 0; i < shift; i++ )
                            {
                                bool start_sign = sign32( dregs[ ea_reg ].l );
                                setflags_cx( start_sign );
                                dregs[ ea_reg ].l <<= 1;
                                if ( start_sign != sign32( dregs[ ea_reg ].l ) )
                                    sign_changed = true;
                            }
                            setflag_n( sign32( dregs[ ea_reg ].l ) );
                            setflag_z( 0 == dregs[ ea_reg ].l );
                        }
                    }
                    else
                    {
                        if ( 0 == op_size )
                        {
                            for ( uint16_t i = 0; i < shift; i++ )
                            {
                                bool start_sign = sign8( dregs[ ea_reg ].b );
                                setflags_cx( dregs[ ea_reg ].b & 1 );
                                dregs[ ea_reg ].b >>= 1;
                                if ( is_arithmetic && start_sign ) // ASR
                                    dregs[ ea_reg ].b |= 0x80;
                                if ( start_sign != sign8( dregs[ ea_reg ].b ) )
                                    sign_changed = true;
                            }
                            setflag_n( sign8( dregs[ ea_reg ].b ) );
                            setflag_z( 0 == dregs[ ea_reg ].b );
                        }
                        else if ( 1 == op_size )
                        {
                            for ( uint16_t i = 0; i < shift; i++ )
                            {
                                bool start_sign = sign16( dregs[ ea_reg ].w );
                                setflags_cx( dregs[ ea_reg ].w & 1 );
                                dregs[ ea_reg ].w >>= 1;
                                if ( is_arithmetic && start_sign ) // ASR
                                    dregs[ ea_reg ].w |= 0x8000;
                                if ( start_sign != sign16( dregs[ ea_reg ].w ) )
                                    sign_changed = true;
                            }
                            setflag_n( sign16( dregs[ ea_reg ].w ) );
                            setflag_z( 0 == dregs[ ea_reg ].w );
                        }
                        else
                        {
                            for ( uint16_t i = 0; i < shift; i++ )
                            {
                                bool start_sign = sign32( dregs[ ea_reg ].l );
                                setflags_cx( dregs[ ea_reg ].l & 1 );
                                dregs[ ea_reg ].l >>= 1;
                                if ( is_arithmetic && start_sign ) // ASR
                                    dregs[ ea_reg ].l |= 0x80000000;
                                if ( start_sign != sign32( dregs[ ea_reg ].l ) )
                                    sign_changed = true;
                            }
                            setflag_n( sign32( dregs[ ea_reg ].l ) );
                            setflag_z( 0 == dregs[ ea_reg ].l );
                        }
                    }
                    if ( is_arithmetic )
                        setflag_v( sign_changed );
                    else
                        setflag_v( false );
                    if ( 0 == shift )
                        setflag_c( false );
                }
                else if ( 2 == bits4_3 ) // ROXd register
                {
                    bool is_left = opbit( 8 );
                    uint16_t shift = 0;
                    bool is_imm = !opbit( 5 );
                    if ( is_imm )
                    {
                        shift = op_reg;
                        if ( 0 == shift )
                            shift = 8;
                    }
                    else
                        shift = dregs[ op_reg ].l % 64;

                   if ( is_left )
                   {
                       if ( 0 == op_size )
                       {
                           for ( uint16_t i = 0; i < shift; i++ )
                           {
                               uint8_t xset = flag_x();
                               setflags_cx( sign8( dregs[ ea_reg ].b ) );
                               dregs[ ea_reg ].b <<= 1;
                               dregs[ ea_reg ].b |= xset;
                           }
                           setflag_n( sign8( dregs[ ea_reg ].b ) );
                           setflag_z( 0 == dregs[ ea_reg ].b );
                       }
                       else if ( 1 == op_size )
                       {
                           for ( uint16_t i = 0; i < shift; i++ )
                           {
                               uint16_t xset = flag_x();
                               setflags_cx( sign16( dregs[ ea_reg ].w ) );
                               dregs[ ea_reg ].w <<= 1;
                               dregs[ ea_reg ].w |= xset;
                           }
                           setflag_n( sign16( dregs[ ea_reg ].w ) );
                           setflag_z( 0 == dregs[ ea_reg ].w );
                       }
                       else
                       {
                           for ( uint16_t i = 0; i < shift; i++ )
                           {
                               uint32_t xset = flag_x();
                               setflags_cx( sign32( dregs[ ea_reg ].l ) );
                               dregs[ ea_reg ].l <<= 1;
                               dregs[ ea_reg ].l |= xset;
                           }
                           setflag_n( sign32( dregs[ ea_reg ].l ) );
                           setflag_z( 0 == dregs[ ea_reg ].l );
                       }
                   }
                   else
                   {
                       if ( 0 == op_size )
                       {
                           for ( uint16_t i = 0; i < shift; i++ )
                           {
                               uint8_t xset = ( flag_x() << 7 );
                               setflags_cx( dregs[ ea_reg ].b & 1 );
                               dregs[ ea_reg ].b >>= 1;
                               dregs[ ea_reg ].b |= xset;
                           }
                           setflag_n( sign8( dregs[ ea_reg ].b ) );
                           setflag_z( 0 == dregs[ ea_reg ].b );
                       }
                       else if ( 1 == op_size )
                       {
                           for ( uint16_t i = 0; i < shift; i++ )
                           {
                               uint16_t xset = ( flag_x() << 15 );
                               setflags_cx( dregs[ ea_reg ].w & 1 );
                               dregs[ ea_reg ].w >>= 1;
                               dregs[ ea_reg ].w |= xset;
                           }
                           setflag_n( sign16( dregs[ ea_reg ].w ) );
                           setflag_z( 0 == dregs[ ea_reg ].w );
                       }
                       else
                       {
                           for ( uint16_t i = 0; i < shift; i++ )
                           {
                               uint32_t xset = ( flag_x() << 31 );
                               setflags_cx( dregs[ ea_reg ].l & 1 );
                               dregs[ ea_reg ].l >>= 1;
                               dregs[ ea_reg ].l |= xset;
                           }
                           setflag_n( sign32( dregs[ ea_reg ].l ) );
                           setflag_z( 0 == dregs[ ea_reg ].l );
                       }
                   }

                    if ( 0 == shift )
                        setflag_c( flag_x() );
                    setflag_v( false );
                }
                else if ( 3 == bits4_3 ) // ROd register
                {
                    bool is_left = opbit( 8 );
                    uint16_t shift = 0;
                    bool is_imm = !opbit( 5 );
                    if ( is_imm )
                    {
                        shift = op_reg;
                        if ( 0 == shift )
                            shift = 8;
                    }
                    else
                        shift = dregs[ op_reg ].l % 64;

                    if ( is_left )
                    {
                        if ( 0 == op_size )
                        {
                            for ( uint16_t i = 0; i < shift; i++ )
                            {
                                setflag_c( sign8( dregs[ ea_reg ].b ) );
                                dregs[ ea_reg ].b <<= 1;
                                dregs[ ea_reg ].b |= (uint8_t) flag_c();
                            }
                            setflag_n( sign8( dregs[ ea_reg ].b ) );
                            setflag_z( 0 == dregs[ ea_reg ].b );
                        }
                        else if ( 1 == op_size )
                        {
                            for ( uint16_t i = 0; i < shift; i++ )
                            {
                                setflag_c( sign16( dregs[ ea_reg ].w ) );
                                dregs[ ea_reg ].w <<= 1;
                                dregs[ ea_reg ].w |= (uint16_t) flag_c();
                            }
                            setflag_n( sign16( dregs[ ea_reg ].w ) );
                            setflag_z( 0 == dregs[ ea_reg ].w );
                        }
                        else
                        {
                            for ( uint16_t i = 0; i < shift; i++ )
                            {
                                setflag_c( sign32( dregs[ ea_reg ].l ) );
                                dregs[ ea_reg ].l <<= 1;
                                dregs[ ea_reg ].l |= (uint32_t) flag_c();
                            }
                            setflag_n( sign32( dregs[ ea_reg ].l ) );
                            setflag_z( 0 == dregs[ ea_reg ].l );
                        }
                    }
                    else
                    {
                        if ( 0 == op_size )
                        {
                            for ( uint16_t i = 0; i < shift; i++ )
                            {
                                setflag_c( dregs[ ea_reg ].b & 1 );
                                dregs[ ea_reg ].b >>= 1;
                                dregs[ ea_reg ].b |= ( (uint8_t) flag_c() ) << 7;
                            }
                            setflag_n( sign8( dregs[ ea_reg ].b ) );
                            setflag_z( 0 == dregs[ ea_reg ].b );
                        }
                        else if ( 1 == op_size )
                        {
                            for ( uint16_t i = 0; i < shift; i++ )
                            {
                                setflag_c( dregs[ ea_reg ].w & 1 );
                                dregs[ ea_reg ].w >>= 1;
                                dregs[ ea_reg ].w |= ( (uint16_t) flag_c() ) << 15;
                            }
                            setflag_n( sign16( dregs[ ea_reg ].w ) );
                            setflag_z( 0 == dregs[ ea_reg ].w );
                        }
                        else
                        {
                            for ( uint16_t i = 0; i < shift; i++ )
                            {
                                setflag_c( dregs[ ea_reg ].l & 1 );
                                dregs[ ea_reg ].l >>= 1;
                                dregs[ ea_reg ].l |= ( (uint32_t) flag_c() ) << 31;
                            }
                            setflag_n( sign32( dregs[ ea_reg ].l ) );
                            setflag_z( 0 == dregs[ ea_reg ].l );
                        }
                    }
                    if ( 0 == shift )
                        setflag_c( false );
                    setflag_v( false );
                }
                break;
            }
            default:
                unhandled();
        }

        pc += 2;       // 7.8% of runtime
        cycles++;      // 2.6% of runtime
    } // for

    return cycles; // for now, instructions not cycles
} //run
