* 68000 for cp/m 68k assembler version of app to prove you can't win at tic-tac-toe
* u is for unrolled for loops
.data

* not sure where this is referenced. a typo somewhere?
t: .dc.b 0

board: .dc.b 0,0,0,0,0,0,0,0,0
moves_string: .dc.b 'moves: %lu',10,0
failure_string: .dc.b 'result isn't a tie',10,0

.globl winprocs
winprocs:
.ds 36

piecex equ 1
pieceo equ 2
pieceb equ 0

winsc equ 6
tiesc equ 5
losesc equ 4
maxsc equ 9
minsc equ 2
defiters equ 10

.text

mm_max:
    addq.l #1, r7

    cmpi.b #4, r5
    blt _max_no_winner_check

    move.l r6, r1
    lsl.l #2, r1
    move.l (r13, r1), r8
    jsr (r8)

    cmpi.b #pieceo, r0
    bne _max_no_winner_check
    moveq.l #losesc, r0
    rts

  _max_no_winner_check:
    move.w r6, -(sp)
    move.w r2, -(sp)
    move.w r4, -(sp)
    moveq.l #minsc, r2
    addq.l #1, r5

    tst.b 0(r14)
    beq _xm_0

  _xt_1:
    tst.b 1(r14)
    beq _xm_1

  _xt_2:
    tst.b 2(r14)
    beq _xm_2

  _xt_3:
    tst.b 3(r14)
    beq _xm_3

  _xt_4:
    tst.b 4(r14)
    beq _xm_4

  _xt_5:
    tst.b 5(r14)
    beq _xm_5

  _xt_6:
    tst.b 6(r14)
    beq _xm_6

  _xt_7:
    tst.b 7(r14)
    beq _xm_7

  _xt_8:
    tst.b 8(r14)
    beq _xm_8
    bra _max_load_value_return

  _xm_0:
    moveq.l #0, r6
    move.b #piecex, (r14, r6)
    jsr mm_min
    clr.b (r14, r6)
    cmpi.b #winsc, r0
    beq _max_restore_value
    cmp.b r2, r0
    ble _xt_1
    cmp.b r3, r0
    bge _max_restore_value
    move.l r0, r2
    cmp.b r4, r2
    ble _xt_1
    move.l r2, r4
    bra _xt_1

  _xm_1:
    moveq.l #1, r6
    move.b #piecex, (r14, r6)
    jsr mm_min
    clr.b (r14, r6)
    cmpi.b #winsc, r0
    beq _max_restore_value
    cmp.b r2, r0
    ble _xt_2
    cmp.b r3, r0
    bge _max_restore_value
    move.l r0, r2
    cmp.b r4, r2
    ble _xt_2
    move.l r2, r4
    bra _xt_2

  _xm_2:
    moveq.l #2, r6
    move.b #piecex, (r14, r6)
    jsr mm_min
    clr.b (r14, r6)
    cmpi.b #winsc, r0
    beq _max_restore_value
    cmp.b r2, r0
    ble _xt_3
    cmp.b r3, r0
    bge _max_restore_value
    move.l r0, r2
    cmp.b r4, r2
    ble _xt_3
    move.l r2, r4
    bra _xt_3

  _xm_3:
    moveq.l #3, r6
    move.b #piecex, (r14, r6)
    jsr mm_min
    clr.b (r14, r6)
    cmpi.b #winsc, r0
    beq _max_restore_value
    cmp.b r2, r0
    ble _xt_4
    cmp.b r3, r0
    bge _max_restore_value
    move.l r0, r2
    cmp.b r4, r2
    ble _xt_4
    move.l r2, r4
    bra _xt_4

  _xm_4:
    moveq.l #4, r6
    move.b #piecex, (r14, r6)
    jsr mm_min
    clr.b (r14, r6)
    cmpi.b #winsc, r0
    beq _max_restore_value
    cmp.b r2, r0
    ble _xt_5
    cmp.b r3, r0
    bge _max_restore_value
    move.l r0, r2
    cmp.b r4, r2
    ble _xt_5
    move.l r2, r4
    bra _xt_5

  _xm_5:
    moveq.l #5, r6
    move.b #piecex, (r14, r6)
    jsr mm_min
    clr.b (r14, r6)
    cmpi.b #winsc, r0
    beq _max_restore_value
    cmp.b r2, r0
    ble _xt_6
    cmp.b r3, r0
    bge _max_restore_value
    move.l r0, r2
    cmp.b r4, r2
    ble _xt_6
    move.l r2, r4
    bra _xt_6

  _xm_6:
    moveq.l #6, r6
    move.b #piecex, (r14, r6)
    jsr mm_min
    clr.b (r14, r6)
    cmpi.b #winsc, r0
    beq _max_restore_value
    cmp.b r2, r0
    ble _xt_7
    cmp.b r3, r0
    bge _max_restore_value
    move.l r0, r2
    cmp.b r4, r2
    ble _xt_7
    move.l r2, r4
    bra _xt_7

  _xm_7:
    moveq.l #7, r6
    move.b #piecex, (r14, r6)
    jsr mm_min
    clr.b (r14, r6)
    cmpi.b #winsc, r0
    beq _max_restore_value
    cmp.b r2, r0
    ble _xt_8
    cmp.b r3, r0
    bge _max_restore_value
    move.l r0, r2
    cmp.b r4, r2
    ble _xt_8
    move.l r2, r4
    bra _xt_8

  _xm_8:
    moveq.l #8, r6
    move.b #piecex, (r14, r6)
    jsr mm_min
    clr.b (r14, r6)
    cmp.b r2, r0
    bge _max_restore_value

  _max_load_value_return:
    move.l r2, r0

  _max_restore_value:
    move.w (sp)+, r4
    move.w (sp)+, r2
    move.w (sp)+, r6
    subq.l #1, r5
    rts

