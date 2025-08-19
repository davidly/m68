/*
  nqueens.c

  A simple implementation of the N-Queens problem using backtracking.
  This program counts the number of solutions for placing N queens on an
  N x N chessboard such that no two queens threaten each other.
  Mirror and flip optimizations are not applied.

    n   fundamental     all
    1   1               1
    2   0               0
    3   0               0
    4   1               2
    5   2               10
    6   1               4
    7   6               40
    8   12              92
    9   46              352
    10  92              724
    11  341             2,680
    12  1,787           14,200
    13  9,233           73,712
    14  45,752          365,596
    15  285,053         2,279,184
    16  1,846,955       14,772,512
*/

#include <stdio.h>

#define N 8 // largest board size to solve

long solutions = 0;
int board[N];

void printSolution( int n )
{
    int r;
    for ( r = 0; r < n; r++ )
    {
        printf( "%2d ", board[r] );
        printf( "\n" );
    }
    printf( "\n" );
} //printSolution

// this assumes pieces are placed in order from column 0 to n-1

bool isSafe( int row, int col, int n )
{
    int c, r;

    if ( 0xff != board[ row ] )
        return false;

    for ( r = row - 1, c = col - 1; r >= 0 && c >= 0; r--, c-- )
        if ( board[r] == c )
            return false;

    for ( r = row + 1, c = col - 1; c >= 0 && r < n; r++, c-- )
        if ( board[r] == c )
            return false;

    return true;
} //isSafe

void solve( int col, int n )
{
    int r;

    if ( col == n )
    {
        //printSolution( n );
        solutions++;
        return;
    }

    for ( r = 0; r < n; r++ )
    {
        if ( isSafe( r, col, n ) )
        {
            board[r] = col;
            solve( col + 1, n );
            board[r] = 0xff;
        }
    }
} //solve

int main()
{
    int n;
    for ( n = 0; n < N; n++ )
        board[ n ] = 0xff;
    printf( "  size    solutions\n" );
    for ( n = 1; n <= N; n++ )
    {
        solve( 0, n );
        printf("  %4u   %10lu\n", n, solutions );
        solutions = 0;
    }
    return 0;
} //main
