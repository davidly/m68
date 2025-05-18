C fortran version of matrix multiplication benchmark from BYTE magazine
C should produce the sum of 465880
      program mm
      integer*4 i, j, k
      integer*4 sum
      integer*4 a(21,21)
      integer*4 b(21,21)
      integer*4 c(21,21)

C initialize A
      do 110 i = 1, 20
      do 100 j = 1, 20
      a( i, j ) = i + j
  100 continue
  110 continue

C initialize B
      do 210 i = 1, 20
      do 200 j = 1, 20
      b( i, j ) = ( i + j ) / j
  200 continue
  210 continue

C initialize C
      do 310 i = 1, 20
      do 300 j = 1, 20
      c( i, j ) = 0
  300 continue
  310 continue

C matrix multiply
      do 410 i = 1, 20
      do 405 j = 1, 20
      do 400 k = 1, 20
      c( i, j ) = c( i, j ) + a( i, k ) * b( k, j )
  400 continue
  405 continue
  410 continue

C sum values in C
      sum = 0
      do 510 i = 1, 20
      do 500 j = 1, 20
      sum = sum + c( i, j )
  500 continue
  510 continue

C print the sum of values in C
  600 write( *, 2000 ) sum
  610 write( *, 2010 )
 2000 format( I10 )
 2010 format( 'mm completed with great success' )
      end
