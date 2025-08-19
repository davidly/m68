* 68000 for cp/m 68k assembler version of app to prove you can't win at tic-tac-toe
* u in the name is for unrolled for loops
* register usage:
*    a7    stack pointer
*    a6    board
*    a5    winproc array
*    a4+a3 number of iterations to run
*    d7    global move count
*    d6    current move
*    d5    current depth
*    d4    alpha
*    d3    beta
*    d2    value
*    

.text
_start:
    move.l 4(a7), a0                     * pointer to base page
    adda.l #$81, a0
    jsr atou
    tst d0
    bne _arguments_complete
    move.l #defiters, d0

  _arguments_complete:
    move.l d0, a4
    move.l d0, a3
    movea.l #board, a6
    movea.l #winprocs, a5

  _loop_again:
    clr.l d7
    moveq #0, d0
    jsr run_move

    moveq #1, d0
    jsr run_move

    moveq #4, d0
    jsr run_move

    subq.l #1, a4
    cmpa #0, a4
    bne _loop_again

    move.w d7, d0
    jsr show_num

    move.l #moves_string, d1
    move.l #9, d0
    trap #2

    move.l a3, d0
    jsr show_num

    move.l #iterations_string, d1
    move.l #9, d0
    trap #2

    clr.l d0                * exit code of 0
    rts

run_move:
    move.w d0, -(a7)
    move.b #piecex, (a6, d0)
    clr.l d5
    move.l d0, d6
    moveq.l #maxsc, d3
    moveq.l #minsc, d4
    jsr mm_min
    move.w (a7)+, d0
    clr.b (a6, d0)
    rts

mm_max:
    addq.l #1, d7

    cmpi.b #4, d5
    blt _max_no_winner_check

    move.l d6, d1
    lsl.l #2, d1
    move.l (a5, d1), a0
    jsr (a0)

    cmpi.b #pieceo, d0
    bne _max_no_winner_check
    moveq.l #losesc, d0
    rts

  _max_no_winner_check:
    movem.w d2/d4/d6, -(a7)
    moveq.l #minsc, d2
    addq.l #1, d5

    tst.b 0(a6)
    beq _xm_0

  _xt_1:
    tst.b 1(a6)
    beq _xm_1

  _xt_2:
    tst.b 2(a6)
    beq _xm_2

  _xt_3:
    tst.b 3(a6)
    beq _xm_3

  _xt_4:
    tst.b 4(a6)
    beq _xm_4

  _xt_5:
    tst.b 5(a6)
    beq _xm_5

  _xt_6:
    tst.b 6(a6)
    beq _xm_6

  _xt_7:
    tst.b 7(a6)
    beq _xm_7

  _xt_8:
    tst.b 8(a6)
    beq _xm_8
    bra _max_load_value_return

  _xm_0:
    moveq.l #0, d6
    move.b #piecex, (a6, d6)
    jsr mm_min
    clr.b (a6, d6)
    cmpi.b #winsc, d0
    beq _max_restore_value
    cmp.b d2, d0
    ble _xt_1
    cmp.b d3, d0
    bge _max_restore_value
    move.l d0, d2
    cmp.b d4, d2
    ble _xt_1
    move.l d2, d4
    bra _xt_1

  _xm_1:
    moveq.l #1, d6
    move.b #piecex, (a6, d6)
    jsr mm_min
    clr.b (a6, d6)
    cmpi.b #winsc, d0
    beq _max_restore_value
    cmp.b d2, d0
    ble _xt_2
    cmp.b d3, d0
    bge _max_restore_value
    move.l d0, d2
    cmp.b d4, d2
    ble _xt_2
    move.l d2, d4
    bra _xt_2

  _xm_2:
    moveq.l #2, d6
    move.b #piecex, (a6, d6)
    jsr mm_min
    clr.b (a6, d6)
    cmpi.b #winsc, d0
    beq _max_restore_value
    cmp.b d2, d0
    ble _xt_3
    cmp.b d3, d0
    bge _max_restore_value
    move.l d0, d2
    cmp.b d4, d2
    ble _xt_3
    move.l d2, d4
    bra _xt_3

  _xm_3:
    moveq.l #3, d6
    move.b #piecex, (a6, d6)
    jsr mm_min
    clr.b (a6, d6)
    cmpi.b #winsc, d0
    beq _max_restore_value
    cmp.b d2, d0
    ble _xt_4
    cmp.b d3, d0
    bge _max_restore_value
    move.l d0, d2
    cmp.b d4, d2
    ble _xt_4
    move.l d2, d4
    bra _xt_4

  _xm_4:
    moveq.l #4, d6
    move.b #piecex, (a6, d6)
    jsr mm_min
    clr.b (a6, d6)
    cmpi.b #winsc, d0
    beq _max_restore_value
    cmp.b d2, d0
    ble _xt_5
    cmp.b d3, d0
    bge _max_restore_value
    move.l d0, d2
    cmp.b d4, d2
    ble _xt_5
    move.l d2, d4
    bra _xt_5

  _xm_5:
    moveq.l #5, d6
    move.b #piecex, (a6, d6)
    jsr mm_min
    clr.b (a6, d6)
    cmpi.b #winsc, d0
    beq _max_restore_value
    cmp.b d2, d0
    ble _xt_6
    cmp.b d3, d0
    bge _max_restore_value
    move.l d0, d2
    cmp.b d4, d2
    ble _xt_6
    move.l d2, d4
    bra _xt_6

  _xm_6:
    moveq.l #6, d6
    move.b #piecex, (a6, d6)
    jsr mm_min
    clr.b (a6, d6)
    cmpi.b #winsc, d0
    beq _max_restore_value
    cmp.b d2, d0
    ble _xt_7
    cmp.b d3, d0
    bge _max_restore_value
    move.l d0, d2
    cmp.b d4, d2
    ble _xt_7
    move.l d2, d4
    bra _xt_7

  _xm_7:
    moveq.l #7, d6
    move.b #piecex, (a6, d6)
    jsr mm_min
    clr.b (a6, d6)
    cmpi.b #winsc, d0
    beq _max_restore_value
    cmp.b d2, d0
    ble _xt_8
    cmp.b d3, d0
    bge _max_restore_value
    move.l d0, d2
    cmp.b d4, d2
    ble _xt_8
    move.l d2, d4
    bra _xt_8

  _xm_8:
    moveq.l #8, d6
    move.b #piecex, (a6, d6)
    jsr mm_min
    clr.b (a6, d6)
    cmp.b d2, d0
    bge _max_restore_value

  _max_load_value_return:
    move.l d2, d0

  _max_restore_value:
    movem.w (a7)+, d6/d4/d2
    subq.l #1, d5
    rts

