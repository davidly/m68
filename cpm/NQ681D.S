*  A simple implementation of the N-Queens problem using backtracking.
*  This program counts the number of solutions for placing N queens on an
*  N x N chessboard such that no two queens threaten each other.*  Mirror and flip optimizations are not applied.
*  This version uses a 1-dimensional array of rows that contain the column # of a placed queen or 0xff for empty.
*
*    n   fundamental     all
*    1   1               1
*    2   0               0
*    3   0               0
*    4   1               2
*    5   2               10
*    6   1               4
*    7   6               40
*    8   12              92
*    9   46              352
*    10  92              724
*    11  341             2,680
*    12  1,787           14,200
*    13  9,233           73,712
*    14  45,752          365,596
*    15  285,053         2,279,184
*    16  1,846,955       14,772,512
*
* a7 stack pointer
* a6 board -- an array of rows containing 255 for blank or the column were a queen is located
* d7 n -- current board size 1..maxsize
* d6 r -- current row
* d5 c -- current column
* d4 solutions -- # of solutions found for current board sized n
* d3 lr -- local row variable
* d2 lc -- local column variable

maxsize equ 8
boardsize equ maxsize
.globl board
.comm board,boardsize

.text
_start:
    move.l #header_string, d1
    bsr print_string

    movea.l #board, a0         * initialize board to all 0xff values
    move.l #boardsize-1, d0
  init_next:
    move.b #$ff, (a0)+
    dbf d0, init_next

  init_done:
    movea.l #board, a6
    moveq #1, d7               * start with a board size of 1
  inc_board:
    moveq #0, d5               * starting column is 0
    clr.l d4                   * clear solution count
    bsr solve
    move.l d7, d0
    bsr show_num               * show board size
    move.l d4, d0
    bsr show_num               * show solution count
    move.l #crlf_string, d1
    bsr print_string
    addq.l #1, d7              * increment n
    cmp.w #maxsize, d7         * all done? Word-size (.w) comparison is sufficient since larger boards are impractical to compute
    ble inc_board

    clr.l d0                   * exit code
    rts                        * return to the OS

solve:
    cmp.l d7, d5               * compare c and n.  if ( c == n ) { solutions++; return; }
    beq found_solution
    move.l d6, -(a7)           * push/save r
    clr.l d6                   * r = 0

  next_r:                      * start of row iteration loop for the current column for ( r = 0; r < n; r++ )
    cmp.b #$ff, (a6,d6)        * if ( issafe( r, col, n ) ) { ... if ( 0xff != board[ row ] ) return false;
    bne spot_not_safe
    move.l d5, d2              * move c to lc, a local copy. start of inlined call:
    dbf d2, not_fc             * for ( r = row - 1, c = col - 1; r >= 0 && c >= 0; r--, c-- ) if ( board[r] == c ) return false;
    bra spot_safe              * if we're on the first column there can't be any other queens on either diagonal
  not_fc:                      * not first column
    move.l d6, d3              
  next_b:
    dbf d3, not_fr:            * move up a row
    bra safe_c                 * if we're on the first row there are no queens above, but there may be some below
  not_fr:
    cmp.b (a6,d3), d2          * looking diagonally up and to the left does a queen match the current position?
    beq spot_not_safe          * if so, the new spot isn't safe
    dbf d2, next_b             * move left a column and check if should loop

  safe_c:                      * for ( r = row + 1, c = col - 1; c >= 0 && r < n; r++, c-- ) if ( board[r] == c ) return false;
    move.l d6, d3              * move r to lr, a local copy
    move.l d5, d2              * move c to lc, a local copy
  next_c:
    addq.l #1, d3              * move down a row
    cmp d7, d3                 * past the last row?
    beq spot_safe              * if row is == # of rows then we're done
    dbf d2, nc_more            * move left a column. if the column is < 0 then we're done
    bra spot_safe
  nc_more:
    cmp.b (a6,d3), d2          * is there a queen at that position?
    beq spot_not_safe          * if so we're not safe
    bra next_c                 * loop again on the diagonal down and left

  spot_safe:
    move.b d5, (a6,d6)         * set a queen at the board position. board[r] = col;
    addq.l #1, d5              * increment the column
    bsr solve                  * recurse.  solve( c + 1 );
    subq.l #1, d5              * restore the column
    move.b #$ff, (a6,d6)       * clear the board position.  board[r] = 0xff;

  spot_not_safe:
    addq.l #1, d6              * move to the next row
    cmp.l d6, d7               * are we past the bottom row? if ( r == n )
    beq solve_done
    bra next_r                 * loop for the next r

  found_solution:
    addq.l #1, d4              * increment solution count
    rts
  solve_done:
    move.l (a7)+, d6           * pop/restore r
    rts

show_num:                      * print the unsigned number in d0.l right-justified 8 columns. Max value is 655350
    movea.l #num_buf8, a0
    move.b #' ', (a0)+
    move.b #' ', (a0)+
    move.b #' ', (a0)+
    move.b #' ', (a0)+
    move.b #' ', (a0)+
    move.b #' ', (a0)+
    move.b #' ', (a0)+
    move.b #'0', (a0)+

  next_digit:                  * repeatedly divide by 10 and the remainder is the next digit
    tst.l d0
    beq show_string
    divu #10, d0               * result is a word in d0.w, so the max value is 655350
    move.l d0, d1
    swap d1                    * get the remainder to the low half of d1
    add.b #'0', d1
    move.b d1, -(a0)           * update the string with the next-lowest digit
    andi.l #$ffff, d0          * clear out the remainder from the upper half of d0
    bra next_digit

  show_string:
    move.l #num_buf8, d1
    bra print_string           * could instead just fall through but code might get inserted later

print_string:                  * prints the $-terminated string in d1 to the console
    move.l #9, d0
    trap #2
    rts

.data

num_buf8: .dc.b '        ','$'
header_string: .dc.b '    size   count',13,10,'$'
crlf_string: .dc.b 13,10,'$'

.end
