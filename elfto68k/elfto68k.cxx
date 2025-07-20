#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <vector>

#include <djl_os.hxx>

#ifndef _WIN32
    #include <unistd.h>
    #include <stdlib.h>
#endif

using namespace std;

bool trace_status = false;

// 68000
#define ELF_MACHINE_ISA 0x04

bool g_hostIsLittleEndian = true;              // is the host little endian?

uint64_t swap_endian64( uint64_t x )
{
    if ( g_hostIsLittleEndian )
        return flip_endian64( x );
    return x;
} //swap_endian64

uint32_t swap_endian32( uint32_t x )
{
    if ( g_hostIsLittleEndian )
        return flip_endian32( x );
    return x;
} //swap_endian32

uint16_t swap_endian16( uint16_t x )
{
    if ( g_hostIsLittleEndian )
        return flip_endian16( x );
    return x;
} //swap_endian16

#pragma pack( push, 1 )

struct HeaderCPM68K    // .68k executable files for cp/m 68k v1.3
{
    uint16_t signature;
    uint32_t cb_text;
    uint32_t cb_data;
    uint32_t cb_bss;
    uint32_t cb_symbols;
    uint32_t reserved;
    uint32_t text_start;
    uint16_t relocation_flag; // two bytes, but only the low byte has the flag

    void swap_endianness()
    {
        signature = swap_endian16( signature );
        cb_text = swap_endian32( cb_text );
        cb_data = swap_endian32( cb_data );
        cb_bss = swap_endian32( cb_bss );
        cb_symbols = swap_endian32( cb_symbols );
        reserved = swap_endian32( reserved );
        text_start = swap_endian32( text_start );
        relocation_flag = swap_endian16( relocation_flag );
    }

    void trace()
    {
        printf( "cpm68k executable header:\n" );
        printf( "  signature: %#x\n", signature );
        printf( "  cb_text: %#x\n", cb_text );
        printf( "  cb_data: %#x\n", cb_data );
        printf( "  cb_bss: %#x\n", cb_bss );
        printf( "  cb_symbols: %#x\n", cb_symbols );
        printf( "  reserved: %#x\n", reserved );
        printf( "  text_start: %#x\n", text_start );
        printf( "  relocation_flag: %#x\n", relocation_flag );
    }
};

struct ElfHeader32
{
    uint32_t magic;
    uint8_t bit_width;
    uint8_t endianness;
    uint8_t elf_version;
    uint8_t os_abi;
    uint8_t os_avi_version;
    uint8_t padding[ 7 ];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint32_t entry_point;
    uint32_t program_header_table;
    uint32_t section_header_table;
    uint32_t flags;
    uint16_t header_size;
    uint16_t program_header_table_size;
    uint16_t program_header_table_entries;
    uint16_t section_header_table_size;
    uint16_t section_header_table_entries;
    uint16_t section_with_section_names;

    void swap_endianness()
    {
        type = swap_endian16( type );
        machine = swap_endian16( machine );
        version = swap_endian32( version );
        entry_point = swap_endian32( entry_point );
        program_header_table = swap_endian32( program_header_table );
        section_header_table = swap_endian32( section_header_table );
        flags = swap_endian32( flags );
        header_size = swap_endian16( header_size );
        program_header_table_size = swap_endian16( program_header_table_size );
        program_header_table_entries = swap_endian16( program_header_table_entries );
        section_header_table_size = swap_endian16( section_header_table_size );
        section_header_table_entries = swap_endian16( section_header_table_entries );
        section_with_section_names = swap_endian16( section_with_section_names );
    }

    void trace()
    {
        printf( "bit width %d\n", bit_width );
        printf( "type %d\n", type );
        printf( "machine %d\n", machine );
    }
};

struct ElfProgramHeader32
{
    uint32_t type;
    uint32_t offset_in_image;
    uint32_t virtual_address;
    uint32_t physical_address;
    uint32_t file_size;
    uint32_t memory_size;
    uint32_t flags;
    uint32_t alignment;

    void swap_endianness()
    {
        type = swap_endian32( type );
        offset_in_image = swap_endian32( offset_in_image );
        virtual_address = swap_endian32( virtual_address );
        physical_address = swap_endian32( physical_address );
        file_size = swap_endian32( file_size );
        flags = swap_endian32( flags );
        memory_size = swap_endian32( memory_size );
        alignment = swap_endian32( alignment );
    } //swap_endianness

