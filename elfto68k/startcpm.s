# The bare minimum to start C apps that use syscall-less newlib on the 68000.
# Per calling conventions, d0, d1, a0, and a1 are scratch. others must be preserved.

.text
.global _start
.type _start, @function
_start:
    move.l 4(%a7), (g_base_page)
    move.l %d1, (g_eh_data)
    move.l %d0, (arg0_value)
    jsr __libc_init_array       /* initialize the C runtime */

    clr.l %d4                   /* argc */
    move.l #argvals, %a5        /* argv */
    clr.l %d6                   /* envp */

    move.l #1, %d4              /* cp/m 68k has no way of knowing the app name! */
    move.l (arg0_value).l, %a0  /* ...but elfto68k writes the name to d0 */
    move.l %a0, (%a5)
    move.l %a5, %a4
    adda.l #4, %a4              /* a4 points to the next argv to write */

    move.l 4(%a7), %a0          /* load the base page pointer */
    adda.l #128, %a0            /* point to command tail len */
    clr.l %d3
    move.b (%a0), %d3           /* tail length in d3 */

    adda.l #1, %a0              /* make a copy of the command tail because it'll get reused as the DMA later */
    move.l %d3, -(%a7)
    move.l %a0, -(%a7)
    move.l #cmdtail_copy, -(%a7)
    jsr memcpy
    adda.l #12, %a7

    move.l #cmdtail_copy, %a0
    clr.l %d0                   /* offset into the command tail */
  _next_argument:               /* copy a pointer to each argument to the argvals array */
    cmp.l %d0, %d3
    beq _call_main
    addi.l #1, %d4
    move.l %a0, (%a4)
    adda.l #4, %a4              /* a4 points to the next argv to write */
  _next_sp_check:
    cmpi.b #32, (%a0)
    bne _notspace
    clr.b (%a0)
    adda.l #1, %a0
    addi.l #1, %d0
    bra _next_argument
  _notspace:
    adda.l #1, %a0
    addi.l #1, %d0
    cmp.l %d0, %d3              /* at the end of the string? */
    beq _call_main
    bra _next_sp_check

  _call_main:
    move.l %d6, -(%a7)          /* push main()'s 3 arguments on the stack */
    move.l %a5, -(%a7)
    move.l %d4, -(%a7)
    jsr main
    adda.l #12, %a7
    move.l %d0, %d7             /* save app exit code */

    jsr __libc_fini_array       /* ask the C runtime to shut down */

    move.l %d7, -(%a7)          /* push the exit code */
    jsr exit_cpm

.global _init
.type _init, @function
_init:
    # called by __libc_init_array at app startup.
    # make sure C++ exception objects are registered so unwind can find them.
    # _init_nlcpm also initializes brk and other global data required by nlcpm.c
    jsr _init_nlcpm
    rts

.global _fini
.type _fini, @function
_fini:
    # called by __libc_fini_array at clean app shutdown
    jsr _fini_nlcpm
    rts

.global exit_cpm
.type exit_cpm, @function
exit_cpm:
    move.l 4(%a7), %d1          /* put app exit code in 1st syscall argument register */
    clr.l %d0                   /* the cp/m exit function is 0 */
    trap #2                     /* no coming back from this */

.global bdos_cpm
.type bdos_cpm, @function
bdos_cpm:
    move.l 4(%a7), %d0          /* cp/m syscall ID */
    move.l 8(%a7), %d1          /* the one and only argument */
    trap #2                     /* no errno-style errors are returned; layer above must set errno */
    rts

.data
.align 4
argvals: .zero 64 * 4           /* array of pointers to command line arguments */
cmdtail_copy: .zero 128         /* a copy of the command tail because it gets overwritten */
arg0_value: .zero 4             /* temporary place to store the app name for argv[0] */

