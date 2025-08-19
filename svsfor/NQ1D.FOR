C fortran 77 version.
C recursion doesn't work, so simulate it using go to
C b -- the board
C c -- current column
C r -- current row
C s -- solution count
C n -- current number of columns to solve for
C rstack -- stack to save current r when recursing
C safe -- returns if a board position is safe
C lr and lc -- local r and c

C nqueens is too long for svs fortran, so use nqueen as program name
      program nqueen
      integer*1 b(12), rstack(12)
      integer*1 n, c, r, safe, lc, lr
      integer*2 s
      common /area/ b,rstack,n,c,r,safe,lc,lr,s

      write( 1, 1 )
   1  format( ' size     solutions' )

      do 4 r = 0, 10, 1
          b(r) = -1
   4  continue

      n = 1
   5  if ( n .gt. 8 ) go to 30
          c = 0
          s = 0
          call solve
          write( 1, 10 ) n, s
  10      format( I5, I14 )
          n = n + 1
          go to 5
  30  end

 100  subroutine solve
      integer*1 b(12), rstack(12)
      integer*1 n, c, r, safe, lc, lr
      integer*2 s
      common /area/ b,rstack,n,c,r,safe,lc,lr,s

 101  if ( c .ne. n ) go to 110
          s = s + 1
          go to 160
 110  r = 0
 140  call issafe
      if ( safe .eq. 0 ) go to 300
          b(r) = c
          rstack(c) = r
          c = c + 1
          go to 101
 160      c = c - 1
          r = rstack(c)
          b(r) = -1
 300  r = r + 1
      if ( r .lt. n ) go to 140
      if ( c .ne. 0 ) go to 160
      end

 500  subroutine issafe
      integer*1 b(12), rstack(12)
      integer*1 n, c, r, safe, lc, lr
      integer*2 s
      common /area/ b,rstack,n,c,r,safe,lc,lr,s

      safe = 0
      if ( b(r) .ne. -1 ) return
 530  lr = r - 1
      lc = c - 1
 550      if ( ( lr .lt. 0 ) .or. ( lc .lt. 0 ) ) go to 600
          if ( b(lr) .eq. lc ) return
          lr = lr - 1
          lc = lc - 1
          go to 550
 600  lr = r + 1
      lc = c - 1
 620      if ( ( lr .eq. n ) .or. ( lc .lt. 0 ) ) go to 670
          if ( b(lr) .eq. lc ) return
          lr = lr + 1
          lc = lc - 1
          go to 620
 670  safe = 1
      end


