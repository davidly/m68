# the bare minimum to start C apps that use syscall-less newlib on teh 68000
# per calling conventions, d0, d1, a0, and a1 are scratch. others must be preserved.

.text
  .global  _start
  .type    _start, @function
_start:
        jsr __libc_init_array

#        pea __EH_FRAME_BEGIN__
#        jsr __register_frame
#        adda.l 4, %a7

        # get argc, argv, and env onto the stack as arguments for main
        # the stack has argc, a 0-terminated array of args, and a 0-terminated array of environment variables
        # main() needs argc and points to each array pushed onto the stack below that data

        move.l (%a7), %d0           /* put argc in d0 */
        move.l %a7, %d1
        addi.l #4, %d1              /* d1 now points to argv array */
        move.l %d1, %d2
        move.l %d0, %d3
        addi.l #1, %d3              /* +1 to get past the null final entry in argv */
        lsl #2, %d3                 /* multiply by 4 bytes each */
        add.l %d3, %d2              /* d2 now points to the env array */

        move.l %d2, -(%a7)          /* push the 3 arguments on the stack */
        move.l %d1, -(%a7)
        move.l %d0, -(%a7)

        jsr main
        bra exit_emulator

  .global _init
  .type _init, @function
_init:
        # call C and C++ initialization functions (this happens for free in newlib)
    rts

  .global exit_emulator
  .type exit_emulator, @function
exit_emulator:
        move.l %d0, %d1             /*  put app exit code in 1st syscall argument register */
        move.l #93, %d0             /*  linux exit function */
        trap #0
  busy_loop:
        bra busy_loop

  .global syscall1
  .type syscall1, @function
syscall1:
    move.l 4(%a7), %d0              /* linux syscall ID */
    move.l 8(%a7), %d1              /* the one and only argument */
    trap #0
    rts

  .global syscall2
  .type syscall2, @function
syscall2:
    move.l %d2, -(%a7)
    move.l 8(%a7), %d0              /* linux syscall ID */
    move.l 12(%a7), %d1             /* the first argument */
    move.l 16(%a7), %d2             /* the second argument */
    trap #0
    move.l (%a7)+, %d2
    rts

  .global syscall3
  .type syscall3, @function
syscall3:
    movem.l %d2/%d3, -(%a7)
    move.l 12(%a7), %d0             /* linux syscall ID */
    move.l 16(%a7), %d1             /* the first argument */
    move.l 20(%a7), %d2             /* the second argument */
    move.l 24(%a7), %d3             /* the third argument */
    trap #0
    movem.l (%a7)+, %d3/%d2
    rts

  .global syscall4
  .type syscall4, @function
syscall4:
    movem.l %d2/%d3/%d4, -(%a7)
    move.l 16(%a7), %d0             /* linux syscall ID */
    move.l 20(%a7), %d1             /* the first argument */
    move.l 24(%a7), %d2             /* the second argument */
    move.l 28(%a7), %d3             /* the third argument */
    trap #0
    movem.l (%a7)+, %d4/%d3/%d2
    rts

  .global syscall5
  .type syscall5, @function
syscall5:
    movem.l %d2/%d3/%d4/%d5, -(%a7)
    move.l 20(%a7), %d0             /* linux syscall ID */
    move.l 24(%a7), %d1             /* the first argument */
    move.l 28(%a7), %d2             /* the second argument */
    move.l 32(%a7), %d3             /* the third argument */
    move.l 36(%a7), %d4             /* the fourth argument */
    move.l 40(%a7), %d5             /* the fifth argument */
    trap #0
    movem.l (%a7)+, %d5/%d4/%d3/%d2
    rts

  .global syscall6
  .type syscall6, @function
syscall6:
    movem.l %d2/%d3/%d4/%d5/%d6, -(%a7)
    move.l 24(%a7), %d0             /* linux syscall ID */
    move.l 28(%a7), %d1             /* the first argument */
    move.l 32(%a7), %d2             /* the second argument */
    move.l 36(%a7), %d3             /* the third argument */
    move.l 40(%a7), %d4             /* the fourth argument */
    move.l 44(%a7), %d5             /* the fifth argument */
    move.l 48(%a7), %d6             /* the sixth argument */
    trap #0
    movem.l (%a7)+, %d6/%d5/%d4/%d3/%d2
    rts


