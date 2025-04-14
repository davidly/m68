#pragma once

#include <stdint.h>

struct MMapEntry
{
    uint64_t address;
    uint64_t length;
};

class CMMap
{
    private:
        vector<MMapEntry> entries; // sorted by address
        uint64_t base;
        uint64_t length;
        uint64_t peak;
        uint8_t * pmem;

        size_t binary_search( uint64_t key )
        {
            if ( 0 == entries.size() )
               return (size_t) -1;
            
            size_t low = 0;
            size_t high = entries.size() - 1;
            while ( low <= high )
            {
                size_t mid = low + ( ( high - low ) / 2 );
                if ( entries[ mid ].address == key )
                    return mid;
                if ( key < entries[ mid ].address )
                    high = mid - 1;
                else
                    low = mid + 1;
            }
            return (size_t) -1;
        } //binary_search

        void zero_entry( size_t i )
        {
            memset( pmem + entries[ i ].address, 0, entries[ i ].length );
        } //zero_entry

        void validate()
        {
#ifndef NDEBUG
            for ( size_t i = 0; i < entries.size(); i++ )
            {
                MMapEntry & entry = entries[ i ];
                if ( i < ( entries.size() - 1 ) )
                {
                    assert( entry.address < entries[ i + 1 ].address );
                    assert( ( entry.address + entry.length ) <= entries[ i + 1 ].address );
                }
                else
                    assert( ( entry.address + entry.length ) <= ( base + length ) );
            }
#endif
        } //validate

    public:
        CMMap() : base( 0 ), length( 0 ), peak( 0 ), pmem( 0 ) {}
        ~CMMap() { validate(); }
        uint64_t peak_usage() { return peak; }

        void initialize( uint64_t b, uint64_t l, uint8_t * p )
        {
            base = b;
            length = l;
            pmem = p;
        } //initialize

        void trace_allocations()
        {
            if ( entries.size() )
            {
                uint64_t beyond = 0;
                tracer.Trace( "  app has %zu mmap allocations. address, size:\n", entries.size() );
                uint64_t total = 0;
                for ( size_t i = 0; i < entries.size(); i++ )
                {
                    MMapEntry & entry = entries[ i ];
                    tracer.Trace( "    %zu: %llx, %llu == %llx\n", i, entry.address, entry.length, entry.length );
                    total += entry.length;
                    beyond = entry.address + entry.length;
                }
                tracer.Trace( "    total memory in use: %llu bytes spanning %llu bytes\n", total, beyond - base );
            }
        } //trace_allocations

        uint64_t allocate( uint64_t l )
        {
            assert( 0 == ( l & 0xfff ) );
            trace_allocations();

            if ( 0 == entries.size() )
            {
                tracer.Trace( "  adding a first mmap entry, l %llu, length %llu\n", l, length );
                if ( l > length )
                {
                    tracer.Trace( "  mmap alloc request %llu larger than reserved size %llu\n", l, length );
                    return 0;
                }

                tracer.Trace( "  in mmap allocate, base %#lx\n", base );

                MMapEntry entry = { base, l };
                entries.push_back( entry );
                zero_entry( 0 );
                peak = l;
                trace_allocations();
                validate();
                return base;
            }
            else
            {
                // first look for a gap large enough to work

                for ( size_t i = 0; i < entries.size() - 1; i++ )
                {
                    MMapEntry & entry = entries[ i ];

                    uint64_t gapSize = entries[ i + 1 ].address  - ( entry.address + entry.length );
                    if ( gapSize >= l )
                    {
                        uint64_t result = entry.address + entry.length;
                        MMapEntry newEntry = { result, l };
                        entries.insert( i + 1 + entries.begin(), newEntry );
                        tracer.Trace( "  inserted in the gap, result %llx\n", result );
                        zero_entry( i + 1 );
                        trace_allocations();
                        validate();
                        return result;
                    }
                }

                // add a new entry at the end if space is available

                tracer.Trace( "  no sufficient gap found; adding a subsequent mmap entry\n" );

                MMapEntry & entry = entries[ entries.size() - 1 ];
                uint64_t free_offset = entry.address + entry.length;

                if ( l < ( length - ( free_offset - base ) ) )
                {
                    MMapEntry newentry = { free_offset, l };
                    entries.push_back( newentry );
                    zero_entry( entries.size() - 1 );
                    trace_allocations();
                    validate();
                    peak = ( free_offset - base + l );
                    return free_offset;
                }
            }

            tracer.Trace( "  mmap alloc request %llu can't be met\n", l );
            return 0;
        } //allocate