mm_min:
    addq.l #1, d7

    cmpi.b #4, d5
    blt _min_no_winner_check

    move.l d6, d1
    lsl.l #2, d1
    move.l (a5, d1), a0
    jsr (a0)

    cmpi.b #piecex, d0
    bne _min_not_x
    moveq.l #winsc, d0
    rts

  _min_not_x:
    cmpi.b #8, d5
    bne _min_no_winner_check
    moveq.l #tiesc, d0
    rts

  _min_no_winner_check:
    movem.w d2/d3/d6, -(a7)
    moveq.l #maxsc, d2
    addq.l #1, d5

    tst.b 0(a6)
    beq _nm_0

  _nt_1:
    tst.b 1(a6)
    beq _nm_1

  _nt_2:
    tst.b 2(a6)
    beq _nm_2

  _nt_3:
    tst.b 3(a6)
    beq _nm_3

  _nt_4:
    tst.b 4(a6)
    beq _nm_4

  _nt_5:
    tst.b 5(a6)
    beq _nm_5

  _nt_6:
    tst.b 6(a6)
    beq _nm_6

  _nt_7:
    tst.b 7(a6)
    beq _nm_7

  _nt_8:
    tst.b 8(a6)
    beq _nm_8
    bra _min_load_value_return

  _nm_0:
    moveq.l #0, d6
    move.b #pieceo, (a6, d6)
    jsr mm_max
    clr.b (a6, d6)
    cmpi.b #losesc, d0
    beq _min_restore_value
    cmp.b d2, d0
    bge _nt_1
    cmp.b d4, d0
    ble _min_restore_value
    move.l d0, d2
    cmp.b d3, d2
    bge _nt_1
    move.l d2, d3
    bra _nt_1

  _nm_1:
    moveq.l #1, d6
    move.b #pieceo, (a6, d6)
    jsr mm_max
    clr.b (a6, d6)
    cmpi.b #losesc, d0
    beq _min_restore_value
    cmp.b d2, d0
    bge _nt_2
    cmp.b d4, d0
    ble _min_restore_value
    move.l d0, d2
    cmp.b d3, d2
    bge _nt_2
    move.l d2, d3
    bra _nt_2

  _nm_2:
    moveq.l #2, d6
    move.b #pieceo, (a6, d6)
    jsr mm_max
    clr.b (a6, d6)
    cmpi.b #losesc, d0
    beq _min_restore_value
    cmp.b d2, d0
    bge _nt_3
    cmp.b d4, d0
    ble _min_restore_value
    move.l d0, d2
    cmp.b d3, d2
    bge _nt_3
    move.l d2, d3
    bra _nt_3

  _nm_3:
    moveq.l #3, d6
    move.b #pieceo, (a6, d6)
    jsr mm_max
    clr.b (a6, d6)
    cmpi.b #losesc, d0
    beq _min_restore_value
    cmp.b d2, d0
    bge _nt_4
    cmp.b d4, d0
    ble _min_restore_value
    move.l d0, d2
    cmp.b d3, d2
    bge _nt_4
    move.l d2, d3
    bra _nt_4

  _nm_4:
    moveq.l #4, d6
    move.b #pieceo, (a6, d6)
    jsr mm_max
    clr.b (a6, d6)
    cmpi.b #losesc, d0
    beq _min_restore_value
    cmp.b d2, d0
    bge _nt_5
    cmp.b d4, d0
    ble _min_restore_value
    move.l d0, d2
    cmp.b d3, d2
    bge _nt_5
    move.l d2, d3
    bra _nt_5

  _nm_5:
    moveq.l #5, d6
    move.b #pieceo, (a6, d6)
    jsr mm_max
    clr.b (a6, d6)
    cmpi.b #losesc, d0
    beq _min_restore_value
    cmp.b d2, d0
    bge _nt_6
    cmp.b d4, d0
    ble _min_restore_value
    move.l d0, d2
    cmp.b d3, d2
    bge _nt_6
    move.l d2, d3
    bra _nt_6

  _nm_6:
    moveq.l #6, d6
    move.b #pieceo, (a6, d6)
    jsr mm_max
    clr.b (a6, d6)
    cmpi.b #losesc, d0
    beq _min_restore_value
    cmp.b d2, d0
    bge _nt_7
    cmp.b d4, d0
    ble _min_restore_value
    move.l d0, d2
    cmp.b d3, d2
    bge _nt_7
    move.l d2, d3
    bra _nt_7

  _nm_7:
    moveq.l #7, d6
    move.b #pieceo, (a6, d6)
    jsr mm_max
    clr.b (a6, d6)
    cmpi.b #losesc, d0
    beq _min_restore_value
    cmp.b d2, d0
    bge _nt_8
    cmp.b d4, d0
    ble _min_restore_value
    move.l d0, d2
    cmp.b d3, d2
    bge _nt_8
    move.l d2, d3
    bra _nt_8

  _nm_8:
    moveq.l #8, d6
    move.b #pieceo, (a6, d6)
    jsr mm_max
    clr.b (a6, d6)
    cmp.b d2, d0
    ble _min_restore_value

  _min_load_value_return:
    move.l d2, d0

  _min_restore_value:
    movem.w (a7)+, d6/d3/d2
    subq.l #1, d5
    rts

