// Copyright (c) 2024-present The Qtum Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QTUM_THREAD_PRIORITY_H
#define QTUM_THREAD_PRIORITY_H

#ifndef WIN32
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#else
#include <windows.h>
#endif

#ifdef WIN32
inline void SetThreadPriority(int nPriority)
{
    HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());
    if (hThread != NULL)
    {
        ::SetThreadPriority(hThread, nPriority);
        CloseHandle(hThread);
    }
}
#else

#define THREAD_PRIORITY_LOWEST          PRIO_MAX
#define THREAD_PRIORITY_BELOW_NORMAL    2
#define THREAD_PRIORITY_NORMAL          0
#define THREAD_PRIORITY_ABOVE_NORMAL    0

inline void SetThreadPriority(int nPriority)
{
    // It's unclear if it's even possible to change thread priorities on Linux,
    // but we really and truly need it for the generation threads.
#ifdef PRIO_THREAD
    setpriority(PRIO_THREAD, 0, nPriority);
#else
    setpriority(PRIO_PROCESS, 0, nPriority);
#endif
}
#endif

#endif // QTUM_THREAD_PRIORITY_H
