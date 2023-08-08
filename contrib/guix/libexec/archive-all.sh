#!/usr/bin/env sh
pwd
if [ -z $1 ]; then
    echo "You must specify a super-archive name."
    exit 1
fi

if test -f "$1.tar"; then
    rm "$1.tar"
fi
if test -f "$1.tar.gz"; then
    rm "$1.tar.gz"
fi

git archive --prefix "$1/" -o "$1.tar" HEAD
git submodule foreach --recursive "git archive --prefix=$1/\$displaypath/ --output=\$sha1.tar HEAD && tar --concatenate --file=$(pwd)/$1.tar \$sha1.tar && rm \$sha1.tar"

gzip "$1.tar"

if [ $# -gt 1 ]; then
    if [ "$1.tar.gz" != "$2" ]; then
        mv "$1.tar.gz" "$2"
    fi
fi