proc0:
    move.b (a6), d0
    cmp.b 1(a6), d0
    bne _p0_n_a
    cmp.b 2(a6), d0
    bne _p0_n_a
    rts

  _p0_n_a:
    cmp.b 3(a6), d0
    bne _p0_n_b
    cmp.b 6(a6), d0
    bne _p0_n_b
    rts

  _p0_n_b:
    cmp.b 4(a6), d0
    bne _p0_r_0
    cmp.b 8(a6), d0
    bne _p0_r_0
    rts

  _p0_r_0:
    clr.b d0
    rts

proc1:
    move.b 1(a6), d0
    cmp.b (a6), d0
    bne _p1_n_a
    cmp.b 2(a6), d0
    bne _p1_n_a
    rts

  _p1_n_a:
    cmp.b 4(a6), d0
    bne _p1_r_0
    cmp.b 7(a6), d0
    bne _p1_r_0
    rts

  _p1_r_0:
    clr.b d0
    rts

proc2:
    move.b 2(a6), d0
    cmp.b (a6), d0
    bne _p2_n_a
    cmp.b 1(a6), d0
    bne _p2_n_a
    rts

  _p2_n_a:
    cmp.b 5(a6), d0
    bne _p2_n_b
    cmp.b 8(a6), d0
    bne _p2_n_b
    rts

  _p2_n_b:
    cmp.b 4(a6), d0
    bne _p2_r_0
    cmp.b 6(a6), d0
    bne _p2_r_0
    rts

  _p2_r_0:
    clr.b d0
    rts

proc3:
    move.b 3(a6), d0
    cmp.b (a6), d0
    bne _p3_n_a
    cmp.b 6(a6), d0
    bne _p3_n_a
    rts

  _p3_n_a:
    cmp.b 4(a6), d0
    bne _p3_r_0
    cmp.b 5(a6), d0
    bne _p3_r_0
    rts

  _p3_r_0:
    clr.b d0
    rts