mm_min:
    addq.l #1, r7

    cmpi.b #4, r5
    blt _min_no_winner_check

    move.l r6, r1
    lsl.l #2, r1
    move.l (r13, r1), r8
    jsr (r8)

    cmpi.b #piecex, r0
    bne _min_not_x
    moveq.l #winsc, r0
    rts

  _min_not_x:
    cmpi.b #8, r5
    bne _min_no_winner_check
    moveq.l #tiesc, r0
    rts

  _min_no_winner_check:
    move.w r6, -(sp)
    move.w r2, -(sp)
    move.w r3, -(sp)
    moveq.l #maxsc, r2
    addq.l #1, r5

    tst.b 0(r14)
    beq _nm_0

  _nt_1:
    tst.b 1(r14)
    beq _nm_1

  _nt_2:
    tst.b 2(r14)
    beq _nm_2

  _nt_3:
    tst.b 3(r14)
    beq _nm_3

  _nt_4:
    tst.b 4(r14)
    beq _nm_4

  _nt_5:
    tst.b 5(r14)
    beq _nm_5

  _nt_6:
    tst.b 6(r14)
    beq _nm_6

  _nt_7:
    tst.b 7(r14)
    beq _nm_7

  _nt_8:
    tst.b 8(r14)
    beq _nm_8
    bra _min_load_value_return

  _nm_0:
    moveq.l #0, r6
    move.b #pieceo, (r14, r6)
    jsr mm_max
    clr.b (r14, r6)
    cmpi.b #losesc, r0
    beq _min_restore_value
    cmp.b r2, r0
    bge _nt_1
    cmp.b r4, r0
    ble _min_restore_value
    move.l r0, r2
    cmp.b r3, r2
    bge _nt_1
    move.l r2, r3
    bra _nt_1

  _nm_1:
    moveq.l #1, r6
    move.b #pieceo, (r14, r6)
    jsr mm_max
    clr.b (r14, r6)
    cmpi.b #losesc, r0
    beq _min_restore_value
    cmp.b r2, r0
    bge _nt_2
    cmp.b r4, r0
    ble _min_restore_value
    move.l r0, r2
    cmp.b r3, r2
    bge _nt_2
    move.l r2, r3
    bra _nt_2

  _nm_2:
    moveq.l #2, r6
    move.b #pieceo, (r14, r6)
    jsr mm_max
    clr.b (r14, r6)
    cmpi.b #losesc, r0
    beq _min_restore_value
    cmp.b r2, r0
    bge _nt_3
    cmp.b r4, r0
    ble _min_restore_value
    move.l r0, r2
    cmp.b r3, r2
    bge _nt_3
    move.l r2, r3
    bra _nt_3

  _nm_3:
    moveq.l #3, r6
    move.b #pieceo, (r14, r6)
    jsr mm_max
    clr.b (r14, r6)
    cmpi.b #losesc, r0
    beq _min_restore_value
    cmp.b r2, r0
    bge _nt_4
    cmp.b r4, r0
    ble _min_restore_value
    move.l r0, r2
    cmp.b r3, r2
    bge _nt_4
    move.l r2, r3
    bra _nt_4

  _nm_4:
    moveq.l #4, r6
    move.b #pieceo, (r14, r6)
    jsr mm_max
    clr.b (r14, r6)
    cmpi.b #losesc, r0
    beq _min_restore_value
    cmp.b r2, r0
    bge _nt_5
    cmp.b r4, r0
    ble _min_restore_value
    move.l r0, r2
    cmp.b r3, r2
    bge _nt_5
    move.l r2, r3
    bra _nt_5

  _nm_5:
    moveq.l #5, r6
    move.b #pieceo, (r14, r6)
    jsr mm_max
    clr.b (r14, r6)
    cmpi.b #losesc, r0
    beq _min_restore_value
    cmp.b r2, r0
    bge _nt_6
    cmp.b r4, r0
    ble _min_restore_value
    move.l r0, r2
    cmp.b r3, r2
    bge _nt_6
    move.l r2, r3
    bra _nt_6

  _nm_6:
    moveq.l #6, r6
    move.b #pieceo, (r14, r6)
    jsr mm_max
    clr.b (r14, r6)
    cmpi.b #losesc, r0
    beq _min_restore_value
    cmp.b r2, r0
    bge _nt_7
    cmp.b r4, r0
    ble _min_restore_value
    move.l r0, r2
    cmp.b r3, r2
    bge _nt_7
    move.l r2, r3
    bra _nt_7

  _nm_7:
    moveq.l #7, r6
    move.b #pieceo, (r14, r6)
    jsr mm_max
    clr.b (r14, r6)
    cmpi.b #losesc, r0
    beq _min_restore_value
    cmp.b r2, r0
    bge _nt_8
    cmp.b r4, r0
    ble _min_restore_value
    move.l r0, r2
    cmp.b r3, r2
    bge _nt_8
    move.l r2, r3
    bra _nt_8

  _nm_8:
    moveq.l #8, r6
    move.b #pieceo, (r14, r6)
    jsr mm_max
    clr.b (r14, r6)
    cmp.b r2, r0
    ble _min_restore_value

  _min_load_value_return:
    move.l r2, r0

  _min_restore_value:
    move.w (sp)+, r3
    move.w (sp)+, r2
    move.w (sp)+, r6
    subq.l #1, r5
    rts