    const char * show_type() const
    {
        uint32_t basetype = ( type & 0xf );

        if ( 0 == basetype )
            return "unused";
        if ( 1 == basetype )
            return "load";
        if ( 2 == basetype )
            return "dynamic";
        if ( 3 == basetype )
            return "interp";
        if ( 4 == basetype )
            return "note";
        if ( 5 == basetype )
            return "shlib";
        if ( 6 == basetype )
            return "phdr";
        if ( 7 == basetype )
            return "tls";
        if ( 8 == basetype )
            return "num";
        return "unknown";
    }

    const char * show_flags()
    {
        if ( 7 == flags )
            return "rwe";
        if ( 6 == flags )
            return "rw";
        if ( 5 == flags )
            return "rx";
        if ( 4 == flags )
            return "r";
        if ( 3 == flags )
            return "wx";
        if ( 2 == flags )
            return "w";
        if ( 1 == flags )
            return "x";

        return "";
    }
};

struct ElfSectionHeader32
{
    uint32_t name_offset;
    uint32_t type;
    uint32_t flags;
    uint32_t address;
    uint32_t offset;
    uint32_t size;
    uint32_t link;
    uint32_t info;
    uint32_t address_alignment;
    uint32_t entry_size;

    void swap_endianness()
    {
        name_offset = swap_endian32( name_offset );
        type = swap_endian32( type );
        flags = swap_endian32( flags );
        address = swap_endian32( address );
        offset = swap_endian32( offset );
        size = swap_endian32( size );
        link = swap_endian32( link );
        info = swap_endian32( info );
        address_alignment = swap_endian32( address_alignment );
        entry_size = swap_endian32( entry_size );
    } //swap_endianness

    const char * show_type() const
    {
        uint32_t basetype = ( type & 0xf );

        if ( 0 == basetype )
            return "unused";
        if ( 1 == basetype )
            return "program data";
        if ( 2 == basetype )
            return "symbol table";
        if ( 3 == basetype )
            return "string table";
        if ( 4 == basetype )
            return "relocation entries with addends";
        if ( 5 == basetype )
            return "symbol hash table";
        if ( 6 == basetype )
            return "dynamic";
        if ( 7 == basetype )
            return "note";
        if ( 8 == basetype )
            return "nobits";
        if ( 9 == basetype )
            return "relocation entries without addends";
        if ( 10 == basetype )
            return "shlib";
        if ( 11 == basetype )
            return "dynsym";
        if ( 12 == basetype )
            return "num";
        if ( 14 == basetype )
            return "initialization functions";
        if ( 15 == basetype )
            return "termination functions";
        return "unknown";
    }

    const char * show_flags() const
    {
        static char ac[ 80 ];
        ac[0] = 0;

        if ( flags & 0x1 )
            strcat( ac, "write, " );
        if ( flags & 0x2 )
            strcat( ac, "alloc, " );
        if ( flags & 0x4 )
            strcat( ac, "executable, " );
        if ( flags & 0x10 )
            strcat( ac, "merge, " );
        if ( flags & 0x20 )
            strcat( ac, "asciz strings, " );
        return ac;
    }
};

#pragma pack(pop)

static const char * image_type( uint16_t e_type )
{
    if ( 0 == e_type )
        return "et none";
    if ( 1 == e_type )
        return "et relocatable file";
    if ( 2 == e_type )
        return "et executable";
    if ( 3 == e_type )
        return "et dynamic linked shared object";
    if ( 4 == e_type )
        return "et core file";
    return "et unknown";
} //image_type

void usage( const char * err = 0 )
{
    if ( err )
        printf( "error: %s\n", err );
    printf( "usage: elfto68k [-v] <file>.elf\n" );
    printf( "       creates <file>.68K\n" );
    printf( "       -v      verbose information\n" );
    exit( 1 );
} //usage

