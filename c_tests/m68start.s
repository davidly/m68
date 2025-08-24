# the bare minimum to start C apps that use syscall-less newlib on the 68000
# per calling conventions, d0, d1, a0, and a1 are scratch. others must be preserved.

.equ AT_EH_FRAME_BEGIN, 0x69690069

.text
  .global  _start
  .type    _start, @function
_start:
    move.l %a7, (g_initial_sp)  /* save the original stack address so EHstart can crawl it */
    jsr __libc_init_array

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
    move.l %d2, environ         /* update C global environment pointer */

    move.l %d2, -(%a7)          /* push the 3 arguments on the stack */
    move.l %d1, -(%a7)
    move.l %d0, -(%a7)

    jsr main
    adda.l #12, %a7
    move.l %d0, %d7             /* save app exit code */

    jsr __libc_fini_array

    move.l %d7, -(%a7)          /* push the exit code */
    jsr exit_emulator

  .global _init
  .type _init, @function
_init:
    # called by __libc_init_array at app startup
    # make sure C++ exception objects are registered so unwind can find them.

    move.l #AT_EH_FRAME_BEGIN, %d0
    move.l %d0, -(%a7)
    jsr getauxval
    adda.l #4, %a7
    tst %d0
    beq _init_done              /* any C++ exception frames to register? */
    move.l %d0, -(%a7)
    jsr __register_frame
    adda.l #4, %a7
  _init_done:
    rts

  .global _fini
  .type _fini, @function
_fini:
    # called by __libc_fini_array at clean app shutdown
    move.l #AT_EH_FRAME_BEGIN, %d0
    move.l %d0, -(%a7)
    jsr getauxval
    adda.l #4, %a7
    tst %d0
    beq _fini_done              /* any C++ exception frames to deregister? */
    move.l %d0, -(%a7)
    jsr __deregister_frame
    adda.l #4, %a7
  _fini_done:
    rts

  .global exit_emulator
  .type exit_emulator, @function
exit_emulator:
    move.l 4(%a7), %d1          /* put app exit code in 1st syscall argument register */
    move.l #93, %d0             /* linux exit function */
    trap #0                     /* no coming back from this */ 

  .global syscall
  .type syscall, @function
syscall:
    movem.l %d2/%d3/%d4/%d5/%d6/%a6, -(%a7)     /* save these registers. d0 and d1 aren't preserved */
    lea 28(%a7), %a6                            /* point a6 at the syscall ID and arguments. 28 = 4 * ( 6 saved registers + 1 return address ) */
    movem.l (%a6)+, %d0/%d1/%d2/%d3/%d4/%d5/%d6 /* put the syscall ID and arguments in regs d0..d6 */
    trap #0                                     /* linux syscall */
    cmp.l #-4095, %d0                           /* this really can be one compare then check the carry flag... */
    jcs sc_just_return                          /* check for an error. if ( ( result < 0 ) && ( result > -4096 ) ) */
    move.l %d0, %d1                             /* there was an error, so update errno and return -1 */
    jsr __errno                                 /* the address to errno is now in d0 */
    move.l %d0, %a0
    neg.l %d1
    move.l %d1, (%a0)                           /* errno = -result; */
    moveq #-1, %d0                              /* return -1 */
  sc_just_return:
    movem.l (%a7)+, %a6/%d6/%d5/%d4/%d3/%d2     /* restore saved registers */
    rts

