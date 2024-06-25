/*
 * ODK Runner
 * Copyright (C) 2024 Damien Goutte-Gattat
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "util.h"

#if defined(ODK_RUNNER_LINUX)
#include <sys/sysinfo.h>

#elif defined(ODK_RUNNER_MACOS)
#include <sys/sysctl.h>

#elif defined(ODK_RUNNER_WINDOWS)
#include <windows.h>

#endif

/**
 * Gets the amount of physical memory available.
 *
 * @return The total physical memory (in bytes), or 0 if we couldn't get
 *         that information.
 */
size_t
get_physical_memory(void)
{
    size_t phys_mem = 0;

#if defined(ODK_RUNNER_LINUX)
    struct sysinfo info;

    if ( sysinfo(&info) != -1 )
        phys_mem = info.totalram;

#elif defined(ODK_RUNNER_MACOS)
    int mib_name[] = { CTL_HW, HW_MEMSIZE };
    size_t len = sizeof(phys_mem);

    if ( sysctl(mib_name, 2, &phys_mem, &len, NULL, 0) == -1 )
        phys_mem = 0;

#elif defined(ODK_RUNNER_WINDOWS)
    MEMORYSTATUSEX statex;

    statex.dwLength = sizeof(statex);
    if ( GlobalMemoryStatusEx(&statex) != 0 )
        phys_mem = statex.ullTotalPhys;

#endif

    return phys_mem;
}