proc4:
    move.b 4(a6), d0
    cmp.b (a6), d0
    bne _p4_n_a
    cmp.b 8(a6), d0
    bne _p4_n_a
    rts

  _p4_n_a:
    cmp.b 2(a6), d0
    bne _p4_n_b
    cmp.b 6(a6), d0
    bne _p4_n_b
    rts

  _p4_n_b:
    cmp.b 1(a6), d0
    bne _p4_n_c
    cmp.b 7(a6), d0

    bne _p4_n_c
    rts

  _p4_n_c:
    cmp.b 3(a6), d0
    bne _p4_r_0
    cmp.b 5(a6), d0
    bne _p4_r_0
    rts

  _p4_r_0:
    clr.b d0
    rts

proc5:
    move.b 5(a6), d0
    cmp.b 3(a6), d0
    bne _p5_n_a
    cmp.b 4(a6), d0
    bne _p5_n_a
    rts

  _p5_n_a:
    cmp.b 2(a6), d0
    bne _p5_r_0
    cmp.b 8(a6), d0
    bne _p5_r_0
    rts

  _p5_r_0:
    clr.b d0
    rts

proc6:
    move.b 6(a6), d0
    cmp.b 2(a6), d0
    bne _p6_n_a
    cmp.b 4(a6), d0
    bne _p6_n_a
    rts

  _p6_n_a:
    cmp.b (a6), d0
    bne _p6_n_b
    cmp.b 3(a6), d0
    bne _p6_n_b
    rts

  _p6_n_b:
    cmp.b 7(a6), d0
    bne _p6_r_0
    cmp.b 8(a6), d0
    bne _p6_r_0
    rts

  _p6_r_0:
    clr.b d0
    rts

proc7:
    move.b 7(a6), d0
    cmp.b 1(a6), d0
    bne _p7_n_a
    cmp.b 4(a6), d0
    bne _p7_n_a
    rts

  _p7_n_a:
    cmp.b 6(a6), d0
    bne _p7_r_0
    cmp.b 8(a6), d0
    bne _p7_r_0
    rts

  _p7_r_0:
    clr.b d0
    rts

proc8:
    move.b 8(a6), d0
    cmp.b (a6), d0
    bne _p8_n_a
    cmp.b 4(a6), d0
    bne _p8_n_a
    rts

  _p8_n_a:
    cmp.b 2(a6), d0
    bne _p8_n_b
    cmp.b 5(a6), d0
    bne _p8_n_b
    rts

  _p8_n_b:
    cmp.b 6(a6), d0
    bne _p8_r_0
    cmp.b 7(a6), d0
    bne _p8_r_0
    rts

  _p8_r_0:
    clr.b d0
    rts

atou:                       * converts asciiz string in a0 to an unsigned word in d0
    clr.l d0
  au_space:
    cmpi.b #32, (a0)
    bne au_next
    addi.l #1, a0
    bra au_space
  au_next:
    move.b (a0)+, d1
    beq au_done
    cmp.b #48, d1
    blt au_done
    cmp.b #57, d1
    bgt au_done
    mulu #10, d0
    sub.b #48, d1
    add.l d1, d0
    bra au_next
  au_done:
    rts

show_num:                   * print the unsigned number in d0.w
    movea.l #num_buf, a0

  next_digit:
    tst.w d0
    beq swap_digits
    andi.l #$ffff, d0
    divu #10, d0
    move.l d0, d1
    swap d1
    add.w #48, d1
    move.b d1, (a0)+
    bra next_digit

  swap_digits:
    movea.l #num_buf, a1
    move.b #'$', (a0)
    subq.l #1, a0

  next_swap:
    cmpa.l a1, a0
    ble show_string
    move.b (a0), d0
    move.b (a1), (a0)
    move.b d0, (a1)+
    subq.l #1, a0
    bra next_swap

  show_string:
    move.l #9, d0
    move.l #num_buf, d1
    trap #2
    rts

.data

.globl winprocs
winprocs:
  .dc.l proc0
  .dc.l proc1
  .dc.l proc2
  .dc.l proc3
  .dc.l proc4
  .dc.l proc5
  .dc.l proc6
  .dc.l proc7
  .dc.l proc8

* not sure where this is referenced. a typo somewhere?
t: .dc.b 0

board: .dc.b 0,0,0,0,0,0,0,0,0
moves_string: .dc.b ' moves',13,10,'$'
iterations_string: .dc.b ' iterations',13,10,'$'
num_buf: .dc.b '            '

piecex equ 1
pieceo equ 2
pieceb equ 0

winsc equ 6
tiesc equ 5
losesc equ 4
maxsc equ 9
minsc equ 2
defiters equ 1

.end

