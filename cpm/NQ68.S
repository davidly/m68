* a7 stack pointer
* a6 board
* a5 pointer to position of current move
* a4 in issafe(), pointer to position check for a queen
* d7 n -- current board size 1..maxsize
* d6 r -- current row
* d5 c -- current column
* d4 solutions -- # of solutions found for current board sized n
* d3 lr -- local row variable in issafe()
* d2 lc -- local column variable in issafe()

maxsize equ 8                 * beyond 11 it gets pretty slow
boardsize equ maxsize*maxsize
.globl board
.comm board,boardsize

.text
_start:
    move.l #header_string, d1
    bsr print_string

    movea.l #board, a0         * initialize board to all zeroes
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
    jsr solve
    move.l d7, d0
    jsr show_num               * show board size
    move.l d4, d0
    jsr show_num               * show solution count
    move.l #crlf_string, d1
    bsr print_string
    addq.l #1, d7              * increment n
    cmp.w #maxsize, d7         * all done?
    ble inc_board

    clr.l d0                   * exit code
    rts                        * return to the OS

solve:
    cmp.l d5, d7               * compare c and n.  if ( c == n ) { solutions++; return; }
    beq found_solution
    move.l a5, -(a7)           * push/save address of board position
    move.w d6, -(a7)           * push/save r
    clr.l d6                   * r = 0
    lea (a6,d5), a5            * a5 now points to the top row for the current column
  next_r:                      * for ( r = 0; r < n; r++ )
    bsr issafe                 * if ( isSafe( r, c, n ) ) {
    beq inc_r                  * no need to test d0 since Z status flag is set by move/clr on issafe() exit
    move.b #1, (a5)            * set a queen at the board position.  board[r][c] = true;
    addq.l #1, d5              * increment the column
    bsr solve                  * recurse.  solve( c + 1 );
    subq.l #1, d5              * restore the column
    clr.b (a5)                 * clear the board position.  board[r][c] = false; }
  inc_r:
    addq.l #1, d6              * move to the next row
    cmp.l d6, d7               * are we past the bottom row? if ( r == n )
    beq solve_done
    adda.l #maxsize, a5        * more rows to go so update current board pointer
    bra next_r                 * loop for the next r
  found_solution:
    addq.l #1, d4              * increment solution count
    rts
  solve_done:
    move.w (a7)+, d6           * pop/restore r
    move.l (a7)+, a5           * pop/restore address to board position
    rts

* returns 1 in d0 if the board position is safe from other queens or 0 otherwise. Z status flag is set to reflect d0 value.
issafe:
    move.l d5, d2              * move c to lc, a local copy
    beq safe1                  * if there are no columns to the left the queen is safe
    subq.l #1, d2              * for ( lc = c - 1; lc >= 0 lc-- ) if ( board[r][lc] ) return false;
    move.l a5, a4              * a5 has the pointer to the board position being checked
  next_a:
    tst.b -(a4)                * is there a queen there?
    dbne d2, next_a            * if no queen is there, decrement d2.w and if d2.w isn't -1, loop. condition flags not set.
    bne safe0                  * if a queen is on the last position checked we're not safe

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
    bne safe0                  * if a queen is on the last position checked we're not safe

  safe_c:                      * for ( lr = r, lc = c; lc >= 0 && lr < n; lr++, lc-- ) if ( board[lr][lc] ) return false;
    move.l d6, d3              * move r to lr, a local copy
    movea.l a5, a4
    move.l d5, d2              * move c to lc, a local copy
    subq.l #1, d2
  next_c:
    addq.l #1, d3
    cmp.l d3, d7               * compare lr to n
    beq safe1                  * is lr n? if so, we're safe
    adda.l #maxsize-1, a4      * move to the next position to check. down and to left
    tst.b (a4)
    dbne d2, next_c            * if no queen is there, decrement d2.w and if d2.w isn't -1, loop. condition flags not set.
    bne safe0                  * if a queen is on the last position checked we're not safe

  safe1:
    moveq.l #1, d0             * also clears Z. safe from other queens.
    rts
  safe0:
    clr.l d0                   * also sets Z. not safe from other queens.
    rts

show_num:                      * print the unsigned number in d0.w right-justified 6 columns
    movea.l #num_buf6, a0
    move.b #' ', (a0)+
    move.b #' ', (a0)+
    move.b #' ', (a0)+
    move.b #' ', (a0)+
    move.b #' ', (a0)+
    move.b #'0', (a0)+

  next_digit:                  * repeatedly divide by 10 and the remainder is the next digit
    tst.w d0
    beq show_string
    andi.l #$ffff, d0
    divu #10, d0
    move.l d0, d1
    swap d1                    * get the remainder in the low half of d1
    add.b #'0', d1
    move.b d1, -(a0)
    bra next_digit

  show_string:
    move.l #num_buf6, d1
    bra print_string           * could instead just fall through...

print_string:                  * prints the $-terminated string in d1 to the console
    move.l #9, d0
    trap #2
    rts

.data

num_buf6: .dc.b '      ','$'
header_string: .dc.b '  size count',13,10,'$'
crlf_string: .dc.b 13,10,'$'

.end