proc0:
    move.b (r14), r0
    cmp.b 1(r14), r0
    bne _p0_n_a
    cmp.b 2(r14), r0
    bne _p0_n_a
    rts

  _p0_n_a:
    cmp.b 3(r14), r0
    bne _p0_n_b
    cmp.b 6(r14), r0
    bne _p0_n_b
    rts

  _p0_n_b:
    cmp.b 4(r14), r0
    bne _p0_r_0
    cmp.b 8(r14), r0
    bne _p0_r_0
    rts

  _p0_r_0:
    clr.b r0
    rts

proc1:
    move.b 1(r14), r0
    cmp.b (r14), r0
    bne _p1_n_a
    cmp.b 2(r14), r0
    bne _p1_n_a
    rts

  _p1_n_a:
    cmp.b 4(r14), r0
    bne _p1_r_0
    cmp.b 7(r14), r0
    bne _p1_r_0
    rts

  _p1_r_0:
    clr.b r0
    rts

proc2:
    move.b 2(r14), r0
    cmp.b (r14), r0
    bne _p2_n_a
    cmp.b 1(r14), r0
    bne _p2_n_a
    rts

  _p2_n_a:
    cmp.b 5(r14), r0
    bne _p2_n_b
    cmp.b 8(r14), r0
    bne _p2_n_b
    rts

  _p2_n_b:
    cmp.b 4(r14), r0
    bne _p2_r_0
    cmp.b 6(r14), r0
    bne _p2_r_0
    rts

  _p2_r_0:
    clr.b r0
    rts

proc3:
    move.b 3(r14), r0
    cmp.b (r14), r0
    bne _p3_n_a
    cmp.b 6(r14), r0
    bne _p3_n_a
    rts

  _p3_n_a:
    cmp.b 4(r14), r0
    bne _p3_r_0
    cmp.b 5(r14), r0
    bne _p3_r_0
    rts

  _p3_r_0:
    clr.b r0
    rts

