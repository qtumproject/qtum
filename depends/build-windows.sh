#!/bin/sh

# Set platform variables
OS_BITS=32
PLATFORM="i686-w64-mingw32"
CC="/usr/bin/i686-w64-mingw32-gcc-posix"
CXX="/usr/bin/i686-w64-mingw32-g++-posix"
if [ "$1" = "x64" ]; then
    OS_BITS=64
    PLATFORM="x86_64-w64-mingw32"
    CC="/usr/bin/x86_64-w64-mingw32-gcc-posix"
    CXX="/usr/bin/x86_64-w64-mingw32-g++-posix"
fi
OLD_PATH=`pwd`
INSTALL_DIR="$OLD_PATH/$PLATFORM"
LIB_DIR="$INSTALL_DIR/lib"
INCLUDE_DIR="$INSTALL_DIR/include"

# Install compilers if needed
if [ ! -f $CC ]; then
   sudo apt-get install curl g++-mingw-w64-x86-64 mingw-w64-x86-64-dev g++-mingw-w64-i686 mingw-w64-i686-dev
fi

# Make dependencies
make HOST=$PLATFORM

# Build from source code leveldb
LEVELDB_DIR="../src/leveldb"
cd $LEVELDB_DIR
CC="$CC" CXX="$CXX" TARGET_OS="NATIVE_WINDOWS" make
cd $OLD_PATH
rm "$LIB_DIR/libleveldb.a"
rm "$LIB_DIR/libmemenv.a"
rm -r "$INCLUDE_DIR/leveldb"
cp "$LEVELDB_DIR/out-static/libleveldb.a" "$LIB_DIR/libleveldb.a"
cp "$LEVELDB_DIR/out-static/libmemenv.a" "$LIB_DIR/libmemenv.a"
cp -r "$LEVELDB_DIR/include/leveldb" "$INCLUDE_DIR"
mkdir "$INCLUDE_DIR/leveldb/helpers"
cp -r "$LEVELDB_DIR/helpers/memenv/memenv.h" "$INCLUDE_DIR/leveldb/helpers/memenv.h"

# Build eth dependencies
cd ..
./autogen.sh Windows $PLATFORM $INSTALL_DIR
./configure --prefix=$PWD/depends/$PLATFORM --host=$PLATFORM

# Build the application
make && make install

# Remove the symbols for release
cd $INSTALL_DIR/bin
/usr/bin/$PLATFORM-strip *
cd $OLD_PATH
