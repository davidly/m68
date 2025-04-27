* ===============================================================
* File - CPMINI.S
* ===============================================================
* Created       : 12 Jan 1984 by : Clint H. Woeltjen
* Last Modified : 12 Jan 1984 by : Clint H. Woeltjen
* ===============================================================

    .globl    put       * external
    .globl    get       * external
    .globl    reset     * external
    .globl    rewrite   * external
    .globl    assign    * external
    .globl    _bas      * bss
    .globl    _siz      * bss
    .globl    _bpg      * bss
    .globl    output    * bss
    .globl    input     * bss
    .globl    _frl      * bss
    .globl    _tfn      * bss
    .globl    _tom      * bss
    .globl    _sysin    * bss
    .globl    _sysou    * bss
    .globl    _ini      * text
*
    .bss
    .even
*
_tom:
    .ds.l    1
_siz:
    .ds.l    1
_bas:
    .ds.l    1
output:
    .ds.b    64
input:
    .ds.b    64
_bpg:
    .ds.l    1
_frl:
    .ds.l    1
_tfn:
    .ds.l    1
_sysin:
    .ds.l    1
_sysou:
    .ds.l    1
extrabase:
    .ds.l    1
extralen:
    .ds.l    1
frlfirst:
    .ds.l    1
fprev:
    .ds.l    1
fsiz:
    .ds.l    1
frllast:
    .ds.l    1
lprev:
    .ds.l    1
lsiz:
    .ds.l    1
junk:
    .ds.l    1
temp:                 * start of bss
    .ds.l    1
xlen:
    .ds.l    1
ylen:
    .ds.l    1
*
    .text
*
_ini:
    move.l      8(a7),a1            * Address of base page
    move.l      24(a1),a0           * Start of bss
    move.l      28(a1),d0           * length of bss
    subq.l      #1,d0               * for dbra
inilp:
    clr.b       (a0)+
    dbra        d0,inilp            * clear bss
    move.l      a1,_bpg             * Save for everyone else
    move.l      24(a1),a0           * Start of bss
    move.l      28(a1),d7           * length of bss
    adda.l      d7,a0               * start of free memory
    move.l      a0,_bas             * save as base of free memory
    move.l      32(a1),d0           * length of free memory
    subq.l      #8,d0               * to be safe
    move.l      a0,a2               * base of free memory
    adda.l      d0,a2               * top of free memory
    move.l      a2,a6               * lexical stack pointer
    subi.l      #1536,d0            * new length of free memory
    suba.l      #512,a2             * size of lexical stack
    move.l      (a7)+,a3            * return address
    move.l      a7,a4               * stack pointer
    move.l      a2,a7               * new stack pointer
    suba.l      #1024,a2            * size of stack
    move.l      a2,_tom             * top of memory
    move.l      d0,_siz             * save size of free memory
    move.l      d0,32(a1)           * new length of free memory
    move.l      a4,-(a7)            * push old stack pointer
    move.l      a3,-(a7)            * push return address
    move.l      _bpg,temp           * temp := basepage
    addi.l      #24,temp            * temp := temp + 24
    move.l      temp,a4             * extrabase := temp^
    move.l      (a4),d0
    addq.l      #4,a4               * get length of bss
    add.l       (a4),d0             * point to start of free memory
    move.l      d0,junk             * junk := extrabase
    addq.l      #4,d0               * extrabase := extrabase + 4
    move.l      d0,extrabase
    addq.l      #8,temp             * temp := temp + 8
    move.l      temp,a4             * length of free memory
    move.l      (a4),extralen       * extralen := temp^
    move.l      #2,fprev            * frlfirst.prv := nil
    clr.l       fsiz                * frlfirst.siz := 0
    move.l      junk,frlfirst       * frlfirst.nxt := junk
    move.l      frlfirst,a1
    move.l      #frllast,(a1)       * frlfirst.nxt^.nxt := addr(frllast)
    addq.l      #4,a1
    move.l      #frlfirst,(a1)      * frlfirst.nxt^.prv := addr(frlfirst)
    addq.l      #4,a1
    move.l      extralen,(a1)       * frlfirst.nxt^.siz := 0
    move.l      #2,frllast          * frllast.nxt := nil
    clr.l       lsiz                * frllast.siz := 0
    move.l      junk,lprev          * frllast.prv := junk
    move.l      #frlfirst,_frl      * _frl := addr(frlfirst)
    move.l      #$13030,_tfn        * _tfn := '00'
    move.l      #get,_sysin         * _sysin := addr(get)
    move.l      #put,_sysou         * _sysou := addr(put)
    move.l      #input,-(a7)
    move.w      #1,-(a7)
    bsr.b       dummy1
    .dc.b       4,'CON:',0
dummy1:
    jsr         assign               * assign(input,'CON:')
    move.l      #input,-(a7)
    move.w      #-1,-(a7)
    jsr         reset                * reset(input)
    move.l      #output,-(a7)
    move.w      #1,-(a7)
    bsr.b       dummy2
    .dc.b       4,'CON:',0
dummy2:
    jsr         assign               * assign(output,'CON:')
    move.l      #output,-(a7)
    move.w      #-1,-(a7)
    jsr         rewrite              * rewrite(output)
    rts
*
    .end
