* ===============================================================
* File - HLT.S - 68000 MT+ to BDOS Interface
* ===============================================================
* Created       : 10 Oct 1983 by : Clint H. Woeltjen
* Last Modified : 10 Oct 1983 by : Clint H. Woeltjen
* ===============================================================
        .globl    _bdos
        .globl    _hlt
        .text
_hlt:
        clr.w     d0               * System Reset
        trap      #2               * Does not return
_bdos:
        move.l    (a7)+,a1         * pop return address
        move.l    (a7)+,a2         * pointer to parameter
        move.w    (a7)+,d0         * function number
        cmpi.w    #8,d0            * Check type of function
        ble       bdossmall        * Need what is pointed to
        cmpi.w    #14,d0           * Select Disk function
        beq       bdossmall        * Don't need pointer
        cmpi.w    #32,d0           * Set User Code
        beq       bdossmall        * Don't need pointer
        cmpi.w    #37,d0           * Reset Drive
        beq       bdossmall        * Don't need pointer
        cmpi.w    #46,d0           * Get Disk Free Space
        beq       bdossmall        * Don't need pointer
        cmpi.w    #14,d0           * Select Disk function
        beq       bdossmall        * Don't need pointer
        move.l    a2,d1            * Move pointer into position
        bra       dobdos           * Don't need pointer
bdossmall:
        clr.w     d1
        move.b    (a2),d1          * Get parameter
dobdos:
        move.l    a1,-(a7)         * Push Return Address
        trap      #2               * Call BDOS
        move.w    d0,d7            * Returned parameter
        rts
*
        .end
