N=
TARG=clib
CC=c68
AS=as68
# default register usage:
CFLAGS=-WPDLE -SO -M0B -YRd2-d7/a3-a4 -YTd0-d1/a0-a2 -YFa6 -YDa5  
AFLAGS=
O=oss

#
# default action:
# make all versions of all libraries:
#
all:	c m m8

#
# for making C libraries:
# the second letter (s or l) defines the int size
#  s = 16-bit ints
#  l = 32-bit ints
# the third letter (s or l) defines the memory model
#  s = small code, small data
#  l = large code, large data
#
c:		css cls csl cll

css:
	make LIB=c16  TARG=clib O=oss "CFLAGS=$(CFLAGS) -PS" AFLAGS= clib
cls:
	make LIB=c    TARG=clib O=ols "CFLAGS=$(CFLAGS) -PL" AFLAGS=-EINT32 clib
csl:
	make LIB=cl16 TARG=clib O=osl "CFLAGS=$(CFLAGS) -PS -MCD" AFLAGS=-CD clib
cll:
	make LIB=cl   TARG=clib O=oll  \
				"CFLAGS=$(CFLAGS) -PL -MCD" "AFLAGS=-EINT32 -CD" clib

#
# for making IEEE software emulation math libraries:
#
m:		mss mls msl mll

mss:
	make LIB=m16   CC=$(CC) TARG=mlib O=mss \
		"CFLAGS=$(CFLAGS) -D_FLT_MANX -FM -PS" AFLAGS= mlib
mls:
	make LIB=m     CC=$(CC) TARG=mlib O=mls \
		"CFLAGS=$(CFLAGS) -D_FLT_MANX -FM -PL" AFLAGS=-EINT32 mlib
msl:
	make LIB=ml16  CC=$(CC) TARG=mlib O=msl \
		"CFLAGS=$(CFLAGS) -D_FLT_MANX -FM -MCD -PS" AFLAGS=-CD mlib
mll:
	make LIB=ml    CC=$(CC) TARG=mlib O=mll \
		"CFLAGS=$(CFLAGS) -D_FLT_MANX -FM -MCD -PL" "AFLAGS=-CD -EINT32" mlib

#
# for making 68881 math libraries:
#
m8:	m8ss m8ls m8sl m8ll

m8ss:
	make LIB=m816  CC=$(CC) TARG=m8lib O=8ss \
		"CFLAGS=$(CFLAGS) -D_FLT_881 -F8 -PS" AFLAGS= m8lib
m8ls:
	make LIB=m8    CC=$(CC) TARG=m8lib O=8ls \
		"CFLAGS=$(CFLAGS) -D_FLT_881 -F8 -PL" AFLAGS=-EINT32 m8lib
m8sl:
	make LIB=m8l16 CC=$(CC) TARG=m8lib O=8sl \
		"CFLAGS=$(CFLAGS) -D_FLT_881 -F8 -MCD -PS" AFLAGS=-CD m8lib
m8ll:
	make LIB=m8l   CC=$(CC) TARG=m8lib O=8ll \
		"CFLAGS=$(CFLAGS) -D_FLT_881 -F8 -MCD -PL" "AFLAGS=-CD -EINT32" m8lib

#
# delete object modules:
#
all_clean:	c_clean m_clean m8_clean

c_clean:	cssclean clsclean cslclean cllclean

cssclean:
	make O=oss cclean
clsclean:
	make O=ols cclean
cslclean:
	make O=osl cclean
cllclean:
	make O=oll cclean

m_clean:	mssclean mlsclean mslclean mllclean

mssclean:
	make O=mss mclean
mslclean:
	make O=msl mclean
mlsclean:
	make O=mls mclean
mllclean:
	make O=mll mclean

m8_clean:	m8ssclean m8lsclean m8slclean m8llclean

m8ssclean:
	make O=8ss m8clean
m8lsclean:
	make O=8ls m8clean
m8slclean:
	make O=8sl m8clean
m8llclean:
	make O=8ll m8clean

