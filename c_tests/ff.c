#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <ctype.h>
#include <time.h>
#include <cerrno>
#include <cstring>
#include <string.h>
#include <string>
#include <queue>
#include <stdlib.h>
#include <unistd.h>
#include <fnmatch.h>

extern "C" int lstat (const char *__restrict __path, struct stat *__restrict __buf );

#ifndef DT_LNK // newlib doesn't define these

  // these two are goofy/buggy ai-generated implementations that exist because newlib doesn't have them
  #include "fnmatch.c"
  #include "realpath.c"

  #define DT_UNKNOWN 0
  #define DT_FIFO 1
  #define DT_CHR 2
  #define DT_DIR 4
  #define DT_BLK 6
  #define DT_REG 8
  #define DT_LNK 10
  #define DT_SOCK 12
  #define DT_WHT 14

#endif // DT_LNK

bool g_exclude_mnt = true;
bool g_show_info = false;
bool g_case_sensitive = true;
bool g_use_stat = false;

void usage( const char * perr )
{
    if ( 0 != perr )
        printf( "error: %s\n", perr );

    printf( "usage: ff <start> pattern\n" );
    printf( "  finds files given an optional starting folder\n" );
    printf( "  arguments:\n" );
    printf( "      <start>         optional folder where enumeration starts. default is root\n" );
    printf( "      pattern         file pattern to search for, likely enclosed in quotes\n" );
    printf( "      -c              case-insensitive filename matching\n" );
    printf( "      -i              show file information (type, size, last modified time)\n" );
    printf( "      -m              don't exclude files under /mnt, /sys, or /proc\n" );
    printf( "      -s              don't use dirent->d_type; use stat and lstat instead\n" );
    printf( "  examples:\n" );
    printf( "      ff /home/user \"*.txt\"\n" );
    printf( "      ff -c -i .. lesserafim.png\n" );
    printf( "      ff \"*gcc\"\n" );
    exit( 1 );
} //usage

// private strlen, strcmp, tolower, and strlwr to ensure they get inlined for performance

inline int my_strlen( const char * p )
{
    const char * start = p;

    while ( *p )
        p++;

    return (int) ( p - start );
} //my_strlen

inline int my_strcmp( const char * s1, const char * s2 )
{
    while ( *s1 && *s2 && ( *s1 == *s2 ) )
    {
        s1++;
        s2++;
    }

    if ( 0 == *s1 )
    {
        if ( 0 == *s2 )
            return 0;

        return -1;
    }

    if ( 0 == *s2 )
        return 1;

    return (unsigned char) *s1 - (unsigned char) *s2;
} //my_strcmp

inline char my_tolower( char c )
{
    if ( c >= 'A' && c <= 'Z' )
        return c + ( 'a' - 'A' );

    return c;
} //my_tolower

inline char * my_strlwr( char * str ) 
{
    for ( char * p = str; *p; ++p )
        *p = my_tolower( *p );
    return str;
} //my_strlwr

inline bool is_excluded( const char * p )
{
    if ( g_exclude_mnt )
        return ( !my_strcmp( "/mnt", p ) || !my_strcmp( "/sys", p ) || !my_strcmp( "/proc", p ) );

    return false;
} //is_excluded

const char * mode_str( uint16_t mode )
{
    static char ac[ 12 ];
    ac[ 0 ] = S_ISSOCK( mode ) ? 's' : S_ISLNK( mode ) ? 'l' : S_ISREG( mode ) ? ' '  : S_ISBLK( mode ) ? 'b' :
              S_ISDIR( mode )  ? 'd' : S_ISCHR( mode ) ? 'c' : S_ISFIFO( mode ) ? 'f' : '?';
    ac[ 1 ] = ' ';

    ac[ 2 ] = ( mode & S_IRUSR ) ? 'r' : '-';
    ac[ 3 ] = ( mode & S_IWUSR ) ? 'w' : '-';
    ac[ 4 ] = ( mode & S_IXUSR ) ? 'x' : '-';

    ac[ 5 ] = ( mode & S_IRGRP ) ? 'r' : '-';
    ac[ 6 ] = ( mode & S_IWGRP ) ? 'w' : '-';
    ac[ 7 ] = ( mode & S_IXGRP ) ? 'x' : '-';

    ac[ 8 ] = ( mode & S_IROTH ) ? 'r' : '-';
    ac[ 9 ] = ( mode & S_IWOTH ) ? 'w' : '-';
    ac[ 10 ] = ( mode & S_IXOTH ) ? 'x' : '-';

    ac[ 11 ] = 0;
    return ac;
} //mode_str

