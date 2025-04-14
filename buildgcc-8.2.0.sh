#!/bin/bash 
#
# This builds the (Outrun) M68000 cross compiler 'suite'.
# - Binutils
# - GCC (C, C++)
# - Newlib
# - libgcc, libstdc++-v3
#
# Prerequisites:
#   Windows: Can either build from MinGW or MSYS2 
#   MacOS: needs 'brew' installed. Possibly some xcode dependencies (not verified).
#
# MinGW:
#   Download the setup program, and should you have unchecked EVERYTHING including the graphic installer, go to the installation folder and run:
#     bin\mingw-get.exe install msys-base mingw32-base
#  
#   Afterwards, go to the newly created msys\1.0 subfolder and start msys.bat.
#   NOTE: If your user name has a space in it (full name), you should add 'SET USERNAME=SomeUser' to msys.bat before starting.
#
#   In the MSYS prompt, run /postinstall/pi.sh, and set up the MinGW installation folder.
#
# Then copy this script to your home folder and run it.
#
# Packages needed (these are installed as needed by this script):
#   - msys-base (installed above)
#   - mingw32-base (installed above)
#   - msys-wget
#   - mingw32-gmp (dev)
#   - mingw32-mpfr(dev)
#   - mingw32-mpc (dev)
#   - mingw32-gcc-g++
#
# MSYS2:
#   Download installer from www.msys2.org and install.
#   Copy buildgcc.sh script to home folder and execute. It'll install required packages (via pacman) and download necessary archives (wget).
#

# Test for spaces in the current directory.
homefolder=`pwd`
if [[ $homefolder =~ .\ . ]]
then
  echo "Current directory '$homefolder' contains spaces. Compile will fail. Please change to another directory, or set up another user by adding:

  SET USERNAME=SomeUser

in msys.bat"
  exit 1
fi

# Versions.
# Tested with:
# binutils 2.24, gcc 4.9.0, isl 0.12.2, cloog 0.18.1, newlib 2.1.0
# binutils 2.27, gcc 6.1.0, isl 0.16.1, cloog 0.18.1, newlib 2.4.0
# binutils 2.28, gcc 7.1.0, isl 0.16.1, cloog 0.18.1, newlib 2.5.0
BINUTILS_VERSION=binutils-2.32
# let op, 2.32 is de laatste.
GCC_VERSION=gcc-8.2.0
# let op, we hebben al 9.
ISL_VERSION=isl-0.18
CLOOG_VERSION=cloog-0.18.1
# geen idee of deze 2 nog moeten
NEWLIB_VERSION=newlib-3.1.0
# 3.1.0 is uit.

# Set to '-j4' to have faster, but flaky, compiles.
MAKE_MULTI=-j4

# Other settings.
TARGET=m68k-elf
TEMPFOLDER=gcc-temp
LANGUAGES=c,c++

# Detect host platform
HOST_SHELL=
case "$(uname -s)" in 
  MINGW*)
    HOST_SHELL=mingw
    ;;
  MSYS*)
    HOST_SHELL=msys2
	;;
  Darwin)
    HOST_SHELL=mac
	;;
esac

# Set up installation folder.
if [ "$HOST_SHELL" = "msys2" ] || [ "$HOST_SHELL" = "mingw" ]; then
  DEFAULT_ROOT=/c
else
  DEFAULT_ROOT=~
fi
PREFIX=$DEFAULT_ROOT/outrun/$GCC_VERSION

echo Building in: $PREFIX
echo Building languages: $LANGUAGES
echo Building target: $TARGET
echo Using temporary folder: $TEMPFOLDER

# This makes sure we exit if anything unexpected happens.
set -e

if [ ! -d "$TEMPFOLDER" ]; then
  mkdir $TEMPFOLDER
fi

# Install MinGW components.

if [ $HOST_SHELL == msys2 ]; then

  if [ ! -f "$TEMPFOLDER/packages.installed" ]; then
    echo Downloading and installing MSYS2 packages...
    pacman -S --noconfirm tar binutils make gcc wget texinfo diffutils gmp-devel mpfr-devel mpc-devel
    touch "$TEMPFOLDER/packages.installed"
  fi

fi
  
