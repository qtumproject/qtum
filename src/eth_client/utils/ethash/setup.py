#!/usr/bin/env python3

# ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
# Copyright 2019 Pawel Bylica.
# Licensed under the Apache License, Version 2.0.

import os
import subprocess
import shutil
from distutils.errors import CCompilerError
from os import path

from setuptools import setup
from setuptools.command.build_ext import build_ext as setuptools_build_ext


class build_ext(setuptools_build_ext):
    user_options = setuptools_build_ext.user_options + [
        ('skip-cmake-build', None,
         "Skip CMake build assuming the libraries are already installed " +
         "to the dist directory")
    ]

    def initialize_options(self):
        super(build_ext, self).initialize_options()
        self.skip_cmake_build = False

    def run(self):
        build_dir = self.build_temp
        source_dir = path.dirname(path.abspath(__file__))
        install_dir = path.join(source_dir, 'dist')

        cmake_opts = [
            '-DCMAKE_INSTALL_PREFIX={}'.format(install_dir),
            '-DCMAKE_INSTALL_LIBDIR=lib',
            '-DCMAKE_POSITION_INDEPENDENT_CODE=TRUE',
            '-DHUNTER_ENABLED=OFF',
            '-DETHASH_INSTALL_CMAKE_CONFIG=OFF'
        ]

        generator = os.environ.get('GENERATOR')
        if generator:
            cmake_opts.append('-G{}'.format(generator))

        if not self.skip_cmake_build and not os.environ.get('ETHASH_PYTHON_SKIP_BUILD'):
            cmake_cmd = shutil.which('cmake')
            if not cmake_cmd:
                raise CCompilerError(
                    "cmake tool not found but required to build this package")

            r = subprocess.call([cmake_cmd, source_dir] + cmake_opts,
                                cwd=build_dir)
            if r != 0:
                raise CCompilerError(
                    "cmake configuration failed with exit status {}".format(r))
            r = subprocess.call(
                [cmake_cmd, '--build', build_dir, '--target', 'install',
                 '--config', 'Release'])
            if r != 0:
                raise CCompilerError(
                    "cmake build failed with exit status {}".format(r))

        self.library_dirs.append(path.join(install_dir, 'lib'))

        super(build_ext, self).run()


setup(
    name='ethash',
    version='1.0.1',
    description=
    "C/C++ implementation of Ethash – the Ethereum Proof of Work algorithm",
    url='https://github.com/chfast/ethash',
    author='Paweł Bylica',
    author_email='pawel@ethereum.org',
    license='Apache License, Version 2.0',

    package_dir={'': 'bindings/python'},
    packages=['ethash'],
    cffi_modules=['bindings/python/ethash/_build.py:ffibuilder'],

    python_requires='>=3.6',
    setup_requires=['cffi>=1.12'],
    install_requires=['cffi>=1.12'],

    test_suite='tests.test_ethash',

    cmdclass={'build_ext': build_ext},

    classifiers=[
        'License :: OSI Approved :: Apache Software License',
        'Programming Language :: C',
        'Programming Language :: C++',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
    ],
)
