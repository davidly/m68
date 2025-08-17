*  A simple implementation of the N-Queens problem using backtracking.
*  This program counts the number of solutions for placing N queens on an
*  N x N chessboard such that no two queens threaten each other.*  Mirror and flip optimizations are not applied.
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
* a6 board
* a5 pointer to position of current move
* a4 temporary copy of a5
* d7 n -- current board size 1..maxsize
* d6 r -- current row
* d5 c -- current column
* d4 solutions -- # of solutions found for current board sized n
* d3 lr -- local row variable
* d2 lc -- local column variable

maxsize equ 8                  * 14 takes 61 seconds on my machine in the m68 emulator
boardsize equ maxsize*maxsize
.globl board
.comm board,boardsize

.text
_start:
    clr.l a1
    clr.l a2
    clr.l a3
    move.l #header_string, d1
    bsr print_string

    movea.l #board, a0         * initialize board to all zeroes since no other code initializes bss
  init_next:
    cmp.l #board+boardsize, a0
    beq init_done
    clr.b (a0)+
    bra init_next

  init_done:
    movea.l #board, a6
    move.l #1, d7
  inc_board:
    clr.l d5                   * starting column is 0
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
    cmp.l d5, d7               * compare c and n.  if ( c == n ) { solutions++; return; }
    beq found_solution
    movem.l d6/a5, -(a7)       * push/save the address of the current board position and r
    clr.l d6                   * r = 0
    lea (a6,d5), a5            * a5 now points to the top row for the current column

  next_r:                      * start of row iteration loop for the current column (for ( r = 0; r < n; r++ ))
    move.l d5, d2              * move c to lc, a local copy. start of inlined call:  if ( issafe( r, col, n ) ) {
    beq spot_safe              * if there are no columns to the left the queen is safe
    subq.l #1, d2              * for ( lc = c - 1; lc >= 0 lc-- ) if ( board[r][lc] ) return false;
    move.l a5, a4              * a5 has the pointer to the board position being checked
  next_a:
    tst.b -(a4)                * is there a queen one spot to the left?
    dbne d2, next_a            * if no queen is there, decrement d2.w and if d2.w isn't -1, loop. condition flags not set.
    bne spot_not_safe          * if a queen is on the last position checked we're not safe

  safe_b:                      * for ( lr = r, lc = c; lr >= 0 && lc >= 0; lr--, lc-- ) if ( board[lr][lc] ) return false;
    move.l d6, d3              * move r to lr, a local copy
    movea.l a5, a4
    move.l d5, d2              * move c to lc, a local copy
    subq.l #1, d2
  next_b:
    subq.l #1, d3
    bmi safe_c                 * if row < 0 move on to "C" tests
    suba.l #maxsize+1, a4      * move to the next position to check. up and to left
    tst.b (a4)
    dbne d2, next_b            * if no queen is there, decrement d2.w and if d2.w isn't -1, loop. condition flags not set.
    bne spot_not_safe          * if a queen is on the last position checked we're not safe

  safe_c:                      * for ( lr = r, lc = c; lc >= 0 && lr < n; lr++, lc-- ) if ( board[lr][lc] ) return false;
    move.l d6, d3              * move r to lr, a local copy
    movea.l a5, a4
    move.l d5, d2              * move c to lc, a local copy
    subq.l #1, d2
  next_c:
    addq.l #1, d3
    cmp.l d3, d7               * compare lr to n
    beq spot_safe              * is lr n? if so, we're safe
    adda.l #maxsize-1, a4      * move to the next position to check. down and to left
    tst.b (a4)
    dbne d2, next_c            * if no queen is there, decrement d2.w and if d2.w isn't -1, loop. condition flags not set.
    bne spot_not_safe

  spot_safe:
    move.b #1, (a5)            * set a queen at the board position.  board[r][c] = true;
    addq.l #1, d5              * increment the column
    bsr solve                  * recurse.  solve( c + 1 );
    subq.l #1, d5              * restore the column
    clr.b (a5)                 * clear the board position.  board[r][c] = false; }

  spot_not_safe:
    addq.l #1, d6              * move to the next row
    cmp.l d6, d7               * are we past the bottom row? if ( r == n )
    beq solve_done
    adda.l #maxsize, a5        * more rows to go so update current board pointer
    bra next_r                 * loop for the next r

  found_solution:
    addq.l #1, d4              * increment solution count
    rts
  solve_done:
    movem.l (a7)+, a5/d6       * pop/restore r and the address to the current board position
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
