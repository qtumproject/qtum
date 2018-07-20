/**
Copyright (c) 2007 - 2010 Jordan "Earlz/hckr83" Earls  <http://www.Earlz.biz.tm>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.
   
THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This file is part of the x86Lib project.
**/
#ifndef CONFIG_H
#define CONFIG_H

#ifdef __i386__
//#define USE_NATIVE //This means to optimize by using native assembly when available...
#endif

//#define X86_MULTITHREADING //This should be defined to enable multi-threading optimizations(note, these will break most single thread implementations)

#define QTUM_DEBUG //define to enable potentially performance impacting debug features

#endif

/**Locked Memory Access Description
Ok, for this new multi CPU capable branch, memory locking has been added.
The PhysMemory now has Lock, Unlock, and IsLocked member functions, and it has
a new variable in it called locked (of type uint32_t)
Also, at the end of Readxxxx and Writexxxx functions, there is now a new variable
named locked_access (with a default value of 0) If this value is 1, then memory
access should be granted, even though the memory is locked. This basically means
that the memory request is coming from the current CPU which holds the lock.

In most implementations, (unless the X86_MULTITHREADING define is used) the CPU
will check IsLocked and if the memory is already locked(which in most cases, it
actually shouldn't be) then it will break from the currently executing instruction
so that if another CPU holds the lock, it will be able to get to the next CPU in
single threaded implementations.

For PhysMemory preaccess lock checks, basically, what should be attached at the first of all
Readxxxx and Writexxxx functions is
	if(locked_access==0){
		while(locked>0){}
	}
so that if the busmaster is no trying to access memory, and the memory is locked,
then it will halt access until the lock is repelled. (note the infinite loop should never
be got to in single-threaded implementations
**/