proc4:
    move.b 4(r14), r0
    cmp.b (r14), r0
    bne _p4_n_a
    cmp.b 8(r14), r0
    bne _p4_n_a
    rts

  _p4_n_a:
    cmp.b 2(r14), r0
    bne _p4_n_b
    cmp.b 6(r14), r0
    bne _p4_n_b
    rts

  _p4_n_b:
    cmp.b 1(r14), r0
    bne _p4_n_c
    cmp.b 7(r14), r0

    bne _p4_n_c
    rts

  _p4_n_c:
    cmp.b 3(r14), r0
    bne _p4_r_0
    cmp.b 5(r14), r0
    bne _p4_r_0
    rts

  _p4_r_0:
    clr.b r0
    rts

proc5:
    move.b 5(r14), r0
    cmp.b 3(r14), r0
    bne _p5_n_a
    cmp.b 4(r14), r0
    bne _p5_n_a
    rts

  _p5_n_a:
    cmp.b 2(r14), r0
    bne _p5_r_0
    cmp.b 8(r14), r0
    bne _p5_r_0
    rts

  _p5_r_0:
    clr.b r0
    rts

proc6:
    move.b 6(r14), r0
    cmp.b 2(r14), r0
    bne _p6_n_a
    cmp.b 4(r14), r0
    bne _p6_n_a
    rts

  _p6_n_a:
    cmp.b (r14), r0
    bne _p6_n_b
    cmp.b 3(r14), r0
    bne _p6_n_b
    rts

  _p6_n_b:
    cmp.b 7(r14), r0
    bne _p6_r_0
    cmp.b 8(r14), r0
    bne _p6_r_0
    rts

  _p6_r_0:
    clr.b r0
    rts

proc7:
    move.b 7(r14), r0
    cmp.b 1(r14), r0
    bne _p7_n_a
    cmp.b 4(r14), r0
    bne _p7_n_a
    rts

  _p7_n_a:
    cmp.b 6(r14), r0
    bne _p7_r_0
    cmp.b 8(r14), r0
    bne _p7_r_0
    rts

  _p7_r_0:
    clr.b r0
    rts

proc8:
    move.b 8(r14), r0
    cmp.b (r14), r0
    bne _p8_n_a
    cmp.b 4(r14), r0
    bne _p8_n_a
    rts

  _p8_n_a:
    cmp.b 2(r14), r0
    bne _p8_n_b
    cmp.b 5(r14), r0
    bne _p8_n_b
    rts

  _p8_n_b:
    cmp.b 6(r14), r0
    bne _p8_r_0
    cmp.b 7(r14), r0
    bne _p8_r_0
    rts

  _p8_r_0:
    clr.b r0
    rts

run_move:
    move.w r0, -(sp)
    move.b #piecex, (r14, r0)
    clr.l r5
    move.l r0, r6
    moveq.l #maxsc, r3
    moveq.l #minsc, r4
    jsr mm_min

    cmpi #tiesc, r0
    beq _run_move_ok
    pea failure_string
    jsr _printf
    adda #4, sp

  _run_move_ok:
    move.w (sp)+, r0
    clr.b (r14, r0)
    rts


.globl _main
.text
_main:
    move.l #proc0,winprocs
    move.l #proc1,4+winprocs
    move.l #proc2,8+winprocs
    move.l #proc3,12+winprocs
    move.l #proc4,16+winprocs
    move.l #proc5,20+winprocs
    move.l #proc6,24+winprocs
    move.l #proc7,28+winprocs
    move.l #proc8,32+winprocs

    move.l #defiters, a4
    cmpi.w #1, 4(a7)
    beq _arguments_complete
    move.l 6(a7), a0
    move.l 4(a0), d0
    move.l d0, -(a7)
    jsr _atoi
    adda #4, a7
    tst.l d0
    beq _use_default
    move.l d0, a4
    bra _arguments_complete

  _use_default:
    move.l #defiters, a4

  _arguments_complete:
    clr.l r7
    movea.l #board, r14
    movea.l #winprocs, r13

  _loop_again:
    moveq #0, r0
    jsr run_move

    moveq #1, r0
    jsr run_move

    moveq #4, r0
    jsr run_move

    subq.l #1, a4
    cmpa #0, a4
    bne _loop_again

    move.l r7, -(sp)
    pea moves_string
    jsr _printf
    adda #8, sp

    clr.l d0         * exit code of 0
    rts

    .end

