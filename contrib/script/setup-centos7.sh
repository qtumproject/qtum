#!/bin/sh

# Setup Qtum build environment for CentOS 7
yum -y install centos-release-scl
yum -y install devtoolset-8-gcc devtoolset-8-gcc-c++
yum -y install autoconf automake binutils bison ca-certificates curl faketime git libtool patch pkgconfig python3 python3-pip cmake curl-devel gmp-devel libmicrohttpd-devel miniupnpc-devel

DEVTOOLSET="source scl_source enable devtoolset-8"
FILENAME="/etc/profile"
if grep "$DEVTOOLSET" $FILENAME > /dev/null
then
   echo "devtoolset-8 enabled"
else
   echo $DEVTOOLSET | tee -a $FILENAME
   reboot
fi
