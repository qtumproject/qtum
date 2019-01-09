#Copyright (c) 2007 - 2010 Jordan "Earlz/hckr83" Earls  <http://www.Earlz.biz.tm>
#All rights reserved.

#Redistribution and use in source and binary forms, with or without
#modification, are permitted provided that the following conditions
#are met:

#1. Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#2. Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#3. The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#   
#THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
#INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
#AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
#THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
#OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
#ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#This file is part of the x86Lib project.


HDRS =  include/config.h include/opcode_def.h include/x86lib.h include/x86lib_internal.h tests/x86test.h include/elfloader.h
CXX ?= g++
AR ?= ar


TEST_CC ?= i386-elf-gcc
TEST_CFLAGS ?= -Os -nostartfiles -nodefaultlibs -Wl,-z,norelro -Wl,--build-id=none -static


CXX_VM_SRC = vm/x86lib.cpp vm/modrm.cpp vm/device_manager.cpp vm/cpu_helpers.cpp vm/ops/strings.cpp vm/ops/store.cpp vm/ops/maths.cpp \
		  vm/ops/groups.cpp vm/ops/flow.cpp vm/ops/flags.cpp vm/ops/etc.cpp utils/elfloader.cpp

CXX_VM_OBJS = $(subst .cpp,.o,$(CXX_VM_SRC))

CXX_TESTBENCH_SRC = testbench/testbench.cpp

CXX_TESTBENCH_OBJS = $(subst .cpp,.o,$(CXX_TESTBENCH_SRC))

CXX_TEST_SRC = tests/test_main.cpp tests/flag_tests.cpp tests/test_helpers.cpp tests/mov_tests.cpp tests/math_tests.cpp tests/helper_tests.cpp tests/flow_tests.cpp tests/device_tests.cpp tests/etc_tests.cpp tests/group_tests.cpp tests/encoding_tests.cpp
CXX_TEST_OBJS = $(subst .cpp,.o,$(CXX_TEST_SRC))



CXXFLAGS ?= -Wall -fPIC -g -O0 -std=c++11
CXXFLAGS += -DX86LIB_BUILD -I./include -fexceptions

VERSION=1.1

VM_OUTPUTS = libx86lib.a
OUTPUTS = $(VM_OUTPUTS) x86testbench 

default: build

build: $(OUTPUTS)

test: x86lib_tests
	./x86lib_tests -a

x86lib_tests: $(CXX_TEST_OBJS) $(OUTPUTS)
	$(CXX) $(CXXFLAGS) -std=c++11 -o x86lib_tests $(CXX_TEST_OBJS) -lx86lib -L.

$(CXX_TEST_OBJS): $(HDRS) $(CXX_VM_SRC) $(CXX_TEST_SRC)
	$(CXX) $(CXXFLAGS) -std=c++11 -c $*.cpp -o $@

libx86lib.a: $(CXX_VM_OBJS)
	ar crs libx86lib.a $(CXX_VM_OBJS)

x86testbench: $(CXX_TESTBENCH_OBJS) $(VM_OUTPUTS)
	$(CXX) $(CXXFLAGS) -o x86testbench $(CXX_TESTBENCH_OBJS) -lx86lib -L.

$(CXX_TESTBENCH_OBJS): $(HDRS) $(CXX_TESTBENCH_SRC)
	$(CXX) $(CXXFLAGS) -c $*.cpp -o $@

$(CXX_VM_OBJS): $(HDRS) $(CXX_VM_SRC)
	$(CXX) $(CXXFLAGS) -c $*.cpp -o $@

clean:
	rm -f $(CXX_VM_OBJS) $(OUTPUTS) $(CXX_TESTBENCH_OBJS) $(CXX_TEST_OBJS) x86lib_tests

buildtest:
	#i386-elf-gcc -c testos.c -o testos.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra $(testos_CFLAGS)
	#i386-elf-gcc -T linker.ld -o testos.bin -ffreestanding -O2 -nostdlib testos.o -lgcc -Wl,--gc-sections $(testos_CFLAGS) -dead_strip
	yasm -o testbench/test.o testbench/test.asm -f elf32
	i386-elf-gcc -T testbench/linker.ld -o test.elf -ffreestanding -nostdlib testbench/test.o -Wl,--gc-sections $(testos_CFLAGS) -dead_strip -Xlinker -Map=test.elf.map -Xlinker -n
	#strip -s -S test.elf