if [ $HOST_SHELL == mingw ]; then

  if [ ! -f "$TEMPFOLDER/packages.installed" ]; then

    echo Downloading and installing MinGW packages...
    mingw-get install msys-wget
    # mingw-get install mingw32-binutils (already installed)
    # mingw-get install mingw32-gcc (already installed)

    # C++/target runtime libs.
    mingw-get install mingw32-gcc-g++
    mingw-get install mingw32-gmp
    mingw-get install mingw32-mpc
    mingw-get install mingw32-mpfr

    # Redist only
    # mingw-get install msys-zip

    touch "$TEMPFOLDER/packages.installed"
  fi

fi

if [ $HOST_SHELL == mac ]; then

  if [ ! -f "$TEMPFOLDER/packages.installed" ]; then

	brew install wget gmp mpfr libmpc

    touch "$TEMPFOLDER/packages.installed"

  fi

fi
  
# Hack: remove temp folder from failed run.
# echo Removing old temporary data...
# rm -r $TEMPFOLDER

# Download everything.
echo Downloading archives...

if [ ! -d "$TEMPFOLDER/downloads" ]; then
mkdir $TEMPFOLDER/downloads
fi
cd $TEMPFOLDER/downloads

if [[ (! -f "binutils.downloaded") || $(<"binutils.downloaded") != "$BINUTILS_VERSION" ]]; then
  wget http://ftp.gnu.org/pub/gnu/binutils/$BINUTILS_VERSION.tar.bz2
  echo $BINUTILS_VERSION>"binutils.downloaded"
fi

if [[ (! -f "gcc.downloaded") || $(<"gcc.downloaded") != "$GCC_VERSION" ]]; then
  wget http://ftp.gnu.org/pub/gnu/gcc/$GCC_VERSION/$GCC_VERSION.tar.gz
  wget ftp://gcc.gnu.org/pub/gcc/infrastructure/$ISL_VERSION.tar.bz2
  wget ftp://gcc.gnu.org/pub/gcc/infrastructure/$CLOOG_VERSION.tar.gz
  echo $GCC_VERSION>"gcc.downloaded"
fi 

if [[ (! -f "newlib.downloaded") || $(<"newlib.downloaded") != "$NEWLIB_VERSION" ]]; then
  wget ftp://sourceware.org/pub/newlib/$NEWLIB_VERSION.tar.gz
  echo $NEWLIB_VERSION>"newlib.downloaded"
fi

cd ..

# Unpack binutils.
if [[ (! -f "binutils.unpacked") || $(<$"binutils.unpacked") != "$BINUTILS_VERSION" ]]; then

  echo Unpacking binutils...
  tar jxvf ./downloads/$BINUTILS_VERSION.tar.bz2

  if [ $BINUTILS_VERSION = 'binutils-2.31' ]
  then
	# https://patchwork.ozlabs.org/patch/945826/
	mv $BINUTILS_VERSION/libiberty/simple-object-elf.c $BINUTILS_VERSION/libiberty/simple-object-elf.bak
	sed -e 's/ENOTSUP/ENOSYS/g' $BINUTILS_VERSION/libiberty/simple-object-elf.bak > $BINUTILS_VERSION/libiberty/simple-object-elf.c
	rm $BINUTILS_VERSION/libiberty/simple-object-elf.bak
  fi

  echo $BINUTILS_VERSION>"binutils.unpacked"
fi

# Build binutils.
# todo: disable multilib? mcpu=68000.
if [[ (! -f "binutils.built") || $(<"binutils.built") != "$BINUTILS_VERSION" ]]; then
  echo Building binutils...
  if [ ! -d "binutils-obj" ]; then
    mkdir binutils-obj
  fi
  cd binutils-obj
  ../$BINUTILS_VERSION/configure --prefix=$PREFIX --target=$TARGET
  make $MAKE_MULTI
  make install
  cd ..
  echo $BINUTILS_VERSION>"binutils.built"
fi


# Unpack GCC and prerequisites
if [[ (! -f "gcc.unpacked") || $(<$"gcc.unpacked") != "$GCC_VERSION" ]]; then

  echo Unpacking GCC and prerequisites
  tar xvf ./downloads/$GCC_VERSION.tar.gz
  tar jxvf ./downloads/$ISL_VERSION.tar.bz2
  tar xvf ./downloads/$CLOOG_VERSION.tar.gz

  # Move ISL and CLooG into the GCC directory tree.
  mv $ISL_VERSION ./$GCC_VERSION/isl
  mv $CLOOG_VERSION ./$GCC_VERSION/cloog
  echo $GCC_VERSION>"gcc.unpacked"
fi

