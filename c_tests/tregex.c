#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <regex.h>
#include <cstring>

int match_dos_wildcard( const char *filename, const char *pattern )
{
    if ( strlen( filename ) > 12 )
        return false;
    if ( strlen( pattern ) > 12 )
         return false;

    // Convert DOS wildcard pattern to POSIX regex
    char regex[ 50 ];
    char *dest = regex;
    *dest++ = '^';
    while ( *pattern )
    {
        switch ( *pattern )
        {
            case '*':
                *dest++ = '.';
                *dest++ = '*';
                break;
            case '?':
                *dest++ = '.';
                break;
            case '.':
                *dest++ = '\\';
                *dest++ = '.';
                break;
            default:
                *dest++ = *pattern;
        }
        pattern++;
    }
    *dest++ = '$';
    *dest = 0;

    if ( strlen( regex ) > ( 2 + 24 ) )
    {
        printf( "regex expanded too much\n" );
        exit( 1 );
    }

    // Compile the regex
    regex_t regex_compiled;
    int cflags = REG_NOSUB | REG_ICASE;
    int reti = regcomp( &regex_compiled, regex, cflags );
    if ( reti )
    {
        printf( "Could not compile regex, error %#x\n", reti );
        exit( 1 );
    }

    // Match the filename against the regex
    reti = regexec( &regex_compiled, filename, 0, 0, 0 );
    regfree( &regex_compiled );
    return ( 0 == reti );
} //match_dos_wildcard

void test( const char * name, const char * pattern, bool expected )
{
    bool match = match_dos_wildcard( name, pattern);
    if ( match != expected )
    {
        printf( "regex failure. name '%s', pattern '%s', expected %d\n", name, pattern, expected );
        exit( 1 );
    }
} //test

int main( int argc, char * argv[] )
{
    test( "foo.txt", "f*.txt", true );
    test( "f.txt", "f*.txt", true );
    test( "foo.txt", "f*", true );
    test( "f.txt", "f*.txt", true );
    test( "f", "f*.txt", false );
    test( "foo", "f*.txt", false );
    test( "f.txt", "f*", true );
    test( "f", "f*", true );
    test( "foo", "f*", true );
    test( "foo", "?o?", true );
    test( "foo", "*o?", true );
    test( "foo", "*oo", true );
    test( "foo", "f*oo", true );
    test( "foo", "f?oo", false );
    test( "foo", "*oox", false );

    test( "foo.txt", "F*.txt", true );
    test( "f.txt", "f*.TXT", true );
    test( "foo.txt", "F*", true );
    test( "f.txt", "F*.TXT", true );

    test( "foo.pas", "f*.txt", false );
    test( "foo.pas", "*.pas", true );
    test( "foo.PAS", "*.pas", true );
    test( "foo.bas", "*.pas", false );
    test( "fOo.pas", "*.pas", true );

    test( "f_o.pas", "*.pAs", true );
    test( "zf_____o.pas", "*.pAs", true );
    test( "bar.bab", "*.?A?", true );

    printf( "regex test completed with great success\n" );
} //main
