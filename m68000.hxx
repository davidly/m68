#include <djl_os.hxx>

struct m68000;

// callbacks when instructions are executed

extern void emulator_invoke_svc( m68000 & cpu );                                                // called when the linux-style syscall instruction is executed
extern void emulator_invoke_68k_trap15( m68000 & cpu );                                         // called when trap #15 is invoked for an IDE68K emulator system call
extern void emulator_invoke_68k_trap2( m68000 & cpu );                                          // called when trap #2 for digital research cp/m 68k bdos calls
extern void emulator_invoke_68k_trap3( m68000 & cpu );                                          // called when trap #3 for digital research cp/m 68k bios calls
extern const char * emulator_symbol_lookup( uint32_t address, uint32_t & offset );              // returns the best guess for a symbol name and offset for the address
extern void emulator_hard_termination( m68000 & cpu, const char *pcerr, uint64_t error_value ); // show an error and exit

typedef struct datareg_t
{
    union
    {
        uint32_t l;
        #ifdef TARGET_BIG_ENDIAN
            struct { uint16_t filler_w; uint16_t w; };
            struct { uint8_t filler_b1; uint8_t filler_b2; uint8_t filler_b3; uint8_t b; };
        #else
            uint16_t w;
            uint8_t b;
        #endif
    };
} datareg_t;

struct m68000
{
    bool trace_instructions( bool trace );         // enable/disable tracing each instruction
    void end_emulation( void );                    // make the emulator return at the start of the next instruction
    uint64_t run( void );                          // execute the instruction stream

    m68000( vector<uint8_t> & memory, uint32_t base_address, uint32_t start, uint32_t stack_commit, uint32_t top_of_stack )
    {
        reset( memory, base_address, start, stack_commit, top_of_stack );
    } //m68000

    void reset( vector<uint8_t> & memory, uint32_t base_address, uint32_t start, uint32_t stack_commit, uint32_t top_of_stack )
    {
        memset( this, 0, sizeof( *this ) );
        stack_size = stack_commit;                 // remember how much of the top of RAM is allocated to the stack
        stack_top = top_of_stack;                  // where the stack started
        base = base_address;                       // lowest valid address in the app's address space, maps to offset 0 in mem. If not 0, can't update trap vectors.
        mem = memory.data();                       // save the pointer, but don't take ownership
        mem_size = (uint32_t) memory.size();       // how much RAM is allocated ror the 68000
        beyond_mem = mem + memory.size();          // addresses beyond and later are illegal
        membase = mem - base;                      // real pointer to the start of the app's memory (prior to offset)
        sr = 0x2300;                               // set supervisor mode and irq level of 3. all other flags are off
        enforce_pc_sp_constraints = true;          // debug only asserts on values of pc and sp. meaningless after bdos 57

        if ( 0 == base )                           // if not 0, there's no way to address the vector table
        {
            #ifdef TARGET_BIG_ENDIAN
                isp = * (uint32_t *) mem;          // the first 4 bytes of RAM hold the supervisor stack pointer
                pc = * (uint32_t *) ( mem + 4 );   // the next 4 byteshave the boot address
            #else
                isp = flip_endian32( * (uint32_t *) mem );        // the first 4 bytes of RAM hold the supervisor stack pointer
                pc = flip_endian32( * (uint32_t *) ( mem + 4 ) ); // the next 4 byteshave the boot address
            #endif

            aregs[ 7 ] = isp;                      // start out with the supervisor stack
        }

        // after this point, if start is specified it's not a standard 68000 boot sequence. flip into user mode for the app to run.
        // for a standard 68000 boot sequence, pass 0 for start and let the function at vector[1] initialize things.

        if ( 0 != start )
        {
            usp = top_of_stack;                        // points at either the linux startup data or the cp/m 64k return address and base page
            pc = start;                                // app start address
            setflag_s( false );                        // the 68000 boots in supervisor mode, but we're just running an app here not booting an OS
            perhaps_restore_usermode_state();
        }

        tracer.Trace( "pc %x, sr %x, usp %x, isp %x, stack_size %x, base %x, mem_size %x\n", pc, sr, usp, isp, stack_size, base, mem_size );
    } //m68000

    void relax_pc_sp_constraints() { enforce_pc_sp_constraints = false; }

    // addresses to getX/setX should all be word (2-byte) aligned, but the m68k compiler assumes 1-byte alignment works

    inline uint32_t getui32( uint32_t o )
    {
        #ifdef TARGET_BIG_ENDIAN
            return * (uint32_t *) getmem( o );
        #else
            return flip_endian32( * (uint32_t *) getmem( o ) );
        #endif
    } //getui32

    inline uint16_t getui16( uint32_t o )
    {
        #ifdef TARGET_BIG_ENDIAN
            return * (uint16_t *) getmem( o );
        #else
            return flip_endian16( * (uint16_t *) getmem( o ) );
        #endif
    } //getui16

