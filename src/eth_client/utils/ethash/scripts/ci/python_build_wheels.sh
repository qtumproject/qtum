#!/usr/bin/env bash
# -*- coding: utf-8 -*-

# ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
# Copyright 2019 Pawel Bylica.
# Licensed under the Apache License, Version 2.0.

set -eo pipefail

ACTIVATE_DIR=bin

if [ -n "$APPVEYOR" ]; then
    ACTIVATE_DIR=Scripts
    PYTHON_PATHS="/c/Python313-x64:/c/Python313-x64/Scripts /c/Python312-x64:/c/Python312-x64/Scripts /c/Python311-x64:/c/Python311-x64/Scripts /c/Python310-x64:/c/Python310-x64/Scripts"
elif [ -n "$CIRCLECI" ]; then
    if [ "$OSTYPE" = "linux-gnu" ]; then
        PYTHON_PATHS="/opt/python/cp313-cp313/bin /opt/python/cp312-cp312/bin /opt/python/cp311-cp311/bin /opt/python/cp310-cp310/bin"
    else
        # The continuation of the script expects "python" executable name,
        # so make link python -> python3.
        mkdir -p $HOME/bin
        ln -s $(which python3) $HOME/bin/python
        PYTHON_PATHS="$HOME/bin"
    fi
fi

PATH_ORIG=$PATH
for p in $PYTHON_PATHS
do
    PATH="$p:$PATH_ORIG"
    python -m venv venv
    source venv/$ACTIVATE_DIR/activate
    echo '***'
    python --version
    which python
    pip --version
    echo '***'
    pip install --use-pep517 build
    python -m build
    deactivate
    rm -rf venv
done
