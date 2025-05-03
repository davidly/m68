#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

char ac[ 4096 ];
char other[ 4096 ];
char zeroes[ 4096 ] = {0};

#define _countof( X ) ( sizeof( X ) / sizeof( X[0] ) )

extern int memcmp(const void *s1, const void *s2, int n);


#if 0 // these are much slower but are interesting emulator test cases

#define memcmp fake_memcmp
#define memcpy fake_memcpy
#define memset fake_memset
#define strchr fake_strchr
#define strrchr fake_strrchr
#define strstr fake_strstr
#define strlen fake_strlen

int fake_memcmp(const void *s1, const void *s2, int n)
{
    const unsigned char *p1 = (const unsigned char *) s1;
    const unsigned char *p2 = (const unsigned char *) s2;
    while (n-- > 0)
    {
        if (*p1 != *p2)
            return (*p1 < *p2) ? -1 : 1;
        p1++;
        p2++;
    }

    return 0;
}

void * fake_memcpy(void *dest, const void *src, size_t n)
{
    int i;
    char *d = (char *)dest;
    const char *s = (const char *)src;
    for ( i = 0; i < n; i++)
        d[i] = s[i];
    return dest;
}

void * fake_memset(void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char *)s;
    while (n--)
        *p++ = (unsigned char)c;
    return s;
}

char * fake_strchr(const char *str, int c)
{
    while (*str != '\0')
    {
        if (*str == c)
            return (char *)str;
        str++;
    }

    return NULL;
}

char * fake_strrchr(const char *str, int c)
{
    const char *last_occurrence = NULL;
    while (*str != '\0')
    {
        if (*str == c)
            last_occurrence = str;
        str++;
    }
    return (char *)last_occurrence;
}

char * fake_strstr(const char *haystack, const char *needle)
{
    const char * h;
    const char * n;

    if (!*needle)
        return (char *)haystack;

    while (*haystack)
    {
        h = haystack;
        n = needle;

        while (*h && *n && *h == *n)
        {
            h++;
            n++;
        }

        if (!*n)
            return (char *)haystack;

        haystack++;
    }
    return NULL;
}

int fake_strlen(const char *str)
{
    int len = 0;

    while (*str != '\0')
    {
        len++;
        str++;
    }
    return len;
}

#endif

int main( int argc, char * argv[] )
{
    int i, start, end, len, slen, offset, l, loops;
    unsigned int r;
    char alpha[27];
    char orig, *pbang *pattern, *p;
    int loop_count = ( argc > 1 ) ? atoi( argv[ 1 ] ) : 1;

    for ( loops = 0; loops < loop_count; loops++ )
    {
        for ( i = 0; i < sizeof( ac ); i++ )
            ac[ i ] = ( 'a' + ( i % 26 ) );

        printf( "testing strlen\n" );
        for ( i = 0; i < 1000; i++ )
        {
            start = ( (unsigned int) rand() ) % 300;
            end = 1 + start + ( (unsigned int) rand() ) % 3000;
            len = end - start;
            orig = ac[ end ];
            ac[ end ] = 0;
            slen = strlen( ac + start );
            if ( len != slen )
            {
                printf( "strlen failed: iteration %d, len %d, strlen %d, start %d, end %d\n", i, len, slen, start, end );
                exit( 1 );
            }
            ac[ end ] = orig;
        }

        printf( "testing strchr and strrchr\n" );
        for ( i = 0; i < 1000; i++ )
        {
            start = ( (unsigned int) rand() % 300 );
            end = 1 + start + ( (unsigned int) rand() % 70 );
            len = end - start;
            orig = ac[ end ];
            ac[ end ] = '!';
            pbang = strchr( ac + start, '!' );
            if ( !pbang )
            {
                printf( "strchr failed to find char: iteration %d, len %d, start %d, end %d\n", i, len, start, end );
                exit( 1 );
            }
            if ( pbang != ( ac + end ) )
            {
                printf( "strchr offset incorrect: iteration %d, len %d, start %d, end %d\n", i, len, start, end );
                exit( 1 );
            }
            pbang = strrchr( ac + start, '!' );
            if ( !pbang )
            {
                printf( "strrchr failed to find char: iteration %d, len %d, start %d, end %d\n", i, len, start, end );
                exit( 1 );
            }
            if ( pbang != ( ac + end ) )
            {
                printf( "strrchr offset incorrect: iteration %d, len %d, start %d, end %d\n", i, len, start, end );
                exit( 1 );
            }
            if ( strchr( ac + start, '$' ) )
            {
                printf( "strrchr somehow found $: iteration %d, len %d, start %d, end %d\n", i, len, start, end );
                exit( 1 );
            }
            ac[ end ] = orig;
        }

        printf( "testing strstr\n" );
        strcpy( alpha, "abcdefghijklmnopqrstuvwxyz" );
        for ( i = 0; i < 1000; i++ )
        {
            start = ( (unsigned int) rand() % 300 );
            offset = ( (unsigned int) rand() % 26 );
            r = (unsigned int) rand(); // put r in local to work around compiler bug in 68k gcc
            len = 1 + ( r % ( 26 - offset ) );
            if ( ( offset + len ) > 26 )
            {
                printf( "strstr test bug iteration %d r %u, offset %d, len %d\n", i, r, offset, len );
                exit( 1 );
            }
            pattern = alpha + offset;
            p = strstr( ac + start, pattern );
            if ( !p )
            {
                printf( "strstr pattern not found iteration %d, start %d, offset %d, len %d, pattern %s\n", i, start, offset, len, pattern );
                exit( 1 );
            }
            if ( memcmp( p, pattern, len ) )
            {
                printf( "strstr found the wrong pattern iteration %d, start %d, offset %d, len %d, pattern %s\n", i, start, offset, len, pattern );
                exit( 1 );
            }
            if ( strstr( ac + start, "gfe" ) )
            {
                printf( "strstr somehow found gfe. iteration %d, start %d, offset %d\n", i, start, offset );
                exit( 1 );
            }
        }

        printf( "testing memcpy and memcmp\n" );
        for ( i = 0; i < 1000; i++ )
        {
            r = (unsigned int) r;
            start = ( r % 300 );
            r = (unsigned int) r;
            end = 1 + start + ( r % 3000 );
            len = end - start;

            memcpy( other + start, ac + start, len );

            if ( memcmp( other + start, ac + start, len ) )
            {
                printf( "memcmp of memcpy'ed memory failed to find match, iteration %d, len %d, start %d, end %d\n", i, len, start, end );
                exit( 1 );
            }

            memset( other + start, 0, len );

            if ( memcmp( other + start, zeroes + start, len ) )
            {
                printf( "zeroes not found in zero-filled memory, iteration %d, len %d, start %d, end %d\n", i, len, start, end );
                exit( 1 );
            }
        }

        printf( "testing printf\n" );
        for ( i = 0; i < 20; i++ )
        {
            start = ( (unsigned int) rand() % 300 );
            end = 1 + start + ( ( (unsigned int) rand() ) % 70 );
            len = end - start;
            orig = ac[ end ];
            ac[ end ] = 0;

            l = strlen( ac + start );
            printf( "%2d (%2d): %s\n", len, l, ac + start );
            ac[ end ] = orig;
        }
    }

    printf( "tstr completed with great success\n" );
    return 0;
} //main