if [[ (! -f "gcc.built") || $(<$"gcc.built") != "$GCC_VERSION" ]]; then

  # Configure and build GCC (compilers only)
  echo Building GCC...
  if [ ! -d "gcc-obj" ]; then
    mkdir gcc-obj
  fi
  cd gcc-obj
  ../$GCC_VERSION/configure --prefix=$PREFIX --target=$TARGET --enable-languages=$LANGUAGES --with-newlib --disable-libmudflap --disable-libssp --disable-libgomp --disable-libstdcxx-pch --disable-threads --disable-nls --disable-libquadmath --with-gnu-as --with-gnu-ld --without-headers  --disable-multilib --with-cpu=m68000
  make $MAKE_MULTI all-gcc
  make install-gcc
  cd ..

  echo $GCC_VERSION>"gcc.built"
  
  # Copying required .dll files.
  if [ $HOST_SHELL == msys2 ]; then
    # Should be in msys/usr/bin
    cp `where msys-2.0.dll` $PREFIX/bin
    cp `where msys-mpfr-6.dll` $PREFIX/bin
    cp `where msys-mpc-3.dll` $PREFIX/bin
    cp `where msys-gmp-10.dll` $PREFIX/bin
    cp `where msys-gcc_s-seh-1.dll` $PREFIX/bin
  fi

  if [ $HOST_SHELL == mingw ]; then
    cp `where libgmp-10.dll` $PREFIX/bin
    cp `where libmpc-3.dll` $PREFIX/bin
    cp `where libmpfr-4.dll` $PREFIX/bin
    cp `where libgcc_s_dw2-1.dll` $PREFIX/bin
  fi

fi

if [[ (! -f "newlib.unpacked") || $(<$"newlib.unpacked") != "$NEWLIB_VERSION" ]]; then

  # Unpack newlib.
  tar vxf downloads/$NEWLIB_VERSION.tar.gz

  # Patch newlib 2.1.0 compile errors.
  # For some reason the -i parameter doesn't work in MinGW, permission errors on the temporary files.
  if [ $NEWLIB_VERSION = 'newlib-2.1.0' ]
  then

	mv $NEWLIB_VERSION/libgloss/m68k/io-read.c $NEWLIB_VERSION/libgloss/m68k/io-read.bak
	sed -e 's/ssize_t/_READ_WRITE_RETURN_TYPE/g' $NEWLIB_VERSION/libgloss/m68k/io-read.bak > $NEWLIB_VERSION/libgloss/m68k/io-read.c
	rm $NEWLIB_VERSION/libgloss/m68k/io-read.bak

	mv $NEWLIB_VERSION/libgloss/m68k/io-write.c $NEWLIB_VERSION/libgloss/m68k/io-write.bak
	sed -e 's/ssize_t/_READ_WRITE_RETURN_TYPE/g' $NEWLIB_VERSION/libgloss/m68k/io-write.bak > $NEWLIB_VERSION/libgloss/m68k/io-write.c
	rm $NEWLIB_VERSION/libgloss/m68k/io-write.bak

  fi
  
  echo $NEWLIB_VERSION>"newlib.unpacked"

fi

# Add the output folder to our search path. We'll need this if we want to cross compile.
export PATH=$PATH:$PREFIX/bin

if [[ (! -f "newlib.built") || $(<$"newlib.built") != "$NEWLIB_VERSION" ]]; then

  # Compile newlib
  echo Compiling newlib...
  mkdir newlib-obj
  cd newlib-obj
  ../$NEWLIB_VERSION/configure --prefix=$PREFIX --target=$TARGET --disable-newlib-multithread --disable-newlib-io-float --enable-lite-exit --disable-newlib-supplied-syscalls --disable-multilib --with-cpu=m68000
  make $MAKE_MULTI
  make install
  cd ..

  echo $NEWLIB_VERSION>"newlib.built"
  
fi

if [[ (! -f "libgcc.built") || $(<$"libgcc.built") != "$GCC_VERSION" ]]; then

  # Now we can build libgcc and libstdc++-v3
  cd gcc-obj
  echo Building libgcc and libstdc++
  make $MAKE_MULTI all-target-libgcc all-target-libstdc++-v3
  make install-target-libgcc install-target-libstdc++-v3
  cd ..
  
  echo $GCC_VERSION>"libgcc.built"

fi  

# Build redistributable.
# cd $PREFIX
# zip -r -9 outrun-$GCC_VERSION.zip .

echo All done!
echo Output binaries for $TARGET are in $PREFIX
echo It\'s now safe to wipe $TEMPFOLDER