int main( int argc, char * argv[] )
{
    uint16_t tst = 1;
    g_hostIsLittleEndian = ( 1 & ( * (uint8_t *) &tst ) );

    char * pinputfile = 0;

    for ( int arg = 1; arg < argc; arg++ )
    {
        if ( !strcmp( argv[ arg ], "-v" ) )
            trace_status = true;
        else if ( !pinputfile )
            pinputfile = argv[ arg ];
        else
            usage( "invalid argument" );
    }

    FILE * fp = fopen( pinputfile, "rb" );
    if ( !fp )
        usage( "can't open input file" );

    CFile file( fp );

    ElfHeader32 ehead = {0};
    fseek( fp, (long) 0, SEEK_SET );
    size_t read = fread( &ehead, 1, sizeof ehead, fp );

    if ( 0x464c457f != ehead.magic && 0x7f454c46 != ehead.magic )
        usage( "elf image file's magic header is invalid" );

    bool big_endian = ( 2 == ehead.endianness );

    if ( trace_status )
        printf( "image is %s endian\n", big_endian ? "big" : "little" );

    if ( !big_endian )
        usage( "elf image endianness isn't consistent with tool expectations for 68000 binaries" );

    ehead.swap_endianness();

    if ( 2 != ehead.type )
    {
        printf( "e_type is %d == %s\n", ehead.type, image_type( ehead.type ) );
        usage( "elf image type isn't an executable file (2 expected)" );
    }

    if ( ELF_MACHINE_ISA != ehead.machine )
        usage( "elf image machine ISA doesn't match the 68000" );

    if ( 0 == ehead.entry_point )
        usage( "elf entry point is 0, which is invalid" );

    if ( trace_status )
    {
        printf( "header fields:\n" );
        printf( "  entry address: %x\n", ehead.entry_point );
        printf( "  program entries: %u\n", ehead.program_header_table_entries );
        printf( "  program header entry size: %u\n", ehead.program_header_table_size );
        printf( "  program offset: %u == %#x\n", ehead.program_header_table, ehead.program_header_table );
        printf( "  section entries: %u\n", ehead.section_header_table_entries );
        printf( "  section header entry size: %u\n", ehead.section_header_table_size );
        printf( "  section offset: %u == %#x\n", ehead.section_header_table, ehead.section_header_table );
        printf( "  section with section names: %u == %#x\n", ehead.section_with_section_names, ehead.section_with_section_names );
        printf( "  flags: %#x\n", ehead.flags );
    }

    // determine how much RAM to allocate

    uint32_t memory_size = 0;
    uint32_t g_base_address = 0;

    for ( uint16_t ph = 0; ph < ehead.program_header_table_entries; ph++ )
    {
        size_t o = ehead.program_header_table + ( ph * ehead.program_header_table_size );
        if ( trace_status )
            printf( "program header %u at offset %u\n", ph, (unsigned int) o );

        ElfProgramHeader32 head = {0};
        fseek( fp, (long) o, SEEK_SET );
        read = fread( &head, get_min( sizeof( head ), (size_t) ehead.program_header_table_size ), 1, fp );
        if ( 1 != read )
            usage( "can't read program header" );

        head.swap_endianness();

        if ( trace_status )
        {
            printf( "  type: %x / %s\n", head.type, head.show_type() );
            printf( "  offset in image: %#lx\n", head.offset_in_image );
            printf( "  virtual address: %#lx\n", head.virtual_address );
            printf( "  physical address: %#lx\n", head.physical_address );
            printf( "  file size: %#lx\n", head.file_size );
            printf( "  memory size: %#lx\n", head.memory_size );
            printf( "  flags: %#lx / %s\n", head.flags, head.show_flags() );
            printf( "  alignment: %#lx\n", head.alignment );
        }

        if ( 2 == head.type )
            usage( "dynamic linking is not supported by this tool. link your app with -static" );

        uint32_t just_past = head.physical_address + head.memory_size;
        if ( just_past > memory_size )
            memory_size = just_past;

        if ( ( 0 != head.physical_address ) && ( ( 0 == g_base_address ) || g_base_address > head.physical_address ) )
            g_base_address = head.physical_address;
    }

    if ( g_base_address > ( 1024 * 1024 ) )
        usage( "elf physical address is too high" );

    // if it won't waste much RAM, start the address space at 0 so low addresses can be used for things like trap vectors

    uint32_t elf_base_address = g_base_address;
    if ( trace_status )
        printf( "elf base address: %#x\n", elf_base_address );

    if ( g_base_address < 0x10000 )
        g_base_address = 0;

    memory_size -= g_base_address;

    if ( trace_status )
        printf( "memory_size: %x\n", memory_size );

    // first load the section header string table (so we can find the eh frame begin

    vector<char> section_names_string_table;

    for ( uint16_t sh = 0; sh < ehead.section_header_table_entries; sh++ )
    {
        size_t o = ehead.section_header_table + ( sh * ehead.section_header_table_size );
        ElfSectionHeader32 head = {0};

        fseek( fp, (long) o, SEEK_SET );
        read = fread( &head, 1, get_min( sizeof( head ), (size_t) ehead.section_header_table_size ), fp );
        if ( 0 == read )
            usage( "can't read section header" );

        head.swap_endianness();
        if ( 3 == head.type )
        {
            if ( sh == ehead.section_with_section_names )
            {
                section_names_string_table.resize( head.size );
                fseek( fp, (long) head.offset, SEEK_SET );
                read = fread( section_names_string_table.data(), head.size, 1, fp );
                if ( 1 != read )
                    usage( "can't read string table\n" );
                break;
            }
        }
    }

    // look for .eh_frame

    uint32_t the_EH_FRAME_BEGIN = 0; // for C++ programs, the pointer to the argument for __register_frame() at app startup

    for ( uint16_t sh = 0; sh < ehead.section_header_table_entries; sh++ )
    {
        size_t o = ehead.section_header_table + ( sh * ehead.section_header_table_size );
#if 0
        printf( "section header %u at offset %zu == %zx\n", sh, o, o );
#endif

        ElfSectionHeader32 head = {0};

        fseek( fp, (long) o, SEEK_SET );
        read = fread( &head, 1, get_min( sizeof( head ), (size_t) ehead.section_header_table_size ), fp );
        if ( 0 == read )
            usage( "can't read section header" );

        head.swap_endianness();

#if 0
        printf( "  type: %#x / %s\n", head.type, head.show_type() );
        printf( "  name %s, offset: %x\n", & section_names_string_table[ head.name_offset ], head.name_offset );
        printf( "  flags: %#x / %s\n", head.flags, head.show_flags() );
        printf( "  address: %x\n", head.address );
        printf( "  offset: %x\n", head.offset );
        printf( "  size: %x\n", head.size );
        printf( "  link: %x\n", head.link );
        printf( "  info: %x\n", head.info );
        printf( "  address_alignment: %x\n", head.address_alignment );
        printf( "  entry_size: %x\n", head.entry_size );
#endif

        if ( !strcmp( ".eh_frame", & section_names_string_table[ head.name_offset ] ) )
        {
            the_EH_FRAME_BEGIN = head.address;
            break;
        }
    }

    if ( trace_status )
        printf( "the_EH_FRAME_BEGIN: %lx\n", the_EH_FRAME_BEGIN );

    // align to 2 bytes

    if ( memory_size & 1 )
        memory_size++;

    uint32_t past_bss = 0;
    uint32_t elf_start_text = 0;
    uint32_t elf_cb_text = 0;
    uint32_t elf_start_data = 0;
    uint32_t elf_cb_data = 0;

    // read the text and data into memory so it can then be written to the .68K file

    vector<uint8_t> memory;
    memory.resize( memory_size );
    memset( memory.data(), 0, memory_size );
    uint32_t start_bss = 0;

    for ( uint16_t ph = 0; ph < ehead.program_header_table_entries; ph++ )
    {
        size_t o = ehead.program_header_table + ( ph * ehead.program_header_table_size );
        ElfProgramHeader32 head = {0};
        fseek( fp, (long) o, SEEK_SET );
        read = fread( &head, 1, get_min( sizeof( head ), (size_t) ehead.program_header_table_size ), fp );
        if ( 0 == read )
            usage( "can't read program header in order to load it" );
        head.swap_endianness();

        // head.type 1 == load. Other entries will overlap and even have physical addresses, but they are redundant

        if ( 0 != head.file_size && 1 == head.type )
        {
            fseek( fp, (long) head.offset_in_image, SEEK_SET );
            read = fread( memory.data() + head.physical_address - g_base_address, 1, head.file_size, fp );
            if ( 0 == read )
                usage( "can't read image into ram" );

            if ( 5 == head.flags ) // text
            {
                elf_start_text = head.physical_address - g_base_address;
                elf_cb_text = head.file_size;
                if ( trace_status )
                    printf( "  elf start text %lx, elf cb text %lx\n", elf_start_text, elf_cb_text );
            }
            else if ( 6 == head.flags ) // data + bss
            {
                elf_start_data = head.physical_address - g_base_address;
                elf_cb_data = head.file_size;
                start_bss = head.physical_address + head.file_size;
                past_bss = head.physical_address + head.memory_size;

                if ( trace_status )
                {
                    printf( "  elf start data %lx, elf cb data %lx\n", elf_start_data, elf_cb_data );
                    printf( "    start_bss: %x == %u\n", start_bss, start_bss );
                    printf( "    past_bss: %x == %u\n", past_bss, past_bss );
                }
            }
            else
                usage( "unrecognized flags in program header" );

            if ( trace_status )
                printf( "  read type %s: %x bytes into physical address %x - %x then uninitialized to %x \n", head.show_type(), head.file_size,
                        head.physical_address, head.physical_address + head.file_size - 1, head.physical_address + head.memory_size - 1 );
        }
    }

    file.close();

    char acout[ 255 ];
    strcpy( acout, argv[ 1 ] );
    strupr( acout );
    char * pdot = strchr( acout, '.' );
    if ( !pdot )
        pdot = acout + strlen( acout );

    strcpy( pdot, ".68K" );
    FILE * fpout = fopen( acout, "wb" );
    if ( !fpout )
        usage( "can't open output file\n" );

    CFile file_out( fpout );

    // Make the 68K start address the same as the first byte of TEXT from the elf file minus a few bytes for startup code and data.
    //
    // byte length:         instruction/data:
    // ------------         -----------------
    // 4:                   move pointer to argv[0] / file name to a0
    // 2:                   move a0 to d0
    // 6:                   move pointer to the_EH_FRAME_BEGIN to d1
    // 6:                   jump to _start
    // 1 + strlen( acout ): asciiz filename of 68K executable pointed to by d0
    // 0 or 1:              align to 2 bytes per 68000 requirements

    uint32_t cbPreStart = 4 + 2 + 6 + 6 + 1 + strlen( acout );

    bool alignPad = false;
    if ( cbPreStart & 1 ) // align to 2 bytes
    {
        cbPreStart++;
        alignPad = true;
    }

    HeaderCPM68K chead;
    chead.signature = 0x601a; // standard 68K signature
    chead.cb_text = cbPreStart + elf_start_data - elf_start_text; // include space between elf text and data as part of text
    chead.cb_data = elf_cb_data;
    chead.cb_bss = past_bss - start_bss;
    chead.cb_symbols = 0; // no symbols
    chead.text_start = elf_base_address - cbPreStart; // address where code starts and where pc points to begin execution
    chead.relocation_flag = 0xffff; // no relocations, not relocatble.
    chead.reserved = 0;
    if ( trace_status )
        chead.trace();

    chead.swap_endianness();

    fwrite( &chead, sizeof( chead ), 1, fpout );

    // move pointer to argv[0] to a0 ==> lea (d16, pc), a0

    uint16_t moveApp = swap_endian16( 0x41fa );
    fwrite( & moveApp, 2, 1, fpout );

    uint16_t appNamePCOffset = swap_endian16( cbPreStart - 1 - strlen( acout ) - ( alignPad ? 1 : 0 ) - 2 );
    fwrite( & appNamePCOffset, 2, 1, fpout );

    // move.l a0, d0

    uint16_t moveA0D0 = swap_endian16( 0x2008 );
    fwrite( & moveA0D0, 2, 1, fpout );

    // move pointer to eh frame data to d1 ==> move.l ##XXXX, d1

    uint16_t moveEH = swap_endian16( 0x223c );
    fwrite( & moveEH, 2, 1, fpout );

    uint32_t ehAddress = swap_endian32( the_EH_FRAME_BEGIN );
    fwrite( & ehAddress, 4, 1, fpout );

    // jmp to _start / entry_point

    uint16_t jmp = swap_endian16( 0x4ef9 );
    fwrite( & jmp, 2, 1, fpout );

    uint32_t addr = swap_endian32( ehead.entry_point );
    fwrite( & addr, 4, 1, fpout );

    // write argv[0] / the binary name

    fwrite( acout, 1 + strlen( acout ), 1, fpout );

    if ( alignPad )
    {
        char zero = 0;
        fwrite( & zero, 1, 1, fpout );
    }

    if ( trace_status )
        printf( "writing data + text: %lx bytes\n", memory_size - elf_base_address );

    fwrite( memory.data() + elf_base_address, (size_t) ( memory_size - elf_base_address ), (size_t) 1, fpout );

    file_out.close();

    if ( trace_status )
        printf( "elf file size: %d --->>> 68K file size: %d\n", portable_filelen( pinputfile ), portable_filelen( acout ) );

    printf( "elfto68k successfully created %s\n", acout );

    return 0;
} //main