    inline uint8_t getui8( uint32_t o ) { return * (uint8_t *) getmem( o ); }

    inline void setui32( uint32_t o, uint32_t val )
    {
        #ifdef TARGET_BIG_ENDIAN
            * (uint32_t *) getmem( o ) = val;
        #else
            * (uint32_t *) getmem( o ) = flip_endian32( val );
        #endif
    } //setui32

    inline void setui16( uint32_t o, uint16_t val )
    {
        #ifdef TARGET_BIG_ENDIAN
            * (uint16_t *) getmem( o ) = val;
        #else
            * (uint16_t *) getmem( o ) = flip_endian16( val );
        #endif
    } //setui16

    inline void setui8( uint32_t o, uint8_t val ) { * (uint8_t *) getmem( o ) = val; }

    inline uint32_t get_vm_address( uint32_t offset )
    {
        return base + offset;
    } //get_vm_address

    inline uint8_t * getmem( uint32_t offset )
    {
        #ifdef NDEBUG
            return membase + offset;
        #else
            uint8_t * r = membase + offset;

            if ( r >= beyond_mem )
                emulator_hard_termination( *this, "memory reference beyond address space:", offset );

            if ( r < mem )
                emulator_hard_termination( *this, "memory reference prior to address space:", offset );

            return r;
        #endif
    } //getmem

    uint32_t host_to_vm_address( void * p )
    {
        return (uint32_t) ( (uint8_t *) p - mem + base );
    } //host_to_vm_address

    bool is_address_valid( uint32_t offset )
    {
        uint8_t * r = membase + offset;
        return ( ( r < beyond_mem ) && ( r >= mem ) );
    } //is_address_valid

    inline uint32_t getoffset( uint32_t address )
    {
        return address - base;
    } //getoffset

    inline void set_supervisor_state()
    {
        if ( !flag_s() ) // if already in supervisor mode then ignore
        {
            usp = aregs[ 7 ];
            aregs[ 7 ] = isp;
            tracer.Trace( "now in supervisor mode, isp %#x, usp %#x\n", isp, usp );
            setflag_s( true );
        }
    } //set_supervisor_state

    datareg_t dregs[ 8 ];
    uint32_t aregs[ 8 ];
    uint32_t pc;

private:
    uint16_t sr;                                   // lower 8 bits is ccr (user accessible). upper 8 bits is supervisor only
    uint32_t isp;                                  // interrupt/supervisor stack pointer (ssp == isp on 68000) when executing handlers
    uint32_t usp;                                  // usermode stack pointer

    uint16_t op;                                   // the currently executing opcode found at pc
    uint16_t hi4;                                  // bits 15..12
    uint16_t op_reg;                               // bits 11..9
    uint16_t op_mode;                              // bits 8..6
    uint16_t op_size;                              // bits 7..6
    uint16_t ea_mode;                              // bits 5..3
    uint16_t ea_reg;                               // bits 2..0

    uint8_t * mem;                                 // RAM for the 68000 is here
    uint8_t * beyond_mem;                          // first byte beyond the RAM
    uint32_t base;                                 // base address of the process per the elf file
    uint8_t * membase;                             // host pointer to base of vm's memory (mem - base) to make getmem() faster
    uint32_t stack_size;
    uint32_t stack_top;
    uint32_t mem_size;
    uint64_t cycles_so_far;                        // just an instruction count for now
    bool enforce_pc_sp_constraints;                // after bdos 57 all bets are off for what's legal for pc and sp

    inline void perhaps_restore_usermode_state()
    {
        if ( !flag_s() ) // only call this function when SR may have changed out of supervisor mode
        {
            isp = aregs[ 7 ];
            aregs[ 7 ] = usp;
            tracer.Trace( "now in user mode, isp %#x, usp %#x\n", isp, usp );
        }
    } //perhaps_restore_usermode_state

    inline uint16_t opbits( uint32_t lowbit, uint32_t len ) const
    {
        uint32_t val = ( op >> lowbit );
        assert( 16 != len ); // the next line of code wouldn't work but there are no callers that do this
        return ( val & ( ( 1 << len ) - 1 ) );
    } //opbits

    inline uint16_t opbit( uint32_t bit ) const { return ( 1 & ( op >> bit ) ); }

    // it seems slower to not use an "if ( f )", but multiple assignment with shift is faster on both x64 and 68000

    inline void setflag_c( bool f ) { sr &= 0xfffe; sr |= ( 0 != f ); }
    inline void setflag_v( bool f ) { sr &= 0xfffd; sr |= ( ( 0 != f ) << 1 ); }
    inline void setflag_z( bool f ) { sr &= 0xfffb; sr |= ( ( 0 != f ) << 2 ); }
    inline void setflag_n( bool f ) { sr &= 0xfff7; sr |= ( ( 0 != f ) << 3 ); }
    inline void setflag_x( bool f ) { sr &= 0xffef; sr |= ( ( 0 != f ) << 4 ); }
    inline void setflag_s( bool f ) { sr &= 0xdfff; sr |= ( ( 0 != f ) << 13 ); }
    inline void setflag_t( bool f ) { sr &= 0x7fff; sr |= ( ( 0 != f ) << 15 ); }