#
# The following dependencies (clib, mlib, m8lib, cclean, mclean, m8clean)
# are used when make re-invokes itself. i.e. when you type "make css"
# make invokes make again to create clib, with appropriate macros defined.
# Don't try to make these directly.
#
clib:
	cd misc
	make $N CC=$(CC) AS=$(AS) "CFLAGS=$(CFLAGS)" "AFLAGS=$(AFLAGS)" O=$O $(TARG)
	cd ..\stdio
	make $N CC=$(CC) AS=$(AS) "CFLAGS=$(CFLAGS)" "AFLAGS=$(AFLAGS)" O=$O $(TARG)
	cd ..\stdlib
	make $N CC=$(CC) AS=$(AS) "CFLAGS=$(CFLAGS)" "AFLAGS=$(AFLAGS)" O=$O $(TARG)
	cd ..\string
	make $N CC=$(CC) AS=$(AS) "CFLAGS=$(CFLAGS)" "AFLAGS=$(AFLAGS)" O=$O $(TARG)
	cd ..\time
	make $N CC=$(CC) AS=$(AS) "CFLAGS=$(CFLAGS)" "AFLAGS=$(AFLAGS)" O=$O $(TARG)
	cd ..\rom68
	make $N CC=$(CC) AS=$(AS) "CFLAGS=$(CFLAGS)" "AFLAGS=$(AFLAGS)" O=$O $(TARG)
	cd ..\inp
	del $(LIB).out
	make $N LIB=$(LIB) O=$O
	cd ..

mlib:
	cd stdlib
	make $N CC=$(CC) AS=$(AS) "CFLAGS=$(CFLAGS)" "AFLAGS=$(AFLAGS)" O=$O $(TARG)
	cd ..\stdio
	make $N CC=$(CC) AS=$(AS) "CFLAGS=$(CFLAGS) -DFLOAT" "AFLAGS=$(AFLAGS)" O=$O $(TARG)
	cd ..\time
	make $N CC=$(CC) AS=$(AS) "CFLAGS=$(CFLAGS)" "AFLAGS=$(AFLAGS)" O=$O $(TARG)
	cd ..\mx_ieee
	make $N CC=$(CC) AS=$(AS) "CFLAGS=$(CFLAGS)" "AFLAGS=$(AFLAGS)" O=$O $(TARG)
	cd ..\inp
	del $(LIB).out
	make $N LIB=$(LIB)
	cd ..

m8lib:
	cd m881
	make $N CC=$(CC) AS=$(AS) "CFLAGS=$(CFLAGS)" "AFLAGS=$(AFLAGS)" O=$O $(TARG)
	cd ..\stdlib
	make $N CC=$(CC) AS=$(AS) "CFLAGS=$(CFLAGS)" "AFLAGS=$(AFLAGS)" O=$O $(TARG)
	cd ..\stdio
	make $N CC=$(CC) AS=$(AS) "CFLAGS=$(CFLAGS) -DFLOAT" "AFLAGS=$(AFLAGS)" O=$O $(TARG)
	cd ..\time
	make $N CC=$(CC) AS=$(AS) "CFLAGS=$(CFLAGS)" "AFLAGS=$(AFLAGS)" O=$O $(TARG)
	cd ..\inp
	del $(LIB).out
	make $N LIB=$(LIB)
	cd ..

cclean:
	cd misc
	del *.$O 
	cd ..\stdio
	del *.$O 
	cd ..\stdlib
	del *.$O
	cd ..\string
	del *.$O
	cd ..\sysio
	del *.$O
	cd ..\time
	del *.$O
	cd ..

mclean:
	cd mx_ieee
	del *.$O 
	cd ..\stdlib
	del *.$O 
	cd ..\stdio
	del *.$O 
	cd ..\time
	del *.$O 
	cd ..

m8clean:
	cd m881
	del *.$O 
	cd ..\stdlib
	del *.$O 
	cd ..\stdio
	del *.$O 
	cd ..\time
	del *.$O 
	cd ..