        bool free( uint64_t a, uint64_t l )
        {
            trace_allocations();
            size_t match = binary_search( a );
            if ( -1 != match )
            {
                if ( l < entries[ match ].length )
                    entries[ match ].length = l;
                else
                    entries.erase( entries.begin() + match );
                trace_allocations();
                validate();
            }
            else
            {
                tracer.Trace( "  munmap/free can't find entry %llu to free\n", a );
                return false;
            }

            return true;
        } //free

        uint64_t resize( uint64_t a, uint64_t old_l, uint64_t new_l, bool may_move )
        {
            assert( 0 == ( new_l & 0xfff ) );
            trace_allocations();
            size_t match = binary_search( a );
            if ( -1 != match )
            {
                if ( new_l <= old_l )
                {
                    tracer.Trace( "  mremap/resize entry size shrunk\n" );
                    entries[ match ].length = new_l;
                    validate();
                    return entries[ match ].address;
                }

                // check if it can be extended in place

                if ( ( ( match == ( entries.size() - 1 ) ) && ( ( entries[ match ].address + new_l ) <= length ) ) ||
                       ( ( entries[ match ].address + new_l ) < entries[ match + 1 ].address ) )
                {
                    tracer.Trace( "  mremap extending entry %zu in place from size %llu to %llu\n", match, old_l, new_l );
                    entries[ match ].length = new_l;
                    validate();
                    return a;
                }

                if ( may_move )
                {
                    // first look for a gap large enough to work
    
                    for ( size_t i = 0; i < entries.size() - 1; i++ )
                    {
                        MMapEntry & entry = entries[ i ];
    
                        uint64_t gapSize = entries[ i + 1 ].address  - ( entry.address + entry.length );
                        if ( gapSize >= new_l )
                        {
                            uint64_t result = entry.address + entry.length;
                            MMapEntry newEntry = { result, new_l };
                            tracer.Trace( "  mremap inserted in gap pmem %p, dst %#llx, src %#llx, old len %lld new len %lld\n",
                                          pmem, newEntry.address, entries[ match ].address, entries[ match ].length, new_l );
                            memcpy( pmem + newEntry.address, pmem + entries[ match ].address, entries[ match ].length );
                            memset( pmem + newEntry.address + entries[ match ].length, 0, new_l - entries[ match ].length );
                            entries.insert( i + 1 + entries.begin(), newEntry );
                            match = binary_search( a ); // it may have moved after the insert
                            entries.erase( entries.begin() + match );
                            trace_allocations();
                            validate();
                            return result;
                        }
                    }

                    // create a new entry at the end and memcpy the old to the new

                    MMapEntry & lastEntry = entries[ entries.size() - 1 ];
                    uint64_t free_offset = lastEntry.address + lastEntry.length;

                    if ( new_l < ( length - ( free_offset - base ) ) )
                    {
                        tracer.Trace( "  free_offset %llx, new_l %llx\n", free_offset, new_l );
                        MMapEntry newEntry = { free_offset, new_l };
                        entries.push_back( newEntry );
                        tracer.Trace( "  mremap added at end pmem %p, dst %#llx, src %#llx, old len %lld\n",
                                      pmem, newEntry.address, entries[ match ].address, entries[ match ].length );
                        memcpy( pmem + newEntry.address, pmem + entries[ match ].address, entries[ match ].length );
                        memset( pmem + newEntry.address + entries[ match ].length, 0, new_l - entries[ match ].length );
                        entries.erase( entries.begin() + match );
                        trace_allocations();
                        validate();
                        peak = ( free_offset - base + new_l );
                        return free_offset;
                    }

                    tracer.Trace( "  insufficient RAM left, so giving up on resize\n" );
                }
                else
                    tracer.Trace( "  can't move the address, so giving up on resize\n" );
            }
            else
                tracer.Trace( "  mremap/resize can't find entry %llu to resize\n", a );

            return 0;
        } //resize
};