    inline void setflags_cx( bool f ) { setflag_c( f ); setflag_x( f ); }
    inline void clearflags_cv() { sr &= 0xfffc; }

    inline bool flag_c() { return ( 0 != ( sr &      1 ) ); }
    inline bool flag_v() { return ( 0 != ( sr &      2 ) ); }
    inline bool flag_z() { return ( 0 != ( sr &      4 ) ); }
    inline bool flag_n() { return ( 0 != ( sr &      8 ) ); }
    inline bool flag_x() { return ( 0 != ( sr &   0x10 ) ); }
    inline bool flag_s() { return ( 0 != ( sr & 0x2000 ) ); }
    inline bool flag_t() { return ( 0 != ( sr & 0x8000 ) ); } // t1. t0 is always 0 on a 68000

    inline static int32_t sign_extend( uint32_t x, uint32_t high_bit )
    {
        assert( high_bit < 31 );
        x &= ( 1 << ( high_bit + 1 ) ) - 1; // clear bits above the high bit since they may contain noise
        const int32_t m = ( (uint32_t) 1 ) << high_bit;
        return ( x ^ m ) - m;
    } //sign_extend

    inline static int16_t sign_extend16( uint16_t x, uint16_t high_bit )
    {
        assert( high_bit < 15 );
        x &= ( 1 << ( high_bit + 1 ) ) - 1; // clear bits above the high bit since they may contain noise
        const int16_t m = ( (uint16_t) 1 ) << high_bit;
        return ( x ^ m ) - m;
    } //sign_extend16

    void trace_state( void );                  // trace the machine current status
    void unhandled( void );
    const char * render_flags( void );
    uint8_t effective_value8( uint32_t x );
    uint16_t effective_value16( uint32_t x );
    uint32_t effective_value32( uint32_t x );
    uint32_t effective_address( void );
    uint32_t effective_address2( uint16_t m, uint16_t reg );
    const char * effective_string( void );
    const char * effective_string2( uint16_t m, uint16_t reg );
    int32_t get_ea_displacement();
    uint32_t sub32( uint32_t a, uint32_t b, bool setflags, bool setx, bool subx );
    uint16_t sub16( uint16_t a, uint16_t b, bool setflags, bool setx, bool subx );
    uint8_t sub8( uint8_t a, uint8_t b, bool setflags, bool setx, bool subx );
    uint32_t add32( uint32_t a, uint32_t b, bool setflags, bool setx, bool addx );
    uint16_t add16( uint16_t a, uint16_t b, bool setflags, bool setx, bool addx );
    uint8_t add8( uint8_t a, uint8_t b, bool setflags, bool setx, bool addx );
    bool check_condition( uint16_t c );
    template < typename T, typename W > void set_flags( T a, T b, T result, W result_wide, bool setx, bool xbehavior, bool addition );
    uint8_t bcd_add( uint8_t a, uint8_t b );
    uint8_t bcd_sub( uint8_t a, uint8_t b );
    bool handle_trap( uint16_t vector, uint32_t pc_return );

    inline char get_size() const { return ( 0 == op_size ) ? 'b' : ( 1 == op_size ) ? 'w' : ( 2 == op_size ) ? 'l' : '?'; }

    inline void push( uint32_t x )
    {
        aregs[ 7 ] -= 4;
        setui32( aregs[ 7 ], x );
    } //push

    inline uint32_t pop()
    {
        uint32_t result = getui32( aregs[ 7 ] );
        aregs[ 7 ] += 4;
        return result;
    } //pop

    inline void push16( uint16_t x )
    {
        aregs[ 7 ] -= 2;
        setui16( aregs[ 7 ], x );
    } //push16

    inline uint16_t pop16()
    {
        uint16_t result = getui16( aregs[ 7 ] );
        aregs[ 7 ] += 2;
        return result;
    } //pop16

    inline void set_nzcv8( uint8_t val )
    {
        setflag_n( 0 != ( 0x80 & val ) );
        setflag_z( 0 == val );
        clearflags_cv();
    } //set_nzcv8

    inline void set_nzcv16( uint16_t val )
    {
        setflag_n( 0 != ( 0x8000 & val ) );
        setflag_z( 0 == val );
        clearflags_cv();
    } //set_nzcv16

    inline void set_nzcv32( uint32_t val )
    {
        setflag_n( 0 != ( 0x80000000 & val ) );
        setflag_z( 0 == val );
        clearflags_cv();
    } //set_nzcv32
};