void search( const char * pstart, const char * ppattern )
{
    std::queue<std::string> q;
    q.push( pstart );

    while ( !q.empty() )
    {
        std::string current = q.front();
        q.pop();

        DIR * dir = opendir( current.c_str() );
        if ( 0 == dir )
        {
            if ( EACCES != errno && ENOENT != errno ) // if we don't have access or it's not a real file then don't print an error
                printf( "can't open starting folder '%s', error %d\n", current.c_str(), errno );
            continue;
        }
        
        static char next[ 2048 ]; // windows will return very long paths these days (if running wsl and/or an emulator)
        strcpy( next, current.c_str() );
        int dir_len = strlen( next );

        if ( '/' != next[  dir_len - 1 ] )
        {
            next[ dir_len++ ] = '/';
            next[ dir_len ] = 0;
        }

        while ( struct dirent * pentry = readdir( dir ) )
        {
            // skip . and ..
            if ( ( '.' == pentry->d_name[ 0 ] ) && ( ( 0 == pentry->d_name[ 1 ] ) || ( ( '.' == pentry->d_name[ 1 ] ) && ( 0 == pentry->d_name[ 2 ] ) ) ) )
                continue;

            if ( ( my_strlen( pentry->d_name ) + dir_len ) >= sizeof( next ) )
            {
                printf( "error: path too long, skipping '%s'\n", pentry->d_name );
                continue;
            }

            strcpy( next +  dir_len, pentry->d_name );

            if ( is_excluded( next ) )
                continue;

            if ( g_use_stat )
                pentry->d_type = DT_UNKNOWN; // really just for testing stat() and lstat()
            
            bool is_dir = false;
            
            if ( DT_UNKNOWN == pentry->d_type )
            {
                struct stat st;
                int result = stat( next, & st );
                if ( result )
                {
                    //printf( "error: stat failed for '%s', errno: %d\n", next, errno );
                    continue;
                }
                
                is_dir = S_ISDIR( st.st_mode );
            }
            else
                is_dir = ( DT_DIR == pentry->d_type );
        
            struct stat st_link;
            bool lstat_retrieved = false;

            if ( is_dir )
            {
                bool is_link = false;

                if ( DT_UNKNOWN == pentry->d_type ) // some file systems don't support setting d_type
                {
                    int result = lstat( next, & st_link );
                    if ( result )
                    {
                        //printf( "error: lstat failed for '%s', errno: %d\n", next, errno );
                        continue;
                    }
                    lstat_retrieved = true;
                    is_link = S_ISLNK( st_link.st_mode );
                }
                else
                    is_link = ( DT_LNK == pentry->d_type );

                if ( !is_link )  // only queue directories, not links to directories
                    q.push( next );
            }
        
            if ( !g_case_sensitive )
                my_strlwr( pentry->d_name );

            if ( !fnmatch( ppattern, pentry->d_name, 0 ) )
            {
                if ( g_show_info )
                {
                    if ( !lstat_retrieved )
                    {
                        int result = lstat( next, & st_link );
                        if ( result )
                        {
                            //printf( "error: stat failed for '%s', errno: %d\n", next, errno );
                            continue;
                        }
                    }

                    struct tm * timeInfo = localtime( & st_link.st_mtime );
                    char buffer[ 80 ];
                    int result = strftime( buffer, sizeof( buffer ), "%Y-%m-%d %H:%M:%S", timeInfo );
                    if ( 0 == result )
                    {
                        printf( "can't format date/time for %s\n", next );
                        continue;
                    }

                    printf( "%s  %13ld  %s  ", mode_str( (uint16_t) st_link.st_mode ), st_link.st_size, buffer );
                }

                printf( "%s\n", next );
             }
        }

        int result = closedir( dir );
        if ( 0 != result )
            printf( "error: closedir after enumeration failed, errno: %d\n", errno );
    }
} //search

int main( int argc, char * argv[] )
{
    char * pstart = 0;
    char * ppattern = 0;

    for ( int i = 1; i < argc; i++ )
    {
        if ( !strcmp( argv[ i ], "-c" ) )
            g_case_sensitive = false;
        else if ( !strcmp( argv[ i ], "-i" ) )
            g_show_info = true;
        else if ( !strcmp( argv[ i ], "-m" ) )
            g_exclude_mnt = false;
        else if ( !strcmp( argv[ i ], "-s" ) )
            g_use_stat = false;
        else if ( '-' == argv[i][0] )
            usage( "unrecognized argument" );
        else if ( !ppattern )
            ppattern = argv[ i ];
        else if ( !pstart )
        {
            pstart = ppattern;
            ppattern = argv[ i ];
        }
        else
            usage( "too many arguments" );
    }

    if ( 0 == ppattern )
        usage( "missing pattern argument" );

    char acslash[ 2 ];
    strcpy( acslash, "/" );

    if ( 0 == pstart )
        pstart = acslash;

    char * presolved = realpath( pstart, 0 );
    if ( !presolved )
        usage( "usable to resolve starting path" );

    if ( !g_case_sensitive )
        my_strlwr( ppattern );

    search( presolved, ppattern );
    free( presolved );

    return 0;
} //main
