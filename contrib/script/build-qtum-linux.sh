#!/bin/sh

# Build Qtum package on Linux OS, for example: qtum-22.1-x86_64-pc-linux-gnu.tar.gz

BUILD_PARAM=$1

if RECENT_TAG="$(git describe --exact-match HEAD 2> /dev/null)"; then
    VERSION="${RECENT_TAG#v}"
else
    VERSION="$(git rev-parse --short=12 HEAD)"
fi
DISTNAME="qtum-${VERSION}"

SRC_DIR=$PWD
cd ../../
./autogen.sh
cd ./depends
make clean
make $BUILD_PARAM
HOST="$(./config.guess 2> /dev/null)"
if [[ ! -d "./$HOST" ]]
then
  echo "HOST platform not built."
  exit 1
fi
cd ..
CONFIG_SITE=$PWD/depends/$HOST/share/config.site ./configure --disable-ccache --disable-maintainer-mode --disable-dependency-tracking --enable-glibc-back-compat --enable-reduce-exports --disable-bench --disable-gui-tests --disable-fuzz-binary CFLAGS="-O2" CXXFLAGS="-O2" LDFLAGS="-static-libstdc++ -Wl,-O2"
make clean
make $BUILD_PARAM
INSTALLPATH="${PWD}/installed/${HOST}"
rm -r ${INSTALLPATH}
make install DESTDIR=${INSTALLPATH}
mv ${INSTALLPATH}/usr/local ${INSTALLPATH}/usr/${DISTNAME}
DISTPATH=${INSTALLPATH}/usr
cp ./README.md ${DISTPATH}/${DISTNAME}
cd ${DISTPATH}
find . -name "lib*.la" -delete
find . -name "lib*.a" -delete
rm -rf ./${DISTNAME}/lib/pkgconfig
cd ${DISTPATH}/${DISTNAME}/bin
strip *
cd ${DISTPATH}/${DISTNAME}/lib
strip *
cd ${DISTPATH}
find ${DISTNAME} -not -name "*.dbg" | sort | tar --no-recursion --mode='u+rw,go+r-w,a+X' --owner=0 --group=0 -c -T - | gzip -9n > ${SRC_DIR}/${DISTNAME}-${HOST}.tar.gz
cd ${SRC_DIR}
sha256sum ${DISTNAME}-${HOST}.tar.gz > ${DISTNAME}-${HOST}.hash
